#include <u.h>
#include <gc/gc.h>
#include <ds/ds.h>
#include "c.h"

static Node *stmt(void);
static Node *pif(void);
static Node *pfor(void);
static Node *dowhile(void);
static Node *pwhile(void);
static Node *block(void);
static Node *preturn(void);
static Node *pswitch(void);
static Node *pdefault(void);
static Node *pcase(void);
static Node *pcontinue(void);
static Node *pbreak(void);
static Node *stmt(void);
static Node *exprstmt(void);
static Node *expr(void);
static Node *assignexpr(void);
static Node *constexpr(void);
static Node *condexpr(void);
static Node *logorexpr(void);
static Node *logandexpr(void);
static Node *orexpr(void);
static Node *xorexpr(void);
static Node *andexpr(void);
static Node *eqlexpr(void);
static Node *relexpr(void);
static Node *shiftexpr(void);
static Node *addexpr(void);
static Node *mulexpr(void);
static Node *castexpr(void);
static Node *unaryexpr(void);
static Node *postexpr(void);
static Node *primaryexpr(void);
static Node *declorstmt(void);
static Node *decl(void);
static Node *declinit(void);
static Node *fbody(SrcPos *, char *, CTy *);
static CTy  *declspecs(int *);
static CTy  *pstruct(void);
static CTy  *penum(void);
static CTy  *typename(void);
static CTy  *declarator(CTy *, char **, Node **);
static CTy  *directdeclarator(CTy *, char **);
static CTy  *declaratortail(CTy *);
static Node *ipromote(Node *);
static CTy  *usualarithconv(Node **, Node **);
static Node *mkcast(SrcPos *, Node *, CTy *);
static void  expect(int);
static int   islval(Node *);
static int   isassignop(int);

Tok *tok;
Tok *nexttok;

#define MAXSCOPES 1024
static int nscopes;
static Map *tags[MAXSCOPES];
static Map *syms[MAXSCOPES];

#define MAXLABELDEPTH 2048
static int   switchdepth;
static int   brkdepth;
static int   contdepth;
static char *breaks[MAXLABELDEPTH];
static char *conts[MAXLABELDEPTH];
static Node *switches[MAXLABELDEPTH];

Map *labels;
Vec *gotos;

int localoffset;
int labelcount;

char *
newlabel(void)
{
	char *s;
	int   n;

	n = snprintf(0, 0, ".L%d", labelcount);
	if(n < 0)
		panic("internal error\n");
	n += 1;
	s = gcmalloc(n);
	snprintf(s, n, ".L%d", labelcount);
	labelcount++;
	return s;
}

static void
pushswitch(Node *n)
{
	switches[switchdepth] = n;
	switchdepth += 1;
}

static void
popswitch(void)
{
	switchdepth -= 1;
	if(switchdepth < 0)
		panic("internal error\n");
}

static Node *
curswitch(void)
{
	if(switchdepth == 0)
		return 0;
	return switches[switchdepth - 1];
}

static void
pushcontbrk(char *lcont, char *lbreak)
{
	conts[contdepth] = lcont;
	breaks[brkdepth] = lbreak;
	brkdepth += 1;
	contdepth += 1;
}

static void
popcontbrk(void)
{
	brkdepth -= 1;
	contdepth -= 1;
	if(brkdepth < 0 || contdepth < 0)
		panic("internal error\n");
}

static char *
curcont()
{
	if(contdepth == 0)
		return 0;
	return conts[contdepth - 1];
}

static char *
curbrk()
{
	if(brkdepth == 0)
		return 0;
	return breaks[brkdepth - 1];
}

static void
pushbrk(char *lbreak)
{
	breaks[brkdepth] = lbreak;
	brkdepth += 1;
}

static void
popbrk(void)
{
	brkdepth -= 1;
	if(brkdepth < 0)
		panic("internal error\n");
}


static void
popscope(void)
{
	nscopes -= 1;
	if(nscopes < 0)
		errorf("bug: scope underflow\n");
	syms[nscopes] = 0;
	tags[nscopes] = 0;
}

static void
pushscope(void)
{
	syms[nscopes] = map();
	tags[nscopes] = map();
	nscopes += 1;
	if(nscopes > MAXSCOPES)
		errorf("scope depth exceeded maximum\n");
}

static int
isglobal(void)
{
	return nscopes == 1;
}

static int
islval(Node *n)
{
	switch(n->t) {
	case NUNOP:
		if(n->Unop.op == '*')
			return 1;
		return 0;
	case NIDENT:
		return 1;
	case NIDX:
		return 1;
	case NSEL:
		return 1;
	}
	return 0;
}

static int
define(Map *scope[], char *k, void *v)
{
	Map *m;
	
	m = scope[nscopes - 1];
	if(mapget(m, k))
		return 0;
	mapset(m, k, v);
	return 1; 
}

static void *
lookup(Map *scope[], char *k)
{
	int   i;
	void *v;
	
	i = nscopes;
	while(i--) {
		v = mapget(scope[i], k);
		if(v)
			return v;
	}
	return 0;
}

static void
definesym(char *name, Sym *sym)
{
	/* Handle valid redefines */
	if(!define(syms, name, sym)) {
		/* TODO:
		 Check if types are compatible.
		*/
		 errorposf(sym->pos, "redefinition of symbol %s", name);
	}
}

static CTy *
newtype(int type)
{
	CTy *t;

	t = gcmalloc(sizeof(CTy));
	t->t = type;
	return t;
}

static CTy *
mkptr(CTy *t)
{
	CTy *p;

	p = newtype(CPTR);
	p->Ptr.subty = t;
	p->size = 8;
	p->align = 8;
	return p;
}

