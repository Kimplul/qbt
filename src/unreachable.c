#include <qbt/unreachable.h>

void remove_unvisited(struct fn *f, int visited)
{
	struct vec new = vec_create(sizeof(struct blk *));
	foreach_blk(bi, f->blks) {
		struct blk *b = blk_at(f->blks, bi);
		if (b->visited < visited)
			destroy_block(b);
		else
			vec_append(&new, &b);
	}

	vec_destroy(&f->blks);
	f->blks = new;
}
