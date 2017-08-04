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

static FILE *outf;

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
	out("; Compiled with care...\n");
}

void
emitsym(Sym *sym)
{
	if (isfunc(sym->type))
		panic("emitsym precondition failed");

	out("; %s:%d:%d %s\n", sym->pos->file, sym->pos->line, sym->pos->col, sym->name);
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
	if (sym->k != SYMGLOBAL || !isfunc(sym->type))
		panic("emitfuncstart precondition failed");

	out("%s() {\n", sym->name);

	curfunc = sym;
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

}
