/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

%{

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <qbt/parser.h>
#include <qbt/debug.h>
#include <qbt/nodes.h>

%}

%locations

%define parse.trace
%define parse.error verbose
%define api.pure full
%define lr.type ielr

%lex-param {void *scanner} {struct parser *parser}
%parse-param {void *scanner} {struct parser* parser}

%union {
	struct val val;
	enum val_type type;
	int64_t integer;
	char *str;
};

%token <integer> INT
%token <str> STRING
%token <str> ID

%token LEXASSIGN "="
%token LEXCOLON ":"
%token LEXBANG "!"
%token LEXSTAR "*"
%token LEXDIV "/"
%token LEXREM "%"
%token LEXMINUS "-"
%token LEXPLUS "+"
%token LEXAND "&"
%token LEXLT "<"
%token LEXGT ">"
%token LEXLE "<="
%token LEXGE ">="
%token LEXNE "!="
%token LEXEQ "=="
%token LEXCOMMA ","
%token LEXLPAREN "("
%token LEXRPAREN ")"
%token LEXLBRACE "{"
%token LEXRBRACE "}"
%token LEXFATARROW "=>"
%token LEXTHINARROW "->"
%token LEXTO ">>"
%token LEXFROM "<<"
%token LEXSEMI ";"

%token LEXI9 "i9"
%token LEXI27 "i27"

%nterm <val> mem_loc
%nterm <str> mem_base
%nterm <integer> mem_off
%nterm <integer> int
%nterm <type> type

%nterm <str> id addr local label
%nterm <val> arg
%nterm <val> ret

%nterm <integer> placeholder

%{

/** Modifies the signature of yylex to fit our parser better. */
#define YY_DECL int yylex(YYSTYPE *yylval, YYLTYPE *yylloc, \
	                  void *yyscanner, struct parser *parser)

/**
 * Declare yylex.
 *
 * @param yylval Bison current value.
 * @param yylloc Bison location info.
 * @param yyscanner Flex scanner.
 * @param parser Current parser state.
 * @return \c 0 when succesful, \c 1 otherwise.
 * More info on yylex() can be found in the flex manual.
 */
YY_DECL;

/**
 * Convert bison location info to our own source location info.
 *
 * @param yylloc Bison location info.
 * @return Internal location info.
 */
static struct src_loc src_loc(YYLTYPE yylloc);

/**
 * Print parsing error.
 * Automatically called by bison.
 *
 * @param yylloc Location of error.
 * @param lexer Lexer.
 * @param parser Parser state.
 * @param msg Message to print.
 */
static void yyerror(YYLTYPE *yylloc, void *lexer,
		struct parser *parser, const char *msg);

/**
 * Try to convert escape code to its actual value.
 * I.e. '\n' -> 0x0a.
 *
 * @param c Escape character without backslash.
 * @return Corresponding value.
 */
static long long match_escape(char c);

/**
 * Similar to strdup() but skips quotation marks that would
 * otherwise be included.
 * I.e. "something" -> something.
 *
 * @param s String to clone, with quotation marks surrounding it.
 * @return Identical string but without quotation marks around it.
 */
static char *clone_string(const char *s);

static inline struct val do_idalloc(struct parser *p, const char *id)
{
	int64_t t = idalloc(p->f, id);
	return tmp_val(t);
}

static inline struct val do_idtoval(struct parser *p, const char *id)
{
	int64_t t = idmatch(p->f, id);
	if (t < 0) {
		error("no such temporary: %s\n", id);
		abort();
	}

	return tmp_val(t);
}

static inline void do_new_block(struct parser *p,
	enum insn_type type,
	struct val a0,
	struct val a1,
	const char *label)
{
	finish_block(p->b, type, a0, a1, label);
	p->b = p->b->s1 = new_block(p->f);
}

static inline void do_insadd(struct parser *p,
	enum insn_type cmp,
	enum val_type t,
	struct val r,
	struct val a0,
	struct val a1)
{
	insadd(p->b, cmp, t, r, a0, a1);
}

