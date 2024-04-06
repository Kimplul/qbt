#include <assert.h>
#include <stdlib.h>
#include <qbt/abi.h>

static const int64_t ar_map[] = {
	RA0, RA1, RA2, RA3, RA4, RA5, RA6, RA7, RA8, RA9,
	RA10, RA11, RA12, RA13, RA14, RA15, RA16, RA17, RA18, RA19,
	RA20, RA21, RA22, RA23, RA24
};

static struct val nth_ar(int64_t nth)
{
	return reg_val(ar_map[nth]);
}

static struct insn rewrite_param(struct insn n)
{
	assert(n.type == PARAM);
	assert(n.in[1].class == IMM);
	int64_t nth_param = n.in[1].v;
	assert(nth_param >= 0 && nth_param < 25
	       && "stack argument passing not yet supported");

	return insn_create(MOVE, I27, n.out, nth_ar(nth_param), noclass());
}

static struct insn rewrite_retval(struct insn n)
{
	assert(n.type == RETVAL);
	assert(n.in[1].class == IMM);
	int64_t nth_retval = n.in[1].v;
	assert(nth_retval >= 0 && nth_retval < 25);
	n = insn_create(MOVE, I27, n.out, nth_ar(nth_retval), noclass());
	set_insn_flags(&n, CALL_TEARDOWN);
	return n;
}

static struct insn rewrite_arg(struct insn n)
{
	assert(n.type == ARG);
	assert(n.in[1].class == IMM);
	int64_t nth_arg = n.in[1].v;
	assert(nth_arg >= 0 && nth_arg < 25);

	if (n.in[0].class == TMP || n.in[0].class == REG) {
		n = insn_create(MOVE, I27, nth_ar(nth_arg), n.in[0],
		                   noclass());
		set_insn_flags(&n, CALL_SETUP);
		return n;
	}
	else if (n.in[0].class == IMM || n.in[0].class == REF) {
		n = insn_create(COPY, I27, nth_ar(nth_arg), n.in[0],
		                   noclass());
		set_insn_flags(&n, CALL_SETUP);
		return n;
	}

	assert("illegal arg type");
	abort();
}

static struct insn rewrite_retarg(struct insn n)
{
	assert(n.type == RETARG);
	assert(n.in[1].class == IMM);
	int64_t nth_arg = n.in[1].v;
	assert(nth_arg >= 0 && nth_arg < 25);

	if (n.in[0].class == TMP || n.in[0].class == REG)
		return insn_create(MOVE, I27, nth_ar(nth_arg), n.in[0],
		                   noclass());
	else if (n.in[0].class == IMM || n.in[0].class == REF)
		return insn_create(COPY, I27, nth_ar(nth_arg), n.in[0],
		                   noclass());

	assert("illegal retval type");
	abort();
}

void abi0(struct fn *f)
{
	foreach_blk(bi, f->blks) {
		struct blk *b = blk_at(f->blks, bi);
		foreach_insn(i, b->insns) {
			struct insn n = insn_at(b->insns, i);
			if (n.type == PARAM) {
				struct insn p = rewrite_param(n);
				insn_at(b->insns, i) = p;
			}

			else if (n.type == RETVAL) {
				struct insn r = rewrite_retval(n);
				insn_at(b->insns, i) = r;
			}

			else if (n.type == ARG) {
				struct insn a = rewrite_arg(n);
				insn_at(b->insns, i) = a;
			}

			else if (n.type == RETARG) {
				struct insn a = rewrite_retarg(n);
				insn_at(b->insns, i) = a;
			}
			else if (n.type == CALL) {
				set_insn_flags(&n, CALL_SETUP | CALL_TEARDOWN);
				insn_at(b->insns, i) = n;
			}
		}
	}
}

