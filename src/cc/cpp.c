#include <u.h>
#include <gc/gc.h>
#include <ds/ds.h>
#include "c.h"

#define MAXINCLUDE 128

int    nlexers;
Lexer *lexers[MAXINCLUDE];

static void
pushlex(char *path)
{
	Lexer *l;

	if(nlexers == MAXINCLUDE)
		panic("include depth limit reached!");
	l = gcmalloc(sizeof(Lexer));
	l->pos.file = path;
	l->prevpos.file = path;
	l->markpos.file = path;
	l->pos.line = 1;
	l->pos.col = 1;
	l->f = fopen(path, "r");
	if (!l->f)
		errorf("error opening file %s\n", path);
	lexers[nlexers] = l;
	nlexers += 1;
}

static void
poplex()
{
	nlexers--;
	fclose(lexers[nlexers]->f);
}

static void
doinclude(Vec *v)
{
	Tok  *t;
	char *dir;

	t = vecget(v, 0);
	if(v->len != 2)
		errorposf(&t->pos, "include directive expects one param");
	t = vecget(v, 1);
	if(t->k != TOKSTR)
		errorposf(&t->pos, "include expects a string");
	dir = t->v;
	dir += 1;
	dir[strlen(dir) - 1] = 0;
	pushlex(dir);
}

static void
dodirective(Vec *v)
{
	Tok  *t;
	char *dir;

	t = vecget(v, 0);
	dir = t->v;
	if(strcmp(dir, "include") == 0) {
		doinclude(v);
	} else {
		errorposf(&t->pos, "invalid directive %s", dir);
	}
}

static void
directive()
{
	Tok *t;
	Vec *v;

	v = vec();
	while(1) {
	start:
		t = pp();
		if(t->k == TOKEOF)
			break;
		if(t->k == '\\' && t->beforenl)
			goto start;
		vecappend(v, t);
		if(t->beforenl)
			break;
	}
	dodirective(v);
}

Tok *
pp()
{
	Tok *t;
	
	t = lex(lexers[nlexers - 1]);
	if(t->k == TOKEOF && nlexers == 1)
		return t;
	if(t->k == TOKEOF && nlexers > 1) {
		poplex();
		return pp();
	}
	if(t->k == '#') {
		directive();
		return pp();
	}
	return t;
}

void
cppinit(char *path)
{
	nlexers = 0;
	pushlex(path);
}