static inline void do_new_function(struct parser *p, const char *name)
{
	finish_function(p->f, name);
	vec_append(&p->fns, &(struct fn_map){.id = name, .fn = p->f});
	p->f = new_function();
	p->b = blk_at(p->f->blks, 0);
}

static inline char *do_strdup(struct parser *p, const char *s)
{
	char *new = strdup(s);
	vec_append(&p->strs, &new);
	return new;
}

static inline char *do_strclone(struct parser *p, const char *s)
{
	char *new = clone_string(s);
	vec_append(&p->strs, &new);
	return new;
}

static inline void do_new_label(struct parser *p, const char *s)
{
	new_label(p->f, p->b, s);
}

static inline size_t do_cur_insn(struct parser *p)
{
	return vec_len(&p->b->insns);
}

static inline void do_ins_replace(struct parser *p, size_t i, struct insn n)
{
	insn_at(p->b->insns, i) = n;
}

#define INS_REPLACE(i, n)\
	do_ins_replace(parser, i, n)

#define CUR_INSN()\
	do_cur_insn(parser)

#define INSADD(o, t, r, a0, a1)\
	do_insadd(parser, o, t, r, a0, a1)

#define IDALLOC(i)\
	do_idalloc(parser, i)

#define IDTOVAL(i)\
	do_idtoval(parser, i)

#define NEW_BLOCK(c, a0, a1, l)\
	do_new_block(parser, c, a0, a1, l)

#define NEW_FUNCTION(n)\
	do_new_function(parser, n)

#define DUP_STR(s)\
	do_strdup(parser, s)

#define CLONE_STR(s)\
	do_strclone(parser, s)

#define NEW_LABEL(s)\
	do_new_label(parser, s)

%}

%start input;
%%

id
	: ID { $$ = DUP_STR($1); }
	| STRING { $$ = CLONE_STR($1); }

int
	: INT

type
	: "i9" { $$ = I9; }
	| "i27" { $$ = I27; }

const
	: "i9" int
	| "i27" int

consts
	: const "," consts
	| const ","
	| const

opt_consts
	: consts
	| {}

data
	: id "=" "{" opt_consts "}"

param
	: type id {
		struct val t = IDALLOC($[id]);
		INSADD(PARAM, $[type], t, noclass(), imm_val(parser->idx++, I27));
	}

params
	: param "," params
	| param ","
	| param

opt_params
	: params
	| {}

label
	: id ":" {
		if (empty_block(parser->b)) {
			parser->b->name = $[id];
		} else {
			NEW_BLOCK(J, noclass(), noclass(), NULL);
			parser->b->name = $[id];
		}
		NEW_LABEL($[id]);
	}

arg
	: id {
		$$ = IDTOVAL($[id]);
	}
	| type int {
		$$ = imm_val($[int], $[type]);
	}

arith
	: type id "=" arg "+" arg {
		struct val t = IDALLOC($[id]);
		INSADD(ADD, $[type], t, $4, $6);
	}
	| type id "=" arg "-" arg {
		struct val t = IDALLOC($[id]);
		INSADD(SUB, $[type], t, $4, $6);
	}
	| type id "=" arg "*" arg {
		struct val t = IDALLOC($[id]);
		INSADD(MUL, $[type], t, $4, $6);
	}
	| type id "=" arg "/" arg {
		struct val t = IDALLOC($[id]);
		INSADD(DIV, $[type], t, $4, $6);
	}
	| type id "=" arg "%" arg {
		struct val t = IDALLOC($[id]);
		INSADD(REM, $[type], t, $4, $6);
	}

addr
	: "&" id { $$ = $2; }

imm
	: type id "=" int {
		struct val t = IDALLOC($[id]);
		INSADD(COPY, $[type], t, imm_val($[int], I27), noclass());
	}
	| type id "=" addr {
		struct val t = IDALLOC($[id]);
		INSADD(COPY, $[type], t, imm_ref($[addr]), noclass());
	}

move
	: type id "=" id {
		struct val t = IDALLOC($2);
		struct val f = IDTOVAL($4);
		INSADD(MOVE, $[type], t, f, noclass());
	}

mem_base
	: id

mem_off
	: int

mem_loc
	: "(" mem_base mem_off ")" {$$ = mem_val(IDTOVAL($[mem_base]).r, $[mem_off]);}