static CTy *
mkprimtype(int type, int sig)
{
	CTy *t;
	
	t = newtype(CPRIM);
	t->Prim.type = type;
	t->Prim.issigned = sig;
	switch(t->Prim.type){
	case PRIMCHAR:
		t->size = 1;
		t->align = 1;
		break;
	case PRIMSHORT:
		t->size = 2;
		t->align = 2;
		break;
	case PRIMINT:
		t->size = 4;
		t->align = 4;
		break;
	case PRIMLONG:
		t->size = 8;
		t->align = 8;
		break;
	case PRIMLLONG:
		t->size = 8;
		t->align = 8;
		break;
	default:
		panic("internal error mkprimtype\n");
	}
	return t;
}

static NameTy *
newnamety(char *n, CTy *t)
{
	NameTy *nt;
	
	nt = gcmalloc(sizeof(NameTy));
	nt->name = n;
	nt->type = t;
	return nt;
}

static Node *
mknode(int type, SrcPos *p)
{
	Node *n;

	n = gcmalloc(sizeof(Node));
	n->pos = *p;
	n->t = type;
	return n;
}

static Node *
mkblock(SrcPos *p, Vec *v)
{
	Node *n;

	n = mknode(NBLOCK, p);
	n->Block.stmts = v;
	return n;
}

static Node *
mkcomma(SrcPos *p, Vec *v)
{
	Node *n;

	n = mknode(NCOMMA, p);
	n->Comma.exprs = v;
	return n;
}

static Node *
mkincdec(SrcPos *p, int op, int post, Node *operand)
{
	Node *n;

	if(!islval(operand))
		errorposf(&operand->pos, "++ and -- expects an lvalue");
	n = mknode(NINCDEC, p);
	n->Incdec.op = op;
	n->Incdec.post = post;
	n->Incdec.operand = operand;
	n->type = operand->type;
	return n;
}

static Node *
mkbinop(SrcPos *p, int op, Node *l, Node *r)
{
	Node *n;
	CTy  *t;
	
	l = ipromote(l);
	r = ipromote(r);
	t = usualarithconv(&l, &r);
	n = mknode(NBINOP, p);
	n->Binop.op = op;
	n->Binop.l = l;
	n->Binop.r = r;
	n->type = t;
	return n;
}

static Node *
mkassign(SrcPos *p, int op, Node *l, Node *r)
{
	Node *n;
	CTy  *t;

	if(!islval(l))
		errorposf(&l->pos, "assign expects an lvalue");
	r = mkcast(p, r, l->type);
	t = l->type;
	n = mknode(NASSIGN, p);
	switch(op) {
	case '=':
		n->Assign.op = '=';
		break;
	case TOKADDASS:
		n->Assign.op = '+';
		break;
	case TOKSUBASS:
		n->Assign.op = '-';
		break;
	case TOKORASS:
		n->Assign.op = '|';
		break;
	case TOKANDASS:
		n->Assign.op = '&';
		break;
	case TOKMULASS:
		n->Assign.op = '*';
		break;
	default:
		panic("mkassign");
	}
	n->Assign.l = l;
	n->Assign.r = r;
	n->type = t;
	return n;
}

static Node *
mkunop(SrcPos *p, int op, Node *o)
{
	Node *n;
	
	n = mknode(NUNOP, p);
	n->Unop.op = op;
	switch(op) {
	case '&':
		if(!islval(o))
			errorposf(&o->pos, "& expects an lvalue");
		n->type = mkptr(o->type);
		break;
	case '*':
		if(!isptr(o->type))
			errorposf(&o->pos, "cannot deref non pointer");
		n->type = o->type->Ptr.subty;
		break;
	default:
		o = ipromote(o);
		n->type = o->type;
		break;
	}
	n->Unop.operand = o;
	return n;
}

static Node *
mkcast(SrcPos *p, Node *o, CTy *to)
{
	Node *n;
	
	if(sametype(o->type, to))
		return o;
	n = mknode(NCAST, p);
	n->type = to;
	n->Cast.operand = o;
	return n;
}

static Node *
ipromote(Node *n)
{
	if(!isitype(n->type))
		errorposf(&n->pos, "internal error - ipromote expects itype got %d", n->type->t);
	switch(n->type->Prim.type) {
	case PRIMCHAR:
	case PRIMSHORT:
		if(n->type->Prim.issigned)
			return mkcast(&n->pos, n, mkprimtype(PRIMINT, 1));
		else
			return mkcast(&n->pos, n, mkprimtype(PRIMINT, 0));
	}
	return n;
}

static CTy *
usualarithconv(Node **a, Node **b)
{   
	Node **large, **small;
	CTy   *t;

	if(!isarithtype((*a)->type) || !isarithtype((*b)->type))
		panic("internal error\n");
	if(convrank((*a)->type) < convrank((*b)->type)) {
		large = a;
		small = b;
	} else {
		large = b;
		small = a;
	}
	if(isftype((*large)->type)) {
		*small = mkcast(&(*small)->pos, *small, (*large)->type);
		return (*large)->type;
	}
	*large = ipromote(*large);
	*small = ipromote(*small);
	if(sametype((*large)->type, (*small)->type))
		return (*large)->type;
	if((*large)->type->Prim.issigned == (*small)->type->Prim.issigned ) {
		*small = mkcast(&(*small)->pos, *small, (*large)->type);
		return (*large)->type;
	}
	if(!(*large)->type->Prim.issigned) {
		*small = mkcast(&(*small)->pos, *small, (*large)->type);
		return (*large)->type;
	}
	if((*large)->type->Prim.issigned && canrepresent((*large)->type, (*small)->type)) {
		*small = mkcast(&(*small)->pos, *small, (*large)->type);
		return (*large)->type;
	}
	t = mkprimtype((*large)->type->Prim.type, 0);
	*large = mkcast(&(*large)->pos, *large, t);
	*small = mkcast(&(*small)->pos, *small, t);
	return t;
}

