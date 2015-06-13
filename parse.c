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
tok2str(int t) 
{
	switch(t) {
	case TOKIDENT:  return "ident";
	case TOKNUMBER: return "number";
	case '(':       return "(";
	case ')':		return ")";	
	case '{':		return "{";
	case '}':		return "}";
	case ';':		return ";";
	default:		return "unknown";
	}
}

void
expect(int kind) 
{
	if(tok != kind)
		error("expected token %s", tok2str(kind));
}

void 
parse(void)
{
	for(;;) {
		next();
		if(tok == TOKEOF)
			break;
		decl();
	}
}

Node *
decl(void) 
{
	printf("%s %s\n", tok2str(tok), tokval);
}

Node *
stmt(void)
{
	switch(tok) {
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
		return pblock();
	default:
		return exprstmt();
	}
}

Node *
pif(void)
{
	expect(TOKIF);
	if(tok == '{') {
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
	ex
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
pblock(void)
{
	expect('{');
	while(tok != '}' && tok != TOKEOF)
		pstmt();
	expect('}');
}