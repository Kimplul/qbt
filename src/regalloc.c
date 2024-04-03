#include <assert.h>
#include <stdlib.h>

#include <qbt/regalloc.h>
#include <qbt/vec.h>
#include <qbt/abi.h>

/* first try to use temporaries, then callee-save, then finally args.
 * Idea is that callee-save cost is paid once at the start of the function call,
 * after that they're free. Argument registers are generally important to keep
 * free to make sure function calls in loops etc. don't have to shuffle
 * registers around as much */
static const int64_t tr_map[] = {
	RT0, RT1, RT2, RT3, RT4, RT5, RT6, RT7, RT8, RT9,
	RT10, RT11, RT12, RT13, RT14, RT15, RT16, RT17, RT18, RT19,
	RT20, RT21, RT22, RT23,

	RS0, RS1, RS2, RS3, RS4, RS5, RS6, RS7, RS8, RS9,
	RS10, RS11, RS12, RS13, RS14, RS15, RS16, RS17, RS18, RS19,
	RS20, RS21, RS22, RS23,

	RA0, RA1, RA2, RA3, RA4, RA5, RA6, RA7, RA8, RA9,
	RA10, RA11, RA12, RA13, RA14, RA15, RA16, RA17, RA18, RA19,
	RA20, RA21, RA22, RA23,
};

#define reg_at(v, i) \
	vect_at(struct val, v, i)

static bool has_rewrite_rule(struct vec *rmap, struct val t)
{
	if (t.r >= (int64_t)vec_len(rmap))
		return false;

	struct val r = reg_at(*rmap, t.r);
	return r.class != NOCLASS;
}

static struct val rewrite_tmp(struct vec *rmap, struct val t)
{
	assert(t.class == TMP);
	assert(t.r < (int64_t)vec_len(rmap));
	struct val r = reg_at(*rmap, t.r);
	assert(r.class != NOCLASS);
	return r;
}

static void add_rewrite_rule(struct vec *rmap, struct val from, struct val to)
{
	assert(from.class == TMP);
	assert(to.class == REG);
	/* note <=, we go one 'beyond' just to make sure that 0 fits */
	while ((int64_t)vec_len(rmap) <= from.r) {
		struct val no = noclass();
		vec_append(rmap, &no);
	}

	reg_at(*rmap, from.r) = to;
}

static void add_hint(struct vec *hints, struct val from, struct val to)
{
	/* kind of a hack */
	add_rewrite_rule(hints, from, to);
}

static struct val get_hint(struct vec *hints, struct val from)
{
	if ((int64_t)vec_len(hints) <= from.r)
		return noclass();

	return rewrite_tmp(hints, from);
}

struct lifetime {
	struct val v;
	size_t start;
	size_t end;
	size_t used;
};

#define lifetime_at(lifetimes, i) \
	vect_at(struct lifetime, lifetimes, i)

#define foreach_lifetime(iter, lifetimes) \
	foreach_vec(iter, lifetimes)

static void add_def(struct vec *lifetimes, struct val v, size_t i)
{
	while ((int64_t)vec_len(lifetimes) <= v.r) {
		vec_append(lifetimes, &(struct lifetime){noclass(), 0, 0, 0});
	}

	/* I guess a def isn't techincally a 'use' but good enough, we assume
	 * that unused variables have already been removed by some earlier stage
	 * (not currently true but that's the intention) */
	lifetime_at(*lifetimes, v.r) = (struct lifetime){v, i, 0, 1};
}

static void add_use(struct vec *lifetimes, struct val v, size_t i)
{
	assert((int64_t)vec_len(lifetimes) > v.r);
	struct lifetime l = lifetime_at(*lifetimes, v.r);
	assert(l.used);
	l.used++;
	l.end = i;
	lifetime_at(*lifetimes, v.r) = l;
}

static void collect_lifetimes(struct blk *b, struct vec *hints,
                              struct vec *lifetimes)
{
	foreach_blk_param(pi, b->params) {
		struct val v = blk_param_at(b->params, pi);
		add_def(lifetimes, v, 0);
	}

	size_t pos = 1;
	foreach_insn(ii, b->insns) {
		struct insn i = insn_at(b->insns, ii);
		if (i.in[0].class == TMP)
			add_use(lifetimes, i.in[0], pos);

		if (i.in[1].class == TMP)
			add_use(lifetimes, i.in[1], pos);

		if (i.out.class == TMP)
			add_def(lifetimes, i.out, pos);

		/* collect some early hints */
		if (i.type == MOVE) {
			/* input arguments, retvals */
			if (i.out.class == TMP && i.in[0].class == REG)
				add_hint(hints, i.out, i.in[0]);

			if (i.out.class == REG && i.in[0].class == TMP)
				add_hint(hints, i.in[0], i.out);
		}

		pos++;
	}

	if (b->cmp[0].class == TMP)
		add_use(lifetimes, b->cmp[0], pos);

	if (b->cmp[1].class == TMP)
		add_use(lifetimes, b->cmp[1], pos);

	foreach_blk_param(pi, b->args1) {
		struct val v = blk_param_at(b->args1, pi);
		add_use(lifetimes, v, pos);
	}

