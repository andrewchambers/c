#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "ds/ds.h"
#include "cc/c.h"

static void emitexpr(Node *);
static void emitstmt(Node *);

static FILE *o;

static void
out(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	if(vfprintf(o, fmt, va) < 0)
		errorf("Error printing\n");
	va_end(va);
}

static void
emitfunc(Node *f)
{
	Vec *v;
	int i;
	
	out(".text\n");
	out(".globl %s\n", f->Func.name);
	out("%s:\n", f->Func.name);
	out("pushq %%rbp\n");
	out("movq %%rsp, %%rbp\n");
	v = f->Func.body->Block.stmts;
	for(i = 0; i < v->len; i++)
		emitstmt(vecget(v, i));
	out("leave\n");
	out("ret\n");
}

static void
emitloadreg(int sz)
{
	switch(sz) {
	case 8:
		out("movq (%%rax), %%rax\n");
		break;
	case 4:
		out("movslq (%%rax), %%rax\n");
		break;
	case 2:
		out("movswq (%%rax), %%rax\n");
		break;
	case 1:
		out("movsbq (%%rax), %%rax\n");
		break;
	default:
		errorf("internal error emitloadreg\n");
	}
}

static void
emitstorereg(int sz)
{
	switch(sz) {
	case 8:
		out("movq %%rbx, (%%rax)\n");
		break;
	case 4:
		out("movl %%ebx, (%%rax)\n");
		break;
	case 2:
		out("movw %%bx, (%%rax)\n");
		break;
	case 1:
		out("movb %%bl, (%%rax)\n");
		break;
	default:
		errorf("internal emitstorereg\n");
	}
}

static void
emitreturn(Node *r)
{
	emitexpr(r->Return.expr);
	out("leave\n");
	out("ret\n");
}

static void
emitassign(Node *l, Node *r)
{
	Sym *sym;

	emitexpr(r);
	out("movq %%rax, %%rbx\n");
	switch(l->t) {
	case NUNOP:
		if(l->Unop.op != '*')
			errorf("internal error");
		out("pushq %%rbx\n");
		emitexpr(l->Unop.operand);
		out("popq %%rbx\n");
		emitstorereg(8);
		break;
	case NIDENT:
		sym = l->Ident.sym;
		switch(sym->sclass) {
		case SCSTATIC:
		case SCGLOBAL:
			out("leaq %s(%%rip), %%rax\n", sym->label);
			emitstorereg(8);
			break;
		case SCAUTO:
			out("leaq %d(%%rbp), %%rax\n", sym->offset);
			emitstorereg(8);
			break;
		}
		break;
	default:
		errorf("unimplemented emitassign\n");
	}
}

static void
emitaddr(Node *n)
{
	Sym *sym;

	switch(n->t) {
	case NIDENT:
		sym = n->Ident.sym;
		switch(sym->sclass) {
		case SCSTATIC:
		case SCGLOBAL:
			out("leaq %s(%%rip), %%rax\n", sym->label);
			break;
		case SCAUTO:
			out("leaq %d(%%rbp), %%rax\n", sym->offset);
			break;
		}
		break;
	default:
		errorf("unimplemented emitaddr\n");
	}
}

static void
emitbinop(Node *n)
{
	int   op;
	char *lset;
	char *lafter;
	char *opc;
	
	op = n->Binop.op;
	if(op == '=') {
		emitassign(n->Binop.l, n->Binop.r);
		return;
	}
	emitexpr(n->Binop.l);
	out("pushq %%rax\n");
	emitexpr(n->Binop.r);
	out("movq %%rax, %%rcx\n");
	out("popq %%rax\n");
	switch(op) {
	case '+':
		out("addq %%rcx, %%rax\n");
		break;
	case '-':
		out("subq %%rcx, %%rax\n");
		break;
	case '*':
		out("imul %%rcx, %%rax\n");
		break;
	case '/':
		out("cqto\n");
		out("idiv %%rcx\n");
		break;
	case '%':
		out("idiv %%rcx\n");
		out("mov %%rdx, %%rax\n");
		break;
	case '|':
		out("or %%rcx, %%rax\n");
		break;
	case '&':
		out("and %%rcx, %%rax\n");
		break;
	case '^':
		out("xor %%rcx, %%rax\n");
		break;
	case TOKSHR:
		out("sar %%cl, %%rax\n");
		break;
	case TOKSHL:
		out("sal %%cl, %%rax\n");
		break;
	case TOKEQL:
	case TOKNEQ:
	case '>':
	case '<':
		lset = newlabel();
		lafter = newlabel();
		switch(op) {
		case TOKEQL:
			opc = "jz";
			break;
		case TOKNEQ:
			opc = "jnz";
			break;
		case '<':
			opc = "jl";
			break;
		case '>':
			opc = "jg";
			break;
		}
		out("cmp %%rcx, %%rax\n");
		out("%s %s\n", opc, lset);
		out("movq $0, %%rax\n");
		out("jmp %s\n", lafter);
		out("%s:\n", lset);
		out("movq $1, %%rax\n");
		out("%s:\n", lafter);
		break;
	default:
		errorf("unimplemented binop\n");
	}
}

static void
emitunop(Node *n)
{
	switch(n->Unop.op) {
	case '*':
		emitexpr(n->Unop.operand);
		emitloadreg(8);
		break;
	case '&':
		emitaddr(n->Unop.operand);
		break;
	case '~':
		emitexpr(n->Unop.operand);
		out("notq %%rax\n");
		break;
	case '!':
		emitexpr(n->Unop.operand);
		out("xorq %%rcx, %%rcx\n");
		out("testq %%rax, %%rax\n");
		out("setnz %%cl\n");
		out("movq %%rcx, %%rax\n");
		break;
	case '-':
		emitexpr(n->Unop.operand);
		out("neg %%rax\n");
		break;
	default:
		errorf("unimplemented unop %d\n", n->Unop.op);
	}
}

