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
static void  declspecs(int *specs, CTy **ty);
static void  pstruct(void);
static void  penum(void);
static CTy  *typename(void);
static CTy  *declarator(CTy *, char **name);
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
        error("bug: scope underflow\n");
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
        error("scope depth exceeded maximum\n");
}

static int
isglobal()
{
	return nscopes == 1;
}

static int
definesym(Map *scope[], char *k, void *v)
{
    Map *m;
    
    m = scope[nscopes - 1];
    if(mapget(m, k))
        return 0;
    mapset(m, k, v);
    return 1; 
}

static void *
lookupsym(Map *scope[], char *k)
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
mknode(int type)
{
	Node *n;

	n = ccmalloc(sizeof(Node));
	n->t = type;
	return n;
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
		errorpos(&tok->pos,"expected %s", tokktostr(kind));
    next();
}

Node * 
parse()
{
	nscopes = 0;
	pushscope();
	next();
	for(;;) {
		next();
		if(tok->k == TOKEOF)
			break;
		decl();
	}
	popscope();
	return 0;
}

static void
params(void)
{
	int sclass;
	CTy *basety;
	char *name;

	if(tok->k == ')')
		return;
	declspecs(&sclass, &basety);
	declarator(basety, &name);
	while(tok->k == ',') {
		next();
		if(tok->k == TOKELLIPSIS) {
			next();
			break;
		}
		declspecs(&sclass, &basety);
		declarator(basety, &name);
	}
}

static Node *
decl()
{
    int sclass;
    char *name;
    CTy  *basety;
    SrcPos *pos;
    Map **symtab;

    pos = &tok->pos;
    declspecs(&sclass, &basety);
    declarator(basety, &name);
    if(sclass == SCTYPEDEF)
    	symtab = types;
    else
    	symtab = vars;
    if(name)
    	if(!definesym(symtab, name, "TODO"))
        	errorpos(pos, "redefinition of symbol %s", name);
    if(isglobal() && tok->k == '{') {
		block();
		return 0;
	}
	while(tok->k == ',') {
	    next();
	    pos = &tok->pos;
	    declarator(basety, &name);
        if(name)
        	if(!definesym(symtab, name, "TODO"))
            	errorpos(pos, "redefinition of symbol %s", name);
	}
	return 0;
}

static int
issclasstok(Tok *t) {
	switch(tok->k) {
	case TOKEXTERN:
	case TOKSTATIC:
	case TOKREGISTER:
	case TOKCONST:
	case TOKAUTO:
	    return 1;
	}
	return 0;
}

static void
declspecs(int *sclass, CTy **basety)
{
	int done;
	int voidcnt;
	int signedcnt;
	int unsignedcnt;
	int charcnt;
	int intcnt;
	int shortcnt;
	int longcnt;
	int floatcnt;
	int doublecnt;
	Sym *sym;
	
	*sclass = SCNONE;
	done = 0;
	
	voidcnt = 0;
	signedcnt = 0;
	unsignedcnt = 0;
	charcnt = 0;
	intcnt = 0;
	shortcnt = 0;
	longcnt = 0;
	floatcnt = 0;
	doublecnt = 0;
	
	while(!done) {
		if(issclasstok(tok))
	        if(*sclass != SCNONE)
	            errorpos(&tok->pos, "multiple storage classes in declaration specifiers.");
		switch(tok->k) {
		case TOKEXTERN:
			*sclass = SCEXTERN;
			next();
			break;    
		case TOKSTATIC:
		    *sclass = SCSTATIC;
			next();
			break;
		case TOKREGISTER:
		    *sclass = SCREGISTER;
			next();
			break;
		case TOKAUTO:
		    *sclass = SCAUTO;
			next();
			break;
		case TOKTYPEDEF:
			*sclass = SCTYPEDEF;
			next();
			break;
		case TOKCONST:
		case TOKVOLATILE:
		    next();
		    break;
		case TOKSTRUCT:
		case TOKUNION:
			pstruct();
			done = 1;
			break;
		case TOKENUM:
			penum();
			done = 1;
			break;
		case TOKVOID:
		    voidcnt += 1;
		    next();
		    break;
		case TOKCHAR:
		    charcnt += 1;
		    next();
		    break;
		case TOKSHORT:
		    shortcnt += 1;
		    next();
		    break;
		case TOKINT:
		    intcnt += 1;
		    next();
		    break;
		case TOKLONG:
		    longcnt += 1;
		    next();
		    break;
		case TOKFLOAT:
		    floatcnt += 1;
		    next();
		    break;
		case TOKDOUBLE:
		    doublecnt += 1;
		    next();
		    break;
		case TOKSIGNED:
		    signedcnt += 1;
		    next();
		    break;
		case TOKUNSIGNED:
		    unsignedcnt += 1;
			next();
			break;
		case TOKIDENT:
			sym = lookupsym(types, tok->v);
			if(sym) {
				next();
				done = 1;
				break;
			}
			/* fallthrough */
		default:
			done = 1;
			break;
		}
	}
	*basety = mktype(CINT);
}

