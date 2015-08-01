#include <stdio.h>
#include <string.h>
#include "mem/mem.h"
#include "ds/ds.h"
#include "c.h"

Node  *parse(void);
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
static CTy  *declspecs(int *specs);
static CTy  *pstruct(void);
static CTy  *penum(void);
static CTy  *typename(void);
static CTy  *declarator(CTy *, char **name, Node **init);
static CTy  *directdeclarator(CTy *, char **name); 
static CTy  *declaratortail(CTy *);

static void  expect(int);

Tok *tok;
Tok *nexttok;

#define MAXSCOPES 1024

static int nscopes;
static Map *tags[MAXSCOPES];
static Map *types[MAXSCOPES];
static Map *vars[MAXSCOPES];

static void
popscope()
{
	nscopes -= 1;
	if(nscopes < 0)
		errorf("bug: scope underflow\n");
	vars[nscopes] = 0;
	tags[nscopes] = 0;
	types[nscopes] = 0;
}

static void
pushscope()
{
	vars[nscopes] = map();
	tags[nscopes] = map();
	types[nscopes] = map();
	nscopes += 1;
	if(nscopes > MAXSCOPES)
		errorf("scope depth exceeded maximum\n");
}

static int
isglobal()
{
	return nscopes == 1;
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
	int i;
	void *v;
	
	i = nscopes;
	while(i--) {
		v = mapget(scope[i], k);
		if(v)
			return v;
	}
	return 0;
}

static CTy *
newtype(int type)
{
	CTy *t;

	t = zmalloc(sizeof(CTy));
	t->t = type;
	return t;
}

static CTy *
mkprimtype(int type, int sig)
{
	CTy *t;
	
	t = newtype(CPRIM);
	t->Prim.type = type;
	t->Prim.issigned = sig;
	return t;
}

static NameTy *
newnamety(char *n, CTy *t)
{
	NameTy *nt;
	
	nt = zmalloc(sizeof(NameTy));
	nt->name = n;
	nt->type = t;
	return nt;
}

static Node *
newnode(int type, SrcPos *p)
{
	Node *n;

	n = zmalloc(sizeof(Node));
	n->pos = *p;
	n->t = type;
	return n;
}

static Node *
mkfunc(SrcPos *p, CTy *t, char *name, Node *body)
{
	Node *n;

	n = newnode(NFUNC, p);
	n->type = t;
	n->Func.body = body;
	n->Func.name = name;
	return n;
}

static Node *
mkblock(SrcPos *p, Vec *v)
{
	Node *n;

	n = newnode(NBLOCK, p);
	n->Block.stmts = v;
	return n;
}

static Node *
mkcomma(SrcPos *p, Vec *v)
{
	Node *n;

	n = newnode(NCOMMA, p);
	n->Comma.exprs = v;
	return n;
}

static Node *
mkbinop(SrcPos *p, int op, Node *l, Node *r)
{
	Node *n;

	n = newnode(NBINOP, p);
	n->Binop.op = op;
	n->Binop.l = l;
	n->Binop.r = r;
	return n;
}

static Node *
mkunop(SrcPos *p, int op, Node *o)
{
	Node *n;

	n = newnode(NUNOP, p);
	n->Unop.op = op;
	n->Unop.operand = 0;
	return n;
}

static Node *
mkcast(SrcPos *p, Node *o, CTy *to)
{
	Node *n;
	
	n = newnode(NCAST, p);
	n->type = to;
	n->Cast.operand = o;
	return n;
}

static Node *
mkfor(SrcPos *p, Node *init, Node *cond, Node *step, Node *stmt)
{
	Node *n;
	
	n = newnode(NFOR, p);
	n->For.init = init;
	n->For.cond = cond;
	n->For.step = step;
	n->For.stmt = stmt;
	return n;
}

static Node *
mkwhile(SrcPos *p, Node *expr, Node *stmt)
{
	Node *n;
	
	n = newnode(NWHILE, p);
	n->While.expr = expr;
	n->While.stmt = stmt;
	return n;
}

