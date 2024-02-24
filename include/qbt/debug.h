#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

/* quite a lot of DNA shared with ek,
 * should I make a shared library out of this or something? */

struct file_ctx {
	const char *fname;
	const char *fbuf;
};

enum issue_level {
	SRC_INFO,
	SRC_WARN,
	SRC_ERROR,
};

struct src_loc {
	int first_line;
	int last_line;
	int first_col;
	int last_col;
};

struct src_issue {
	enum issue_level level;
	struct src_loc loc;
	struct file_ctx fctx;
};

void src_issue(struct src_issue issue, const char *err_msg, ...);

void internal_error(const char *fmt, ...);
void internal_warn(const char *fmt, ...);

#if DEBUG
/**
 * Print debugging message. Only active if \c DEBUG is defined,
 *
 * @param x Format string. Follows standard printf() formatting.
 */
#define debug(x, ...) \
	do {fprintf(stderr, "debug: " x "\n",##__VA_ARGS__);} while(0)
#else
#define debug(x, ...)
#endif

/**
 * Print error message.
 *
 * @param x Format string. Follows standard printf() formatting.
 */
#define error(x, ...) \
	do {fprintf(stderr, "error: " x "\n",##__VA_ARGS__);} while(0)

/**
 * Print warning message.
 *
 * @param x Format string. Follows standard printf() formatting.
 */
#define warn(x, ...) \
	do {fprintf(stderr, "warn: " x "\n",##__VA_ARGS__);} while(0)

/**
 * Print info message.
 *
 * @param x Format string. Follows standard printf() formatting.
 */
#define info(x, ...) \
	do {fprintf(stderr, "info: " x "\n",##__VA_ARGS__);} while(0)

#endif /* DEBUG_H */
