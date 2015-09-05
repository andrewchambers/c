#include <u.h>
#include <gc/gc.h>
#include <ds/ds.h>
#include "c.h"

static Const *
mkconst(char *p, int64 v)
{
	Const *c;

	c = gcmalloc(sizeof(Const));
	c->p = p;
	c->v = v;
	return c;
}

static Const *
foldbinop(Node *n)
{
	Const *l, *r;

	l = foldexpr(n->Binop.l);
	r = foldexpr(n->Binop.r);
	if(!l || !r)
		return 0;
	if(isitype(n->type)) {
		switch(n->Binop.op) {
		case '+':
			if(l->p && r->p)
				return 0;
			if(l->p)
				mkconst(l->p, l->v + r->v);
			if(r->p)
				mkconst(r->p, l->v + r->v);
			return mkconst(0, l->v + r->v);
		case '-':
			if(l->p || r->p) {
				if(l->p && !r->p)
					return mkconst(l->p, l->v - r->v);
				if(!l->p && r->p)
					return 0;
				if(strcmp(l->p, r->p) == 0)
					return mkconst(0, l->v - r->v);
				return 0;
			}
			return mkconst(0, l->v - r->v);
		case '*':
			if(l->p || r->p)
				return 0;
			return mkconst(0, l->v * r->v);
		case '/':
			if(l->p || r->p)
				return 0;
			if(r->v == 0)
				return 0;
			return mkconst(0, l->v / r->v);
		default:
			panic("unimplemented fold binop %d", n->Binop.op);
		}
	}
	panic("unimplemented fold binop");
	return 0;
}

static Const *
foldaddr(Node *n)
{
	Sym *sym;

	if(n->Unop.operand->t != NIDENT)
		return 0;
	sym = n->Unop.operand->Ident.sym;
	if(sym->k != SYMGLOBAL)
		return 0;
	return mkconst(sym->Global.label, 0);
}

static Const *
foldunop(Node *n)
{
	Const *c;

	switch(n->Unop.op) {
	case '&':
		return foldaddr(n);
	case '-':
		c = foldexpr(n->Unop.operand);
		if(!c)
			return 0;
		if(c->p)
			return 0;
		return mkconst(0, -c->v);
	default:
		panic("unimplemented fold unop %d", n->Binop.op);
	}
	panic("unimplemented fold unop");
	return 0;
}

Const *
foldexpr(Node *n)
{
	switch(n->t) {
	case NBINOP:
		return foldbinop(n);
	case NUNOP:
		return foldunop(n);
	case NNUM:
		return mkconst(0, n->Num.v);
	default:
		return 0;
	}
}