static Node *
mkdowhile(SrcPos *p, Node *expr, Node *stmt)
{
	Node *n;
	
	n = newnode(NDOWHILE, p);
	n->DoWhile.expr = expr;
	n->DoWhile.stmt = stmt;
	return n;
}

static Node *
mkswitch(SrcPos *p, Node *expr, Node *stmt)
{
	Node *n;
	
	n = newnode(NSWITCH, p);
	n->DoWhile.expr = expr;
	n->DoWhile.stmt = stmt;
	return n;
}

static Node *
mkif(SrcPos *p, Node *expr, Node *iftrue, Node *iffalse)
{
	Node *n;
	
	n = newnode(NIF, p);
	n->If.expr = expr;
	n->If.iftrue = iftrue;
	n->If.iffalse = iffalse;
	return n;
}

static int
convrank(CTy *t)
{
	if(t->t != CPRIM)
		errorf("internal error\n");
	switch(t->Prim.type){
	case PRIMCHAR:
		return 0;
	case PRIMSHORT:
		return 1;
	case PRIMINT:
		return 2;
	case PRIMLONG:
		return 3;
	case PRIMLLONG:
		return 4;
	case PRIMFLOAT:
		return 5;
	case PRIMDOUBLE:
		return 6;
	case PRIMLDOUBLE:
		return 7;
	}
	errorf("internal error\n");
	return -1;
}

static int 
compatiblestruct(CTy *l, CTy *r)
{
	/* TODO */
	return 0;
}

static int 
sametype(CTy *l, CTy *r)
{
	/* TODO */
	switch(l->t) {
	case CVOID:
		if(r->t != CVOID)
			return 0;
		return 1;
	case CPRIM:
		if(r->t != CPRIM)
			return 0;
		if(l->Prim.issigned != r->Prim.issigned)
			return 0;
		if(l->Prim.type != r->Prim.type)
			return 0;
		return 1;
	}
	return 0;
}

static int
isftype(CTy *t)
{
	if(t->t != CPRIM)
		return 0;
	switch(t->Prim.type){
	case PRIMFLOAT:
	case PRIMDOUBLE:
	case PRIMLDOUBLE:
		return 1;
	}
	return 0;
}

static int
isitype(CTy *t)
{
	if(t->t != CPRIM)
		return 0;
	switch(t->Prim.type){
	case PRIMCHAR:
	case PRIMSHORT:
	case PRIMINT:
	case PRIMLONG:
	case PRIMLLONG:
		return 1;
	}
	return 0;
}

static int
isarithtype(CTy *t)
{
	return isftype(t) || isitype(t);
}

static int
isptr(CTy *t)
{
	return t->t == CPTR;
}

static int
isassignable(CTy *to, CTy *from)
{
	if((isarithtype(to) || isptr(to)) &&
		(isarithtype(from) || isptr(from)))
		return 1;
	if(compatiblestruct(to, from))
		return 1;
	return 0;
}

static unsigned long long int
getmaxval(CTy *l)
{
	switch(l->Prim.type) {
	case PRIMCHAR:
		if(l->Prim.issigned)
			return 0x7f;
		else
			return 0xff;
	case PRIMSHORT:
		if(l->Prim.issigned)
			return 0x7fff;
		else
			return  0xffff;
	case PRIMINT:
	case PRIMLONG:
		if(l->Prim.issigned)
			return 0x7fffffff;
		else
			return 0xffffffff;
	case PRIMLLONG:
		if(l->Prim.issigned)
			return 0x7fffffffffffffff;
		else
			return 0xffffffffffffffff;
	}
	errorf("internal error\n");
	return 0;
}

static signed long long int
getminval(CTy *l)
{
	if(!l->Prim.issigned)
		return 0;
	switch(l->Prim.type) {
	case PRIMCHAR:
		return 0xff;
	case PRIMSHORT:
		return 0xffff;
	case PRIMINT:
	case PRIMLONG:
		return 0xffffffffl;
	case PRIMLLONG:
		return 0xffffffffffffffff;
	}
	errorf("internal error\n");
	return 0;
}