mem
	: type id "<<" mem_loc {
		struct val t = IDALLOC($[id]);
		INSADD(LOAD, $[type], t, $[mem_loc], noclass());
	}
	| id ">>" type mem_loc {
		struct val t = IDALLOC($[id]);
		INSADD(STORE, $[type], noclass(), t, $[mem_loc]);
	}

stack
	: type id "=" "alloc" int {
		struct val t = IDALLOC($[id]);
		INSADD(ALLOC, $[type], t, imm_val($[int], I27), noclass());
	}

cond
	: type id "=" arg "==" arg {
		struct val t = IDALLOC($[id]);
		INSADD(EQ, $[type], t, $4, $6);
	}
	| type id "=" arg "!=" arg {
		struct val t = IDALLOC($[id]);
		INSADD(NE, $[type], t, $4, $6);
	}
	| type id "=" arg "<=" arg {
		struct val t = IDALLOC($[id]);
		INSADD(LE, $[type], t, $4, $6);
	}
	| type id "=" arg ">=" arg {
		struct val t = IDALLOC($[id]);
		INSADD(GE, $[type], t, $4, $6);
	}
	| type id "=" arg "<"  arg {
		struct val t = IDALLOC($[id]);
		INSADD(LT, $[type], t, $4, $6);
	}
	| type id "=" arg ">"  arg {
		struct val t = IDALLOC($[id]);
		INSADD(GT, $[type], t, $4, $6);
	}

logic
	: type id "=" "!" arg {
		struct val t = IDALLOC($[id]);
		INSADD(NOT, $[type], t, $5, noclass());
	}
	| type id "=" "-" arg {
		struct val t = IDALLOC($[id]);
		INSADD(NEG, $[type], t, $5, noclass());
	}
	| type id "=" arg "<<" arg {
		struct val t = IDALLOC($[id]);
		INSADD(LSHIFT, $[type], t, $4, $6);
	}
	| type id "=" arg ">>" arg {
		struct val t = IDALLOC($[id]);
		INSADD(RSHIFT, $[type], t, $4, $6);
	}

local
	: id

branch
	: arg "==" arg "->" local {
		NEW_BLOCK(BEQ, $1, $3, $[local]);
	}
	| arg "!=" arg "->" local {
		NEW_BLOCK(BNE, $1, $3, $[local]);
	}
	| arg "<=" arg "->" local {
		NEW_BLOCK(BLE, $1, $3, $[local]);
	}
	| arg ">=" arg "->" local {
		NEW_BLOCK(BGE, $1, $3, $[local]);
	}
	| arg "<"  arg "->" local {
		NEW_BLOCK(BLT, $1, $3, $[local]);
	}
	| arg ">"  arg "->" local {
		NEW_BLOCK(BGT, $1, $3, $[local]);
	}
	| "->" local {
		NEW_BLOCK(J, noclass(), noclass(), $[local]);
	}

ret
	: id {
		$$ = IDALLOC($[id]);
	}

call_ret
	: ret {
		INSADD(RETVAL, NOTYPE, $[ret], noclass(), imm_val(parser->idx++, I27));
	}

call_rets
	: call_ret "," call_rets
	| call_ret ","
	| call_ret

opt_call_rets
	: call_rets
	| {}

call_arg
	: arg {
		INSADD(ARG, NOTYPE, noclass(), $[arg], imm_val(parser->idx++, I27));
	}

call_args
	: call_arg "," call_args
	| call_arg ","
	| call_arg

opt_call_args
	: call_args
	| {}

/* empty rule for starting counts */
reset_index
	: {parser->idx = 0;}

placeholder
	: {
		$$ = CUR_INSN();
		INSADD(CALL, NOTYPE, noclass(), noclass(), noclass());
	}

call
	: addr reset_index "(" opt_call_args ")"
	  "=>" placeholder reset_index "(" opt_call_rets ")" {
		/* kind of hacky but works */
		INS_REPLACE($[placeholder],
			insn_create(CALL, NOTYPE,
				noclass(), imm_ref($[addr]), noclass()));
	}
	| id reset_index "(" opt_call_args ")"
	  "=>" placeholder reset_index "(" opt_call_rets ")" {
		INS_REPLACE($[placeholder],
			insn_create(CALL, NOTYPE,
				noclass(), IDTOVAL($[id]), noclass()));
	  }

