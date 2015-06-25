#include <stdio.h>
#include <string.h>
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
    if (nscopes < 0)
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
    if (nscopes > MAXSCOPES)
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
mktype(int type)
{
	CTy *t;

	t = ccmalloc(sizeof(CTy));
	t->t = type;
	return t;
}

static Node *
mknode(int type, SrcPos *p)
{
	Node *n;

	n = ccmalloc(sizeof(Node));
	n->pos = *p;
	n->t = type;
	return n;
}

static Node *
mkbinop(SrcPos *p, int op, Node *l, Node *r)
{
	Node *n;

	n = mknode(NBINOP, p);
	n->Binop.op = op;
	n->Binop.l = l;
	n->Binop.r = r;
	return n;
}

static Node *
mkunop(SrcPos *p, int op, Node *o)
{
	Node *n;

	n = mknode(NUNOP, p);
	n->Unop.op = op;
	n->Unop.operand = 0;
	return n;
}

static Node *
mkcast(SrcPos *p, Node *o, CTy *to)
{
    Node *n;
    
    n = mknode(NCAST, p);
    n->type = to;
    n->Cast.operand = o;
    return n;
}

static NameTy *
mknamety(char *n, CTy *t)
{
    NameTy *nt;
    
    nt = ccmalloc(sizeof(NameTy));
    nt->name = n;
    nt->type = t;
    return nt;
}

static void
next(void)
{
	tok = nexttok;
	nexttok = lex();
}

static void
expect(int kind) 
{
	if(tok->k != kind)
		errorposf(&tok->pos,"expected %s, got %s", 
			tokktostr(kind), tokktostr(tok->k));
    next();
}

