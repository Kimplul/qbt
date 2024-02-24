#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <qbt/nodes.h>
#include <qbt/debug.h>
#include <qbt/vec.h>

void insadd(struct blk *b, enum insn_type o, enum val_type t, struct val r, struct val a0, struct val a1)
{
	struct insn i = insn_create(o, t, r, a0, a1);
	vec_append(&b->insns, &i);
}

int64_t idalloc(struct fn *f, const char *id)
{
	int64_t t = idmatch(f, id);
	if (t >= 0) {
		return t;
	}

	t = f->ntmp++;
	vec_append(&f->tmps, &(struct tmp_map){.id = id, .v = t});
	return t;
}

int64_t idmatch(struct fn *f, const char *id)
{
	foreach_tmp(i, f->tmps) {
		struct tmp_map t = tmp_at(f->tmps, i);
		if (strcmp(t.id, id) == 0)
			return t.v;
	}

	return -1;
}

void finish_block(struct blk *b, enum insn_type cmp, struct val a0, struct val a1, const char *label)
{
	assert(cmp >= BEQ && cmp <= RET && "illegal comparison type for block");
	b->btype = cmp;
	b->cmp[0] = a0;
	b->cmp[1] = a1;
	b->to = label;
}

struct blk *new_block(struct fn *f)
{
	struct blk *b = calloc(1, sizeof(struct blk));
	b->id = ++f->nblk;
	b->insns = vec_create(sizeof(struct insn));
	vec_append(&f->blks, &b);
	return b;
}

static struct label_map label_find(struct fn *f, const char *name)
{
	foreach_label(i, f->labels) {
		struct label_map m = label_at(f->labels, i);
		if (strcmp(m.id, name) == 0)
			return m;
	}

	return (struct label_map){.id = NULL, .b = NULL};
}

bool blk_empty(struct blk *b)
{
	return vec_len(&b->insns) == 0;
}

void finish_function(struct fn *f, const char *name)
{
	/** @todo check that returns match */
	f->name = name;
	/* last block should always be empty and can be removed */
	struct blk *last_blk = blk_pop(f->blks);
	if (!blk_empty(last_blk)) {
		error("last block not empty, %s missing return?", name);
		abort();
	}

	destroy_block(last_blk);

	foreach_blk(i, f->blks) {
		struct blk *b = blk_at(f->blks, i);
		if (!b->to)
			continue;

		struct label_map m = label_find(f, b->to);
		if (!m.id) {
			error("no label %s", b->to);
			abort();
		}

		b->s2 = m.b;
	}

}

struct fn *new_function()
{
	struct fn *f = calloc(1, sizeof(struct fn));
	f->blks = vec_create(sizeof(struct blk *));
	f->tmps = vec_create(sizeof(struct tmp_map));
	f->labels = vec_create(sizeof(struct label_map));
	/* empty block */
	new_block(f);
	return f;
}

void destroy_function(struct fn *f)
{
	vec_destroy(&f->tmps);
	vec_destroy(&f->labels);

	foreach_blk(i, f->blks) {
		struct blk *b = blk_at(f->blks, i);
		destroy_block(b);
	}
	vec_destroy(&f->blks);

	free(f);
}

void destroy_block(struct blk *b)
{
	vec_destroy(&b->insns);
	free(b);
}

void new_label(struct fn *f, struct blk *b, const char *name)
{
	vec_append(&f->labels, &(struct label_map){.id = name, .b = b});
}

static const char *op_str(enum insn_type n) {
#define CASE(I) case I: return #I;
	switch (n) {
		FOREACH_INSN_TYPE(CASE);
	}
#undef CASE
	return "unknown";
}

void dump_val(struct val val) {
	long long r = val.r;
	long long v = val.v;
	const char *s = val.s;

	switch (val.class) {
	case REG: printf("r%lli", r); break;
	case TMP: printf("t%lli", r); break;
	case IMM: printf("%lli", v); break;
	case MEM: printf("(r%lli, %lli)", r, v); break;
	case REF: printf("\"%s\"", s); break;
	case NOCLASS: break;
	}
}

void dump_insn(struct insn i)
{
	printf("\t");

	if (hasclass(i.out)) {
		dump_val(i.out);
		printf(" ");
	}

	printf("%s", op_str(i.type));

	if (hasclass(i.in[0])) {
		printf(" ");
		dump_val(i.in[0]);
	}

	if (hasclass(i.in[1])) {
		printf(" ");
		dump_val(i.in[1]);
	}

	printf("\n");
}

bool return_blk(struct blk *b)
{
	return b->btype == RET;
}

void dump_block(struct blk *b)
{
	printf("\t/*** block %lld ", (long long)b->id);
	if (b->name) printf("(%s) ", b->name);
	printf("***/\n");

	foreach_insn(i, b->insns) {
		struct insn n = insn_at(b->insns, i);
		dump_insn(n);
	}

	if (return_blk(b)) {
		printf("\n");
		return;
	}

	if (b->btype != J) {
		assert(b->s2);
		struct blk *s2 = b->s2;
		printf("\t%s ", op_str(b->btype));
		dump_val(b->cmp[0]);
		printf(" ");
		dump_val(b->cmp[1]);
		printf(" -> %lli\n", (long long)s2->id);
	}
	printf("\n");
}

void dump_function(struct fn *f) {
	printf("/*** function %s ***/\n", f->name);
	foreach_blk(i, f->blks) {
		struct blk *b = blk_at(f->blks, i);
		dump_block(b);
	}
	printf("\n");
}