static void
next(void)
{
	tok = nexttok;
	nexttok = pp();
}

static void
expect(int kind) 
{
	if(tok->k != kind)
		errorposf(&tok->pos,"expected %s, got %s", 
			tokktostr(kind), tokktostr(tok->k));
	next();
}

void
parseinit()
{
	switchdepth = 0;
	brkdepth = 0;
	contdepth = 0;
	nscopes = 0;
	pushscope();
	next();
	next();
}

Node * 
parsenext()
{
	if(tok->k == TOKEOF)
		return 0;
	return decl();
}

static void
params(CTy *fty)
{
	int     sclass;
	CTy    *t;
	char   *name;
	SrcPos *pos;

	fty->Func.isvararg = 0;
	if(tok->k == ')')
		return;
	for(;;) {
		pos = &tok->pos;
		t = declspecs(&sclass);
		t = declarator(t, &name, 0);
		if(sclass != SCNONE)
			errorposf(pos, "storage class not allowed in parameter decl");
		vecappend(fty->Func.params, newnamety(name, t));
		if(tok->k != ',')
			break;
		next();
	}
	if(tok->k == TOKELLIPSIS) {
		fty->Func.isvararg = 1;
		next();
	}
}

static Sym *
mksym(SrcPos *p, int sclass, char *name, CTy *t)
{
	Sym *s;
	int  sz;

	s = gcmalloc(sizeof(Sym));
	s->pos = p;
	s->sclass = sclass;
	s->name = name;
	s->type = t;
	switch(sclass) {
	case SCGLOBAL:
		s->label = name;
		break;
	case SCSTATIC:
		s->label = newlabel();
		break;
	case SCAUTO:
		if(isglobal())
			errorposf(p, "automatic storage outside of function");
		sz = t->size;
		if(sz < 8)
			sz = 8;
		if(sz % 8)
			sz = sz + 8 - (sz % 8); /* TODO: stack alignment requirements? */
		localoffset -= sz;
		s->offset = localoffset;
		break;
	default:
		panic("internal error");
	}
	return s;
}

static Node *
decl()
{
	Node   *n, *init;
	char   *name;
	CTy    *type, *basety;
	SrcPos *pos;
	Sym    *sym;
	Vec    *syms;
	int     sclass;

	pos = &tok->pos;
	syms  = vec();
	basety = declspecs(&sclass);
	while(tok->k != ';' && tok->k != TOKEOF) {
		type = declarator(basety, &name, &init);
		switch(sclass){
		case SCNONE:
			if(isglobal()) {
				sclass = SCGLOBAL;
			} else {
				sclass = SCAUTO;
			}
			break;
		case SCTYPEDEF:
			if(init)
				errorposf(pos, "typedef cannot have an initializer");
			break;
		}
		if(!name)
			errorposf(pos, "decl needs to specify a name");
		sym = mksym(pos, sclass, name, type);
		vecappend(syms, sym);
		definesym(name, sym);
		if(isglobal() && tok->k == '{') {
			if(init)
				errorposf(pos, "function declaration has an initializer");
			return fbody(pos, name, type);
		}
		if(tok->k == ',')
			next();
		else
			break;
	}
	expect(';');
	n = mknode(NDECL, pos);
	n->Decl.sclass = sclass;
	n->Decl.syms = syms;
	return n;
}

static Node *
fbody(SrcPos *pos, char *name, CTy *type)
{
	Node   *n, *body, *gotofixup;
	int     i;
	char   *l;
	NameTy *nt;
	Vec    *params;
	Sym    *sym;

	params = vec();
	localoffset = 0;
	if(type->t != CFUNC)
		errorposf(pos, "expected a function");
	pushscope();
	labels = map();
	gotos = vec();
	for(i = 0; i < type->Func.params->len; i++) {
		nt = vecget(type->Func.params, i);
		if(nt->name) {
			sym = mksym(pos, SCAUTO, nt->name, nt->type);
			definesym(nt->name, sym);
			vecappend(params, sym);
		}
	}
	body = block();
	popscope();
	for(i = 0 ; i < gotos->len ; i++) {
		gotofixup = vecget(gotos, i);
		l = mapget(labels, gotofixup->Goto.name);
		if(!l)
			errorposf(&gotofixup->pos, "goto target does not exist");
		gotofixup->Goto.l = l;
	}
	n = mknode(NFUNC, pos);
	n->type = type;
	n->Func.body = body;
	n->Func.name = name;
	n->Func.localsz = localoffset;
	n->Func.params = params;
	return n;
}

static int
issclasstok(Tok *t) {
	switch(tok->k) {
	case TOKEXTERN:
	case TOKSTATIC:
	case TOKREGISTER:
	case TOKTYPEDEF:
	case TOKAUTO:
		return 1;
	}
	return 0;
}

