#include "all.h"

enum {
	Ki = -1, /* matches Kw and Kl */
	Ka = -2, /* matches all classes */
};

static struct {
	short op;
	short cls;
	char *asm;
} omap[] = {
	{ Oadd,    Ki, "add%i %=, %0, %1" },
	{ Oadd,    Ka, "fadd.s %=, %0, %1" },
	{ Osub,    Ki, "sub%i %=, %0, %1" },
	{ Osub,    Ka, "fsub.s %=, %0, %1" },
	{ Oneg,    Ki, "neg %=, %0" },
	{ Oneg,    Ka, "fneg.s %=, %0" },
	{ Odiv,    Ki, "div %=, %0, %1" },
	{ Odiv,    Ka, "fdiv.s %=, %0, %1" },
	{ Orem,    Ki, "rem %=, %0, %1" },
	{ Orem,    Kl, "rem %=, %0, %1" },
	{ Oudiv,   Ki, "divu %=, %0, %1" },
	{ Ourem,   Ki, "remu %=, %0, %1" },
	{ Omul,    Ki, "mul %=, %0, %1" },
	{ Omul,    Ka, "fmul.s %=, %0, %1" },
	{ Oand,    Ki, "and%i %=, %0, %1" },
	{ Oor,     Ki, "or%i %=, %0, %1" },
	{ Oxor,    Ki, "xor%i %=, %0, %1" },
	{ Osar,    Ki, "sra%i %=, %0, %1" },
	{ Oshr,    Ki, "srl%i %=, %0, %1" },
	{ Oshl,    Ki, "sll%i %=, %0, %1" },
	{ Ocsltl,  Ki, "slt%i %=, %0, %1" },
	{ Ocultl,  Ki, "slt%iu %=, %0, %1" },
	{ Oceqs,   Ki, "feq.s %=, %0, %1" },
	{ Ocges,   Ki, "flt.s %=, %1, %0" },
	{ Ocgts,   Ki, "fle.s %=, %1, %0" },
	{ Ocles,   Ki, "fle.s %=, %0, %1" },
	{ Oclts,   Ki, "flt.s %=, %0, %1" },
	{ Oceqd,   Ki, "feq.s %=, %0, %1" },
	{ Ocged,   Ki, "flt.s %=, %1, %0" },
	{ Ocgtd,   Ki, "fle.s %=, %1, %0" },
	{ Ocled,   Ki, "fle.s %=, %0, %1" },
	{ Ocltd,   Ki, "flt.s %=, %0, %1" },
	{ Ostoreb, Kw, "sb %0, %M1" },
	{ Ostoreh, Kw, "sh %0, %M1" },
	{ Ostorew, Kw, "sw %0, %M1" },
	{ Ostorel, Ki, "sw %0, %M1" },
	{ Ostores, Kw, "fsw %0, %M1" },
	{ Ostored, Kw, "fsw %0, %M1" },
	{ Oloadsb, Ki, "lb %=, %M0" },
	{ Oloadub, Ki, "lbu %=, %M0" },
	{ Oloadsh, Ki, "lh %=, %M0" },
	{ Oloaduh, Ki, "lhu %=, %M0" },
	{ Oloadsw, Ki, "lw %=, %M0" },
	/* riscv64 always sign-extends 32-bit
	 * values stored in 64-bit registers
	 */
	{ Oloaduw, Kw, "lw %=, %M0" },
	{ Oloaduw, Kl, "lwu %=, %M0" },
	{ Oload,   Kw, "lw %=, %M0" },
	{ Oload,   Kl, "lw %=, %M0" },
	{ Oload,   Ka, "flw %=, %M0" },
	{ Oextsb,  Ki, "slli %=, %0, 24\n\tsrai %=, %=, 24" },
	{ Oextub,  Ki, "zext.b %=, %0" },
	{ Oextsh,  Ki, "sext.h %=, %0" },
	{ Oextuh,  Ki, "slli %=, %0, 16\n\tsrli %=, %=, 16" }, 
	{ Oextsw,  Kl, "sext.w %=, %0" },
	{ Oextuw,  Kl, "zext.w %=, %0" },
	{ Otruncd, Ks, "fmv.s %=, %0" }, 
	{ Oexts,   Kd, "fmv.s %=, %0" },
	{ Ostosi,  Kw, "fcvt.w.s %=, %0" },
	{ Ostosi,  Kl, "fcvt.l.s %=, %0, rtz" },
	{ Ostoui,  Kw, "fcvt.wu.s %=, %0" },
	{ Ostoui,  Kl, "fcvt.lu.s %=, %0, rtz" },
	{ Odtosi,  Kw, "fcvt.w.s %=, %0" },
	{ Odtosi,  Kl, "fcvt.l.s %=, %0, rtz" },
	{ Odtoui,  Kw, "fcvt.wu.d %=, %0" },
	{ Oswtof,  Ka, "fcvt.s.w %=, %0" },
	{ Ouwtof,  Ka, "fcvt.s.wu %=, %0" },
	{ Osltof,  Ka, "fcvt.%k.l %=, %0" },
	{ Oultof,  Ka, "fcvt.%k.lu %=, %0" },
	{ Ocast,   Kw, "fmv.x.w %=, %0" },
	{ Ocast,   Kl, "fmv.x.w %=, %0" },
	{ Ocast,   Ks, "fmv.w.x %=, %0" },
	{ Ocast,   Kd, "fmv.w.x %=, %0" },
	{ Ocopy,   Ki, "mv %=, %0" },
	{ Ocopy,   Ka, "fmv.s %=, %0" },
	{ Oswap,   Ki, "mv %?, %0\n\tmv %0, %1\n\tmv %1, %?" },
	{ Oswap,   Ka, "fmv.s %?, %0\n\tfmv.s %0, %1\n\tfmv.s %1, %?" },
	{ Oreqz,   Ki, "seqz %=, %0" },
	{ Ornez,   Ki, "snez %=, %0" },
	{ Ocall,   Kw, "jalr %0" },
	{ NOp, 0, 0 }
};

