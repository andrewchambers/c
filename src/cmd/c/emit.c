#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "ds/ds.h"
#include "cc/c.h"

static void emitstmt(Node *n);

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
emitreturn(Node *r)
{
	emitstmt(r->Return.expr);
	out("leave\n");
	out("ret\n");
}

static void
emitstmt(Node *n)
{
	switch(n->t){
	case NRETURN:
		emitreturn(n);
		return;
	case NNUM:
		out("movq $%s, %%rax\n", n->Num.v);
		return;
	case NBINOP:
		out("XXX binop\n");
		return;
	default:
		errorf("unimplemented emit stmt\n");
	}	
}

static void
emitglobaldecl(Node *n)
{
	int  i;
	Sym *sym;

	if(n->Decl.sclass == SCTYPEDEF)
		return;
	if(n->Decl.sclass != SCGLOBAL && n->Decl.sclass != SCGLOBAL)
		abort(); /* Invariant violated */
	for(i = 0; i < n->Decl.syms->len ; i++) {
		sym = vecget(n->Decl.syms, i);
		if(sym->sclass == SCGLOBAL)
			out(".global %s\n", sym->label);
		out("%s:\n", sym->label);
		out(".lcomm %s, %d\n", sym->label, 8);
	}

}

static void
emitglobal(Node *n)
{
	switch(n->t){
	case NFUNC:
		emitfunc(n);
		return;
	case NDECL:
		emitglobaldecl(n);
		return;
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