static CTy *
declspecs(int *sclass)
{
	CTy    *t;
	SrcPos *pos;
	Sym    *sym;
	int     bits;

	enum {
		BITCHAR = 1<<0,
		BITSHORT = 1<<1,
		BITINT = 1<<2,
		BITLONG = 1<<3,
		BITLONGLONG = 1<<4,
		BITSIGNED = 1<<5,
		BITUNSIGNED = 1<<6,
		BITFLOAT = 1<<7,
		BITDOUBLE = 1<<8,
		BITENUM = 1<<9,
		BITSTRUCT = 1<<10,
		BITVOID = 1<<11,
		BITIDENT = 1<<12,
	};

	t = 0;
	bits = 0;
	pos = &tok->pos;
	*sclass = SCNONE;

	for(;;) {
		if(issclasstok(tok)) {
			if(*sclass != SCNONE)
				errorposf(pos, "multiple storage classes in declaration specifiers.");
			switch(tok->k) {
			case TOKEXTERN:
				*sclass = SCEXTERN;
				break;
			case TOKSTATIC:
				*sclass = SCSTATIC;
				break;
			case TOKREGISTER:
				*sclass = SCREGISTER;
				break;
			case TOKAUTO:
				*sclass = SCAUTO;
				break;
			case TOKTYPEDEF:
				*sclass = SCTYPEDEF;
				break;
			}
			next();
			continue;
		}
		switch(tok->k) {
		case TOKCONST:
		case TOKVOLATILE:
			next();
			break;
		case TOKSTRUCT:
		case TOKUNION:
			if(bits)
				goto err;
			bits |= BITSTRUCT;
			t = pstruct();
			goto done;
		case TOKENUM:
			if(bits)
				goto err;
			bits |= BITENUM;
			t = penum();
			goto done;
		case TOKVOID:
			if(bits&BITVOID)
				goto err;
			bits |= BITVOID;
			next();
			goto done;
		case TOKCHAR:
			if(bits&BITCHAR)
				goto err;
			bits |= BITCHAR;
			next();
			break;
		case TOKSHORT:
			if(bits&BITSHORT)
				goto err;
			bits |= BITSHORT;
			next();
			break;
		case TOKINT:
			if(bits&BITINT)
				goto err;
			bits |= BITINT;
			next();
			break;
		case TOKLONG:
			if(bits&BITLONGLONG)
				goto err;
			if(bits&BITLONG) {
				bits &= ~BITLONG;
				bits |= BITLONGLONG;
			} else {
				bits |= BITLONG;
			}
			next();
			break;
		case TOKFLOAT:
			if(bits&BITFLOAT)
				goto err;
			bits |= BITFLOAT;
			next();
			break;
		case TOKDOUBLE:
			if(bits&BITDOUBLE)
				goto err;
			bits |= BITDOUBLE;
			next();
			break;
		case TOKSIGNED:
			if(bits&BITSIGNED)
				goto err;
			bits |= BITSIGNED;
			next();
			break;
		case TOKUNSIGNED:
			if(bits&BITUNSIGNED)
				goto err;
			bits |= BITUNSIGNED;
			next();
			break;
		case TOKIDENT:
			sym = lookup(syms, tok->v);
			if(sym && sym->sclass == SCTYPEDEF)
				t = sym->type;
			if(t && !bits) {
				bits |= BITIDENT;
				next();
				goto done;
			}
			/* fallthrough */
		default:
			goto done;
		}
	}
	done:
	switch(bits){
	case BITFLOAT:
		return mkprimtype(PRIMFLOAT, 0);
	case BITDOUBLE:
		return mkprimtype(PRIMDOUBLE, 0);
	case BITLONG|BITDOUBLE:
		return mkprimtype(PRIMLDOUBLE, 0);
	case BITSIGNED|BITCHAR:
	case BITCHAR:
		return mkprimtype(PRIMCHAR, 1);
	case BITUNSIGNED|BITCHAR:
		return mkprimtype(PRIMCHAR, 0);
	case BITSIGNED|BITSHORT|BITINT:
	case BITSHORT|BITINT:
	case BITSHORT:
		return mkprimtype(PRIMSHORT, 1);
	case BITUNSIGNED|BITSHORT|BITINT:
	case BITUNSIGNED|BITSHORT:
		return mkprimtype(PRIMSHORT, 0);
	case BITSIGNED|BITINT:
	case BITSIGNED:
	case BITINT:
	case 0:
		return mkprimtype(PRIMINT, 1);
	case BITUNSIGNED|BITINT:
	case BITUNSIGNED:
		return mkprimtype(PRIMINT, 0);
	case BITSIGNED|BITLONG|BITINT:
	case BITSIGNED|BITLONG:
	case BITLONG|BITINT:
	case BITLONG:
		return mkprimtype(PRIMLONG, 1);
	case BITUNSIGNED|BITLONG|BITINT:
	case BITUNSIGNED|BITLONG:
		return mkprimtype(PRIMLONG, 0);
	case BITSIGNED|BITLONGLONG|BITINT:
	case BITSIGNED|BITLONGLONG:
	case BITLONGLONG|BITINT:
	case BITLONGLONG:
		return mkprimtype(PRIMLLONG, 1);
	case BITUNSIGNED|BITLONGLONG|BITINT:
	case BITUNSIGNED|BITLONGLONG:
		return mkprimtype(PRIMLLONG, 0);
	case BITVOID:
		t = newtype(CVOID);
		return t;
	case BITENUM:
		/* TODO */
	case BITSTRUCT:
	case BITIDENT:
		return t;
	default:
		goto err;
	}
	err:
	errorposf(pos, "invalid declaration specifiers");
	return 0;
}

/* Declarator is what introduces names into the program. */
static CTy *
declarator(CTy *basety, char **name, Node **init) 
{
	CTy *t, *subt;

	while (tok->k == TOKCONST || tok->k == TOKVOLATILE)
		next();
	switch(tok->k) {
	case '*':
		next();
		subt = declarator(basety, name, init);
		t = mkptr(subt);
		return t;
	default:
		t = directdeclarator(basety, name);
		if(tok->k == '=') {
			if(!init)
				errorposf(&tok->pos, "unexpected initializer");
			next();
			*init = declinit();
		} else {
			if(init)
				*init = 0;
		}
		return t; 
	}

}

