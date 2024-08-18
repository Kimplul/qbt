#include <assert.h>
#include <qbt/unreachable.h>

static void add_val(struct vec *map, struct val v)
{
	assert(v.class == TMP);
	size_t i = v.r;
	while (vec_len(map) <= i) {
		struct val no = noclass();
		vec_append(map, &no);
	}

	blk_param_at(*map, i) = v;
}

static void remove_val(struct vec *map, struct val v)
{
	assert(v.class == TMP);
	size_t i = v.r;
	if (vec_len(map) <= i)
		return;

	blk_param_at(*map, i) = noclass();
}

static bool has_val(struct vec *map, struct val v)
{
	assert(v.class == TMP);
	size_t i = v.r;
	if (vec_len(map) <= i)
		return false;

	struct val p = blk_param_at(*map, i);
	return p.class != NOCLASS;
}

static bool entered(struct blk *b, int visited)
{
	return b->visited >= visited + 1;
}

static void enter(struct blk *b)
{
	b->visited++;
}

static bool done(struct blk *b, int visited)
{
	return b->visited >= visited + 2;
}

static void leave(struct blk *b)
{
	b->visited++;
}

static void build_params(struct blk *b, int visited)
{
	if (done(b, visited))
		return;

	/* recursive functionality made iterative by using an external
	 * stack, just think of pushing to the stack as doing a recursive call
	 * */
	struct vec stack = vec_create(sizeof(struct blk *));
	vec_append(&stack, &b);

top:
	if (vec_len(&stack) == 0) {
		vec_destroy(&stack);
		return;
	}

	b = vect_pop(struct blk *, stack);
	if (done(b, visited))
		goto top;

	if (return_blk(b)) {
		b->s1 = NULL;
		b->s2 = NULL;
	}

	/* this is the first time here, queue up work for us */
	if (!entered(b, visited)) {
		/* put ourselves back onto the stack */
		vec_append(&stack, &b);

		if (b->s1)
			vec_append(&stack, &b->s1);
		if (b->s2 && b->s2 != b->s1)
			vec_append(&stack, &b->s2);

		enter(b);
		goto top;
	}

	/* second time through, do actual work now that dependencies (should) be
	 * done */
	struct vec generated = vec_create(sizeof(struct val));
	struct vec forward = vec_create(sizeof(struct val));
	struct vec required = vec_create(sizeof(struct val));

	/* first, collect all forwards */
	if (b->s1) {
		foreach_blk_param(pi, b->s1->params) {
			struct val p = blk_param_at(b->s1->params, pi);
			add_val(&forward, p);
		}
	}

	if (b->s2 && b->s2 != b->s1) {
		foreach_blk_param(pi, b->s2->params) {
			struct val p = blk_param_at(b->s2->params, pi);
			add_val(&forward, p);
		}
	}

	/* go through each instruction and add inputs to required and outputs to
	 * generated */
	foreach_insn(ii, b->insns) {
		struct insn i = insn_at(b->insns, ii);
		struct val in1 = i.in[0];
		if (in1.class == TMP && !has_val(&generated, in1))
			add_val(&required, in1);

		struct val in2 = i.in[1];
		if (in2.class == TMP && !has_val(&generated, in2))
			add_val(&required, in2);

		struct val out = i.out;
		if (out.class == TMP) {
			add_val(&generated, out);
			/* we generate this value so no need to forward it */
			if (has_val(&forward, out))
				remove_val(&forward, out);
		}
	}

	if (b->cmp[0].class == TMP && !has_val(&generated, b->cmp[0]))
		add_val(&required, b->cmp[0]);

	if (b->cmp[1].class == TMP && !has_val(&generated, b->cmp[1]))
		add_val(&required, b->cmp[1]);

	/* add values that need forwarding to our required list */
	foreach_blk_param(pi, forward) {
		/* if value already is in list, don't do anything */
		struct val p = blk_param_at(forward, pi);

		/* skip empty slots */
		if (p.class == NOCLASS)
			continue;

		if (has_val(&required, p))
			continue;

		add_val(&required, p);
	}

	/* convert from the hashmap-like structure to regular vector */
	foreach_blk_param(pi, required) {
		struct val p = blk_param_at(required, pi);
		if (p.class == NOCLASS)
			continue;

		vec_append(&b->params, &p);
	}

	vec_destroy(&required);
	vec_destroy(&forward);
	vec_destroy(&generated);
	leave(b); // mark us ready
	goto top;
}

