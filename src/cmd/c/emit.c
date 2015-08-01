#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "ds/ds.h"
#include "cc/c.h"

static void emitn(Node *n);

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
		emitn(vecget(v, i));
	out("leave\n");
	out("ret\n");
}

static void
emitreturn(Node *r)
{
	emitn(r->Return.expr);
	out("leave\n");
	out("ret\n");
}

static void
emitn(Node *n)
{
	switch(n->t){
	case NFUNC:
		emitfunc(n);
		return;
	case NRETURN:
		emitreturn(n);
		return;
	case NNUM:
		out("movq $%s, %%rax\n", n->Num.v);
		return;
	}
}

void
emitinit(FILE *out)
{
	o = out;
}

void
emit()
{
	Node *n;

	while(1) {
		n = parsenext();
		if(!n)
			return;
		emitn(n);
	}
}