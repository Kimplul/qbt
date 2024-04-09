#include <assert.h>
#include <stdlib.h>

#include <qbt/regalloc.h>
#include <qbt/vec.h>
#include <qbt/abi.h>

/* first try to use temporaries, then callee-save, then finally args.
 * Idea is that callee-save cost is paid once at the start of the function call,
 * after that they're free. Argument registers are generally important to keep
 * free to make sure function calls in loops etc. don't have to shuffle
 * registers around as much. The preferred order can probably be bikeshedded to death,
 * eventually would be kind of cool to add some heuristic for which category
 * might be best suited for a specific location. 
 * One possible one would be that callee-save should be preferred if the
 * lifetime overlaps a function call? */
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

	return reg_at(*hints, from.r);
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
	lifetime_at(*lifetimes, v.r) = (struct lifetime){v, i, i, 1};
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
                              struct vec *lifetimes, struct vec *rmap, struct vec *calls)
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
			if (i.out.class == TMP && i.in[0].class == REG) {
				add_hint(hints, i.out, i.in[0]);
				/* retvals must be treated as reserving that
				 * virtual register */
				add_rewrite_rule(rmap, i.out, i.in[0]);
			}

			if (i.out.class == REG && i.in[0].class == TMP)
				add_hint(hints, i.in[0], i.out);
		}

		/* collect calls, to be used later */
		if (i.type == CALL)
			vec_append(calls, &pos);

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

static  void build_active_between(struct vec *active, struct vec *lifetimes, size_t start, size_t end)
{
	foreach_lifetime(li, *lifetimes) {
		struct lifetime l = lifetime_at(*lifetimes, li);
		if (l.used == 0)
			continue;

		if (l.end < start)
			continue;

		if (l.start > end)
			continue;

		vec_append(active, &l);
	}
}

static void build_active(struct vec *active, struct vec *lifetimes, size_t i)
{
	struct lifetime ref = lifetime_at(*lifetimes, i);
	build_active_between(active, lifetimes, ref.start, ref.end);
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
	/* right now I'm thinking that spills should behave like SAVE/RESTORE,
	 * i.e. they get an index and the spilled number of registers is counted
	 * somewhere and added to the frame size like call registers. */
	assert(0 &&
	       "ran out of registers, time to implement proper spill handling");
	abort();
}

static size_t highest_sreg(struct val f)
{
	assert(f.class == REG);
	switch (f.r) {
		case RS0: return 1;
		case RS1: return 2;
		case RS2: return 3;
		case RS3: return 4;
		case RS4: return 5;
		case RS5: return 6;
		case RS6: return 7;
		case RS7: return 8;
		case RS8: return 9;
		case RS9: return 10;
		case RS10: return 11;
		case RS11: return 12;
		case RS12: return 13;
		case RS13: return 14;
		case RS14: return 15;
		case RS15: return 16;
		case RS16: return 17;
		case RS17: return 18;
		case RS18: return 19;
		case RS19: return 20;
		case RS20: return 21;
		case RS21: return 22;
		case RS22: return 23;
		case RS23: return 24;
	}

	return 0;
}

static size_t build_rmap(struct vec *hints, struct vec *lifetimes,
                       struct vec *rmap)
{
	size_t max_callee_save = 0;
	struct vec active = vec_create(sizeof(struct lifetime));
	struct vec reserved = vec_create(sizeof(struct val));

	foreach_lifetime(li, *lifetimes) {
		struct lifetime l = lifetime_at(*lifetimes, li);
		if (l.used == 0)
			continue;

		if (has_rewrite_rule(rmap, l.v))
			continue;

		vec_reset(&active);
		vec_reset(&reserved);
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
		if (highest_sreg(f) > max_callee_save)
			max_callee_save = highest_sreg(f);

		add_rewrite_rule(rmap, l.v, f);
	}

	vec_destroy(&active);
	vec_destroy(&reserved);

	return max_callee_save;
}

/* has_calls is kind of in an iffy place but I guess it's fine */
static void do_rewrites(struct blk *b, struct vec *rmap)
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

static bool callee_save(struct val r)
{
	assert(r.class == REG);
	switch (r.r) {
		case RS0: return true;
		case RS1: return true;
		case RS2: return true;
		case RS3: return true;
		case RS4: return true;
		case RS5: return true;
		case RS6: return true;
		case RS7: return true;
		case RS8: return true;
		case RS9: return true;
		case RS10: return true;
		case RS11: return true;
		case RS12: return true;
		case RS13: return true;
		case RS14: return true;
		case RS15: return true;
		case RS16: return true;
		case RS17: return true;
		case RS18: return true;
		case RS19: return true;
		case RS20: return true;
		case RS21: return true;
		case RS22: return true;
		case RS23: return true;
	}

	return false;
}

