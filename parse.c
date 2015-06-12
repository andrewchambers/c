#include <stdarg.h>
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

int   tok;
char *tokval;
int   line;
int   col;

char *
tokstr(int t) 
{
	switch(t) {
		default:
			return "unknown";
	}
}

void
parse_error(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
}

void
expect(int kind) 
{
	if(tok != kind)
		parse_error("expected token %s", tokstr(kind));
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