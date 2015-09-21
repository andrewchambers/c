#include <u.h>
#include <gc/gc.h>
#include <ds/ds.h>
#include "c.h"

#define MAXINCLUDE 128

typedef enum {
	OBJMACRO,
	FUNCMACRO,
	BUILTINMACRO,
} MacroKind;
typedef struct Macro Macro;
struct Macro {
	MacroKind k;
	union {
		struct {
			Vec  *toks;
		} Obj;
		struct {
		} Func;
	};
};

int    nlexers;
Lexer *lexers[MAXINCLUDE];

/* List of tokens inserted into the stream */
List *toks;
Map  *macros;

static Tok *ppnoexpand();
static int64 ifexpr();

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
	l->nl = 1;
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
include()
{
	panic("unimplemented");
}

static int
validmacrotok(Tokkind k)
{
	switch(k) {
	case TOKIDENT:
		return 1;
	default:
		return 0;
	}
}

static Macro *
lookupmacro(Tok *t)
{
	if(!validmacrotok(t->k))
		return 0;
	return mapget(macros, t->v);
}

static void
define()
{
	Macro *m;
	Tok   *n, *t;

	n = ppnoexpand();
	if(!validmacrotok(n->k))
		errorposf(&n->pos, "invalid macro name %s", n->v);
	if(lookupmacro(n))
		errorposf(&n->pos, "redefinition of macro %s", n->v);
	m = gcmalloc(sizeof(Macro));
	m->k = OBJMACRO;
	m->Obj.toks = vec();
	while(1) {
		t = ppnoexpand();
		if(t->k == TOKDIREND)
			break;
		vecappend(m->Obj.toks, t);
	}
	mapset(macros, n->v, m);
}

static void
undef()
{
	Tok *t;

	t = ppnoexpand();
	if(!lookupmacro(t))
		errorposf(&t->pos, "cannot #undef macro that isn't defined");
	mapdel(macros, t->v);
	t = ppnoexpand();
	if(t->k != TOKDIREND)
		errorposf(&t->pos, "garbage at end of #undef");
}

static void
pif()
{
	ifexpr();
	panic("unimplemented");
}

static void
elseif()
{
	panic("unimplemented");
}

static void
pelse()
{
	panic("unimplemented");
}

static void
endif()
{
	panic("unimplemented");
}

static int64
ifexpr()
{
	return 0;
}

static void
directive()
{
	Tok  *t;
	char *dir;

	t = ppnoexpand();
	dir = t->v;
	if(strcmp(dir, "include") == 0)
		include();
	else if(strcmp(dir, "define") == 0)
		define();
	else if(strcmp(dir, "undef") == 0)
		undef();
	else if(strcmp(dir, "if") == 0)
		pif();
	else if(strcmp(dir, "elseif") == 0)
		elseif();
	else if(strcmp(dir, "else") == 0)
		pelse();
	else if(strcmp(dir, "endif") == 0)
		endif();
	else
		errorposf(&t->pos, "invalid directive %s", dir);
}

static struct {char *kw; int t;} keywordlut[] = {
	{"auto", TOKAUTO},
	{"break", TOKBREAK},
	{"case", TOKCASE},
	{"char", TOKCHAR},
	{"const", TOKCONST},
	{"continue", TOKCONTINUE},
	{"default", TOKDEFAULT},
	{"do", TOKDO},
	{"double", TOKDOUBLE},
	{"else", TOKELSE},
	{"enum", TOKENUM},
	{"extern", TOKEXTERN},
	{"float", TOKFLOAT},
	{"for", TOKFOR},
	{"goto", TOKGOTO},
	{"if", TOKIF},
	{"int", TOKINT},
	{"long", TOKLONG},
	{"register", TOKREGISTER},
	{"return", TOKRETURN},
	{"short", TOKSHORT},
	{"signed", TOKSIGNED},
	{"sizeof", TOKSIZEOF},
	{"static", TOKSTATIC},
	{"struct", TOKSTRUCT},
	{"switch", TOKSWITCH},
	{"typedef", TOKTYPEDEF},
	{"union", TOKUNION},
	{"unsigned", TOKUNSIGNED},
	{"void", TOKVOID},
	{"volatile", TOKVOLATILE},
	{"while", TOKWHILE},
	{0, 0}
};

static int
identkind(char *s) {
	int i;

	i = 0;
	while(keywordlut[i].kw) {
		if(strcmp(keywordlut[i].kw, s) == 0)
			return keywordlut[i].t;
		i++;
	}
	return TOKIDENT;
}

static Tok *
ppnoexpand()
{
	Tok *t;

	if(toks->len)
		return listpopfront(toks);
	t = lex(lexers[nlexers - 1]);
	if(t->k == TOKEOF && nlexers == 1)
		return t;
	if(t->k == TOKEOF && nlexers > 1) {
		poplex();
		return ppnoexpand();
	}
	if(t->k == TOKIDENT)
		t->k = identkind(t->v);
	return t;
}

Tok *
pp()
{
	int     i, depth;
	Macro   *m;
	Tok     *t1, *t2, *expanded;

	t1 = ppnoexpand();
	if(t1->k == TOKDIRSTART && toks->len == 0) {
		directive();
		return pp();
	}
	m = lookupmacro(t1);
	if(!m)
		return t1;
	switch(m->k) {
	case FUNCMACRO:
		t2 = pp();
		if(t2->k != '(') {
			listappend(toks, t2);
			return t1;
		}
		depth = 1;
		while(depth != 0) {
			if(t2->k == TOKEOF) {
				panic("XXX");
			}
			if(t2->k == '(') {
				depth++;
			} else if(t2->k == ')') {
				depth--;
			} else if(t2->k == ',') {
				if(depth == 1) {

				}
			} else {

			}
			t2 = pp();
		}
		return t1;
	case OBJMACRO:
		if(strsethas(t1->hs, t1->v))
			return t1;
		for(i = 0; i < m->Obj.toks->len; i++) {
			expanded = gcmalloc(sizeof(Tok));
			*expanded = *(Tok*)vecget(m->Obj.toks, i);
			expanded->hs = strsetadd(expanded->hs, t1->v);
			listappend(toks, expanded);
		}
		return pp();
	default:
		panic("unimplemented");
	}
}

void
cppinit(char *path)
{
	nlexers = 0;
	toks = list();
	macros = map();
	pushlex(path);
}