static void insn_insert_before_call(struct blk *b, struct insn save, ssize_t pos)
{
	assert((insn_at(b->insns, pos)).type == CALL);
	struct insn setup;
	do {
		if (pos < 0)
			break;

		setup = insn_at(b->insns, pos);
		pos--;
	} while (has_insn_flag(setup, CALL_SETUP));

	insn_insert(b, save, pos + 1);
}

static void insn_insert_after_call(struct blk *b, struct insn restore, size_t pos)
{
	assert((insn_at(b->insns, pos)).type == CALL);
	size_t max = vec_len(&b->insns);

	struct insn setup;
	do {
		if (pos == max)
			break;

		setup = insn_at(b->insns, pos);
		pos++;
	} while (has_insn_flag(setup, CALL_TEARDOWN));

	insn_insert(b, restore, pos);
}

static size_t do_call_saves(struct blk *b, struct vec *lifetimes, struct vec *rmap, struct vec *calls)
{
	size_t offset = 0;
	size_t max_counter = 0;
	struct vec active = vec_create(sizeof(struct lifetime));
	struct vec reserved = vec_create(sizeof(struct val));

	foreach_vec(ci, *calls) {
		size_t call_pos = vect_at(size_t, *calls, ci);

		vec_reset(&active);
		vec_reset(&reserved);
		build_active_between(&active, lifetimes, call_pos, call_pos);
		build_reserved(&reserved, &active, rmap);

		/* what follows is a slight bit of index counting, a bit
		 * difficult to follow but not too bad */

		/* add a save right before call, offset is how many
		 * save/restores we've already added, very important */
		size_t counter = 0;
		foreach_val(ri, reserved) {
			struct val r = val_at(reserved, ri);
			assert(r.class == REG);
			if (callee_save(r))
				continue;

			struct insn save = insn_create(SAVE, NOTYPE,
							noclass(), r,
							imm_val(counter, I27), 0);

			insn_insert_before_call(b, save, call_pos + offset - 1);
			offset++;
			counter++;
		}

		call_pos += offset;

		counter = 0;
		foreach_val(ri, reserved) {
			struct val r = val_at(reserved, ri);
			if (callee_save(r))
				continue;

			/* add a restore right after call ~area~ */
			/* hmm, restore should maybe put r as its output to be
			 * more consistent... */
			struct insn restore = insn_create(RESTORE, NOTYPE,
							noclass(), r,
							imm_val(counter, I27), 0);

			insn_insert_after_call(b, restore, call_pos - 1);
			counter++;
			offset++;
		}

		if (counter > max_counter)
			max_counter = counter;
	}

	vec_destroy(&active);
	vec_destroy(&reserved);

	return max_counter;
}

/* assumes ssa form */
void regalloc(struct fn *f)
{
	struct vec hints = vec_create(sizeof(struct val));
	struct vec lifetimes = vec_create(sizeof(struct lifetime));
	struct vec rmap = vec_create(sizeof(struct val));
	struct vec calls = vec_create(sizeof(size_t));

	/* here it would probably make sense to iterate through lifetimes in
	 * order of priority, like loop nesting/count etc */
	foreach_blk(bi, f->blks) {
		struct blk *b = blk_at(f->blks, bi);
		vec_reset(&lifetimes);
		vec_reset(&calls);

		/** @todo collect hints from args/params */
		collect_lifetimes(b, &hints, &lifetimes, &rmap, &calls);
		size_t max_callee_save = build_rmap(&hints, &lifetimes, &rmap);

		do_rewrites(b, &rmap);
		size_t max_call_save = do_call_saves(b, &lifetimes, &rmap, &calls);
		/** @todo forward_hints(b, &hints, &rmap) */
		if (vec_len(&calls) != 0)
			f->has_calls = true;

		if (max_callee_save > f->max_callee_save)
			f->max_callee_save = max_callee_save;

		if (max_call_save > f->max_call_save)
			f->max_call_save = max_call_save;
	}

	vec_destroy(&calls);
	vec_destroy(&hints);
	vec_destroy(&lifetimes);
	vec_destroy(&rmap);
}