static CTy *
directdeclarator(CTy *basety, char **name) 
{
	*name = 0;
	switch(tok->k) {
	case '(':
		expect('(');
		basety = declarator(basety, name, 0);
		expect(')');
		return declaratortail(basety);
	case TOKIDENT:
		if(name)
			*name = tok->v;
		next();
		return declaratortail(basety);
	default:
		if(!name)
			errorposf(&tok->pos, "expected ident or ( but got %s", tokktostr(tok->k));
		return declaratortail(basety);
	}
	errorf("unreachable");
	return 0;
}

static CTy *
declaratortail(CTy *basety)
{
	CTy  *t, *newt;
	Node *c;
	
	t = basety;
	for(;;) {
		c = 0;
		switch (tok->k) {
		case '[':
			newt = newtype(CARR);
			newt->Arr.subty = t;
			newt->Arr.dim = -1;
			next();
			if(tok->k != ']')
				c = constexpr();
			expect(']');
			if(c) {
				if(c->t != NNUM)
					errorposf(&tok->pos, "currently only constant sized arrays supported");
				newt->Arr.dim = c->Num.v;
				newt->size = newt->Arr.dim * newt->Arr.subty->size;
				newt->align = newt->Arr.subty->align;
			}
			t = newt;
			break;
		case '(':
			newt = newtype(CFUNC);
			newt->Func.rtype = basety;
			newt->Func.params = vec();
			next();
			params(newt);
			if(tok->k != ')')
				errorposf(&tok->pos, "expected valid parameter or )");
			next();
			t = newt;
			break;
		default:
			return t;
		}
	}
}

static void
pstructbody(CTy *strct)
{
	SrcPos *p;
	char   *name;
	int     sclass;
	CTy    *t, *basety;

	expect('{');
	while(tok->k != '}') {
		basety = declspecs(&sclass);
		for(;;) {
			p = &tok->pos;
			t = declarator(basety, &name, 0);
			if(tok->k == ':') {
				next();
				constexpr();
			}
			addstructmember(p, strct, name, t);
			if(tok->k != ',')
				break;
			next();
		}
		expect(';');
	}
	expect('}');
}

static CTy *
pstruct() 
{
	CTy  *new, *t;
	int   hastagname;

	hastagname = 0;
	if(tok->k != TOKUNION && tok->k != TOKSTRUCT)
		errorposf(&tok->pos, "expected union or struct");
	new = newtype(CSTRUCT);
	new->Struct.isunion = tok->k == TOKUNION;
	new->Struct.members = vec();
	new->align = 8;
	next();
	if(tok->k == TOKIDENT) {
		hastagname = 1;
		new->Struct.name = tok->v;
		next();
	}
	if(!hastagname && tok->k != '{')
		errorposf(&tok->pos, "expected struct or union declaration");
	if(hastagname && tok->k != '{') {
		t = lookup(tags, new->Struct.name);
		if(t) {
			if(t->t != CSTRUCT || t->Struct.isunion != new->Struct.isunion)
				errorposf(&tok->pos, "%s already declared with a differet tag kind", new->Struct.name);
			return t;
		}
		new->Struct.unspecified = 1;
		if(!define(tags, new->Struct.name, new))
			panic("internal error pstruct\n");
		return new;
	}
	pstructbody(new);
	if(!hastagname)
		return new;
	/* TODO overwrite if unspecified in current scope */
	if(!define(tags, new->Struct.name, new))
		errorposf(&tok->pos, "redefinition of tag %s", new->Struct.name);
	return new;
}

static CTy *
penum()
{
	CTy *t;

	expect(TOKENUM);
	if(tok->k == TOKIDENT) {
		if(!define(tags, tok->v, "TODO"))
			errorposf(&tok->pos, "redefinition of tag %s", tok->v);
		next();
	}
	expect('{');
	for(;;) {
		/* if(!definesym(tok->v, "TODO"))
			errorposf(&tok->pos, "redefinition of symbol %s", tok->v);
		*/
		expect(TOKIDENT);
		if(tok->k == '=') {
			next();
			constexpr();
		}
		if(tok->k != ',')
			break;
		next();
	}
	expect('}');
	t = newtype(CPRIM);
	t->Prim.type = PRIMENUM;
	return t;
}

static Node *
pif(void)
{
	SrcPos *p;
	Node   *n, *e, *t, *f;
	
	p = &tok->pos;
	expect(TOKIF);
	expect('(');
	e = expr();
	expect(')');
	t = stmt();
	if(tok->k != TOKELSE) {
		f = 0;
	} else {
		expect(TOKELSE);
		f = stmt();
	}
	n = mknode(NIF, p);
	n->If.lelse = newlabel();
	n->If.expr = e;
	n->If.iftrue = t;
	n->If.iffalse = f;
	return n;
}

static Node *
pfor(void)
{
	SrcPos *p;
	Node   *n, *i, *c, *s, *st;
	char   *lcont, *lbreak;

	i = 0;
	c = 0;
	s = 0;
	st = 0;
	lcont = newlabel();
	lbreak = newlabel();
	p = &tok->pos;
	expect(TOKFOR);
	expect('(');
	if(tok->k == ';') {
		next();
	} else {
		i = expr();
		expect(';');
	}
	if(tok->k == ';') {
		next();
	} else {
		c = expr();
		expect(';');
	}
	if(tok->k != ')')
		s = expr();
	expect(')');
	pushcontbrk(lcont, lbreak);
	st = stmt();
	popcontbrk();
	n = mknode(NFOR, p);
	n->For.lstart = lcont;
	n->For.lend = lbreak;
	n->For.init = i;
	n->For.cond = c;
	n->For.step = s;
	n->For.stmt = st;
	return n;
}

