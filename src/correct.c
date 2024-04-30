#include <assert.h>
#include <qbt/opt.h>

static size_t spill_ref(struct blk *b, size_t ii, struct insn i, size_t idx,
                        size_t ri)
{
	struct val tmp = tmp_val(ri++);
	struct insn new = insn_create(ADDR, I27, tmp, i.in[idx], noclass(), 0);
	i.in[idx] = tmp;
	insn_at(b->insns, ii) = i;
	insn_insert(b, new, ii);
	return ri;
}

static size_t spill_imm(struct blk *b, size_t ii, struct insn i, size_t idx,
                        size_t ri)
{
	struct val tmp = tmp_val(ri++);
	struct insn new = insn_create(COPY, I27, tmp, i.in[idx], noclass(), 0);
	i.in[idx] = tmp;
	insn_at(b->insns, ii) = i;
	insn_insert(b, new, ii);
	return ri;
}

static size_t correct_arith(struct blk *b, size_t ii, struct insn i, size_t ri)
{
	if (i.in[0].class == IMM) {
		/* swap so that immediates are 'outermost' */
		struct val tmp = i.in[0];
		i.in[0] = i.in[1];
		i.in[1] = tmp;
		insn_at(b->insns, ii) = i;
		return ri;
	}

	if (i.in[0].class == IMM)
		return spill_imm(b, ii, i, 0, ri);

	return ri;
}

static size_t correct_positional_arith(struct blk *b, size_t ii, struct insn i,
                                       size_t ri)
{
	if (i.in[0].class == IMM)
		return spill_imm(b, ii, i, 0, ri);

	return ri;
}

static size_t correct_branch(struct blk *b, size_t ii, struct insn i, size_t ri)
{
	if (i.in[0].class == IMM)
		return spill_imm(b, ii, i, 0, ri);

	if (i.in[1].class == IMM)
		return spill_imm(b, ii, i, 1, ri);

	return ri;
}

static size_t correct_relations(struct blk *b, size_t ii, struct insn i,
                                size_t ri)
{
	if (i.in[0].class == IMM)
		return spill_imm(b, ii, i, 0, ri);

	return ri;
}

static size_t correct_store(struct blk *b, size_t ii, struct insn i, size_t ri)
{
	if (i.in[1].class == IMM)
		return spill_imm(b, ii, i, 1, ri);

	return ri;
}

static void add_rewrite_rule(struct vec *rmap, struct val from, struct val to)
{
	assert(from.class == TMP);
	assert(to.class == TMP);

	while ((int64_t)vec_len(rmap) <= from.r) {
		struct val no = noclass();
		vec_append(rmap, &no);
	}

	val_at(*rmap, from.r) = to;
}

static bool has_rewrite_rule(struct vec *rmap, struct val t)
{
	if (t.r >= (int64_t)vec_len(rmap))
		return false;

	struct val r = val_at(*rmap, t.r);
	return r.class != NOCLASS;
}

static struct val rewrite_tmp(struct vec *rmap, struct val t)
{
	assert(t.class == TMP);
	assert(t.r < (int64_t)vec_len(rmap));
	struct val r = val_at(*rmap, t.r);
	assert(r.class != NOCLASS);
	return r;
}

static size_t correct_addr(struct blk *b, struct vec *rewrite_addrs, size_t ii,
                           struct insn i, size_t ri)
{
	if (i.in[0].class == TMP) {
		/** @todo this messes with the rest of the corrections, as the
		 * store with the tmp is rewritten to load t1 first, overwriting
		 * t0 */
		/* addr could potentially be defined to move the register into
		 * the location it's specifying? Not a particularly clean
		 * solution but I guess it could work? */
		add_rewrite_rule(rewrite_addrs, i.in[0], i.out);
		return ri;
	}

	return ri;
}