	foreach_blk_param(pi, b->args2) {
		struct val v = blk_param_at(b->args2, pi);
		add_use(lifetimes, v, pos);
	}
}

static void build_active(struct vec *active, struct vec *lifetimes, size_t i)
{
	struct lifetime ref = lifetime_at(*lifetimes, i);
	foreach_lifetime(li, *lifetimes) {
		struct lifetime l = lifetime_at(*lifetimes, li);
		if (l.used == 0)
			continue;

		if (l.end < ref.start)
			continue;

		if (l.start > ref.end)
			continue;

		vec_append(active, &l);
	}
}

static void build_reserved(struct vec *reserved, struct vec *active,
                           struct vec *rmap)
{
	foreach_lifetime(li, *active) {
		struct lifetime l = lifetime_at(*active, li);
		if (l.used == 0)
			continue;

		if (has_rewrite_rule(rmap, l.v)) {
			struct val act = rewrite_tmp(rmap, l.v);
			vec_append(reserved, &act);
		}
	}
}

static bool reg_free(struct vec *reserved, struct val f)
{
	assert(f.class == REG);
	/* not the fastest way in the world, but good enough for now */
	foreach_val(ri, *reserved) {
		struct val r = val_at(*reserved, ri);
		if (same_val(r, f))
			return false;
	}

	return true;
}

static struct val find_free_reg(struct vec *reserved)
{
	for (size_t i = 0; i < sizeof(tr_map) / sizeof(tr_map[0]); ++i) {
		struct val r = reg_val(tr_map[i]);
		if (reg_free(reserved, r))
			return r;
	}

	/* handle spill case later */
	assert(0 &&
	       "ran out of registers, time to implement proper spill handling");
	abort();
}

static void build_rmap(struct vec *hints, struct vec *lifetimes,
                       struct vec *rmap)
{
	struct vec active = vec_create(sizeof(struct lifetime));
	struct vec reserved = vec_create(sizeof(struct val));

	foreach_lifetime(li, *lifetimes) {
		struct lifetime l = lifetime_at(*lifetimes, li);
		if (l.used == 0)
			continue;

		vec_reset(&active);
		build_active(&active, lifetimes, li);
		build_reserved(&reserved, &active, rmap);

		struct val h = get_hint(hints, l.v);
		if (h.class != NOCLASS) {
			if (reg_free(&reserved, h)) {
				add_rewrite_rule(rmap, l.v, h);
				continue;
			}
		}

		/* eventually we should select the spill register only for the
		 * least used register, but that's a bit more complicated than
		 * just this linear scan */
		struct val f = find_free_reg(&reserved);
		add_rewrite_rule(rmap, l.v, f);
	}
}

/* has_calls is kind of in an iffy place but I guess it's fine */
static void do_rewrites(struct blk *b, struct vec *rmap, struct fn *f)
{
	foreach_blk_param(pi, b->params) {
		struct val p = blk_param_at(b->params, pi);
		blk_param_at(b->params, pi) = rewrite_tmp(rmap, p);
	}

	foreach_insn(ii, b->insns) {
		struct insn i = insn_at(b->insns, ii);
		if (i.in[0].class == TMP)
			i.in[0] = rewrite_tmp(rmap, i.in[0]);

		if (i.in[1].class == TMP)
			i.in[1] = rewrite_tmp(rmap, i.in[1]);

		if (i.out.class == TMP)
			i.out = rewrite_tmp(rmap, i.out);

		/** @todo save caller-save at callsites here */
		if (i.type == CALL)
			f->has_calls = true;

		/* write back changes, christ I forget this a lot */
		insn_at(b->insns, ii) = i;
	}

	if (b->cmp[0].class == TMP)
		b->cmp[0] = rewrite_tmp(rmap, b->cmp[0]);

	if (b->cmp[1].class == TMP)
		b->cmp[1] = rewrite_tmp(rmap, b->cmp[1]);

	foreach_blk_param(pi, b->args1) {
		struct val p = blk_param_at(b->args1, pi);
		blk_param_at(b->args1, pi) = rewrite_tmp(rmap, p);
	}

	foreach_blk_param(pi, b->args2) {
		struct val p = blk_param_at(b->args2, pi);
		blk_param_at(b->args2, pi) = rewrite_tmp(rmap, p);
	}
}

/* assumes ssa form */
void regalloc(struct fn *f)
{
	struct vec hints = vec_create(sizeof(struct val));
	struct vec lifetimes = vec_create(sizeof(struct lifetime));
	struct vec rmap = vec_create(sizeof(struct val));

	/* here it would probably make sense to iterate through lifetimes in
	 * order of priority, like loop nesting/count etc */
	foreach_blk(bi, f->blks) {
		struct blk *b = blk_at(f->blks, bi);
		vec_reset(&lifetimes);
		/** @todo collect hints from args/params */
		collect_lifetimes(b, &hints, &lifetimes);
		build_rmap(&hints, &lifetimes, &rmap);
		do_rewrites(b, &rmap, f);
		/** @todo forward_hints(b, &hints, &rmap) */
	}

	vec_destroy(&hints);
	vec_destroy(&lifetimes);
	vec_destroy(&rmap);
}
