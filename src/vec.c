#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <qbt/vec.h>

struct vec vec_create(size_t ns)
{
	return (struct vec) {
		       .n = 0,
		       .s = 1,
		       .ns = ns,
		       .buf = malloc(ns),
	};
}

void vec_reset(struct vec *v)
{
	v->n = 0;
}

size_t vec_len(struct vec *v)
{
	return v->n;
}

void *vec_at(struct vec *v, size_t i)
{
	assert(i < v->n && "out of vector bounds");
	return v->buf + i * v->ns;
}

void *vec_pop(struct vec *v)
{
	assert(v->n && "attempting to pop empty vector");
	v->n--;
	return v->buf + v->n * v->ns;
}

void vec_append(struct vec *v, void *n)
{
	v->n++;
	if (v->n >= v->s) {
		v->s *= 2;
		v->buf = realloc(v->buf, v->s * v->ns);
	}

	void *p = vec_at(v, v->n - 1);
	memcpy(p, n, v->ns);
}

void vec_destroy(struct vec *v)
{
	free(v->buf);
}

void vec_insert(struct vec *v, void *n, size_t i)
{
	assert(i <= vec_len(v));
	if (i == vec_len(v))
		return vec_append(v, n);

	v->n++;
	if (v->n >= v->s) {
		v->s *= 2;
		v->buf = realloc(v->buf, v->s * v->ns);
	}

	void *p = vec_at(v, i);
	size_t elems = v->n - i - 1;
	size_t bytes = elems * v->ns;
	memmove(p + v->ns, p, bytes);
	memcpy(p, n, v->ns);
}