static Node *
pwhile(void)
{
	SrcPos *p;
	Node   *n, *e, *s;
	char   *lcont, *lbreak;
	
	lcont = newlabel();
	lbreak = newlabel();
	p = &tok->pos;	
	expect(TOKWHILE);
	expect('(');
	e = expr();
	expect(')');
	pushcontbrk(lcont, lbreak);
	s = stmt();
	popcontbrk();
	n = mknode(NWHILE, p);
	n->While.lstart = lcont;
	n->While.lend = lbreak;
	n->While.expr = e;
	n->While.stmt = s;
	return n;
}

static Node *
dowhile(void)
{
	SrcPos *p;
	Node   *n, *e, *s;
	char   *lstart, *lcont, *lbreak;
	
	lstart = newlabel();
	lcont = newlabel();
	lbreak = newlabel();
	p = &tok->pos;
	expect(TOKDO);
	pushcontbrk(lcont, lbreak);
	s = stmt();
	popcontbrk();
	expect(TOKWHILE);
	expect('(');
	e = expr();
	expect(')');
	expect(';');
	n = mknode(NDOWHILE, p);
	n->DoWhile.lstart = lstart;
	n->DoWhile.lcond = lcont;
	n->DoWhile.lend = lbreak;
	n->DoWhile.expr = e;
	n->DoWhile.stmt = s;
	return n;
}

static Node *
pswitch(void)
{
	SrcPos *p;
	Node   *n, *e, *s;
	char   *lbreak;
	
	lbreak = newlabel();
	p = &tok->pos;
	expect(TOKSWITCH);
	expect('(');
	e = expr();
	expect(')');
	n = mknode(NSWITCH, p);
	n->Switch.lend = lbreak;
	n->Switch.expr = e;
	n->Switch.cases = vec();
	pushbrk(lbreak);
	pushswitch(n);
	s = stmt();
	popswitch();
	popbrk();
	n->Switch.stmt = s;
	return n;
}

static Node *
pgoto()
{
	Node *n;

	n = mknode(NGOTO, &tok->pos);
	expect(TOKGOTO);
	n->Goto.name = tok->v;
	expect(TOKIDENT);
	expect(';');
	vecappend(gotos, n);
	return n;
}

static int
istypename(char *n)
{
	Sym *sym;

	sym = lookup(syms, nexttok->v);
	if(sym && sym->sclass == SCTYPEDEF)
		return 1;
	return 0;
}

static int
istypestart(Tok *t)
{
	switch(t->k) {
	case TOKENUM:
	case TOKSTRUCT:
	case TOKUNION:
	case TOKVOID:
	case TOKCHAR:
	case TOKSHORT:
	case TOKINT:
	case TOKLONG:
	case TOKSIGNED:
	case TOKUNSIGNED:
		return 1;
	case TOKIDENT:	
		return istypename(t->v);
	}
	return 0;
}

static int
isdeclstart(Tok *t)
{
	if(istypestart(t))
		return 1;
	switch(tok->k) {
	case TOKREGISTER:
	case TOKSTATIC:
	case TOKAUTO:
	case TOKCONST:
	case TOKVOLATILE:
		return 1;
	case TOKIDENT:
		return istypename(t->v);
	}
	return 0;
}

static Node *
declorstmt()
{
	if(isdeclstart(tok))
		return decl();
	return stmt();
}

static Node *
stmt(void)
{
	Tok  *t;
	Node *n;
	char *label;

	if(tok->k == TOKIDENT && nexttok->k == ':') {
		t = tok;
		label = newlabel();
		next();
		next();
		if(mapget(labels, t->v))
			errorposf(&t->pos, "redefinition of label %s", t->v);
		mapset(labels, t->v, label);
		n = mknode(NLABELED, &t->pos);
		n->Labeled.stmt = stmt();
		n->Labeled.l = label;
		return n;
	}
	switch(tok->k) {
	case TOKIF:
		return pif();
	case TOKFOR:
		return pfor();
	case TOKWHILE:
		return pwhile();
	case TOKDO:
		return dowhile();
	case TOKRETURN:
		return preturn();
	case TOKSWITCH:
		return pswitch();
	case TOKCASE:
		return pcase();
	case TOKDEFAULT:
		return pdefault();
	case TOKBREAK:
		return pbreak();
	case TOKCONTINUE:
		return pcontinue();
	case TOKGOTO:
		return pgoto();
	case '{':
		return block();
	default:
		return exprstmt();
	}
}

static Node *
declinit(void)
{
	Node *n;

	if(tok->k != '{')
		return assignexpr();
	expect('{');
	n = mknode(NINIT, &tok->pos);
	n->Init.inits = vec();
	for(;;) {
		if(tok->k == '}')
			break;
		switch(tok->k){
		case '[':
			next();
			constexpr();
			expect(']');
			expect('=');
			/* TODO */
			vecappend(n->Init.inits, declinit());
			break;
		case '.':
			next();
			expect(TOKIDENT);
			expect('=');
			/* TODO */
			vecappend(n->Init.inits, declinit());
			break;
		default:
			vecappend(n->Init.inits, declinit());
			break;
		}
		if(tok->k != ',')
			break;
		next();
	}
	expect('}');
	return n;
}

