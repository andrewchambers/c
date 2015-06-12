#include <stdio.h>
#include "cc.h"

void  parse(void);
Node* decl(void);
Node* stmt(void);
Node* pif(void);
Node* pfor(void);
Node* dowhile(void);
Node* pwhile(void);
Node* preturn(void);
Node* stmt(void);
Node* exprstmt(void);
void  expect(int);


char *
tokstr(int t) 
{
	switch(t) {
	default:
		return "unknown";
	}
}

void
expect(int kind) 
{
	if(tok != kind)
		error("expected token %s", tokstr(kind));
}

void 
parse(void)
{
	for(;;) {
		next();
		if(tok == EOF)
			break;
		decl();
	}
}

Node *
decl(void) 
{
	puts(tokval);
}

Node *
stmt(void)
{
	switch(tok) {
	case TOKIF:
		pif();
	case TOKFOR:
		pfor();
	case TOKWHILE:
		pwhile();
	case TOKDO:
		dowhile();
	case TOKRETURN:
		preturn();
	default:
		exprstmt();
	}
}

Node *
pif(void)
{
	expect(TOKIF);
	if(tok == '{') {
		expect('{');
		for(;;) {
			stmt();		
		}
		expect('}');
	}
}

Node *
pfor(void)
{

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