static void collect_params(struct blk *b, int visited)
{
	if (done(b, visited))
		return;

	struct vec stack = vec_create(sizeof(struct blk *));
	vec_append(&stack, &b);

top:
	if (vec_len(&stack) == 0) {
		vec_destroy(&stack);
		return;
	}

	b = vect_pop(struct blk *, stack);
	if (done(b, visited))
		goto top;

	if (!entered(b, visited)) {
		vec_append(&stack, &b);

		if (b->s1)
			vec_append(&stack, &b->s1);

		if (b->s2 && b->s2 != b->s1)
			vec_append(&stack, &b->s2);

		enter(b);
		goto top;
	}

	if (b->s1)
	foreach_blk_param(pi, b->s1->params) {
		struct val p = blk_param_at(b->s1->params, pi);
		vec_append(&b->args1, &p);
	}

	if (b->s2)
	foreach_blk_param(pi, b->s2->params) {
		struct val p = blk_param_at(b->s2->params, pi);
		vec_append(&b->args2, &p);
	}

	leave(b);
	goto top;
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

static struct val rewrite_tmp(struct vec *rmap, struct val from)
{
	assert(from.class == TMP);
	assert(from.r <= (int64_t)vec_len(rmap));
	struct val v = val_at(*rmap, from.r);
	assert(v.class == TMP);
	return v;
}

static size_t rename_temps(struct blk *b, size_t i)
{
	struct vec rmap = vec_create(sizeof(struct val));

	foreach_blk_param(pi, b->params) {
		struct val p = blk_param_at(b->params, pi);
		/* this is very similar to what's going on in regalloc.c, hmm */
		add_rewrite_rule(&rmap, p, tmp_val(i++));
		blk_param_at(b->params, pi) = rewrite_tmp(&rmap, p);
	}

	foreach_insn(ii, b->insns) {
		struct insn n = insn_at(b->insns, ii);

		if (n.in[0].class == TMP)
			n.in[0] = rewrite_tmp(&rmap, n.in[0]);

		if (n.in[1].class == TMP)
			n.in[1] = rewrite_tmp(&rmap, n.in[1]);

		if (n.out.class == TMP) {
			add_rewrite_rule(&rmap, n.out, tmp_val(i++));
			n.out = rewrite_tmp(&rmap, n.out);
		}

		insn_at(b->insns, ii) = n;
	}

	if (b->cmp[0].class == TMP)
		b->cmp[0] = rewrite_tmp(&rmap, b->cmp[0]);

	if (b->cmp[1].class == TMP)
		b->cmp[1] = rewrite_tmp(&rmap, b->cmp[1]);

	foreach_blk_param(pi, b->args1) {
		struct val p = blk_param_at(b->args1, pi);
		blk_param_at(b->args1, pi) = rewrite_tmp(&rmap, p);
	}

	foreach_blk_param(pi, b->args2) {
		struct val p = blk_param_at(b->args2, pi);
		blk_param_at(b->args2, pi) = rewrite_tmp(&rmap, p);
	}

	vec_destroy(&rmap);
	return i;
}

size_t ssa(struct fn *f)
{
	/* do a depth-first traversal of blocks, mark locations within as either
	 * generated (i.e. we assign something to the register) or required
	 * (i.e. it must be taken as a block parameter) */
	struct blk *b = blk_at(f->blks, 0);
	build_params(b, b->visited);
	remove_unvisited(f, b->visited);
	collect_params(b, b->visited);

	size_t i = 0;
	foreach_blk(bi, f->blks) {
		struct blk *b = blk_at(f->blks, bi);
		i = rename_temps(b, i);
	}

	return i;
}
