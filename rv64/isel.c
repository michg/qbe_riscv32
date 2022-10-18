#include "all.h"


static int
memarg(Ref *r, int op, Ins *i)
{
	if (isload(op) || op == Ocall)
		return r == &i->arg[0];
	if (isstore(op))
		return r == &i->arg[1];
	return 0;
}

static int
immarg(Ref *r, int op, Ins *i)
{
	return rv64_op[op].imm && r == &i->arg[1];
}

static void
fixarg(Ref *r, int k, Ins *i, Fn *fn)
{
	char buf[32];
	Ref r0, r1;
	int s, n, op;
	Con *c;

	r0 = r1 = *r;
	op = i ? i->op : Ocopy;
	switch (rtype(r0)) {
	case RCon:
		c = &fn->con[r0.val];
		if (c->type == CAddr && memarg(r, op, i))
			break;
		if (c->type == CBits && immarg(r, op, i))
		if (-2048 <= c->bits.i && c->bits.i < 2048)
			break;
		r1 = newtmp("isel", k, fn);
		if (KBASE(k) == 1) {
			/* load floating points from memory
			 * slots, they can't be used as
			 * immediates
			 */
			assert(c->type == CBits);
			n = gasstash(&c->bits, (c->flt == 2) ? 8 : 4);
			vgrow(&fn->con, ++fn->ncon);
			c = &fn->con[fn->ncon-1];
			sprintf(buf, "fp%d", n);
			*c = (Con){.type = CAddr, .local = 1};
			c->label = intern(buf);
			emit(Oload, k, r1, CON(c-fn->con), R);
			break;
		}
		emit(Ocopy, k, r1, r0, R);
		break;
	case RTmp:
		if (isreg(r0))
			break;
		s = fn->tmp[r0.val].slot;
		if (s != -1) {
			/* aggregate passed by value on
			 * stack, or fast local address,
			 * replace with slot if we can
			 */
			if (memarg(r, op, i)) {
				r1 = SLOT(s);
				break;
			}
			r1 = newtmp("isel", k, fn);
			emit(Oaddr, k, r1, SLOT(s), R);
			break;
		}
		break;
	}
	*r = r1;
}

static void
negate(Ref *pr, Fn *fn)
{
	Ref r;

	r = newtmp("isel", Kw, fn);
	emit(Oxor, Kw, *pr, r, getcon(1, fn));
	*pr = r;
}


	enum ftypes {
		fadd,
		fsub,
		fmul,
		fdiv,
		fneg,
		fcpy,
		sw2f,
		uw2f,
		f2sw,
		f2uw,
		feq,
		fle,
		flt,
		NFTYPES
	};

	struct sfpar {
		char *fname;
	};

	struct sfpar sffuncs[] = {
		[fadd] = {"float32_add"},
		[fsub] = {"float32_sub"},
		[fmul] = {"float32_mul"},
		[fdiv] = {"float32_div"},
		[sw2f] = {"int32_to_float32"},
		[uw2f] = {"uint32_to_float32"},
		[f2sw] = {"float32_to_int32_round_to_zero"},
		[f2uw] = {"float32_to_uint32"},
		[feq] =  {"float32_eq"},
		[fle] =  {"float32_le"},
		[flt] =  {"float32_lt"},
		[fneg] = {"float32_neg"},
	};



static void
selfloat(Ins i, Fn *fn) {
	Ins *icmp;
	Con *c;
	int ftype;
	int parc;
	switch(i.op) {
	case Oadd:
		ftype = fadd;
		break;
	case Osub:
		ftype = fsub;
		break;
	case Omul:
		ftype = fmul;
		break;
	case Odiv:
		ftype = fdiv;
		break;
	case Oswtof:
	case Osltof:
		ftype = sw2f;
		break;
	case Ouwtof:
	case Oultof:
		ftype = uw2f;
		break;
	case Odtosi:
	case Ostosi:
		ftype = f2sw;
		break;
	case Odtoui:
	case Ostoui:
		ftype = f2uw;
		break;
	case Oceqs:
	case Oceqd:
		ftype = feq;
		break;
	case Ocles:
	case Ocled:
		ftype = fle;
		break;
	case Oclts:
	case Ocltd:
		 ftype = flt;
		 break;
	case Oneg:
		 ftype = fneg;
		 break;
	default:
		assertf(0, "unknown softfunction %s\r\n",optab[i.op].name);
	}
	parc = req(i.arg[1], R) ? 1:2;
	vgrow(&fn->con, ++fn->ncon);
	c = &fn->con[fn->ncon-1];
	c->type = CAddr;
	c->label = intern(sffuncs[ftype].fname);
	emit(Ocopy, i.cls, i.to, TMP(A0), R);
	icmp = curi;
	fixarg(&icmp->arg[0], argcls(icmp, 0), icmp, fn);
	emit(Ocall, i.cls, R, CON(c-fn->con), CALL((parc<<4) + 1));
	emit(Ocopy, argcls(&i, 0), TMP(A0), i.arg[0], R);
	icmp = curi;
	fixarg(&icmp->arg[0], argcls(&i, 0), icmp, fn);
	if(parc == 2) {
		emit(Ocopy, argcls(&i, 0), TMP(A1), i.arg[1], R);
		icmp = curi;
		fixarg(&icmp->arg[0], argcls(&i, 0), icmp, fn);
	}
}