static Node *
exprstmt(void)
{
	Node *n;
	
	n = mknode(NEXPRSTMT, &tok->pos);
	if(tok->k == ';') {
		next();
		return n;
	}
	n->ExprStmt.expr = expr();
	expect(';');
	return n;
}

static Node *
preturn(void)
{   
	Node *n;

	n = mknode(NRETURN, &tok->pos);
	expect(TOKRETURN);
	if(tok->k != ';')
		n->Return.expr = expr();
	expect(';');
	return n;
}

static Node *
pcontinue(void)
{
	SrcPos *pos;
	Node   *n;
	char   *l;
	
	pos = &tok->pos;
	n = mknode(NGOTO, pos);
	l = curcont();
	if(!l)
		errorposf(pos, "continue without parent statement");
	n->Goto.l = l;
	expect(TOKCONTINUE);
	expect(';');
	return n;
}

static Node *
pbreak(void)
{
	SrcPos *pos;
	Node   *n;
	char   *l;
	
	pos = &tok->pos;
	n = mknode(NGOTO, pos);
	l = curbrk();
	if(!l)
		errorposf(pos, "break without parent statement");
	n->Goto.l = l;
	expect(TOKBREAK);
	expect(';');
	return n;
}

static Node *
pdefault(void)
{
	SrcPos *pos;
	Node   *n, *s;
	char   *l;

	pos = &tok->pos;
	l = newlabel();
	n = mknode(NLABELED, pos);
	n->Labeled.l = l;
	s = curswitch();
	if(s->Switch.ldefault)
		errorposf(pos, "switch already has default");
	s->Switch.ldefault = l;
	expect(TOKDEFAULT);
	expect(':');
	n->Labeled.stmt = stmt();
	return n;
}

static Node *
pcase(void)
{
	SrcPos *pos;
	Node   *n;
	Node   *s;

	pos = &tok->pos;
	s = curswitch();
	expect(TOKCASE);
	n = mknode(NCASE, pos);
	n->Case.expr = constexpr();
	expect(':');
	n->Case.l = newlabel();
	n->Case.stmt = stmt();
	vecappend(s->Switch.cases, n);
	return n;
}

static Node *
block(void)
{
	Vec    *v;
	SrcPos *p;

	v = vec();
	pushscope();
	p = &tok->pos;
	expect('{');
	while(tok->k != '}' && tok->k != TOKEOF)
		vecappend(v, declorstmt());
	expect('}');
	popscope();
	return mkblock(p, v);
}

static Node *
expr(void)
{
	SrcPos *p;
	Vec    *v;
	Node   *n;

	p = &tok->pos;
	n = assignexpr();
	if(tok->k == ',') {
		v = vec();
		vecappend(v, n);
		while(tok->k == ',') {
			next();
			vecappend(v, assignexpr());
		}
		n = mkcomma(p, v);
	}
	return n;
}

static int
isassignop(int k)
{
	switch(k) {
	case '=':
	case TOKADDASS:
	case TOKSUBASS:
	case TOKMULASS:
	case TOKDIVASS:
	case TOKMODASS:
	case TOKORASS:
	case TOKANDASS:
		return 1;
	}
	return 0;
}

