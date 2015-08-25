#include <u.h>
#include <gc/gc.h>
#include <ds/ds.h>
#include <cc/c.h>

static char *expr(Node *);
static void  stmt(Node *);

static FILE *o;
static int vcounter;

void
emitinit(FILE *out)
{
	o = out;
}

static char *
newv()
{
	int   n;
	char *v;

	n = snprintf(0, 0, "%%v%d", vcounter);
	if(n < 0)
		panic("internal error");
	n++;
	v = gcmalloc(n);
	if(snprintf(v, n, "%%v%d", vcounter) < 0)
		panic("internal error");
	vcounter += 1;
	return v;
}

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
outi(char *fmt, ...)
{
	va_list va;

	out("  ");
	va_start(va, fmt);
	if(vfprintf(o, fmt, va) < 0)
		errorf("Error printing\n");
	va_end(va);
}


static void
store(CTy *t, char *v, char *p)
{
	if(isitype(t) || isptr(t)) {
		switch(t->size) {
		case 8:
			outi("storel %s, %s\n", v, p);
			break;
		case 4:
			outi("storew %s, %s\n", v, p);
			break;
		case 2:
		case 1:
		default:
			panic("internal error\n");
		}
		return;
	}
	if(isstruct(t)) {
		return;
	}
	if(isarray(t)) {
		return;
	}
	if(isfunc(t)) {
		return;
	}
	errorf("unimplemented store %d\n", t->t);
}

static char *
load(CTy *t, char *p)
{
	char *v;

	if(isitype(t) || isptr(t)) {
		v = newv();
		switch(t->size) {
		case 8:
			outi("%s =l load %s\n", v, p);
			return v;
		case 4:
			outi("%s =w load %s\n", v, p);
			return v;
		case 2:
		case 1:
			break;
		}
	}
	errorf("unimplemented load\n");
	return 0;
}

static char *
addr(Node *n)
{
	Sym *sym;

	switch(n->t) {
	case NIDENT:
		sym = n->Ident.sym;
		switch(sym->sclass) {
			panic("unimplemented addr");
		case SCAUTO:
			return sym->stkloc.label;
		default:
			panic("unimplemented addr");
		}
	default:
		;
	}
	panic("unimplemented addr");
	return 0;
}

static char *
assign(Node *n)
{
	Node *l, *r;
	char *lv, *rv;
	int op;

	op = n->Assign.op;
	l = n->Assign.l;
	r = n->Assign.r;
	if(op == '=') {
		rv = expr(r);
		lv = addr(l);
		if(!isptr(l->type) && !isitype(l->type))
			errorf("unimplemented assign\n");
		store(l->type, rv, lv);
		return rv;
	}
	panic("unimplemented assign");
	return 0;
}

static void
block(Node *n)
{
	Vec *v;
	int  i;

	v = n->Block.stmts;
	for(i = 0; i < v->len ; i++) {
		stmt(vecget(v, i));
	}
}

static void
prologue(Node *f)
{
	int   i;
	Sym  *s;
	char *v;

	out("@start\n");
	for(i = 0; i < f->Func.locals->len; i++) {
		v = newv();
		s = vecget(f->Func.locals, i);
		s->stkloc.label = v;
		outi("%s =l alloc8 %d\n", v, s->type->size);
	}
}

static void
func(Node *f)
{
	vcounter = 0;
	prologue(f);
	block(f->Func.body);
	out("@end\n");
	outi("ret\n");
}

static void
decl(Node *n)
{
	switch(n->Decl.sclass) {
	case SCTYPEDEF:
		break;
	case SCAUTO:
		break;
	case SCSTATIC:
	case SCGLOBAL:
		panic("unimplemented");
		break;
	}
}

static void
ewhile(Node *n)
{
	char *cond, *l;

	l = newlabel();
	out("@%s\n", n->While.lstart);
	cond = expr(n->While.expr);
	outi("jnz %s, @%s, @%s\n", cond, l, n->While.lend);
	out("@%s\n", l);
	stmt(n->While.stmt);
	outi("jmp @%s\n", n->While.lstart);
	out("@%s\n", n->While.lend);
}

