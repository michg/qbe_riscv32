#include "all.h"


char *gasloc, *gassym;
static int gasasm;

void
gasinit(enum Asm asmmode)
{
	gasasm = asmmode;
	switch (gasasm) {
	case Gaself:
		gasloc = ".L";
		gassym = "";
		break;
	case Gasmacho:
		gasloc = "L";
		gassym = "_";
		break;
	}
}

void
gasemitlnk(char *n, Lnk *l, char *s, FILE *f)
{
	char *p;

	if (l->sec) {
		fprintf(f, ".section %s", l->sec);
		if (l->secf)
			fprintf(f, ", %s", l->secf);
	} else {
		fputs(s, f);
	}
	fputc('\n', f);
	if (l->align)
		fprintf(f, ".balign %d\n", l->align);
	p = n[0] == '"' ? "" : gassym;
	if (l->export)
		fprintf(f, ".globl %s%s\n", p, n);
	fprintf(f, "%s%s:\n", p, n);
}

void
gasemitfntail(char *fn, FILE *f)
{
    fprintf(f, ".globl %s_end\n%s_end:\n", fn, fn);
}

void
gasemitdat(Dat *d, FILE *f)
{
	static char *dtoa[] = {
		[DB] = "\t.byte",
		[DH] = "\t.short",
		[DW] = "\t.int",
		[DL] = "\t.quad"
	};
	static int64_t zero;
	float fl;
	char *p;

	switch (d->type) {
	case DStart:
		zero = 0;
		break;
	case DEnd:
		if (zero != -1) {
			gasemitlnk(d->name, d->lnk, ".bss", f);
			fprintf(f, "\t.fill %"PRId64",1,0\n", zero);
		}
		break;
	case DZ:
		if (zero != -1)
			zero += d->u.num;
		else
			fprintf(f, "\t.fill %"PRId64",1,0\n", d->u.num);
		break;
	default:
		if (zero != -1) {
			gasemitlnk(d->name, d->lnk, ".data", f);
			if (zero > 0)
				fprintf(f, "\t.fill %"PRId64",1,0\n", zero);
			zero = -1;
		}
		if (d->isstr) {
			if (d->type != DB)
				err("strings only supported for 'b' currently");
			fprintf(f, "\t.ascii %s\n", d->u.str);
		}
		else if (d->isref) {
			p = d->u.ref.name[0] == '"' ? "" : gassym;
			fprintf(f, "%s %s%s%+"PRId64"\n",
				dtoa[d->type], p, d->u.ref.name,
				d->u.ref.off);
		}
		else {
		    if(d->type == DL) { 
		        if(d->isdflt) {
		          fl = (float) (*(double*) &(d->u.num));
		          d->u.num = *(int64_t*)&fl;
		        }
		        fprintf(f, "%s 0x%"PRIx32"\n",
				dtoa[DW], (unsigned int)d->u.num);
				fprintf(f, "\n\t.int 0\n");
		    } else
			fprintf(f, "%s 0x%"PRIx32"\n",
				dtoa[d->type],(unsigned int)d->u.num);
		}
		break;
	}
}

typedef struct Asmbits Asmbits;

struct Asmbits {
	char bits[16];
	int size;
	Asmbits *link;
};

static Asmbits *stash;

int
gasstash(void *bits, int size)
{
	Asmbits **pb, *b;
	int i;

	assert(size == 4 || size == 8 || size == 16);
	for (pb=&stash, i=0; (b=*pb); pb=&b->link, i++)
		if (size <= b->size)
		if (memcmp(bits, b->bits, size) == 0)
			return i;
	b = emalloc(sizeof *b);
	memcpy(b->bits, bits, size);
	b->size = size;
	b->link = 0;
	*pb = b;
	return i;
}

void
gasemitfin(FILE *f)
{
	Asmbits *b;
	int sz, i;
	float fl;
	double d;

	if (!stash)
		return;
	fprintf(f, "/* floating point constants */\n.data\n");
	for (sz=16; sz>=4; sz/=2)
		for (b=stash, i=0; b; b=b->link, i++) {
			if (b->size == sz) {
				fprintf(f,
					".balign %d\n"
					"%sfp%d:",
					sz, gasloc, i
				);
			    if (sz <= 8) {
    			    if (sz == 4)
    					fl = *(float *)b->bits;
    			    else if (sz == 8) {
    				    d = *(double *)b->bits;
    				    fl = (float) d;
    				}
    			    fprintf(f, "\n\t.int 0x%"PRIx32,
    						*(int32_t*)&fl);
    				fprintf(f, " /* %f */\n", fl);
    				fprintf(f, "\n\t.int 0\n");
    			} else   
				    fprintf(f, "// sz=16 not implemented! \n");
			}
		}
	while ((b=stash)) {
		stash = b->link;
		free(b);
	}
}