static Node *
assignexpr(void)
{
	Tok  *t;
	Node *l, *r;

	l = condexpr();
	if(isassignop(tok->k)) {
		t = tok;
		next();
		r = assignexpr();
		l = mkassign(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
constexpr(void)
{
	return condexpr();
}

/* Aka Ternary operator. */
static Node *
condexpr(void)
{
	/* TODO: implement */
	return logorexpr();
}

static Node *
logorexpr(void)
{
	Tok  *t;
	Node *l, *r;

	l = logandexpr();
	while(tok->k == TOKLOR) {
		t = tok;
		next();
		r = logandexpr();
		l = mkbinop(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
logandexpr(void)
{
	Tok  *t;
	Node *l, *r;

	l = orexpr();
	while(tok->k == TOKLAND) {
		t = tok;
		next();
		r = orexpr();
		l = mkbinop(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
orexpr(void)
{
	Tok  *t;
	Node *l, *r;

	l = xorexpr();
	while(tok->k == '|') {
		t = tok;
		next();
		r = xorexpr();
		l = mkbinop(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
xorexpr(void)
{
	Tok  *t;
	Node *l, *r;

	l = andexpr();
	while(tok->k == '^') {
		t = tok;
		next();
		r = andexpr();
		l = mkbinop(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
andexpr(void) 
{
	Tok  *t;
	Node *l, *r;

	l = eqlexpr();
	while(tok->k == '&') {
		t = tok;
		next();
		r = eqlexpr();
		l = mkbinop(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
eqlexpr(void)
{
	Tok  *t;
	Node *l, *r;

	l = relexpr();
	while(tok->k == TOKEQL || tok->k == TOKNEQ) {
		t = tok;
		next();
		r = relexpr();
		l = mkbinop(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
relexpr(void)
{
	Tok  *t;
	Node *l, *r;

	l = shiftexpr();
	while(tok->k == '>' || tok->k == '<' 
		  || tok->k == TOKLEQ || tok->k == TOKGEQ) {
		t = tok;
		next();
		r = shiftexpr();
		l = mkbinop(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
shiftexpr(void)
{
	Tok  *t;
	Node *l, *r;

	l = addexpr();
	while(tok->k == TOKSHL || tok->k == TOKSHR) {
		t = tok;
		next();
		r = addexpr();
		l = mkbinop(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
addexpr(void)
{
	Tok  *t;
	Node *l, *r;

	l = mulexpr();
	while(tok->k == '+' || tok->k == '-'	) {
		t = tok;
		next();
		r = mulexpr();
		l = mkbinop(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
mulexpr(void)
{
	Tok  *t;
	Node *l, *r;
	
	l = castexpr();
	while(tok->k == '*' || tok->k == '/' || tok->k == '%') {
		t = tok;
		next();
		r = castexpr();
		l = mkbinop(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
castexpr(void)
{
	Tok  *t;
	Node *o;
	CTy  *ty;
	
	if(tok->k == '(' && istypestart(nexttok)) {
		t = tok;
		expect('(');
		ty = typename();
		expect(')');
		o = unaryexpr();
		return mkcast(&t->pos, o, ty);
	}
	return unaryexpr();
}

static CTy *
typename(void)
{
	int   sclass;
	CTy  *t;
	char *name;
	
	t = declspecs(&sclass);
	t = declarator(t, &name, 0);
	return t;
}

static Node *
unaryexpr(void)
{
	Tok  *t;
	Node *n;

	switch (tok->k) {
	case TOKINC:
	case TOKDEC:
		t = tok;
		next();
		n = unaryexpr();
		return mkincdec(&t->pos, t->k, 0, n);
	case '*':
	case '&':
	case '-':
	case '!':
	case '~':
		t = tok;
		next();
		return mkunop(&t->pos, t->k, castexpr());
	case TOKSIZEOF:
		n = mknode(NSIZEOF, &tok->pos);
		next();
		if(tok->k == '(' && istypestart(nexttok)) {
			expect('(');
			typename();
			expect(')');
			return 0;
		}
		n = unaryexpr();
		return n;
	}
	return postexpr();
}

static Node *
postexpr(void)
{
	int   done;
	Tok  *t;
	Node *n1, *n2, *n3;

	n1 = primaryexpr();
	done = 0;
	while(!done) {
		switch(tok->k) {
		case '[':
			t = tok;
			next();
			n2 = expr();
			expect(']');
			n3 = mknode(NIDX, &t->pos);
			if(isptr(n1->type))
				n3->type = n1->type->Ptr.subty;
			else if (isarray(n1->type))
				n3->type = n1->type->Arr.subty;
			else
				errorposf(&t->pos, "can only index an array or pointer");
			n3->Idx.idx = n2;
			n3->Idx.operand = n1;
			n1 = n3;
			break;
		case '.':
			if(!isstruct(n1->type))
				errorposf(&tok->pos, "expected a struct");
			if(n1->type->Struct.unspecified)
				errorposf(&tok->pos, "struct is not realized");
			n2 = mknode(NSEL, &tok->pos);
			next();
			n2->Sel.name = tok->v;
			n2->Sel.operand = n1;
			n2->type = structmemberty(n1->type, tok->v);
			if(!n2->type)
				errorposf(&tok->pos, "struct has no member %s", tok->v);
			expect(TOKIDENT);
			n1 = n2;
			break;
		case TOKARROW:
			if(!isptr(n1->type) && isstruct(n1->type->Ptr.subty))
				errorposf(&tok->pos, "expected a struct pointer");
			if(n1->type->Ptr.subty->Struct.unspecified)
				errorposf(&tok->pos, "struct is not realized");
			n2 = mknode(NSEL, &tok->pos);
			next();
			n2->Sel.name = tok->v;
			n2->Sel.operand = n1;
			n2->Sel.arrow = 1;
			n2->type = structmemberty(n1->type->Ptr.subty, tok->v);
			if(!n2->type)
				errorposf(&tok->pos, "struct pointer has no member %s", tok->v);
			expect(TOKIDENT);
			n1 = n2;
			break;
		case '(':
			n2 = mknode(NCALL, &tok->pos);
			n2->Call.funclike = n1;
			n2->Call.args = vec();
			if(isfunc(n1->type))
				n2->type = n1->type->Func.rtype;
			else if (isfuncptr(n1->type))
				n2->type = n1->type->Ptr.subty->Func.rtype;
			else
				errorposf(&tok->pos, "cannot call non function");
			next();
			if(tok->k != ')') {
				for(;;) {
					vecappend(n2->Call.args, assignexpr());
					if(tok->k != ',') {
						break;
					}
					next();
				}
			}
			expect(')');
			n1 = n2;
			break;
		case TOKINC:
			n1 = mkincdec(&tok->pos, TOKINC, 1, n1);
			next();
			break;
		case TOKDEC:
			n1 = mkincdec(&tok->pos, TOKDEC, 1, n1);
			next();
			break;
		default:
			done = 1;
		}
	}
	return n1;
}

static Node *
primaryexpr(void) 
{
	Sym  *sym;
	Node *n;
	
	switch (tok->k) {
	case TOKIDENT:
		sym = lookup(syms, tok->v);
		if(!sym)
			errorposf(&tok->pos, "undefined symbol %s", tok->v);
		n = mknode(NIDENT, &tok->pos);
		n->Ident.sym = sym;
		n->type = sym->type;
		next();
		return n;
	case TOKNUM:
		n = mknode(NNUM, &tok->pos);
		n->Num.v = atoll(tok->v);
		n->type = mkprimtype(PRIMINT, 1);
		next();
		return n;
	case TOKSTR:
		n = mknode(NSTR, &tok->pos);
		n->Str.v = tok->v;
		n->type = mkptr(mkprimtype(PRIMCHAR, 1));
		next();
		return n;
	case '(':
		next();
		n = expr();
		expect(')');
		return n;
	default:
		errorposf(&tok->pos, "expected an ident, constant, string or (");
	}
	errorf("unreachable.");
	return 0;
}

