#include <stdio.h>
#include <string.h>
#include "c.h"

Node  *parse(void);

static Node *decl(void);
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
static CTy  *typename(void);
static CTy  *declarator(CTy *, int abstract); 
static CTy  *declaratortail(CTy *);
static void  declspecs();
static void  expect(int);
static int   isdeclstart(Tok *);

Tok *tok;
Tok *nexttok;

#define MAXSCOPES 1024

static int nscopes;
static Map *structs[MAXSCOPES];
static Map *types[MAXSCOPES];
static Map *vars[MAXSCOPES];

static void
pushscope()
{
    nscopes -= 1;
    if (nscopes < 0)
        error("bug: scope underflow");
    structs[nscopes] = 0;
}

static void
popscope()
{
    structs[nscopes] = map();
    nscopes += 1;
    if (nscopes > MAXSCOPES)
        error("scope depth exceeded maximum");
}

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

static void *
lookupsym(Map *scope[], char *k)
{
    int i;
    Map *m;
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
mknode(int type) {
	Node *n;

	n = ccmalloc(sizeof(Node));
	n->t = type;
	return n;
}

static void
next(void) {
	tok = nexttok;
	nexttok = lex();
}

static void
expect(int kind) 
{
	if(tok->k != kind)
		errorpos(&tok->pos,"expected %s", tokktostr(kind));
}

Node * 
parse()
{
	next();
	for(;;) {
		next();
		if(tok->k == TOKEOF)
			break;
		decl();
	}
}

static Node *
decl(void) 
{
	declarator(0, 0);
	return 0;
}

static void
declspecs()
{

}

static void
paramdecl(void)
{
	/* TODO */
	expect(TOKINT);
	expect(TOKIDENT);
}


/*
 Declarator

 A declarator is the part of a Decl that specifies
 the name that is to be introduced into the program.

 unsigned int a, *b, **c, *const*d *volatile*e ;
              ^  ^^  ^^^  ^^^^^^^^ ^^^^^^^^^^^

 Direct Declarator

 A direct declarator is missing the pointer prefix.

 e.g.
 unsigned int *a[32], b[];
               ^^^^^  ^^^

 Abstract Declarator

 A delcarator missing an identifier.
*/

static CTy *
declarator(CTy *basety, int abstract) 
{
	while (tok->k == TOKCONST || tok->k == TOKVOLATILE)
		next();
	switch(tok->k) {
	case '*':
		next();
		declarator(basety, abstract);
		return 0;
	case '(':
		next();
		declarator(0, abstract);
		expect(')');
		declaratortail(basety);
		return 0;
	case TOKIDENT:
		next();
		declaratortail(basety);
		return 0;
	default:
		if(abstract) {
			declaratortail(basety);
			return 0;
		}
		errorpos(&tok->pos, "expected ident, '(' or '*' but got %s", tokktostr(tok->k));
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
				assignexpr();
			expect(']');
		case '(':
			next();
			for(;;) {
				if(tok->k == ')')
					break;
				paramdecl();
				if(nexttok->k != ')')
					expect(',');
			}
			expect(')');
			return 0;
		default:
			return basety;
		}
	}
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
	case '{':
		return block();
	default:
		return exprstmt();
	}
}

static Node *
pif(void)
{
	expect(TOKIF);
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
	expect('{');
	while(tok->k != '}' && tok->k != TOKEOF)
		stmt();
	expect('}');
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
	/* TODO: other assign ops. */
	return k == '=';
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
isdeclstart(Tok *t)
{
    /* TODO */
    return 0;
}

static Node *
castexpr(void)
{
	if(tok->k == '(' && isdeclstart(nexttok)) {
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