static void
emitident(Node *n)
{
	int  sz;
	Sym *sym;

	sym = n->Ident.sym;
	switch(sym->sclass) {
	case SCSTATIC:
	case SCGLOBAL:
		out("leaq %s(%%rip), %%rax\n", sym->label);
		break;
	case SCAUTO:
		out("leaq %d(%%rbp), %%rax\n", sym->offset);
		break;
	default:
		errorf("unimplemented ident\n");
	}
	sz = 8; /* TODO: tysize(n->type); */
	emitloadreg(sz);
}

static void
emitif(Node *n)
{
	emitexpr(n->If.expr);
	out("test %%rax, %%rax\n");
	out("jz %s\n", n->If.lelse);
	emitstmt(n->If.iftrue);
	out("%s:\n", n->If.lelse);
	if(n->If.iffalse)
		emitstmt(n->If.iffalse);
}

static void
emitfor(Node *n)
{
	if(n->For.init)
		emitexpr(n->For.init);
	out("%s:\n", n->For.lstart);
	if(n->For.cond)
		emitexpr(n->For.cond);
	out("test %%rax, %%rax\n");
	out("jz %s\n", n->For.lend);
	emitstmt(n->For.stmt);
	if(n->For.step)
		emitexpr(n->For.step);
	out("jmp %s\n", n->For.lstart);
	out("%s:\n", n->For.lend);
}

static void
emitwhile(Node *n)
{
	out("%s:\n", n->While.lstart);
	emitexpr(n->While.expr);
	out("test %%rax, %%rax\n");
	out("jz %s\n", n->While.lend);
	emitstmt(n->While.stmt);
	out("jmp %s\n", n->While.lstart);
	out("%s:\n", n->While.lend);
}

static void
emitdowhile(Node *n)
{
	out("%s:\n", n->DoWhile.lstart);
	emitstmt(n->DoWhile.stmt);
	out("%s:\n", n->DoWhile.lcond);
	emitexpr(n->DoWhile.expr);
	out("test %%rax, %%rax\n");
	out("jz %s\n", n->DoWhile.lend);
	out("jmp %s\n", n->DoWhile.lstart);
	out("%s:\n", n->DoWhile.lend);
}

static void
emitswitch(Node *n)
{
	int   i;
	Node *c;

	emitexpr(n->Switch.expr);
	for(i = 0; i < n->Switch.cases->len; i++) {
		c = vecget(n->Switch.cases, i);
		if(c->Case.expr->t != NNUM)
			errorposf(&c->pos, "unimplemented");
		out("mov $%s, %%rcx\n", c->Case.expr->Num.v);
		out("cmp %%rax, %%rcx\n");
		out("je %s\n", c->Case.l);
	}
	if(n->Switch.ldefault) {
		out("jmp %s\n", n->Switch.ldefault);
	} else {
		out("jmp %s\n", n->Switch.lend);
	}
	emitstmt(n->Switch.stmt);
	out("%s:\n", n->Switch.lend);
}

static void
emitblock(Node *n)
{
	Vec *v;
	int  i;

	v = n->Block.stmts;
	for(i = 0; i < v->len ; i++)
		emitstmt(vecget(v, i));
}

static void
emitexpr(Node *n)
{
	switch(n->t){
	case NNUM:
		out("movq $%s, %%rax\n", n->Num.v);
		break;
	case NIDENT:
		emitident(n);
		break;
	case NUNOP:
		emitunop(n);
		break;
	case NBINOP:
		emitbinop(n);
		break;
	default:
		errorf("unimplemented emit expr %d\n", n->t);
	}
}

static void
emitstmt(Node *n)
{
	switch(n->t){
	case NDECL:
		/* TODO */
		break;
	case NRETURN:
		emitreturn(n);
		break;
	case NIF:
		emitif(n);
		break;
	case NWHILE:
		emitwhile(n);
		break;
	case NFOR:
		emitfor(n);
		break;
	case NDOWHILE:
		emitdowhile(n);
		break;
	case NBLOCK:
		emitblock(n);
		break;
	case NSWITCH:
		emitswitch(n);
		break;
	case NGOTO:
		out("jmp %s\n", n->Goto.l);
		break;
	case NCASE:
		out("%s:\n", n->Case.l);
		emitstmt(n->Case.stmt);
		break;
	case NLABELED:
		out("%s:\n", n->Labeled.l);
		emitstmt(n->Labeled.stmt);
		break;
	case NEXPRSTMT:
		if(n->ExprStmt.expr)
			emitexpr(n->ExprStmt.expr);
		break;
	default:
		errorf("unimplemented emit stmt %d\n", n->t);
	}	
}

static void
emitglobaldecl(Node *n)
{
	int  i;
	Sym *sym;

	if(n->Decl.sclass == SCTYPEDEF)
		return;
	if(n->Decl.sclass != SCGLOBAL && n->Decl.sclass != SCSTATIC)
		abort(); /* Invariant violated */
	for(i = 0; i < n->Decl.syms->len ; i++) {
		sym = vecget(n->Decl.syms, i);
		out(".comm %s, %d, %d\n", sym->label, 8, 8);
	}
}

static void
emitglobal(Node *n)
{
	switch(n->t){
	case NFUNC:
		emitfunc(n);
		break;
	case NDECL:
		emitglobaldecl(n);
		break;
	default:
		errorf("unimplemented emit global\n");
	}
}

void
emitinit(FILE *out)
{
	o = out;
}

void
emit(void)
{
	Node *n;

	while(1) {
		n = parsenext();
		if(!n)
			return;
		emitglobal(n);
	}
}