Node * 
parse()
{
	nscopes = 0;
	pushscope();
	next();
	next();
	for(;;) {
		if(tok->k == TOKEOF)
			break;
		decl();
	}
	popscope();
	return 0;
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
		listappend(fty->Func.params, mknamety(name, t));
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
		if (init)
		    errorposf(pos, "function declaration has an initializer");
		pushscope();
		for(e = ty->Func.params->head; e != 0; e = e->next) {
			nt = e->v;
			if(nt->name) {
				definesym(pos, nt->name, "TODO");
			}
		}
		block();
		popscope();
		return 0;
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
		t = mktype(CPRIM);
		t->Prim.type = PRIMFLOAT;
		t->Prim.issigned = 0;
		return t;
	case BITLONG|BITDOUBLE:
	case BITDOUBLE:
		t = mktype(CPRIM);
		t->Prim.type = PRIMDOUBLE;
		t->Prim.issigned = 0;
		return t;
	case BITSIGNED|BITCHAR:
	case BITCHAR:
		t = mktype(CPRIM);
		t->Prim.type = PRIMCHAR;
		t->Prim.issigned = 1;
		return t;
	case BITUNSIGNED|BITCHAR:
		t = mktype(CPRIM);
		t->Prim.type = PRIMCHAR;
		t->Prim.issigned = 0;
		return t;
	case BITSIGNED|BITSHORT|BITINT:
	case BITSHORT|BITINT:
	case BITSHORT:
		t = mktype(CPRIM);
		t->Prim.type = PRIMSHORT;
		t->Prim.issigned = 1;
		return t;
	case BITUNSIGNED|BITSHORT|BITINT:
	case BITUNSIGNED|BITSHORT:
		t = mktype(CPRIM);
		t->Prim.type = PRIMSHORT;
		t->Prim.issigned = 0;
		return t;
	case BITSIGNED|BITINT:
	case BITSIGNED:
	case BITINT:
	case 0:
		t = mktype(CPRIM);
		t->Prim.type = PRIMINT;
		t->Prim.issigned = 1;
		return t;
	case BITUNSIGNED|BITINT:
	case BITUNSIGNED:
		t = mktype(CPRIM);
		t->Prim.type = PRIMINT;
		t->Prim.issigned = 0;
		return t;
	case BITSIGNED|BITLONG|BITINT:
	case BITSIGNED|BITLONG:
	case BITLONG|BITINT:
	case BITLONG:
		t = mktype(CPRIM);
		t->Prim.type = PRIMLONG;
		t->Prim.issigned = 1;
		return t;
	case BITUNSIGNED|BITLONG|BITINT:
	case BITUNSIGNED|BITLONG:
		t = mktype(CPRIM);
		t->Prim.type = PRIMLONG;
		t->Prim.issigned = 0;
		return t;
	case BITSIGNED|BITLONGLONG|BITINT:
	case BITSIGNED|BITLONGLONG:
	case BITLONGLONG|BITINT:
	case BITLONGLONG:
		t = mktype(CPRIM);
		t->Prim.type = PRIMLLONG;
		t->Prim.issigned = 1;
		return t;
	case BITUNSIGNED|BITLONGLONG|BITINT:
	case BITUNSIGNED|BITLONGLONG:
		t = mktype(CPRIM);
		t->Prim.type = PRIMLLONG;
		t->Prim.issigned = 0;
		return t;
	case BITVOID:
		t = mktype(CVOID);
		return t;
	case BITENUM:
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
		t = mktype(CPTR);
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
			if (tok->k != ']')
				constexpr();
			expect(']');
			break;
		case '(':
			t = mktype(CFUNC);
		    t->Func.rtype = basety;
		    t->Func.params = listnew();
		    next();
			params(t);
			if (tok->k != ')')
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
	if (tok->k == '{') {
	    shoulddefine = 1;
	    expect('{');
	    while(tok->k != '}') {
	        basety = declspecs(&sclass);
            do {
                if (tok->k == ',')
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
    return mktype(CSTRUCT);
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
	t = mktype(CPRIM);
	t->Prim.type = PRIMENUM;
	return t;
}

static Node *
pif(void)
{
	expect(TOKIF);
	expect('(');
	expr();
	expect(')');
	stmt();
	if(tok->k != TOKELSE)
		return 0;
	expect(TOKELSE);
	stmt();
	return 0;
}

static Node *
pfor(void)
{
    Node *n;
    
    n = mknode(NFOR, &tok->pos);
	expect(TOKFOR);
	expect('(');
	if(tok->k == ';') {
	    next();
	} else {
	    n->For.init = expr();
	    expect(';');
	}
	if(tok->k == ';') {
	    next();
	} else {
	    n->For.cond = expr();
	    expect(';');
	}
	if(tok->k != ')')
		n->For.step = expr();
	expect(')');
	n->For.stmt = stmt();
	return n;
}

static Node *
pwhile(void)
{
    Node *n;
    
    n = mknode(NWHILE, &tok->pos);
	expect(TOKWHILE);
	expect('(');
	n->While.expr = expr();
	expect(')');
	n->While.stmt = stmt();
	return 0;
}

static Node *
dowhile(void)
{
	Node *n;

	n = mknode(NDOWHILE, &tok->pos);
	expect(TOKDO);
	n->DoWhile.stmt = stmt();
	expect(TOKWHILE);
	expect('(');
	n->DoWhile.expr = expr();
	expect(')');
	expect(';');
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
        n = mknode(NLABELED, &t->pos);
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
    n = mknode(NINIT, &tok->pos);
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
        if (tok->k != ',')
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

    n = mknode(NRETURN, &tok->pos);
	expect(TOKRETURN);
	if(tok->k != ';')
	    n->Return.expr = expr();
	expect(';');
	return n;
}

static Node *
pswitch(void)
{
    Node *n;
    
    n = mknode(NSWITCH, &tok->pos);
	expect(TOKSWITCH);
	expect('(');
	n->Switch.expr = expr();
	expect(')');
	n->Switch.stmt = stmt();
	return n;
}

static Node *
pcontinue(void)
{
    Node *n;
    
    n = mknode(NGOTO, &tok->pos);
	expect(TOKCONTINUE);
	expect(';');
	return n;
}

static Node *
pbreak(void)
{
    Node *n;
    
    n = mknode(NGOTO, &tok->pos);
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
	pushscope();
	expect('{');
	while(tok->k != '}' && tok->k != TOKEOF)
		declorstmt();
	expect('}');
	popscope();
	return 0;
}

static Node *
expr(void)
{
	Node *n;
	
	for(;;) {
		n = assignexpr();
		if (tok->k != ',')
			break;
		next();
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
	return 0;
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
	return 0;
}

static Node *
unaryexpr(void)
{
    Tok *t;

	switch (tok->k) {
	case TOKINC:
	case TOKDEC:
		next();
		unaryexpr();
		return 0;
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
        next();
        if (tok->k == '(' && istypestart(nexttok)) {
            expect('(');
            typename();
            expect(')');
            return 0;
	    }
        unaryexpr();
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
			n3 = mknode(NIDX, &t->pos);
			n3->Idx.idx = n2;
			n3->Idx.operand = n1;
			n1 = n3;
			break;
		case '.':
			n2 = mknode(NSEL, &tok->pos);
			next();
			n2->Sel.sel = tok->v;
			n2->Sel.operand = n1;
			expect(TOKIDENT);
			n1 = n2;
			break;
		case TOKARROW:
			n2 = mknode(NSEL, &tok->pos);
			next();
			n2->Sel.sel = tok->v;
			n2->Sel.operand = n1;
			n2->Sel.arrow = 1;
			expect(TOKIDENT);
			n1 = n2;
			break;
		case '(':
		    n2 = mknode(NCALL, &tok->pos);
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
			next();
			break;
		case TOKDEC:
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
		n = mknode(NSYM, &tok->pos);
		n->Sym.s = sym;
		n->Sym.n = t->v;
		return n;
	case TOKNUM:
		n = mknode(NNUM, &tok->pos);
		n->Num.v = tok->v;
		next();
		return n;
	case TOKSTR:
		n = mknode(NSTR, &tok->pos);
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

