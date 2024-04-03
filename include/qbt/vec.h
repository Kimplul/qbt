#ifndef VEC_H
#define VEC_H

#include <stddef.h>

struct vec {
	size_t n;
	size_t s;
	size_t ns;
	void *buf;
};

struct vec vec_create(size_t s);
void vec_destroy(struct vec *v);
void vec_reset(struct vec *v);

size_t vec_len(struct vec *v);
void *vec_at(struct vec *v, size_t i);
void *vec_pop(struct vec *v);
void vec_append(struct vec *v, void *n);

#define foreach_vec(iter, v) \
	for (size_t iter = 0, __n = vec_len(&v); iter < __n; ++iter)

#define vect_at(type, v, i) \
	*(type *)vec_at(&v, i)

#define vect_pop(type, v) \
	*(type *)vec_pop(&v)

#endif /* VEC_H */
