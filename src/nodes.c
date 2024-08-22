#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <qbt/nodes.h>
#include <qbt/debug.h>
#include <qbt/vec.h>

void insadd(struct blk *b, enum insn_type o, enum val_type t, struct val r,
            struct val a0, struct val a1, long long v)
{
	struct insn i = insn_create(o, t, r, a0, a1, v);
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

void finish_block(struct blk *b, enum insn_type cmp, struct val a0,
                  struct val a1, const char *label)
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
	b->visited = 0;
	b->insns = vec_create(sizeof(struct insn));
	b->params = vec_create(sizeof(struct val));
	b->args1 = vec_create(sizeof(struct val));
	b->args2 = vec_create(sizeof(struct val));
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

	last_blk = blk_back(f->blks);
	/* somewhat arbitrary restriction but I guess but makes the
	 * implementation a bit simpler */
	assert(last_blk->btype == RET);
	last_blk->s1 = NULL;
	last_blk->s2 = NULL;

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
	f->has_calls = false;
	f->max_callee_save = 0;
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
	vec_destroy(&b->params);
	vec_destroy(&b->args1);
	vec_destroy(&b->args2);
	free(b);
}

void new_label(struct fn *f, struct blk *b, const char *name)
{
	vec_append(&f->labels, &(struct label_map){.id = name, .b = b});
}

bool return_blk(struct blk *b)
{
	return b->btype == RET;
}

#if defined(DEBUG)
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
	fputs("//\t", stdout);

	if (hasclass(i.out)) {
		dump_val(i.out);
		putchar(' ');
	}

	printf("%s", op_str(i.type));

	if (hasclass(i.in[0])) {
		putchar(' ');
		dump_val(i.in[0]);
	}

	if (hasclass(i.in[1])) {
		putchar(' ');
		dump_val(i.in[1]);
	}

	/* crude, but works for now */
	if (i.v)
		printf(" %lli ", i.v);

	if (i.flags)
		putchar('*');

	putchar('\n');
}

static void dump_block(struct blk *b)
{
	printf("//\t/*** block %lld ", (long long)b->id);
	if (b->name) printf("\"%s\" ", b->name);

	putchar('(');
	foreach_blk_param(pi, b->params) {
		struct val v = blk_param_at(b->params, pi);
		dump_val(v);
		fputs(", ", stdout);
	}
	fputs(") ", stdout);

	fputs("***/\n", stdout);

	foreach_insn(i, b->insns) {
		struct insn n = insn_at(b->insns, i);
		dump_insn(n);
	}

	if (return_blk(b)) {
		fputs("//\tRETURN\n", stdout);
		return;
	}

	assert(b->s2);
	struct blk *s2 = b->s2;
	printf("//\t%s ", op_str(b->btype));
	dump_val(b->cmp[0]);
	putchar(' ');
	dump_val(b->cmp[1]);
	printf(" -> %lli (", (long long)s2->id);
	foreach_blk_param(pi, b->args2) {
		struct val a = blk_param_at(b->args2, pi);
		dump_val(a);
		fputs(", ", stdout);
	}
	printf("), else %lli (", (long long)b->s1->id);
	foreach_blk_param(pi, b->args1) {
		struct val a = blk_param_at(b->args1, pi);
		dump_val(a);
		fputs(", ", stdout);
	}
	fputs(")\n", stdout);
}

void dump_function(struct fn *f) {
	printf("//\t/*** function %s ***/\n", f->name);
	foreach_blk(i, f->blks) {
		struct blk *b = blk_at(f->blks, i);
		dump_block(b);
	}
	putchar('\n');
}
#endif /* DEBUG */
