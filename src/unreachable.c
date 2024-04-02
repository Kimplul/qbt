#include <qbt/unreachable.h>

void build_reachable_vec(struct vec *reachable, struct blk *cur)
{
	if (cur->reachable)
		return;

	cur->reachable = true;
	vec_append(reachable, &cur);

	if (cur->btype == RET)
		return;

	if (cur->btype == J)
		cur->s1 = cur->s2;

	if (cur->s1)
		build_reachable_vec(reachable, cur->s1);

	if (cur->s2)
		build_reachable_vec(reachable, cur->s2);
}

void unreachable(struct fn *f)
{
	struct vec reachable = vec_create(sizeof(struct blk *));
	struct blk *b = blk_at(f->blks, 0);
	build_reachable_vec(&reachable, b);
	foreach_blk(bi, f->blks) {
		struct blk *b = blk_at(f->blks, bi);
		if (!b->reachable)
			destroy_block(b);

	}

	vec_destroy(&f->blks);
	f->blks = reachable;
}
