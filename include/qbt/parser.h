#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <qbt/nodes.h>
#include <qbt/vec.h>

struct fn_map {
	const char *id;
	struct fn *fn;
};

struct data_map {
	const char *id;
	struct data *data;
};

struct parser {
	bool failed;
	void *lexer;

	const char *buf;
	const char *fname;

	/* generic index used to count args, params, ret... */
	size_t idx;

	size_t comment_nesting;

	struct vec fns;
	struct vec datas;
	/* the parser owns all strings, could maybe be moved to a helper
	 * resource handler or something in the future */
	struct vec strs;

	/* current function */
	struct fn *f;

	/* current block of current function */
	struct blk *b;
};

struct parser *create_parser();
void parse(struct parser *p, const char *fname, const char *buf);
void destroy_parser(struct parser *p);

#define foreach_fn(iter, v)\
	foreach_vec(iter, v)

#define foreach_data(iter, v)\
	foreach_vec(iter, v)

#define foreach_str(iter, v)\
	foreach_vec(iter, v)

#define fn_at(v, i)\
	vect_at(struct fn_map, v, i)

#define data_at(v, i)\
	vect_at(struct data_map, v, i)

#define str_at(v, i)\
	vect_at(char *, v, i)

#endif /* PARSER_H */