static int
canrepresent(CTy *l, CTy *r)
{
	if(!isitype(l) || !isitype(r))
		errorf("internal error");
	return getmaxval(l) <= getmaxval(r) && getminval(l) >= getminval(r);
}

static Node *
ipromote(Node *n)
{
	if(!isitype(n->type))
		return 0;
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

CTy *
usualarithconv(Node **large, Node **small)
{   
	Node **tmp;
	CTy *t;

	if(!isarithtype((*large)->type) || !isarithtype((*small)->type))
		errorf("internal error\n");
	if(convrank((*large)->type) < convrank((*small)->type)) {
		tmp = large;
		large = small;
		small = tmp;
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
	int sclass;
	CTy *t;
	char *name;
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
		listappend(fty->Func.params, newnamety(name, t));
		if(tok->k != ',')
			break;
		next();
	}
	if(tok->k == TOKELLIPSIS) {
		fty->Func.isvararg = 1;
		next();
	}
}

static void
definetype(SrcPos *pos, char *name, CTy *ty)
{
	/* Handle valid redefines */
	if(!define(types, name, ty)) {
		/* TODO: 
		 Check if types are compatible.
		 errorpos(pos, "redefinition of type %s", name);
		
		*/
	}
}

static void
definesym(SrcPos *pos, char *name, void *sym)
{
   	/* Handle valid redefines */
	if(!define(vars, name, sym)) {
		/* TODO: 
		 Check if types are compatible.
		 errorpos(pos, "redefinition of type %s", name);
		
		*/
	}
}

static Node *
decl()
{
	int sclass;
	char *name;
	CTy  *basety;
	CTy  *ty;
	SrcPos *pos;
	ListEnt *e;
	Node *init;
	Node *fbody;
	NameTy *nt;

	if(tok->k == ';') {
		next();
		return 0;
	}
	pos = &tok->pos;
	basety = declspecs(&sclass);
	ty = declarator(basety, &name, &init);
	if(sclass == SCTYPEDEF && name)
		definetype(pos, name, ty);
	else if(name)
		definesym(pos, name, "TODO");
	if(isglobal() && tok->k == '{') {
		if(ty->t != CFUNC) 
			errorposf(pos, "expected a function");
		if(init)
			errorposf(pos, "function declaration has an initializer");
		if(!name)
			errorposf(pos, "function declaration requires a name");
		pushscope();
		for(e = ty->Func.params->head; e != 0; e = e->next) {
			nt = e->v;
			if(nt->name) {
				definesym(pos, nt->name, "TODO");
			}
		}
		fbody = block();
		popscope();
		return mkfunc(pos, ty, name, fbody);
	}
	while(tok->k == ',') {
		next();
		pos = &tok->pos;
		ty = declarator(basety, &name, &init);
		if(sclass == SCTYPEDEF && name)
			definetype(pos, name, ty);
		else if(name)
			definesym(pos, name, "TODO");
	}
	expect(';');
	return 0;
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
	CTy *t;
	SrcPos *pos;
	int bits;

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
			t = lookup(types, tok->v);
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
		t = newtype(CPTR);
		t->Ptr.subty = subt;
		return subt;
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
	CTy *t;

	for(;;) {
		switch (tok->k) {
		case '[':
			next();
			if(tok->k != ']')
				constexpr();
			expect(']');
			break;
		case '(':
			t = newtype(CFUNC);
			t->Func.rtype = basety;
			t->Func.params = listnew();
			next();
			params(t);
			if(tok->k != ')')
				errorposf(&tok->pos, "expected valid parameter or )");
			next();
			return t;
		default:
			return basety;
		}
	}
}

static CTy *
pstruct() 
{
	char *tagname;
	/* TODO: replace with predeclarations */
	int shoulddefine;
	int sclass;
	char *name;
	CTy *basety;
	/* CTy *t; */

	tagname = 0;
	shoulddefine = 0;
	if(tok->k != TOKUNION && tok->k != TOKSTRUCT)
		errorposf(&tok->pos, "expected union or struct");
	next();
	if(tok->k == TOKIDENT) {
		tagname = tok->v;
		next();
	}
	if(tok->k == '{') {
		shoulddefine = 1;
		expect('{');
		while(tok->k != '}') {
			basety = declspecs(&sclass);
			do {
				if(tok->k == ',')
					next();
				/* t = */ declarator(basety, &name, 0);
			} while (tok->k == ',');
			if(tok->k == ':') {
				next();
				constexpr();
			}
			expect(';');
		}
		expect('}');
	}
	if(tagname && shoulddefine)
		if(!define(tags, tagname, "TODO"))
			errorposf(&tok->pos, "redefinition of tag %s", tagname);
	return newtype(CSTRUCT);
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
	while(1) {
		if(tok->k == '}')
			break;
		if(!define(vars, tok->v, "TODO"))
			errorposf(&tok->pos, "redefinition of symbol %s", tok->v);
		expect(TOKIDENT);
		if(tok->k == '=') {
			next();
			constexpr();
		}
		if(tok->k == ',')
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
	Node *e;
	Node *t;
	Node *f;
	
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
	return mkif(p, e, t, f);
}

static Node *
pfor(void)
{
	SrcPos *p;
	Node *i;
	Node *c;
	Node *s;
	Node *st;
	
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
	st = stmt();
	return mkfor(p, i, c, s, st);
}

static Node *
pwhile(void)
{
	SrcPos *p;
	Node *e;
	Node *s;
	
	p = &tok->pos;	
	expect(TOKWHILE);
	expect('(');
	e = expr();
	expect(')');
	s = stmt();
	return mkwhile(p, e, s);
}

static Node *
dowhile(void)
{
	SrcPos *p;
	Node *e;
	Node *s;

	p = &tok->pos;
	expect(TOKDO);
	s = stmt();
	expect(TOKWHILE);
	expect('(');
	e = expr();
	expect(')');
	expect(';');
	return mkdowhile(p, e, s);
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
		if(lookup(types, nexttok->v))
			return 1;
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
		if(lookup(types, t->v))
			return 1;
	}
	return 0;
}

static Node *
declorstmt()
{
	Node *n;
	Tok *t;

	if(isdeclstart(tok))
		return decl();
	if(tok->k == TOKIDENT && nexttok->k == ':') {
		t = tok;
		next();
		next();
		n = newnode(NLABELED, &t->pos);
		/* TODO make label. */
		n->Labeled.stmt = stmt();
		return n;
	}
	return stmt();
}

static Node *
stmt(void)
{
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
		next();
		expect(TOKIDENT);
		expect(';');
		return 0;
	case '{':
		return block();
	case ';':
		return 0;
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
	n = newnode(NINIT, &tok->pos);
	n->Init.inits = listnew();
	while(1) {
		if(tok->k == '}')
			break;
		switch(tok->k){
		case '[':
			next();
			constexpr();
			expect(']');
			expect('=');
			/* TODO */
			listappend(n->Init.inits, declinit());
			break;
		case '.':
			next();
			expect(TOKIDENT);
			expect('=');
			/* TODO */
			listappend(n->Init.inits, declinit());
			break;
		default:
			listappend(n->Init.inits, declinit());
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

	n = expr();
	expect(';');
	return n;
}

static Node *
preturn(void)
{   
	Node *n;

	n = newnode(NRETURN, &tok->pos);
	expect(TOKRETURN);
	if(tok->k != ';')
		n->Return.expr = expr();
	expect(';');
	return n;
}

static Node *
pswitch(void)
{
	SrcPos *p;
	Node *e;
	Node *s;
	
	p = &tok->pos;
	expect(TOKSWITCH);
	expect('(');
	e = expr();
	expect(')');
	s = stmt();
	return mkswitch(p, e, s);
}

static Node *
pcontinue(void)
{
	Node *n;
	
	n = newnode(NGOTO, &tok->pos);
	expect(TOKCONTINUE);
	expect(';');
	return n;
}

static Node *
pbreak(void)
{
	Node *n;
	
	n = newnode(NGOTO, &tok->pos);
	expect(TOKBREAK);
	expect(';');
	return n;
}

static Node *
pdefault(void)
{
	expect(TOKDEFAULT);
	expect(':');
	stmt();
	return 0;
}

static Node *
pcase(void)
{
	expect(TOKCASE);
	constexpr();
	expect(':');
	stmt();
	return 0;
}

static Node *
block(void)
{
	Vec *v;
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
	Vec *v;
	Node *n;

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
	Node *l;
	Node *r;

	l = condexpr();
	if(isassignop(tok->k)) {
		t = tok;
		next();
		r = assignexpr();
		l = mkbinop(&t->pos, t->k, l, r);
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
	Node *l;
	Node *r;

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
	Node *l;
	Node *r;

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
	Node *l;
	Node *r;

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
	Node *l;
	Node *r;

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
	Node *l;
	Node *r;

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
	Node *l;
	Node *r;

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
	Node *l;
	Node *r;

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
	Node *l;
	Node *r;

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
	Node *l;
	Node *r;

	l = mulexpr();
	while(tok->k == '+' || tok->k == '-') {
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
	Node *l;
	Node *r;
	
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
	Tok *t;
	Node *o;
	CTy *ty;
	
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
	int sclass;
	CTy *t;
	char *name;
	
	t = declspecs(&sclass);
	t = declarator(t, &name, 0);
	return t;
}

static Node *
unaryexpr(void)
{
	Tok *t;
	CTy * ty;
	Node *n;

	switch (tok->k) {
	case TOKINC:
	case TOKDEC:
		t = tok;
		next();
		return mkunop(&tok->pos, t->k, unaryexpr());
	case '*':
	case '+':
	case '-':
	case '!':
	case '~':
	case '&':
		t = tok;
		next();
		return mkunop(&t->pos, t->k, castexpr());
	case TOKSIZEOF:
		n = newnode(NSIZEOF, &tok->pos);
		next();
		if(tok->k == '(' && istypestart(nexttok)) {
			expect('(');
			ty = typename();
			expect(')');
			return 0;
		}
		n = unaryexpr();
		ty = n->type;
		return 0;
	}
	return postexpr();
}

static Node *
postexpr(void)
{
	int done;
	Tok *t;
	Node *n1;
	Node *n2;
	Node *n3;

	n1 = primaryexpr();
	done = 0;
	while(!done) {
		switch(tok->k) {
		case '[':
			t = tok;
			next();
			n2 = expr();
			expect(']');
			n3 = newnode(NIDX, &t->pos);
			n3->Idx.idx = n2;
			n3->Idx.operand = n1;
			n1 = n3;
			break;
		case '.':
			n2 = newnode(NSEL, &tok->pos);
			next();
			n2->Sel.sel = tok->v;
			n2->Sel.operand = n1;
			expect(TOKIDENT);
			n1 = n2;
			break;
		case TOKARROW:
			n2 = newnode(NSEL, &tok->pos);
			next();
			n2->Sel.sel = tok->v;
			n2->Sel.operand = n1;
			n2->Sel.arrow = 1;
			expect(TOKIDENT);
			n1 = n2;
			break;
		case '(':
			n2 = newnode(NCALL, &tok->pos);
			n2->Call.funclike = n1;
			n2->Call.args = listnew();
			next();
			if(tok->k != ')') {
				for(;;) {
					listappend(n2->Call.args, assignexpr());
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
			n1 = mkunop(&tok->pos, TOKINC, n1);
			next();
			break;
		case TOKDEC:
			n1 = mkunop(&tok->pos, TOKINC, n1);
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
	Sym *sym;
	Node *n;
	Tok *t;
	
	switch (tok->k) {
	case TOKIDENT:
		sym = lookup(vars, tok->v);
		if(!sym)
			errorposf(&tok->pos, "undefined symbol %s", tok->v);
		t = tok;
		next();
		n = newnode(NSYM, &tok->pos);
		n->Sym.s = sym;
		n->Sym.n = t->v;
		return n;
	case TOKNUM:
		n = newnode(NNUM, &tok->pos);
		n->Num.v = tok->v;
		next();
		return n;
	case TOKSTR:
		n = newnode(NSTR, &tok->pos);
		n->Str.v = tok->v;
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

