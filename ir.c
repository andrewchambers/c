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
BasicBlock *entrybb;
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

IRVal
nextvreg(char *irtype)
{
	return (IRVal){.kind=IRVReg, .irtype=irtype, .v=vregcount++};
}

IRVal
alloclocal(CTy *ty)
{
	IRVal v;

	if (ty->incomplete)
		panic("cannot alloc a local of incomplete type");

	v = nextvreg("l");
	bbappend(preludebb, (Instruction){.op=Opalloca, .a=v, .c=ty->size});
	return v;
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
	case Opjmp:
		out("jmp @%s\n", term->label1);
		break;
	case Opcond:
		out("jnz ");
		outirval(&term->v);
		out(", @%s, @%s\n", term->label1, term->label2);
		break;
	default:
		panic("unhandled terminator");
	}
}

static void
outalloca(Instruction *instr)
{
	if (instr->op != Opalloca)
		panic("internal error, not a valid alloca");

	outirval(&instr->a);
	out(" =l alloc16 ");
	outirval(&instr->b);
	out("\n");
}

static void
outstore(Instruction *instr)
{
	char *opname;

	switch (instr->op) {
	case Opstorel:
		opname = "storel";
		break;
	case Opstorew:
		opname = "storew";
		break;
	case Opstoreh:
		opname = "storeh";
		break;
	case Opstoreb:
		opname = "storeb";
		break;
	default:
		panic("unhandled load instruction");
	}

	out("%s ", opname);
	outirval(&instr->a);
	out(", ");
	outirval(&instr->b);
	out("\n");
}

static void
outload(Instruction *instr)
{
	char *opname;

	switch (instr->op) {
	case Opload:
		opname = "load";
		break;
	case Oploadsh:
		opname = "loadsh";
		break;
	case Oploadsb:
		opname = "loadsb";
		break;
	case Oploaduh:
		opname = "loaduh";
		break;
	case Oploadub:
		opname = "loadub";
		break;
	default:
		panic("unhandled load instruction");
	}

	outirval(&instr->a);
	out(" =%s %s ", instr->a.irtype, opname);
	outirval(&instr->b);
	out("\n");
}

static int
isstoreinstr(Instruction *instr)
{
	return instr->op >= Opstorel && instr->op <= Opstoreb;
}

static int
isloadinstr(Instruction *instr)
{
	return instr->op >= Opload && instr->op <= Oploadsb;
}

static void
outinstruction(Instruction *instr)
{
	char *opname;

	if (instr->op == Opalloca) {
		outalloca(instr);
		return;
	}

	if (isstoreinstr(instr)) {
		outstore(instr);
		return;
	}

	if (isloadinstr(instr)) {
		outload(instr);
		return;
	}

	switch (instr->op) {
	case Opadd:
		opname = "add";
		break;
	case Opsub:
		opname = "sub";
		break;
	case Opmul:
		opname = "mul";
		break;
	case Opdiv:
		opname = "div";
		break;
	case Oprem:
		opname = "rem";
		break;
	case Opband:
		opname = "and";
		break;
	case Opbor:
		opname = "or";
		break;
	case Opbxor:
		opname = "xor";
		break;
	case Opceql:
		opname = "ceql";
		break;
	case Opceqw:
		opname = "ceqw";
		break;
	default:
		panic("unhandled instruction");
	}

	outirval(&instr->a);
	out(" =%s %s ", instr->a.irtype, opname);
	outirval(&instr->b);
	out(", ");
	outirval(&instr->c);
	out("\n");
}

void
setcurbb(BasicBlock *bb)
{
	currentbb = bb;
	vecappend(basicblocks, bb);
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

	vregcount = 0;
	basicblocks = vec();
	preludebb = mkbasicblock();
	entrybb = mkbasicblock();
	setcurbb(preludebb);
	setcurbb(entrybb);
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

	bbterminate(preludebb, (Terminator){.op=Opjmp, .label1=bbgetlabel(entrybb)});

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
	bb->cap = 16;
	bb->instructions = xmalloc(bb->cap * sizeof(Instruction));
	bb->terminated = 0;
	bb->ninstructions = 0;
	return bb;
}

void
bbappend(BasicBlock *bb, Instruction ins)
{
	Instruction *instrarray;

	if (bb->terminated)
		return;

	if (bb->cap == bb->ninstructions) {
		bb->cap += 64;
		instrarray = xmalloc(bb->cap * sizeof(Instruction));
		bb->instructions = instrarray;
		memcpy(instrarray, bb->instructions, bb->ninstructions * sizeof(Instruction));
	}

	bb->instructions[bb->ninstructions++] = ins;
}

char *
bbgetlabel(BasicBlock *bb)
{
	return vecget(bb->labels, 0);
}

void
bbterminate(BasicBlock *bb, Terminator term)
{
	if (bb->terminated)
		return;

	bb->terminator = term;
	bb->terminated = 1;
}