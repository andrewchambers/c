#include <stdio.h>
#include <string.h>
#include "mem/mem.h"
#include "ds/ds.h"
#include "c.h"

#define MAXINCLUDE 1024

int nlexers;
Lexer *lexers[MAXINCLUDE];

static void
pushlex(char *path)
{
	Lexer *l;

	if(nlexers == MAXINCLUDE)
		errorf("include depth limit reached!");
	l = zmalloc(sizeof(Lexer));
	l->pos.file = path;
	l->prevpos.file = path;
	l->markpos.file = path;
	l->pos.line = 1;
	l->pos.col = 1;
	l->f = fopen(path, "r");
	if (!l->f)
		errorf("error opening file %s.\n", path);
	lexers[nlexers] = l;
	nlexers += 1;
}

static void
poplex()
{
	nlexers--;
	fclose(lexers[nlexers]->f);
}

Tok *
pp()
{
	Tok *t;
	
	t = lex(lexers[nlexers - 1]);
	if(t->k == TOKEOF && nlexers == 1)
			return t;
	poplex();
	return pp();
}

void
cppinit(char *path)
{
	nlexers = 0;
	pushlex(path);
}

