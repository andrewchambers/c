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
void  expect(int);

Tok *tok;
Tok *nexttok;

void
next() {
	tok = nexttok;
	nexttok = lex();
}

void
expect(int kind) 
{
	if(tok->k != kind)
		error("expected %s", tokktostr(kind));
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
	/* puts(tokktostr(tok->k)); */
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
	if(tok->k == '{') {
		expect('{');
		for(;;)
			stmt();		
		expect('}');
	}
}

Node *
pfor(void)
{
	expect(TOKFOR);
}

Node *
pwhile(void)
{

}

Node *
dowhile(void)
{

}

Node *
exprstmt(void)
{

}

Node *
preturn(void)
{

}

Node *
block(void)
{
	expect('{');
	while(tok->k != '}' && tok->k != TOKEOF)
		stmt();
	expect('}');
}