/* Declarator is what introduces names into the program. */
static CTy *
declarator(CTy *basety, char **name) 
{
    CTy *t, *subt;

	while (tok->k == TOKCONST || tok->k == TOKVOLATILE)
		next();
	switch(tok->k) {
	case '*':
		next();
		subt = declarator(basety, name);
		t = mktype(CPTR);
		t->Ptr.subty = subt;
		return t;
	default:
	    return directdeclarator(basety, name);
	}

}

static CTy *
directdeclarator(CTy *basety, char **name) 
{
    *name = 0;
    switch(tok->k) {
	case '(':
		expect('(');
	    basety = declarator(basety, name);
		expect(')');
		return declaratortail(basety);
	case TOKIDENT:
		if(name)
			*name = tok->v;
		next();
		return declaratortail(basety);
	default:
		if(!name)
		    errorpos(&tok->pos, "expected ident or ( but got %s", tokktostr(tok->k));
		return declaratortail(basety);
	}
	error("unreachable");
	return 0;
}

static CTy *
declaratortail(CTy *basety)
{
	for(;;) {
		switch (tok->k) {
		case '[':
			next();
			if (tok->k != ']')
				constexpr();
			expect(']');
			break;
		case '(':
		    next();
			params();
			expect(')');
			break;
		default:
			return basety;
		}
	}
}

static void
pstruct() 
{
    char *tagname;
    /* TODO: replace with predeclarations */
    int shoulddefine;
    int sclass;
    char *name;
    CTy *basety;

    tagname = 0;
    shoulddefine = 0;
    if(tok->k != TOKUNION && tok->k != TOKSTRUCT)
	    errorpos(&tok->pos, "expected union or struct");
	next();
	if(tok->k == TOKIDENT) {
        tagname = tok->v;
		next();
	}
	if (tok->k == '{') {
	    shoulddefine = 1;
	    expect('{');
	    while(tok->k != '}') {
	        declspecs(&sclass, &basety);
            declarator(basety, &name);
	        while(tok->k == ',') {
	            next();
	            declarator(basety, &name);
	        }
		    if(tok->k == ':') {
		        next();
		        constexpr();
		    }
		    expect(';');
	    }
	    expect('}');
	}
	if(tagname && shoulddefine)
		if(!definesym(tags, tagname, "TODO"))
		    errorpos(&tok->pos, "redefinition of tag %s", tagname);
}

static void
penum()
{
	expect(TOKENUM);
	if(tok->k == TOKIDENT) {
		if(!definesym(tags, tok->v, "TODO"))
		    errorpos(&tok->pos, "redefinition of tag %s", tok->v);
		next();
	}
	expect('{');
	while(1) {
		if(tok->k == '}')
			break;
		if(!definesym(vars, tok->v, "TODO"))
		    errorpos(&tok->pos, "redefinition of symbol %s", tok->v);
		expect(TOKIDENT);
		if(tok->k == '=') {
			next();
			constexpr();
		}
		if(tok->k == ',')
			next();
	}
	expect('}');
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
	expect(TOKFOR);
	expect('(');
	if(tok->k != ';')
		expr();
	if(tok->k != ';')
		expr();
	if(tok->k != ')')
		expr();
	expect(')');
	stmt();
	return 0;
}

static Node *
pwhile(void)
{
	expect(TOKWHILE);
	expect('(');
	expr();
	expect(')');
	stmt();
	return 0;
}

static Node *
dowhile(void)
{
	Node *n;

	n = mknode(NDOWHILE);
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
        if(lookupsym(types, nexttok->v))
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
	    if(lookupsym(types, t->v))
	        return 1;
	}
	return 0;
}

static Node *
declorstmt()
{
    Sym *sym;
    
    if(isdeclstart(tok)) {
	    decl();
	    expect(';');
	    return 0;
    }
	if(tok->k == TOKIDENT) {
	    if(nexttok->k == ':') {
	        next();
	        next();
	        stmt();
	        return 0;
	    }
	}
	return stmt();
}

