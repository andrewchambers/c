#include <stdio.h>
#include "lex.h"

Tok *curtok;
Tok *peektok;

void
initlexer(char *name, FILE *f)
{
	pushlexer(name, f);
	nexttok();
	nexttok();
}

void
pushlexer(char *name, FILE *f)
{
	
}

void
nexttok()
{

}
