#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include "util.h"
#include "ctypes.h"
#include "cc.h"
#include "ir.h"

static FILE *outf;

BasicBlock *preludebb;
BasicBlock *currentbb;
Vec        *basicblocks;

int labelcount;

char *
newlabel()
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

int vregcount;

IRVal nextvreg(char *irtype)
{
	return (IRVal){.kind=IRVReg, .irtype=irtype, .v=vregcount++};
}

char *
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

static void
outirval(IRVal *val)
{
	switch (val->kind) {
	case IRConst:
		out("%lld", val->v);
		break;
	case IRVReg:
		out("%%v%d", val->v);
		break;
	default:
		panic("unhandled irval");
	}
}

static void
outterminator(Terminator *term)
{
	switch (term->op) {
	case Opret:
		out("ret ");
		outirval(&term->v);
		out("\n");
		break;
	default:
		panic("unhandled terminator");
	}
}

static void
outinstruction(Instruction *instr)
{
	switch (instr->op) {
	case Opadd:
		outirval(&instr->a);
		out(" =%s add ", instr->a.irtype);
		outirval(&instr->b);
		out(", ");
		outirval(&instr->c);
		out("\n");
		break;
	default:
		panic("unhandled instrction");
	}
}

void endcurbb(Terminator term)
{
	if (currentbb->terminated)
		panic("internal error - current block already terminated.");

	currentbb->terminator = term;
	currentbb->terminated = 1;
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
emitfuncstart()
{
	int i;
	NameTy *namety;

	if (curfunc->k != SYMGLOBAL || !isfunc(curfunc->type))
		panic("emitfuncstart precondition failed");

	out("export\n");
	out("function %s $%s(", ctype2irtype(curfunc->type->Func.rtype), curfunc->name);

	for (i = 0; i < curfunc->type->Func.params->len; i++) {
		namety = vecget(curfunc->type->Func.params, i);
		out("%s %s%s", ctype2irtype(namety->type), namety->type, i == curfunc->type->Func.params->len - 1 ? "" : ",");
	}
	out(") {\n");

	basicblocks = vec();
	preludebb = mkbasicblock();
	currentbb = preludebb;
	vecappend(basicblocks, preludebb);
	vregcount = 0;
}

static void
emitbb(BasicBlock *bb)
{
	int i;

	
	for (i = 0; i < bb->labels->len; i++) {
		out("@%s\n", vecget(bb->labels, i));
	}

	for (i = 0; i < bb->ninstructions; i++) {
		outinstruction(&bb->instructions[i]);
	}

	if (bb->terminated) {
		outterminator(&bb->terminator);
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
	if (fflush(outf) != 0)
		panic("error flushing output");
}

BasicBlock *
mkbasicblock()
{
	BasicBlock *bb;

	bb = xmalloc(sizeof(BasicBlock));
	bb->labels = vec();
	vecappend(bb->labels, newlabel());
	bb->cap = 64;
	bb->instructions = xmalloc(bb->cap * sizeof(Instruction));
	bb->terminated = 0;
	bb->ninstructions = 0;
	return bb;
}

void
bbappend(BasicBlock *bb, Instruction ins)
{
	Instruction *instrarray;

	if (bb->cap == bb->ninstructions) {
		bb->cap += 64;
		instrarray = xmalloc(bb->cap * sizeof(Instruction));
		bb->instructions = instrarray;
		memcpy(instrarray, bb->instructions, bb->ninstructions * sizeof(Instruction));
	}

	bb->instructions[bb->ninstructions++] = ins;
}