static size_t load_rewrite(struct blk *b, struct vec *rewrite_addrs, size_t ii,
                           struct insn i, size_t ri, size_t idx)
{
	struct val addr = rewrite_tmp(rewrite_addrs, i.in[idx]);
	struct val tmp = tmp_val(ri++);
	struct insn new = insn_create(LOAD, I27, tmp, addr, noclass(), 0);
	i.in[idx] = tmp;
	insn_at(b->insns, ii) = i;
	insn_insert(b, new, ii);
	return ri;
}

static size_t store_rewrite(struct blk *b, struct vec *rewrite_addrs, size_t ii,
                            struct insn i, size_t ri)
{
	struct val addr = rewrite_tmp(rewrite_addrs, i.out);
	struct val tmp = tmp_val(ri++);
	struct insn new = insn_create(STORE, I27, tmp, addr, noclass(), 0);
	i.out = tmp;
	insn_at(b->insns, ii) = i;
	insn_insert(b, new, ii + 1);
	return ri;
}

static size_t correct_insn(struct blk *b, struct vec *rewrite_addrs, size_t ii,
                           struct insn i, size_t ri)
{
	/* replace registers referencing rewritten addr */
	if (i.in[0].class == TMP && has_rewrite_rule(rewrite_addrs, i.in[0]))
		return load_rewrite(b, rewrite_addrs, ii, i, ri, 0);

	if (i.in[1].class == TMP  && has_rewrite_rule(rewrite_addrs, i.in[1]))
		return load_rewrite(b, rewrite_addrs, ii, i, ri, 1);

	if (i.out.class == TMP && has_rewrite_rule(rewrite_addrs, i.out)) {
		/* note no return */
		store_rewrite(b, rewrite_addrs, ii, i, ri);
	}

	/* replace references with instructions */
	if (i.type != CALL && i.in[0].class == REF)
		return spill_ref(b, ii, i, 0, ri);

	if (i.in[1].class == REF)
		return spill_ref(b, ii, i, 1, ri);

	switch (i.type) {
	case ADD:
	case MUL:
		return correct_arith(b, ii, i, ri);

	case SUB:
	case DIV:
	case REM:
	case LSHIFT:
	case RSHIFT:
		return correct_positional_arith(b, ii, i, ri);

	case BEQ:
	case BNE:
	case BLE:
	case BGE:
	case BLT:
	case BGT:
	case BNZ:
	case BEZ:
		/* oh wait, this never triggers because branches are at the end
		 * of blocks, duh */
		return correct_branch(b, ii, i, ri);

	case LT:
	case LE:
	case GT:
	case GE:
	case EQ:
	case NE:
		return correct_relations(b, ii, i, ri);

	case STORE:
		return correct_store(b, ii, i, ri);

	case ADDR:
		return correct_addr(b, rewrite_addrs, ii, i, ri);

	default:
	}

	return ri;
}

size_t correct(struct fn *f, size_t ri)
{
	/* some simpler corrections to make sure all instructions follow a
	 * specific pattern. The textual version doesn't have these
	 * restrictions, but they make our lives easier in the future. */

	struct vec rewrite_addrs = vec_create(sizeof(struct val));

	foreach_blk(bi, f->blks) {
		struct blk *b = blk_at(f->blks, bi);
		foreach_insn(ii, b->insns) {
			struct insn i = insn_at(b->insns, ii);
			ri = correct_insn(b, &rewrite_addrs, ii, i, ri);
		}

		if (b->cmp[0].class == IMM) {
			struct val t = tmp_val(ri++);
			insadd(b, COPY, I27, t, b->cmp[0], noclass(), 0);
			b->cmp[0] = t;
		}

		if (b->cmp[1].class == IMM) {
			struct val t = tmp_val(ri++);
			insadd(b, COPY, I27, t, b->cmp[0], noclass(), 0);
			b->cmp[1] = t;
		}
	}

	vec_destroy(&rewrite_addrs);
	return ri;
}
