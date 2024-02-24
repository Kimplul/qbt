#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include <qbt/parser.h>
#include <qbt/debug.h>

static char *read_file(const char *file, FILE *f)
{
	fseek(f, 0, SEEK_END);
	/** @todo check how well standardized this actually is */
	long s = ftell(f);
	if (s == LONG_MAX) {
		error("%s might be a directory", file);
		return NULL;
	}

	fseek(f, 0, SEEK_SET);

	char *buf = malloc(s + 1);
	if (!buf)
		return NULL;

	fread(buf, s + 1, 1, f);
	/* remember terminating null */
	buf[s] = 0;
	return buf;
}

static void usage(FILE *f)
{
	fprintf(f, "qbt <file>.qbt\n");
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		usage(stdout);
		return -1;
	}

	char *fname = argv[1];
	FILE *f = fopen(fname, "rb");
	if (!f) {
		fprintf(stderr, "couldn't open %s\n", fname);
		return -1;
	}

	char *buf = read_file(fname, f);
	fclose(f);

	struct parser *p = create_parser();
	parse(p, fname, buf);

	foreach_fn(i, p->fns) {
		struct fn_map m = fn_at(p->fns, i);
		dump_function(m.fn);
	}

	destroy_parser(p);
	free(buf);
}