static char *rname[] = {
	[FP] = "x8",
	[SP] = "x2", 
	[GP] = "x3",
	[TP] = "x4",
	[RA] = "x1",
	[T6] = "x31",
	[T0] = "x5", "x6", "x7", "x28", "x29", "x30",
	[A0] = "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17",
	[S1] = "x9", "x18", "x19", "x20", "x21", "x22", "x23", "x24",
		   "x25", "x26", "x27",
	[FT0] = "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
			"f28", "f29", "f30",
	[FA0] = "f10", "f11", "f12", "f13", "f14", "f15", "f16", "f17",
	[FS0] = "f8", "f9", "f18", "f19", "f20", "f21", "f22", "f23",
			"f24", "f25", "f26", "f27",
	[FT11] = "f31",
};

static int64_t
slot(int s, Fn *fn)
{
	s = ((int32_t)s << 3) >> 3;
	assert(s <= fn->slot);
	if (s < 0)
		return 4 * -s;
	else
		return -4 * (fn->slot - s);
}

static void
emitaddr(Con *c, FILE *f)
{
	char off[32], *p;

	if (c->bits.i)
		sprintf(off, "+%"PRIi64, c->bits.i);
	else
		off[0] = 0;
	p = c->local ? ".L" : "";
	fprintf(f, "%s%s%s", p, str(c->label), off);
}

static void
emitf(char *s, Ins *i, Fn *fn, FILE *f)
{
	static char clschr[] = {'w', 'l', 's', 'd'};
	Ref r;
	int k, c;
	Con *pc;
	int64_t offset;

	fputc('\t', f);
	for (;;) {
		k = i->cls;
		while ((c = *s++) != '%')
			if (!c) {
				fputc('\n', f);
				return;
			} else
				fputc(c, f);
		switch ((c = *s++)) {
		default:
			die("invalid escape");
		case '?':
			if (KBASE(k) == 0)
				fputs("x31", f);
			else
				fputs("f31", f);;
			break;
		case 'k':
			if (i->cls != Kl)
				fputc(clschr[i->cls], f);
			break;
		case 'i':
			if (!isreg(i->arg[1]))
				fputc('i', f);
			break;
		case '=':
		case '0':
			r = c == '=' ? i->to : i->arg[0];
			assert(isreg(r));
			fputs(rname[r.val], f);
			break;
		case '1':
			r = i->arg[1];
			switch (rtype(r)) {
			default:
				die("invalid second argument");
			case RTmp:
				assert(isreg(r));
				fputs(rname[r.val], f);
				break;
			case RCon:
				pc = &fn->con[r.val];
				assert(pc->type == CBits);
				assert(pc->bits.i >= -2048 && pc->bits.i < 2048);
				fprintf(f, "%d", (int)pc->bits.i);
				break;
			}
			break;
		case 'M':
			c = *s++;
			assert(c == '0' || c == '1');
			r = i->arg[c - '0'];
			switch (rtype(r)) {
			default:
				die("invalid address argument");
			case RTmp:
				fprintf(f, "0(%s)", rname[r.val]);
				break;
			case RCon:
				pc = &fn->con[r.val];
				assert(pc->type == CAddr);
				emitaddr(pc, f);
				if (isstore(i->op)
				|| (isload(i->op) && KBASE(i->cls) == 1)) {
					/* store (and float load)
					 * pseudo-instructions need a
					 * temporary register in which to
					 * load the address
					 */
					fprintf(f, ", x31");
				}
				break;
			case RSlot:
				offset = slot(r.val, fn);
				assert(offset >= -2048 && offset <= 2047);
				fprintf(f, "%d(x8)", (int)offset);
				break;
			}
			break;
		}
	}
}

