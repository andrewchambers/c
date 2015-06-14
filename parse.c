#include <stdio.h>
#include <string.h>
#include "c.h"

void  parse(void);
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
static CTy  *declarator(CTy*, int abstract); 
static CTy  *declaratortail(CTy*);
static void  expect(int);

Tok *tok;
Tok *nexttok;

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

void 
parse(void)
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

static Node *
assignexpr(void)
{
	Node *n;

	n = mknode(NNUMBER);
	n->pos = tok->pos;
	n->Number.v = tok->v;
	expect(TOKNUMBER);
	return n;
}
