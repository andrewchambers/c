#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include "util.h"
#include "ctypes.h"
#include "cc.h"
#include "ir.h"

typedef enum {
	IRConst,
	IRLabel,
	IRVReg
} IRValKind;

typedef struct IRVal {
	IRValKind kind;
	int64 v;
	char  *label;
} IRVal;

typedef enum {
	Opalloca,
	Opret,
	Opjmp
} Opcode; 

typedef struct Instruction {
	Opcode op;
	IRVal a, b, c;
} Instruction;

typedef struct Terminator {
	Opcode op;
	IRVal v;
	char *label1;
	char *label2;
} Terminator;

typedef struct BasicBlock {
	Vec *labels;
	Instruction *instructions;
	Terminator terminator;
	int cap;
	int ninstructions;
	int terminated;
} BasicBlock;

static BasicBlock *
mkbasicblock()
{
	BasicBlock *bb;

	bb = xmalloc(sizeof(BasicBlock));
	bb->labels = vec();
	vecappend(bb->labels, newlabel());
	bb->cap = 32;
	bb->instructions = xmalloc(sizeof(Instruction) * bb->cap);
	bb->terminated = 0;
	bb->ninstructions = 0;
	return bb;
}


static FILE *outf;

Sym        *curfunc;
BasicBlock *preludebb;
BasicBlock *currentbb;
Vec        *basicblocks;

int labelcount;

char *
newlabel(void)
{
	char *s;
	int   n;

	n = snprintf(0, 0, ".L%d", labelcount);
	if(n < 0)
		panic("internal error");
	n += 1;
	s = xmalloc(n);
	if(snprintf(s, n, ".L%d", labelcount) < 0)
		panic("internal error");
	labelcount++;
	return s;
}

static char *
ctype2irtype(CTy *ty)
{
	switch (ty->t) {
	case CVOID:
		return "";
	case CPRIM:
		switch (ty->Prim.type) {
		case PRIMCHAR:
			return "w";
		case PRIMSHORT:
			return "w";
		case PRIMINT:
			return "w";
		case PRIMLONG:
			return "l";
		case PRIMLLONG:
			return "l";
		default:
			panic("unhandled cprim");
		}
	case CPTR:
		return "l";
	default:
		panic("unhandled ctype");
	}
}

void
setiroutput(FILE *f)
{
	outf = f;
}

static void
out(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	if(vfprintf(outf, fmt, va) < 0)
		errorf("error printing\n");
	va_end(va);
}

void
beginmodule()
{
	out("# Compiled with care...\n");
}

void
emitsym(Sym *sym)
{
	if (isfunc(sym->type))
		panic("emitsym precondition failed");

	out("# %s:%d:%d %s\n", sym->pos->file, sym->pos->line, sym->pos->col, sym->name);
	switch(sym->k){
	case SYMGLOBAL:
		break;
	case SYMLOCAL:
		break;
	case SYMENUM:
		break;
	case SYMTYPE:
		break;
	}
	out("\n");
}

void
emitfuncstart(Sym *sym)
{
	int i;
	NameTy *namety;

	if (sym->k != SYMGLOBAL || !isfunc(sym->type))
		panic("emitfuncstart precondition failed");

	curfunc = sym;

	out("function %s $%s(", ctype2irtype(curfunc->type->Func.rtype), curfunc->name);

	for (i = 0; i < curfunc->type->Func.params->len; i++) {
		namety = vecget(curfunc->type->Func.params, i);
		out("%s %s%s", ctype2irtype(namety->type), namety->type, i == curfunc->type->Func.params->len - 1 ? "" : ",");
	}
	out(")\n");

	basicblocks = vec();
	preludebb = mkbasicblock();
	currentbb = preludebb;
	vecappend(basicblocks, preludebb);
}

static void
emitbb(BasicBlock *bb)
{
	int i;

	
	for (i = 0; i < bb->labels->len; i++) {
		out("%s:\n", vecget(bb->labels, i));
	}

	for (i = 0; i < bb->ninstructions; i++) {
		/* bb->instructions[i] */
	}

	if (bb->terminated) {
		
	} else {
		out("ret\n");
	}
}

void
emitfuncend()
{
	int i;

	for (i = 0; i < basicblocks->len; i++) {
		emitbb(vecget(basicblocks, i));
	}
	out("}\n");
}

void
endmodule()
{
	out("# Compiled with %lld bytes allocated.\n", malloctotal);
}
