#ifndef NODES_H
#define NODES_H

#include <stdint.h>
#include <stdbool.h>

#include <qbt/vec.h>

enum insn_type {
	ADD,
	SUB,
	MUL,
	DIV,
	REM,
	CALL,
	LABEL,
	STORE,
	LOAD,
	ALLOC,
	COPY,
	MOVE,
	EQ,
	NE,
	LE,
	GE,
	LT,
	GT,
	NOT,
	NEG,
	LSHIFT,
	RSHIFT,
	BEQ,
	BNE,
	BLE,
	BGE,
	BLT,
	BGT,
	J,
	RET,
	ARG,
	RETARG,
	PARAM,
	RETVAL,
};

#define FOREACH_INSN_TYPE(M) \
	M(ADD) \
	M(SUB) \
	M(MUL) \
	M(DIV) \
	M(REM) \
	M(CALL) \
	M(LABEL) \
	M(STORE) \
	M(LOAD) \
	M(ALLOC) \
	M(COPY) \
	M(MOVE) \
	M(EQ) \
	M(NE) \
	M(LE) \
	M(GE) \
	M(LT) \
	M(GT) \
	M(NOT) \
	M(NEG) \
	M(LSHIFT) \
	M(RSHIFT) \
	M(BEQ) \
	M(BNE) \
	M(BLE) \
	M(BGE) \
	M(BLT) \
	M(BGT) \
	M(J) \
	M(ARG) \
	M(RETARG) \
	M(PARAM) \
	M(RET) \
	M(RETVAL) \

static inline const char *op_str(enum insn_type n) {
#define CASE(I) case I: return #I;
	switch (n) {
	FOREACH_INSN_TYPE(CASE);
	}
#undef CASE
	return "unknown";
}

enum val_class {
	REG,
	TMP,
	IMM,
	MEM,
	REF,
	NOCLASS,
};

enum val_type {
	NOTYPE, I9, I27
};

struct val {
	enum val_class class;
	int64_t r;
	union {
		int64_t v;
		const char *s;
	};
};

struct insn {
	enum insn_type type;
	enum val_type vtype;
	struct val out;
	struct val in[2];
};

struct blk {
	const char *name;
	int64_t id;
	enum insn_type btype;
	struct val cmp[2];
	struct blk *s1;
	struct blk *s2;
	struct vec insns;
	/* input parameters, tmp values effectively */
	struct vec params;
	/* arguments for s1 */
	struct vec args1;
	/* arguments for s2 (if any) */
	struct vec args2;

	/* used to temporarily store label targets */
	const char *to;
	/* used by some algorithms to mark visited */
	int visited;
};

struct fn {
	const char *name;
	size_t ntmp;
	size_t nblk;
	size_t max_callee_save;
	bool has_calls;
	struct vec blks;
	struct vec labels;
	struct vec tmps;
};

static inline bool hasclass(struct val v)
{
	return v.class != NOCLASS;
}

static inline bool hasnoclass(struct val v)
{
	return v.class == NOCLASS;
}

static inline struct val noclass()
{
	return (struct val){
		       .class = NOCLASS,
		       .r = 0,
		       .v = 0,
	};
}

static inline struct val reg_val(int64_t r)
{
	return (struct val){
		       .class = REG,
		       .r = r
	};
}

static inline struct val imm_ref(const char *s)
{
	return (struct val){
		       .class = REF,
		       .r = 0,
		       .s = s,
	};
}

static inline struct val mem_val(int64_t base, int64_t offset)
{
	return (struct val){
		       .class = MEM,
		       .r = base,
		       .v = offset,
	};
}

static inline struct val imm_val(int64_t imm, int64_t type)
{
	return (struct val) {
		       .class = IMM,
		       .r = type,
		       .v = imm,
	};
}

static inline struct val tmp_val(int64_t t)
{
	return (struct val) {
		       .class = TMP,
		       .r = t,
		       .v = 0,
	};
}

static inline bool same_val(struct val v1, struct val v2)
{
	if (v1.class != v2.class)
		return false;

	switch (v1.class) {
	case REG: return v1.r == v2.r;
	case TMP: return v1.r == v2.r;
	case IMM: return v1.v == v2.v;
	case MEM: return v1.v == v2.v;
	case REF: return v1.r == v2.r && v1.v == v2.v;
	case NOCLASS: return true;
	}

	/* shouldn't be reachable */
	return false;
}

static inline struct insn insn_create(enum insn_type o, enum val_type t,
                                      struct val r, struct val a0,
                                      struct val a1)
{
	return (struct insn) {
		       .type = o,
		       .vtype = t,
		       .out = r,
		       .in = {a0, a1}
	};
}

int64_t idalloc(struct fn *f, const char *id);
int64_t idmatch(struct fn *f, const char *id);

void insadd(struct blk *b, enum insn_type o, enum val_type t, struct val r,
            struct val a0, struct val a1);

void finish_block(struct blk *b, enum insn_type cmp, struct val a0,
                  struct val a1, const char *label);
struct blk *new_block(struct fn *f);
void destroy_block(struct blk *b);

void finish_function(struct fn *f, const char *name);
struct fn *new_function();
void dump_function(struct fn *f);
void destroy_function(struct fn *f);

void new_label(struct fn *f, struct blk *b, const char *name);

static inline bool empty_block(struct blk *b) {
	return vec_len(&b->insns) == 0;
}

struct tmp_map {
	const char *id;
	int64_t v;
};

struct label_map {
	const char *id;
	struct blk *b;
};

#define val_at(v, i) \
	vect_at(struct val, v, i)

#define foreach_val(iter, vals) \
	foreach_vec(iter, vals)

#define tmp_at(v, i) \
	vect_at(struct tmp_map, v, i)

#define foreach_tmp(iter, tmps) \
	foreach_vec(iter, tmps)

#define blk_at(v, i) \
	vect_at(struct blk *, v, i)

#define blk_back(v) \
	vect_at(struct blk *, v, vec_len(&v) - 1)

#define blk_pop(v) \
	vect_pop(struct blk *, v)

#define foreach_blk(iter, blocks) \
	foreach_vec(iter, blocks)

#define foreach_blk_param(iter, block_params) \
	foreach_vec(iter, block_params)

#define blk_param_at(v, i) \
	vect_at(struct val, v, i)

#define label_at(v, i) \
	vect_at(struct label_map, v, i)

#define foreach_label(iter, labels) \
	foreach_vec(iter, labels)

#define insn_at(v, i) \
	vect_at(struct insn, v, i)

#define foreach_insn(iter, insns) \
	foreach_vec(iter, insns)

#endif /* NODES_H */