static void
loadcon(Con *c, int r, int k, FILE *f)
{
	char *rn;
	int64_t n;
	rn = rname[r];
	switch (c->type) {
	case CAddr:
		fprintf(f, "\tla %s, ", rn);
		emitaddr(c, f);
		fputc('\n', f);
		break;
	case CBits:
		n = c->bits.i;
		fprintf(f, "\tli %s, 0x%"PRIx32"\n", rn, (int32_t)n);
		break;
	default:
		die("invalid constant");
	}
}

static void
fixslot(Ref *pr, Fn *fn, FILE *f)
{
	Ref r;
	int64_t s;

	r = *pr;
	if (rtype(r) == RSlot) {
		s = slot(r.val, fn);
		if (s < -2048 || s > 2047) {
			fprintf(f, "\tli x31, %"PRId64"\n", s);
			fprintf(f, "\tadd x31, x8, x31\n");
			*pr = TMP(T6);
		}
	}
}




	#define BLIST \
	X(beq) \
	X(bne) \
	X(bge) \
	X(blt) \
	X(bgeu) \
	X(bltu) \

	enum btypes {
		#define X(name) name,
		BLIST
		#undef X
		NBTYPES
	};
	char *bnames[NBTYPES] = {
		#define X(name) [name] = #name,
		BLIST
		#undef X
	};

	int invb[NBTYPES] = {
		[beq] = bne,
		[bne] = beq,
		[bge] = blt,
		[blt] = bge,
		[bgeu] = bltu,
		[bltu] = bgeu
	};

int cbem = 0;
static void
emitins(Ins *i, Fn *fn, Blk *b, int id0, FILE *f)
{
	int o;
	char *rn;
	int64_t s;
	Blk *src;
	Con *con;
	int bname;


	switch (i->op) {
	default:
		if (isload(i->op))
			fixslot(&i->arg[0], fn, f);
		else if (isstore(i->op))
			fixslot(&i->arg[1], fn, f);
	Table:
		/* most instructions are just pulled out of
		 * the table omap[], some special cases are
		 * detailed below */
		for (o=0;; o++) {
			/* this linear search should really be a binary
			 * search */
			if (omap[o].op == NOp)
				die("no match for %s(%c)",
					optab[i->op].name, "wlsd"[i->cls]);
			if (omap[o].op == i->op)
			if (omap[o].cls == i->cls || omap[o].cls == Ka
			|| (omap[o].cls == Ki && KBASE(i->cls) == 0))
				break;
		}
		emitf(omap[o].asm, i, fn, f);
		return;
	case Ocopy:
		if (req(i->to, i->arg[0]))
			return;
		if (rtype(i->to) == RSlot) {
			switch (rtype(i->arg[0])) {
			case RSlot:
			case RCon:
				die("unimplemented");
				break;
			default:
				assert(isreg(i->arg[0]));
				i->arg[1] = i->to;
				i->to = R;
				switch (i->cls) {
				case Kw: i->op = Ostorew; break;
				case Kl: i->op = Ostorel; break;
				case Ks: i->op = Ostores; break;
				case Kd: i->op = Ostored; break;
				}
				fixslot(&i->arg[1], fn, f);
				goto Table;
			}
			return;
		}
		assert(isreg(i->to));
		switch (rtype(i->arg[0])) {
		case RCon:
			loadcon(&fn->con[i->arg[0].val], i->to.val, i->cls, f);
			break;
		case RSlot:
			i->op = Oload;
			fixslot(&i->arg[0], fn, f);
			goto Table;
		default:
			assert(isreg(i->arg[0]));
			goto Table;
		}
		return;
	case Onop:
		return;
	case Oaddr:
		assert(rtype(i->arg[0]) == RSlot);
		rn = rname[i->to.val];
		s = slot(i->arg[0].val, fn);
		if (-s < 2048) {
			fprintf(f, "\taddi %s, x8, %"PRId64"\n", rn, s);
		} else {
			fprintf(f,
				"\tli %s, %"PRId64"\n"
				"\taddi %s, x8, %s\n",
				rn, s, rn, rn
			);
		}
		return;
	case Ocall:
		switch (rtype(i->arg[0])) {
		case RCon:
			con = &fn->con[i->arg[0].val];
			if (con->type != CAddr || con->bits.i)
				goto invalid;
			fprintf(f, "\tjal %s\n", str(con->label));
			break;
		case RTmp:
			emitf("jalr %0", i, fn, f);
			break;
		default:
		invalid:
			die("invalid call argument");
		}
		return;
	case Osalloc:
		emitf("sub x2, x2, %0", i, fn, f);
		if (!req(i->to, R))
			emitf("mv %=, x2", i, fn, f);
		return;
	case Oextsw:
	case Oextuw:
		if(i->to.val != i->arg[0].val)
			emitf("mv %=, %0", i, fn, f);
		return;
	case Orbeq:
		bname = beq;
		break;
	case Orbne:
		bname = bne;
		break;
	case Orbge:
		bname = bge;
		break;
	case Orblt:
		bname = blt;
		break;
	case Orbgeu:
		bname = bgeu;
		break;
	case Orbltu:
		bname = bltu;
		break;
	}
	src = b->s2;
	if (b->link == b->s2)
		src = b->s1;
	else
	   bname = invb[bname];
	if (b->jmp.type == Jjnz)
		fprintf(f, "\t%s %s, %s, .L%d\n", bnames[bname], rname[i->arg[0].val], rname[i->arg[1].val], id0+src->id);    
	cbem = 1;
}

