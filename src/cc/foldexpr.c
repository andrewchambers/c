#include <u.h>
#include <mem/mem.h>
#include <ds/ds.h>
#include "cc.h"

static Const *
mkconst(char *p, int64 v)
{
	Const *c;

	c = xmalloc(sizeof(Const));
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
		case TOKSHL:
			if(l->p || r->p)
				return 0;
			return mkconst(0, l->v << r->v);
		case '|':
			if(l->p || r->p)
				return 0;
			return mkconst(0, l->v | r->v);
		default:
			errorposf(&n->pos, "unimplemented fold binop %d", n->Binop.op);
		}
	}
	panic("unimplemented fold binop");
	return 0;
}

static Const *
foldaddr(Node *n)
{
	char *l;
	Sym  *sym;

	if(n->Unop.operand->t == NINIT) {
		l = newlabel();
		penddata(l, n->Unop.operand->type, n->Unop.operand, 0);
		return mkconst(l, 0);
	}
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
foldcast(Node *n)
{
	if(!isitype(n->type))
		return 0;
	if(!isitype(n->Cast.operand->type))
		return 0;
	return foldexpr(n->Cast.operand);
}

Const *
foldident(Node *n)
{
	Sym *sym;

	sym = n->Ident.sym;
	switch(sym->k) {
	case SYMENUM:
		return mkconst(0, sym->Enum.v);
	default:
		return 0;
	}
}

Const *
foldexpr(Node *n)
{
	char *l;
	CTy  *ty;

	switch(n->t) {
	case NBINOP:
		return foldbinop(n);
	case NUNOP:
		return foldunop(n);
	case NNUM:
		return mkconst(0, n->Num.v);
	case NSIZEOF:
		return mkconst(0, n->Sizeof.type->size);
	case NCAST:
		return foldcast(n);
	case NIDENT:
		return foldident(n);
	case NSTR:
		l = newlabel();
		ty = newtype(CARR);
		ty->Arr.subty = cchar;
		/* XXX DIM? */
		penddata(l, ty, n, 0);
		return mkconst(l, 0);
	default:
		return 0;
	}
}

