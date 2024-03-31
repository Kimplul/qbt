#include <assert.h>

#include <qbt/regalloc.h>
#include <qbt/vec.h>
#include <qbt/abi.h>

static const int64_t tr_map[] = {
	RS0, RS1, RS2, RS3, RS4, RS5, RS6, RS7, RS8, RS9,
	RS10, RS11, RS12, RS13, RS14, RS15, RS16, RS17, RS18, RS19,
	RS20, RS21, RS22, RS23, RS24
};

#define reg_at(v, i)\
	vect_at(int64_t, v, i)

static struct val rewrite_tmp(struct vec rmap, struct val t)
{
	assert(t.class == TMP);
	assert(t.r < (int64_t)vec_len(&rmap));
	int64_t r = reg_at(rmap, t.r);
	assert(r);
	return reg_val(r);
}

static size_t cur_reg = 0;
static void add_rewrite_rule(struct vec *rmap, struct val t)
{
	assert(t.class == TMP);
	assert(cur_reg < 25
			&& "ran out of temp registers, time to implement proper regalloc!");
	/* note <=, we go one 'beyond' just to make sure that 0 fits */
	while ((int64_t)vec_len(rmap) <= t.r) {
		int64_t zero = 0;
		vec_append(rmap, &zero);
	}

	if (reg_at(*rmap, t.r) == 0)
		reg_at(*rmap, t.r) = tr_map[cur_reg++];
}

void regalloc(struct fn *f)
{
	/* very ugly, fix once we get the proper regalloc implemented */
	cur_reg = 0;
	struct vec rmap = vec_create(sizeof(int64_t));

	/* here we would ideally do some kind of lifetime checking, to start
	 * with we only assign to different temporary registers, we have enough
	 * of them to work for some smaller test functions */
	/* lifetime info would probably also be useful for saving callee-save
	 * registers during calls, so that should probably also be done during
	 * register allocation? maybe? */
	foreach_blk(bi, f->blks) {
		struct blk *b = blk_at(f->blks, bi);
		foreach_insn(i, b->insns) {
			struct insn n = insn_at(b->insns, i);
			if (n.in[0].class == TMP) {
				n.in[0] = rewrite_tmp(rmap, n.in[0]);
			}

			if (n.in[1].class == TMP) {
				n.in[1] = rewrite_tmp(rmap, n.in[1]);
			}

			if (n.out.class == TMP) {
				add_rewrite_rule(&rmap, n.out);
				n.out = rewrite_tmp(rmap, n.out);
			}

			/* write back changes */
			insn_at(b->insns, i) = n;

			/* do this here since the register allocation is the
			 * last stage before actually lowering to assembly, so
			 * no possibility of dead code elimination or stuff like
			 * that */
			if (n.type == CALL)
				f->has_calls = true;
		}

		if (b->cmp[0].class == TMP)
			b->cmp[0] = rewrite_tmp(rmap, b->cmp[0]);

		if (b->cmp[1].class == TMP)
			b->cmp[1] = rewrite_tmp(rmap, b->cmp[1]);
	}

	f->max_callee_save = cur_reg;
	vec_destroy(&rmap);
}
