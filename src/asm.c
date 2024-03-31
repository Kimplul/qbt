#include <stdlib.h>
#include <assert.h>
#include <qbt/abi.h>
#include <qbt/asm.h>

static const char *rname(struct val v)
{
	assert(v.class == REG);
	/** @todo use ABI names? */
	switch (v.r) {
		case RX0: return "x0";
		case RX1: return "x1";
		case RX2: return "x2";
		case RX3: return "x3";
		case RX4: return "x4";
		case RX5: return "x5";
		case RX6: return "x6";
		case RX7: return "x7";
		case RX8: return "x8";
		case RX9: return "x9";
		case RX10: return "x10";
		case RX11: return "x11";
		case RX12: return "x12";
		case RX13: return "x13";
		case RX14: return "x14";
		case RX15: return "x15";
		case RX16: return "x16";
		case RX17: return "x17";
		case RX18: return "x18";
		case RX19: return "x19";
		case RX20: return "x20";
		case RX21: return "x21";
		case RX22: return "x22";
		case RX23: return "x23";
		case RX24: return "x24";
		case RX25: return "x25";
		case RX26: return "x26";
		case RX27: return "x27";
		case RX28: return "x28";
		case RX29: return "x29";
		case RX30: return "x30";
		case RX31: return "x31";
		case RX32: return "x32";
		case RX33: return "x33";
		case RX34: return "x34";
		case RX35: return "x35";
		case RX36: return "x36";
		case RX37: return "x37";
		case RX38: return "x38";
		case RX39: return "x39";
		case RX40: return "x40";
		case RX41: return "x41";
		case RX42: return "x42";
		case RX43: return "x43";
		case RX44: return "x44";
		case RX45: return "x45";
		case RX46: return "x46";
		case RX47: return "x47";
		case RX48: return "x48";
		case RX49: return "x49";
		case RX50: return "x50";
		case RX51: return "x51";
		case RX52: return "x52";
		case RX53: return "x53";
		case RX54: return "x54";
		case RX55: return "x55";
		case RX56: return "x56";
		case RX57: return "x57";
		case RX58: return "x58";
		case RX59: return "x59";
		case RX60: return "x60";
		case RX61: return "x61";
		case RX62: return "x62";
		case RX63: return "x63";
		case RX64: return "x64";
		case RX65: return "x65";
		case RX66: return "x66";
		case RX67: return "x67";
		case RX68: return "x68";
		case RX69: return "x69";
		case RX70: return "x70";
		case RX71: return "x71";
		case RX72: return "x72";
		case RX73: return "x73";
		case RX74: return "x74";
		case RX75: return "x75";
		case RX76: return "x76";
		case RX77: return "x77";
		case RX78: return "x78";
		case RX79: return "x79";
		case RX80: return "x80";
	}

	assert(0 && "illegal register");
	abort();
}

static void save_state(struct fn *f, FILE *o)
{
	/* stack frame:
	 * last frame
	 * ra
	 * s0
	 * ...
	 * local variables
	 * <- sp
	 */
	fprintf(o, "st w fp, -3(sp)\n");
	if (f->has_calls)
		fprintf(o, "st w ra, -6(fp)\n");

	fprintf(o, "mv fp, sp\n");
	fprintf(o, "addi sp, sp, -%zi\n", f->max_callee_save * 3 + 6);

	for (size_t i = 0; i < f->max_callee_save; ++i) {
		fprintf(o, "st w s%zi, -%zi(fp)\n",
				i, 3 * i + 9);
	}
}

static void restore_state(struct fn *f, FILE *o)
{
	for (size_t i = 0; i < f->max_callee_save; ++i) {
		fprintf(o, "ld w s%zi, -%zi(fp)\n",
				i, 3 * i + 9);
	}

	if (f->has_calls)
		fprintf(o, "ld w ra, -6(fp)\n");

	fprintf(o, "ld w fp, -3(fp)\n");
	fprintf(o, "mv sp, fp\n");
}

static void output_move(struct insn n, FILE *o)
{
	fprintf(o, "mv %s, %s\n",
			rname(n.out), rname(n.in[0]));
}

