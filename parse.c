#include <stdio.h>
#include <string.h>
#include "c.h"

void  parse(void);
Node* decl(void);
Node* stmt(void);
Node* pif(void);
Node* pfor(void);
Node* dowhile(void);
Node* pwhile(void);
Node* block(void);
Node* preturn(void);
Node* stmt(void);
Node* exprstmt(void);
Node* expr(void);
void  expect(int);

Tok *tok;
Tok *nexttok;

static Node *
mknode(int type) {
	Node *n;

	n = ccmalloc(sizeof(Node));
	n->t = type;
	return n;
}

void
next() {
	tok = nexttok;
	nexttok = lex();
}

void
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

Node *
decl(void) 
{
	return 0;
}

Node *
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

Node *
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

Node *
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

Node *
pwhile(void)
{
	expect(TOKWHILE);
	expect('(');
	expr();
	expect(')');
	stmt();
	return 0;
}

Node *
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

Node *
expr(void)
{
	Node *n;

	n = mknode(NNUMBER);
	n->pos = tok->pos;
	n->Number.v = tok->v;
	expect(TOKNUMBER);
	return n;
}


Node *
exprstmt(void)
{
	Node *n;

	n = expr();
	expect(';');
	return n;
}

Node *
preturn(void)
{
	expect(TOKRETURN);
	expr();
	expect(';');
	return 0;
}

Node *
block(void)
{
	expect('{');
	while(tok->k != '}' && tok->k != TOKEOF)
		stmt();
	expect('}');
	return 0;
}