/*

  Stack-frame layout:

  +=============+
  | varargs     |
  |  save area  |
  +-------------+
  |  saved ra   |
  |  saved fp   |
  +-------------+ <- fp
  |    ...      |
  | spill slots |
  |    ...      |
  +-------------+
  |    ...      |
  |   locals    |
  |    ...      |
  +-------------+
  |   padding   |
  +-------------+
  | callee-save |
  |  registers  |
  +=============+

*/

void
rv64_emitfn(Fn *fn, FILE *f)
{
	static int id0;
	int lbl, neg, off, frame, *pr, r;
	Blk *b, *s;
	Ins *i;

	gasemitlnk(fn->name, &fn->lnk, ".text", f);

	if (fn->vararg) {
		/* TODO: only need space for registers
		 * unused by named arguments
		 */
		fprintf(f, "\taddi x2, x2, -32\n");
		for (r=A0; r<=A7; r++)
			fprintf(f,
				"\tsw %s, %d(x2)\n",
				rname[r], 4 * (r - A0)
			);
	}
	fprintf(f, "\tsw x8, -8(x2)\n");
	fprintf(f, "\tsw x1, -4(x2)\n");
	fprintf(f, "\taddi x8, x2, -8\n");

	frame = (8 + 4 * fn->slot + 15) & ~15;
	for (pr=rv64_rclob; *pr>=0; pr++) {
		if (fn->reg & BIT(*pr))
			frame += 4;
	}
	frame = (frame + 15) & ~15;

	if (frame <= 2048)
		fprintf(f,
			"\taddi x2, x2, -%d\n",
			frame
		);
	else
		fprintf(f,
			"\tli x31, %d\n"
			"\tsub x2, x2, x31\n",
			frame
		);
	for (pr=rv64_rclob, off=0; *pr>=0; pr++) {
		if (fn->reg & BIT(*pr)) {
			fprintf(f,
				"\t%s %s, %d(x2)\n",
				*pr < FT0 ? "sw" : "fsw",
				rname[*pr], off
			);
			off += 4;
		}
	}

	for (lbl=0, b=fn->start; b; b=b->link, cbem=0) {
		if (lbl || b->npred > 1)
			fprintf(f, ".L%d:\n", id0+b->id);
		for (i=b->ins; i!=&b->ins[b->nins]; i++)
			emitins(i, fn, b, id0, f);
		lbl = 1;
		switch (b->jmp.type) {
		case Jret0:
			if (fn->dynalloc) {
				if (frame - 8 <= 2048)
					fprintf(f,
						"\taddi x2, x8, -%d\n",
						frame - 8
					);
				else
					fprintf(f,
						"\tli x31, %d\n"
						"\tsub x2, x8, x31\n",
						frame - 8
					);
			}
			for (pr=rv64_rclob, off=0; *pr>=0; pr++) {
				if (fn->reg & BIT(*pr)) {
					fprintf(f,
						"\t%s %s, %d(x2)\n",
						*pr < FT0 ? "lw" : "flw",
						rname[*pr], off
					);
					off += 4;
				}
			}
			fprintf(f,
				"\taddi x2, x8, %d\n"
				"\tlw x1, 4(x8)\n"
				"\tlw x8, 0(x8)\n"
				"\tret\n",
				8 + fn->vararg * 32
			);
			break;
		case Jjmp:
		Jmp:
			if (b->s1 != b->link)
				fprintf(f, "\tj .L%d\n", id0+b->s1->id);
			else
				lbl = 0;
			break;
		case Jjnz:
			neg = 0;
			if (b->link == b->s2) {
				s = b->s1;
				b->s1 = b->s2;
				b->s2 = s;
				neg = 1;
			}
			assert(isreg(b->jmp.arg));
			if(!cbem)
				fprintf(f,
					"\tb%sz %s, .L%d\n",
					neg ? "ne" : "eq",
					rname[b->jmp.arg.val],
					id0+b->s2->id
				);
			goto Jmp;
		}
	}
	id0 += fn->nblk;
}
