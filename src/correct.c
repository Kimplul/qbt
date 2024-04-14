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

static size_t correct_insn(struct blk *b, size_t ii, struct insn i, size_t ri)
{
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

	default:
	}

	return ri;
}

void correct(struct fn *f, size_t ri)
{
	/* some simpler corrections to make sure all instructions follow a
	 * specific pattern. The textual version doesn't have these
	 * restrictions, but they make our lives easier in the future. */
	foreach_blk(bi, f->blks) {
		struct blk *b = blk_at(f->blks, bi);
		foreach_insn(ii, b->insns) {
			struct insn i = insn_at(b->insns, ii);
			ri = correct_insn(b, ii, i, ri);
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
}