static void
eif(Node *n)
{
	char *cond, *ltrue;

	ltrue = newlabel();
	cond = expr(n->If.expr);
	outi("jnz %s, @%s, @%s \n", cond, ltrue, n->If.lelse);
	out("@%s\n", ltrue);
	stmt(n->If.iftrue);
	out("@%s\n", n->If.lelse);
	if(n->If.iffalse)
		stmt(n->If.iffalse);
}

static void
efor(Node *n)
{
	char *cond, *lstmt;

	if(n->For.init)
		expr(n->For.init);
	out("@%s\n", n->For.lstart);
	lstmt = newlabel();
	if(n->For.cond) {
		cond = expr(n->For.cond);
		outi("jnz %s, @%s, @%s\n", cond, lstmt, n->For.lend);
	}
	out("@%s\n", lstmt);
	stmt(n->For.stmt);
	if(n->For.step)
		expr(n->For.step);
	outi("jmp @%s\n", n->For.lstart);
	out("@%s\n", n->For.lend);
}


static void
ereturn(Node *r)
{
	char *v;

	v = expr(r->Return.expr);
	outi("storew %s, $a\n", v);
	outi("ret\n");
}

static char *
ldconst(CTy *t, int64 val)
{
	int   n;
	char *v;

	n = snprintf(0, 0, "%d", (int)val);
	if(n < 0)
		panic("internal error");
	n++;
	v = gcmalloc(n);
	if(snprintf(v, n, "%d", (int)val) < 0)
		panic("internal error");
	return v;
}

static char *
obinop(int op, CTy *t, char *l, char *r)
{
	char *v, *tpfx;

	v = newv();
	switch(t->size) {
	case 8:
		tpfx = "l";
		break;
	case 4:
		tpfx = "w";
		break;
	default:
		panic("bad size obinop");
	}
	switch(op) {
	case '+':
		outi("%s =%s add %s, %s\n", v, tpfx, l, r);
		break;
	case '-':
		outi("%s =%s sub %s, %s\n", v, tpfx, l, r);
		break;
	case '%':
		outi("%s =%s rem %s, %s\n", v, tpfx, l, r);
		break;
	case '<':
		outi("%s =%s cslt %s, %s\n", v, tpfx, l, r);
		break;
	case TOKNEQ:
		outi("%s =%s cne %s, %s\n", v, tpfx, l, r);
		break;
	case TOKEQL:
		outi("%s =%s ceq %s, %s\n", v, tpfx, l, r);
		break;
	default:
		panic("unimplemented op %d", op);
	}
	return v;
}

static char *
binop(Node *n)
{
	char *lv;
	char *rv;

	lv = expr(n->Binop.l);
	rv = expr(n->Binop.r);
	return obinop(n->Binop.op, n->type, lv, rv);
}

static char *
expr(Node *n)
{
	char *v;

	switch(n->t){
	case NIDENT:
		v = addr(n);
		return load(n->type, v);
	case NASSIGN:
		return assign(n);
	case NNUM:
		return ldconst(n->type, n->Num.v);
	case NBINOP:
		return binop(n);
	default:
		errorf("unimplemented emit expr %d\n", n->t);
	}
	return 0;
}

static void
stmt(Node *n)
{
	switch(n->t){
	case NWHILE:
		ewhile(n);
		break;
	case NFOR:
		efor(n);
		break;
	case NIF:
		eif(n);
		break;
	case NBLOCK:
		block(n);
		break;
	case NDECL:
		decl(n);
		break;
	case NRETURN:
		ereturn(n);
		break;
	case NGOTO:
		outi("jmp @%s\n", n->Goto.l);
		break;
	case NEXPRSTMT:
		if(n->ExprStmt.expr)
			expr(n->ExprStmt.expr);
		break;
	default:
		errorf("unimplemented emit stmt %d\n", n->t);
	}
}

static void
global(Node *n)
{
	switch(n->t){
	case NFUNC:
		func(n);
		break;
	case NDECL:
		decl(n);
		break;
	default:
		errorf("unimplemented emit global\n");
	}
}

void
emit(void)
{
	Node *n;

	while(1) {
		n = parsenext();
		if(!n)
			return;
		global(n);
	}
}