proc_ret
	: id {
		INSADD(RETARG, NOTYPE, noclass(), IDTOVAL($[id]), imm_val(parser->idx++, I27));
	}

proc_rets
	: proc_ret "," proc_rets
	| proc_ret ","
	| proc_ret

opt_proc_rets
	: proc_rets
	| {}

return
	: "=>" reset_index "(" opt_proc_rets ")" {
		NEW_BLOCK(RET, noclass(), noclass(), NULL);
	}

insn
	: arith
	| imm
	| move
	| mem
	| stack
	| cond
	| logic
	| branch
	| call
	| return

body
	: insn ";" body
	| label body
	| label
	| insn ";"

/** @todo add in return type checking? */
function
	: id reset_index "(" opt_params ")" "{" body "}" {
		NEW_FUNCTION($[id]);
	}

top
	: data
	| function

unit
	: top unit
	| top

input
	: unit
	| /* empty */

%%

#include "gen_lexer.inc"

static struct src_loc src_loc(YYLTYPE yylloc)
{
	struct src_loc loc;
	loc.first_line = yylloc.first_line;
	loc.last_line = yylloc.last_line;
	loc.first_col = yylloc.first_column;
	loc.last_col = yylloc.last_column;
	return loc;
}

static void yyerror(YYLTYPE *yylloc, void *lexer,
		struct parser *parser, const char *msg)
{
	(void)lexer;

	struct src_issue issue;
	issue.level = SRC_ERROR;
	issue.loc = src_loc(*yylloc);
	issue.fctx.fbuf = parser->buf;
	issue.fctx.fname = parser->fname;
	src_issue(issue, msg);
}

static long long match_escape(char c)
{
	switch (c) {
	case '\'': return '\'';
	case '\\': return '\\';
	case 'a': return '\a';
	case 'b': return '\b';
	case 'f': return '\f';
	case 'n': return '\n';
	case 'r': return '\r';
	case 't': return '\t';
	case 'v': return '\v';
	}

	return c;
}

static char *clone_string(const char *str)
{
	const size_t len = strlen(str) + 1;
	char *buf = malloc(len);
	if (!buf) {
		/* should probably try to handle the error in some way... */
		internal_error("failed allocating buffer for string clone");
		return NULL;
	}

	/* skip quotation marks */
	size_t j = 0;
	for (size_t i = 1; i < len - 2; ++i) {
		char c = str[i];

		if (c == '\\')
			c = match_escape(str[++i]);

		buf[j++] = c;
	}

	buf[j] = 0;
	return buf;

}

struct parser *create_parser()
{
	return calloc(1, sizeof(struct parser));
}

void destroy_parser(struct parser *p)
{
	foreach_fn(i, p->fns) {
		struct fn_map m = fn_at(p->fns, i);
		/* I assume the 'extra' empty function is never appended to the
		 * function vector */
		assert(m.fn != p->f);
		destroy_function(m.fn);
	}
	vec_destroy(&p->fns);
	destroy_function(p->f);

	foreach_str(i, p->strs) {
		char *s = str_at(p->strs, i);
		free(s);
	}
	vec_destroy(&p->strs);

/* data isn't really handled yet properly
	foreach_data(i, p->datas) {
		struct data_map m = data_at(p->datas, i);
		destroy_data(m.data);
		free((void *)m.id);
	}
	*/

	yylex_destroy(p->lexer);
	free(p);
}

void parse(struct parser *p, const char *fname, const char *buf)
{
	p->fname = fname;
	p->buf = buf;

	p->fns = vec_create(sizeof(struct fn_map));
	p->strs = vec_create(sizeof(char *));

	p->f = new_function();
	p->b = blk_at(p->f->blks, 0);
	p->comment_nesting = 0;

	p->failed = false;

	yylex_init(&p->lexer);
	yy_scan_string(buf, p->lexer);
	yyparse(p->lexer, p);
}