static void output_add(struct insn n, FILE *o)
{
	if (n.in[1].class == REG) {
		fprintf(o, "add %s, %s, %s\n",
				rname(n.out), rname(n.in[0]), rname(n.in[1]));
		return;
	}
	else if (n.in[1].class == IMM) {
		/** @todo fix for values larger than what addi allows, mark one
		 * temporary register reserved for the compiler? */
		fprintf(o, "addi %s, %s, %lli\n",
				rname(n.out), rname(n.in[0]), (long long int)n.in[1].v);
		return;
	}

	assert(0 && "illegal value type for add");
	abort();
}

static void output_sub(struct insn n, FILE *o)
{
	if (n.in[1].class == REG) {
		fprintf(o, "sub %s, %s, %s\n",
				rname(n.out), rname(n.in[0]), rname(n.in[1]));
		return;
	}
	else if (n.in[1].class == IMM) {
		if (n.in[1].v >= 0)
			fprintf(o, "addi %s, %s, -%lli\n",
				rname(n.out), rname(n.in[0]), (long long int)n.in[1].v);
		else /* double negative */
			fprintf(o, "addi %s, %s, %lli\n",
				rname(n.out), rname(n.in[0]), (long long int)n.in[1].v);
		return;
	}

	assert(0 && "illegal value type for sub");
	abort();
}

static void output_copy(struct insn n, FILE *o)
{
	fprintf(o, "li %s, %lli\n",
			rname(n.out), (long long int)n.in[0].v);
}

static void output_call(struct insn n, FILE *o)
{
	if (n.in[0].class == REF) {
		fprintf(o, "call ra, %s\n", n.in[0].s);
		return;
	}
	else if (n.in[0].class == REG) {
		fprintf(o, "jalr ra, 0(%s)\n", rname(n.in[0]));
		return;
	}

	assert(0 && "illegal value type for call");
	abort();
}

static void output_insn(struct insn n, FILE *o)
{
	/* one insn directly matches one or more assembly instructions,
	 * we may be missing out on certain optimizations by not using some kind
	 * of matching here but good enough for now */
	switch (n.type) {
		case MOVE: output_move(n, o); break;
		case ADD: output_add(n, o); break;
		case SUB: output_sub(n, o); break;
		case COPY: output_copy(n, o); break;
		case CALL: output_call(n, o); break;
		default: fprintf(stderr, "unimplemented insn: %s\n", op_str(n.type));
			 abort();
	}
}

static void output_blt(struct blk *b, struct fn *f, FILE *o)
{
	assert(b->s2);
	fprintf(o, "blt %s, %s, .%s.%lli\n",
			rname(b->cmp[0]), rname(b->cmp[1]),
				f->name, (long long int)b->s2->id);
}

static void output_ble(struct blk *b, struct fn *f, FILE *o)
{
	assert(b->s2);
	fprintf(o, "ble %s, %s, .%s.%lli\n",
			rname(b->cmp[0]), rname(b->cmp[1]),
				f->name, (long long int)b->s2->id);
}

static void output_ret(struct fn *f, FILE *o)
{
	restore_state(f, o);
	fprintf(o, "ret ra\n");
}

static void output_j(struct blk *b, struct fn *f, FILE *o)
{
	/* the jump is directly to a following block, no need to do anything */
	if (!b->to)
		return;

	fprintf(o, "j .%s.%lli\n", f->name, (long long int)b->s1->id);
}

static void output_branch(struct blk *b, struct fn *f, FILE *o)
{
	switch (b->btype) {
		case RET: output_ret(f, o); break;
		case J:   output_j(b, f, o); break;
		case BLT: output_blt(b, f, o); break;
		case BLE: output_ble(b, f, o); break;
		default: fprintf(stderr, "unimplemented branch: %s\n", op_str(b->btype));
			 abort();
	}
}

void output(struct fn *f, FILE *o)
{
	fprintf(o, "%s:\n", f->name);
	save_state(f, o);
	foreach_blk(bi, f->blks) {
		struct blk *b = blk_at(f->blks, bi);
		fprintf(o, ".%s.%lli:\n",
				f->name, (long long int)b->id);

		foreach_insn(i, b->insns) {
			struct insn n = insn_at(b->insns, i);
			output_insn(n, o);
		}

		output_branch(b, f, o);
	}
}