static void
emitcmpi(Ins i, Fn *fn)
{
   if(opt_softfloat && INRANGE(i.op, Ocmps, Ocmpd1)) selfloat(i, fn);
		else emiti(i);
}

static void
selcmp(Ins i, int k, int op, Fn *fn)
{
	Ins *icmp;
	Ref r, r0, r1;
	int sign, swap, neg;

	switch (op) {
	case Cieq:
		r = newtmp("isel", k, fn);
		emit(Oreqz, i.cls, i.to, r, R);
		emit(Oxor, k, r, i.arg[0], i.arg[1]);
		icmp = curi;
		fixarg(&icmp->arg[0], k, icmp, fn);
		fixarg(&icmp->arg[1], k, icmp, fn);
		return;
	case Cine:
		r = newtmp("isel", k, fn);
		emit(Ornez, i.cls, i.to, r, R);
		emit(Oxor, k, r, i.arg[0], i.arg[1]);
		icmp = curi;
		fixarg(&icmp->arg[0], k, icmp, fn);
		fixarg(&icmp->arg[1], k, icmp, fn);
		return;
	case Cisge: sign = 1, swap = 0, neg = 1; break;
	case Cisgt: sign = 1, swap = 1, neg = 0; break;
	case Cisle: sign = 1, swap = 1, neg = 1; break;
	case Cislt: sign = 1, swap = 0, neg = 0; break;
	case Ciuge: sign = 0, swap = 0, neg = 1; break;
	case Ciugt: sign = 0, swap = 1, neg = 0; break;
	case Ciule: sign = 0, swap = 1, neg = 1; break;
	case Ciult: sign = 0, swap = 0, neg = 0; break;
	case NCmpI+Cfeq:

	case NCmpI+Cfle:
	case NCmpI+Cflt:
		swap = 0, neg = 0;
		break;
	case NCmpI+Cfge:
		swap = 1;
		i.op = Oclts;
		break;
	case NCmpI+Cfgt:
		swap = 1;
		i.op = Ocles;
		break;
	case NCmpI+Cfuo:
		negate(&i.to, fn);
		/* fall through */
	case NCmpI+Cfo:
		r0 = newtmp("isel", i.cls, fn);
		r1 = newtmp("isel", i.cls, fn);
		emit(Oand, i.cls, i.to, r0, r1);
		op = KWIDE(k) ? Oceqd : Oceqs;
		emit(op, i.cls, r0, i.arg[0], i.arg[0]);
		icmp = curi;
		fixarg(&icmp->arg[0], k, icmp, fn);
		fixarg(&icmp->arg[1], k, icmp, fn);
		emit(op, i.cls, r1, i.arg[1], i.arg[1]);
		icmp = curi;
		fixarg(&icmp->arg[0], k, icmp, fn);
		fixarg(&icmp->arg[1], k, icmp, fn);
		return;
	case NCmpI+Cfne:
		swap = 0, neg = 1;
		i.op = KWIDE(k) ? Oceqd : Oceqs;
		break;
	default:
		assert(0 && "unknown comparison");
	}
	if (op < NCmpI)
		i.op = sign ? Ocsltl : Ocultl;
	if (swap) {
		r = i.arg[0];
		i.arg[0] = i.arg[1];
		i.arg[1] = r;
	}
	if (neg)
		negate(&i.to, fn);
	emitcmpi(i, fn);
	icmp = curi;
	fixarg(&icmp->arg[0], k, icmp, fn);
	fixarg(&icmp->arg[1], k, icmp, fn);
}

static void
selbcond(Ins i, int k, int op, Blk *b, Fn *fn){
	int swap;
	Ins *icmp;
	switch(op) {
		case Cieq:
			op = Orbeq; swap = 0;
			break;
		case Cine:
			op = Orbne; swap = 0;
			break;
		case Cisge:
			op = Orbge; swap = 0;
			break;
		case Cisgt:
			op = Orblt; swap = 1;
			break;
		case Cisle:
			op = Orbge; swap = 1;
			break;
		case Cislt:
			op = Orblt; swap = 0;
			break;
		case Ciuge:
			op = Orbgeu; swap = 0;
			break;
		case Ciugt:
			op = Orbltu; swap = 1;
			break;
		case Ciule:
			op = Orbgeu; swap = 1;
			break;
		case Ciult:
			op = Orbltu; swap = 0;
			break;
		default:
		assert(0 && "unknown comparison");
	}
	assert(b->s2);
	if(swap)
		emit(op, i.cls, i.to, i.arg[1], i.arg[0]);
	else
		emit(op, i.cls, i.to, i.arg[0], i.arg[1]);
	icmp = curi;
	fixarg(&icmp->arg[0], k, icmp, fn);
	fixarg(&icmp->arg[1], k, icmp, fn);
}

