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
static Node *stmt(void);
static Node *exprstmt(void);
static Node *expr(void);
static Node *assignexpr(void);
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
static Node *decl(void);
static Node *globaldecl(void);
static void  declspecs();
static void  pstruct();
static void  penum();
static CTy  *typename(void);
static CTy  *declarator(CTy *, int abstract);
static CTy  *directdeclarator(CTy *, int abstract); 
static CTy  *declaratortail(CTy *);

static void  expect(int);

Tok *tok;
Tok *nexttok;

#define MAXSCOPES 1024

static int nscopes;
static Map *structs[MAXSCOPES];
static Map *types[MAXSCOPES];
static Map *vars[MAXSCOPES];

static void
popscope()
{
    nscopes -= 1;
    if (nscopes < 0)
        error("bug: scope underflow\n");
    vars[nscopes] = 0;
    types[nscopes] = 0;
    structs[nscopes] = 0;
}

static void
pushscope()
{
    vars[nscopes] = map();
    types[nscopes] = map();
    structs[nscopes] = map();
    nscopes += 1;
    if (nscopes > MAXSCOPES)
        error("scope depth exceeded maximum\n");
}

static int
isglobal()
{
	return nscopes == 1;
}

/*
static int
definesym(Map *scope[], char *k, void *v)
{
    Map *m;
    
    m = scope[nscopes - 1];
    if(mapget(m, k))
        return 1;
    mapset(m, k, v);
    return 0; 
}
*/

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
		globaldecl();
	}
	popscope();
	return 0;
}

static Node *
decl()
{
	declspecs();
	declarator(0, 0);
	return 0;
}

static Node *
globaldecl()
{
	decl();
	if(isglobal() && tok->k == '{') {
		block();
		return 0;
	}
	expect(';');
	return 0;
}

static void
declspecs()
{
	int done;
	Sym *sym;

	done = 0;
	while(!done) {
		switch(tok->k) {
		case TOKEXTERN:
		case TOKSTATIC:
		case TOKREGISTER:
		case TOKCONST:
		case TOKAUTO:
		case TOKTYPEDEF:
			next();
			break;
		case TOKSTRUCT:
			pstruct();
			done = 1;
			break;
		case TOKENUM:
			penum();
			done = 1;
			break;
		case TOKVOID:
		case TOKCHAR:
		case TOKSHORT:
		case TOKINT:
		case TOKLONG:
		case TOKFLOAT:
		case TOKDOUBLE:
		case TOKSIGNED:
		case TOKUNSIGNED:
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
}

static void
params(void)
{
	if(tok->k == ')')
		return;
	decl();
	while(tok->k == ',') {
		expect(',');
		decl();
	}
}

/* Declarator is what introduces names into the program.
   abstract means the ident is optional. */
static CTy *
declarator(CTy *basety, int abstract) 
{
	while (tok->k == TOKCONST || tok->k == TOKVOLATILE)
		next();
	switch(tok->k) {
	case '*':
		next();
		declarator(basety, 1);
		return 0;
	default:
	    directdeclarator(basety, abstract);
	    return 0;
	}

}

static CTy *
directdeclarator(CTy *basety, int abstract) 
{
    switch(tok->k) {
	case '(':
		expect('(');
	    declarator(0, abstract);
		expect(')');
		declaratortail(basety);
		return 0;
	case TOKIDENT:
		next();
		declaratortail(basety);
		return 0;
	default:
		if(!abstract)
		    errorpos(&tok->pos, "expected ident, ( or * but got %s", tokktostr(tok->k));
		declaratortail(basety);
		return 0;
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
			expect('[');
			if (tok->k != ']')
				assignexpr();
			expect(']');
		case '(':
			expect('(');
			params();
			expect(')');
			return 0;
		default:
			return basety;
		}
	}
}

static void
pstruct() 
{
	expect(TOKSTRUCT);
	if(tok->k == TOKIDENT) 
		next();
	expect('{');
	while(tok->k != '}') {
		expect(TOKINT);
		expect(TOKIDENT);
		expect(';');
	}
	expect('}');
}

static void
penum()
{
	expect(TOKENUM);
	if(tok->k == TOKIDENT) 
		next();
	expect('{');
	while(1) {
		if(tok->k == '}')
			break;
		expect(TOKIDENT);
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
	case '{':
		return block();
	case TOKREGISTER:
	case TOKSTATIC:
	case TOKAUTO:
	case TOKCONST:
	case TOKVOLATILE:
	case TOKVOID:
	case TOKCHAR:
	case TOKSHORT:
	case TOKINT:
	case TOKLONG:
	case TOKSIGNED:
	case TOKUNSIGNED:
	case TOKFLOAT:
	case TOKDOUBLE:
	    return decl();
	case TOKIDENT:
	    sym = lookupsym(types, tok->v);
	    if(sym)
	        decl();
	    /* Not decl, try expr. */
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
block(void)
{
	pushscope();
	expect('{');
	while(tok->k != '}' && tok->k != TOKEOF)
		stmt();
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

static int
istypestart(Tok *t)
{
    /* TODO */
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
	declspecs();
	declarator(0, 1);
	return 0;
}

static Node *
unaryexpr(void)
{
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
		errorpos(&tok->pos, "expected an identifier, constant, string or Expr");
	}
	error("unreachable.");
	return 0;
}