static Node *
stmt(void)
{
    Sym *sym;

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
	case '{':
		return block();
	default:
		return exprstmt();
	}
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
	expect(TOKRETURN);
	expr();
	expect(';');
	return 0;
}

static Node *
pswitch(void)
{
	expect(TOKSWITCH);
	expect('(');
	expr();
	expect(')');
	stmt();
}

static Node *
pcontinue(void)
{
	expect(TOKCONTINUE);
	expect(';');
}

static Node *
pbreak(void)
{
	expect(TOKBREAK);
	expect(';');
}

static Node *
pdefault(void)
{
	expect(TOKDEFAULT);
	expect(':');
	stmt();
}

static Node *
pcase(void)
{
	expect(TOKCASE);
	constexpr();
	expect(':');
	stmt();
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
	
	/* TODO: comma node. */
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
	    return 1;
	}
	return 0;
}

static Node *
assignexpr(void)
{
	condexpr();
	if(isassignop(tok->k)) {
		next();
		assignexpr();
	}
    return 0;
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
    logandexpr();
	while(tok->k == TOKLOR) {
		next();
		logandexpr();
	}
	return 0;
}

static Node *
logandexpr(void)
{
	orexpr();
	while(tok->k == TOKLAND) {
		next();
		orexpr();
	}
	return 0;
}

static Node *
orexpr(void)
{
	xorexpr();
	while(tok->k == '|') {
		next();
		xorexpr();
	}
	return 0;
}

static Node *
xorexpr(void)
{
	andexpr();
	while(tok->k == '^') {
		next();
		andexpr();
	}
	return 0;
}

static Node *
andexpr(void) 
{
	eqlexpr();
	while(tok->k == '&') {
		next();
		eqlexpr();
	}
	return 0;
}

static Node *
eqlexpr(void)
{
	relexpr();
	while(tok->k == TOKEQL || tok->k == TOKNEQ) {
		next();
		relexpr();
	}
	return 0;
}

static Node *
relexpr(void)
{
	shiftexpr();
	while(tok->k == '>' || tok->k == '<' 
	      || tok->k == TOKLEQ || tok->k == TOKGEQ) {
		next();
		shiftexpr();
	}
	return 0;
}

static Node *
shiftexpr(void)
{
	addexpr();
	while(tok->k == TOKSHL || tok->k == TOKSHR) {
		next();
		addexpr();
	}
	return 0;
}

static Node *
addexpr(void)
{
	mulexpr();
	while(tok->k == '+' || tok->k == '-') {
		next();
		mulexpr();
	}
	return 0;
}

static Node *
mulexpr(void)
{
	castexpr();
	while(tok->k == '*' || tok->k == '/' || tok->k == '%') {
		next();
		castexpr();
	}
	return 0;
}

static Node *
castexpr(void)
{
	if(tok->k == '(' && istypestart(nexttok)) {
		expect('(');
		typename();
		expect(')');
		unaryexpr();
		return 0;
	}
	return unaryexpr();
}

static CTy *
typename(void)
{
	int sclass;
	CTy *basety;
	char *name;

	declspecs(&sclass, &basety);
	declarator(basety, &name);
	return 0;
}

static Node *
unaryexpr(void)
{
    Sym *sym;

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
		next();
		castexpr();
		return 0;
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
	primaryexpr();
	for(;;) {
		switch(tok->k) {
		case '[':
			next();
			expr();
			expect(']');
			break;
		case '.':
			next();
			expect(TOKIDENT);
			break;
		case TOKARROW:
			next();
			expect(TOKIDENT);
			break;
		case '(':
			next();
			if(tok->k != ')') {
				for(;;) {
					assignexpr();
					if(tok->k != ',') {
						break;
					}
					next();
				}
			}
			expect(')');
			return 0;
		case TOKINC:
			next();
			break;
		case TOKDEC:
			next();
			break;
		default:
			goto done;
		}
	}
    done:
	return 0;
}


static Node *
primaryexpr(void) 
{
    Sym *sym;
    
	switch (tok->k) {
	case TOKIDENT:
		sym = lookupsym(vars, tok->v);
		if(!sym)
			errorpos(&tok->pos, "undefined symbol %s", tok->v);
		next();
		return 0;
	case TOKNUM:
		next();
		return 0;
	case TOKSTR:
		next();
		return 0;
	case '(':
		next();
		expr();
		expect(')');
		return 0;
	default:
		errorpos(&tok->pos, "expected an ident, constant, string or (");
	}
	error("unreachable.");
	return 0;
}