Ins *nmem;
int dst = -1;

struct {
Ins* ins;
int argno;
Ref r;
} mop;

static void
sel(Ins *i, Blk *b, Fn *fn)
{
	Ins *i0;
	Tmp *t;
	Use *u;
	int ck, cc;
	int s;
	unsigned int j;

	if (INRANGE(i->op, Oalloc, Oalloc1)) {
		i0 = curi - 1;
		salloc(i->to, i->arg[0], fn);
		fixarg(&i0->arg[0], Kl, i0, fn);
		return;
	}

	if (iscmp(i->op, &ck, &cc)) {
		t = &fn->tmp[i->to.val]; 
		for(j=0, u=t->use; u<&t->use[t->nuse] && b->id==u->bid; u++, j++);
		if (b->jmp.type == Jjnz && (KBASE(ck) == 0) && (curi == &insb[NIns]) && j==t->nuse) {
		   selbcond(*i, ck, cc, b, fn);
		} else {
		   selcmp(*i, ck, cc, fn);
		}
		return;
	}
	if(opt_softfloat && (KBASE(i->cls) == 1 || INRANGE(i->op, Ostosi, Odtoui) )
	&& i->op != Ocopy && i->op!=Oload && i->op!=Ocast && i->op!=Otruncd && i->op!=Oexts) {
	   selfloat(*i, fn);
	   return;
	}
	if(mop.ins!=&insb[NIns] && i->op == Oadd && req(i->to, mop.r) && rtype(i->arg[0])==RTmp && fn->tmp[i->arg[0].val].slot!=-1 && rtype(i->arg[1])==RCon) {
	   s = (fn->tmp[i->arg[0].val].slot*4 + fn->con[i->arg[1].val].bits.i);
	   i0 = curi;
	   while(i0 != mop.ins) {
		   if(req(i0->to, mop.r)) {
			   if((i0->op == Oadd) && req(i0->arg[0], mop.r) && (rtype(i0->arg[1])==RCon)) {
				 s += fn->con[i0->arg[1].val].bits.i;
			   } else {
				mop.ins=&insb[NIns];
				break;
			  }
		   }
		  i0++;
	   }
	   if(mop.ins!=&insb[NIns]) {
		  mop.ins->arg[mop.argno] = FSLOT(s >> 2, s & 3);
		  mop.ins=&insb[NIns];
		  return;
	   }
	}
	if (i->op != Onop) {
		emiti(*i);
		i0 = curi; /* fixarg() can change curi */
		if(isload(i0->op) || isstore(i0->op)) {
		  mop.ins = curi;
		  mop.argno = isload(i0->op) ? 0 : 1;
		  mop.r = TMP(i0->arg[mop.argno].val);
		}
		fixarg(&i0->arg[0], argcls(i, 0), i0, fn);
		fixarg(&i0->arg[1], argcls(i, 1), i0, fn);
	}
}

static void
seljmp(Blk *b, Fn *fn)
{
	if (b->jmp.type == Jjnz)
		fixarg(&b->jmp.arg, Kw, 0, fn);
}


void
rv64_isel(Fn *fn)
{
	Blk *b, **sb;
	Ins *i;
	Phi *p;
	uint n;
	int al;
	int64_t sz;

	/* assign slots to fast allocs */
	b = fn->start;
	/* specific to NAlign == 3 */ /* or change n=4 and sz /= 4 below */
	for (al=Oalloc, n=4; al<=Oalloc1; al++, n*=2)
		for (i=b->ins; i<&b->ins[b->nins]; i++)
			if (i->op == al) {
				if (rtype(i->arg[0]) != RCon)
					break;
				sz = fn->con[i->arg[0].val].bits.i;
				if (sz < 0 || sz >= INT_MAX-15)
					err("invalid alloc size %"PRId64, sz);
				sz = (sz + n-1) & -n;
				sz /= 4;
				if (sz > INT_MAX - fn->slot)
					die("alloc too large");
				fn->tmp[i->to.val].slot = fn->slot;
				fn->slot += sz;
				*i = (Ins){.op = Onop};
			}

	for (b=fn->start; b; b=b->link) {
		curi = &insb[NIns];
		nmem = curi;
		mop.ins = curi;
		for (sb=(Blk*[3]){b->s1, b->s2, 0}; *sb; sb++)
			for (p=(*sb)->phi; p; p=p->link) {
				for (n=0; p->blk[n] != b; n++)
					assert(n+1 < p->narg);
				fixarg(&p->arg[n], p->cls, 0, fn);
			}
		seljmp(b, fn);
		for (i=&b->ins[b->nins]; i!=b->ins;)
			sel(--i, b, fn);
		b->nins = &insb[NIns] - curi;
		idup(&b->ins, curi, b->nins);
	}

	if (debug['I']) {
		fprintf(stderr, "\n> After instruction selection:\n");
		printfn(fn, stderr);
	}
}
