#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "cpp.h"


#define MAXTOKSZ 4096

typedef struct Lexer {
	FILE   *f;
	SrcPos pos;
	SrcPos prevpos;
	SrcPos markpos;
	int    nchars;
	int    indirective;
	int    ws;
	int    nl;
	int    includestate;
	char   tokval[MAXTOKSZ+1];
} Lexer;

static void ungetch(Lexer *l, int c);
static int  nextc(Lexer *l);

char *
tokktostr(Tokkind t)
{
	switch(t) {
	case TOKIDENT:      return "ident";
	case TOKNUM:        return "number";
	case TOKIF:         return "if";
	case TOKELSE:       return "else";
	case TOKDO:         return "do";
	case TOKWHILE:      return "while";
	case TOKFOR:        return "for";
	case TOKVOID:       return "void";
	case TOKCHAR:       return "char";
	case TOKCHARLIT:    return "character literal";
	case TOKSHORT:      return "short";
	case TOKINT:        return "int";
	case TOKLONG:       return "long";
	case TOKSIGNED:     return "signed";
	case TOKUNSIGNED:   return "unsigned";
	case TOKEOF:        return "end of file";
	case TOKENUM:       return "enum";
	case TOKCONST:      return "const";
	case TOKRETURN:     return "return";
	case TOKSTR:        return "string";
	case TOKAUTO:       return "auto";
	case TOKEXTERN:     return "extern";
	case TOKSTATIC:     return "static";
	case TOKVOLATILE:   return "volatile";
	case TOKSIZEOF:     return "sizeof";
	case TOKFLOAT:      return "float";
	case TOKDOUBLE:     return "double";
	case TOKTYPEDEF:    return "typedef";
	case TOKUNION:      return "union";
	case TOKSTRUCT:     return "struct";
	case TOKGOTO:       return "goto";
	case TOKSWITCH:     return "switch";
	case TOKCASE:       return "case";
	case TOKDEFAULT:    return "default";
	case TOKCONTINUE:   return "continue";
	case TOKBREAK:      return "break";
	case TOKREGISTER:   return "register";
	case TOKSUBASS:     return "-=";
	case TOKSHR:        return ">>";
	case TOKSHL:        return "<<";
	case TOKORASS:      return "|=";
	case TOKNEQ:        return "!=";
	case TOKMULASS:     return "*=";
	case TOKMODASS:     return "%=";
	case TOKLOR:        return "||";
	case TOKLEQ:        return "<=";
	case TOKLAND:       return "&&";
	case TOKINC:        return "++";
	case TOKGEQ:        return ">=";
	case TOKEQL:        return "==";
	case TOKELLIPSIS:   return "...";
	case TOKDIVASS:     return "/=";
	case TOKDEC:        return "--";
	case TOKARROW:      return "->";
	case TOKANDASS:     return "&=";
	case TOKADDASS:     return "+=";
	case TOKHASH:       return "#";
	case TOKHASHHASH:   return "##";
	case TOKDIRSTART:   return "Directive start";
	case TOKDIREND:     return "Directive end";
	case TOKHEADER:     return "include header";
	case '?':           return "?";
	case '[':           return "[";
	case '+':           return "+";
	case '%':           return "%";
	case '&':           return "&";
	case '*':           return "*";
	case '}':           return "}";
	case '{':           return "{";
	case ']':           return "]";
	case ')':           return ")";
	case '(':           return "(";
	case '.':           return ".";
	case '/':           return "/";
	case '!':           return "!";
	case ':':           return ":";
	case ';':           return ";";
	case '<':           return "<";
	case '>':           return ">";
	case ',':           return ",";
	case '-':           return "-";
	case '|':           return "|";
	case '=':           return "=";
	case '~':           return "~";
	case '^':           return "^";
	case '\\':          return "\\";
	}
	panic("internal error converting token to string: %d", t);
	return 0;
}

enum {
	INCLUDEBEGIN,
	INCLUDEIDENT,
	INCLUDEHEADER
};

/* makes a token, copies v */
static Tok *
mktok(Lexer *l, int kind) {
	Tok *r;

	l->tokval[l->nchars] = 0;
	r = xmalloc(sizeof(Tok));
	r->pos.line = l->markpos.line;
	r->pos.col = l->markpos.col;
	r->pos.file = l->markpos.file;
	r->k = kind;
	r->ws = l->ws;
	r->nl = l->nl;
	l->ws = 0;
	l->nl = 0;
	switch(kind){
	case TOKSTR:
	case TOKNUM:
	case TOKIDENT:
	case TOKCHARLIT:
	case TOKHEADER:
		/* TODO: intern strings */
		r->v = xstrdup(l->tokval);
		break;
	default:
		r->v = tokktostr(kind);
		break;
	}
	/* The lexer needs to be aware of '<>' style includes. */
	switch(l->includestate) {
	case INCLUDEBEGIN:
		if(kind == TOKDIRSTART)
			l->includestate = INCLUDEIDENT;
		else
			l->includestate = INCLUDEBEGIN;
		break;
	case INCLUDEIDENT:
		if(kind == TOKIDENT && strcmp(r->v, "include") == 0)
			l->includestate = INCLUDEHEADER;
		else
			l->includestate = INCLUDEBEGIN;
		break;
	default:
		l->includestate = INCLUDEBEGIN;
		break;
	}
	return r;
}

static int
numberc(int c)
{
	return c >= '0' && c <= '9';  
}

static int
hexnumberc(int c)
{
	if(numberc(c))
		return 1;
	if(c >= 'a' && c <= 'f')
		return 1;
	if(c >= 'A' && c <= 'F')
		return 1;
	return 0;
}

static int
identfirstc(int c)
{
	if(c == '_')
		return 1;
	if(c >= 'a' && c <= 'z')
		return 1;
	if(c >= 'A' && c <= 'Z')
		return 1;
	return 0;
}

static int
identtailc(int c)
{
	if(numberc(c) || identfirstc(c))
		return 1;
	return 0;
}

static int
wsc(int c)
{
	return (c == '\r' || c == '\n' || c == ' ' || c == '\t');
}

static void 
mark(Lexer *l)
{
	l->nchars = 0;
	l->markpos.line = l->pos.line;
	l->markpos.col = l->pos.col;
}

static void 
accept(Lexer *l, int c)
{
	l->tokval[l->nchars] = c;
	l->nchars += 1;
	if(l->nchars > MAXTOKSZ)
		errorposf(&l->markpos, "token too large");
}

static int 
nextc(Lexer *l)
{
	int c;
	l->prevpos.col = l->pos.col;
	l->prevpos.line = l->pos.line;
	c = fgetc(l->f);
	if(c == '\n') {
		l->pos.line += 1;
		l->pos.col = 1;
	} else if(c == '\t')
		l->pos.col += 4;
	else 
		l->pos.col += 1;
	return c;
}

static void
ungetch(Lexer *l, int c) /* avoid name conflict */
{
	l->pos.col = l->prevpos.col;
	l->pos.line = l->prevpos.line;
	ungetc(c, l->f);
}

static Tok *
lex(Lexer *l) 
{
	Tok *t;
	int c, c2;
	
	mark(l);
	if(l->indirective && l->nl) {
		l->indirective = 0;
		t = mktok(l, TOKDIREND);
		l->nl = 1;
		return t;
	}
	c = nextc(l);
	if(c == EOF) {
		if(l->indirective) {
			l->indirective = 0;
			return mktok(l, TOKDIREND);
		}
		return mktok(l, TOKEOF);
	} else if(wsc(c)) {
		l->ws = 1;
		if(c == '\n') 
			l->nl = 1;
		do {
			c = nextc(l);
			if(c == EOF) {
				if(l->indirective) {
					l->indirective = 0;
					return mktok(l, TOKDIREND);
				}
				return mktok(l, TOKEOF);
			}
			if(c == '\n')
				l->nl = 1;
		} while(wsc(c));
		ungetch(l, c);
		return lex(l);
	} else if (c == '"') {
		accept(l, c);
		for(;;) {
			c = nextc(l);
			if(c == EOF)
				errorf("unclosed string\n"); /* TODO error pos */
			accept(l, c);
			if(c == '\\') {
				c = nextc(l);
				if(c == EOF || c == '\n')
					errorf("EOF or newline in string literal");
				accept(l, c);
				continue;
			}
			if(c == '"') /* TODO: escape chars */ 
				return mktok(l, TOKSTR);
		}
	} else if (c == '\'') {
		accept(l, c);
		c = nextc(l);
		if(c == EOF)
			errorf("unclosed char\n"); /* TODO error pos */
		accept(l, c);
		if(c == '\\') {
			c = nextc(l);
			if(c == EOF || c == '\n')
				errorf("EOF or newline in char literal\n");
			accept(l, c);
		}
		c = nextc(l);
		accept(l, c);
		if(c != '\'')
			errorf("char literal expects '\n");
		return mktok(l, TOKCHARLIT);
	} else if(identfirstc(c)) {
		accept(l, c);
		for(;;) {
			c = nextc(l);
			if (!identtailc(c)) {
				ungetch(l, c);
				return mktok(l, TOKIDENT);
			}
			accept(l, c);
		}
	} else if(numberc(c)) {
		accept(l, c);
		c2 = nextc(l);
		if(c == '0' && c2 == 'x') {
			accept(l, c);
			for(;;) {
				c = nextc(l);
				if (!hexnumberc(c)) {
					while(c == 'u' || c == 'l' || c == 'U' || c == 'L') {
						accept(l, c);
						c = nextc(l);
					}
					ungetch(l, c);
					return mktok(l, TOKNUM);
				}
				accept(l, c);
			}
		}
		ungetch(l, c2);
		for(;;) {
			c = nextc(l);
			if (!numberc(c)) {
				while(c == 'u' || c == 'l' || c == 'U' || c == 'L') {
					accept(l, c);
					c = nextc(l);
				}
				ungetch(l, c);
				return mktok(l, TOKNUM);
			}
			accept(l, c);
		}
	} else if(c == '<' && l->includestate == INCLUDEHEADER) {
		for(;;){
			accept(l, c);
			if(c == EOF)
				errorf("eof in include\n");
			if(c == '>')
				return mktok(l, TOKHEADER);
			c = nextc(l);
		}
	} else {
		c2 = nextc(l);
		if(c == '/' && c2 == '*') {
			l->ws = 1;
			for(;;) {
				do {
					c = nextc(l);
					if(c == EOF) {
						return mktok(l, TOKEOF);
					}
					if(c == '\n')
						l->nl = 1;
				} while(c != '*');
				c = nextc(l);
				if(c == EOF)
					return mktok(l, TOKEOF);
				if(c == '/') 
					return lex(l);
				if(c == '\n')
					l->nl = 1;
			}
		} else if(c == '/' && c2 == '/') {
			l->ws = 1;
			do {
				c = nextc(l);
				if (c == EOF)
					return mktok(l, TOKEOF);
			} while(c != '\n');
			l->nl = 1;
			ungetch(l, c);
			return lex(l);
		} else if(c == '.' && c2 == '.') {
			/* TODO, errorpos? */
			c = nextc(l);
			if(c != '.')
				errorf("expected ...\n");
			return mktok(l, TOKELLIPSIS);
		} else if(c == '+' && c2 == '=') return mktok(l, TOKADDASS);
		else if(c == '-' && c2 == '=') return mktok(l, TOKSUBASS);
		else if(c == '-' && c2 == '>') return mktok(l, TOKARROW);
		else if(c == '*' && c2 == '=') return mktok(l, TOKMULASS);
		else if(c == '/' && c2 == '=') return mktok(l, TOKDIVASS);
		else if(c == '%' && c2 == '=') return mktok(l, TOKMODASS);
		else if(c == '&' && c2 == '=') return mktok(l, TOKANDASS);
		else if(c == '|' && c2 == '=') return mktok(l, TOKORASS);
		else if(c == '>' && c2 == '=') return mktok(l, TOKGEQ);
		else if(c == '<' && c2 == '=') return mktok(l, TOKLEQ);
		else if(c == '!' && c2 == '=') return mktok(l, TOKNEQ);
		else if(c == '=' && c2 == '=') return mktok(l, TOKEQL);
		else if(c == '+' && c2 == '+') return mktok(l, TOKINC);
		else if(c == '-' && c2 == '-') return mktok(l, TOKDEC);
		else if(c == '<' && c2 == '<') return mktok(l, TOKSHL);
		else if(c == '>' && c2 == '>') return mktok(l, TOKSHR);
		else if(c == '|' && c2 == '|') return mktok(l, TOKLOR);
		else if(c == '&' && c2 == '&') return mktok(l, TOKLAND);
		else if(c == '#' && c2 == '#') return mktok(l, TOKHASHHASH);
		else if(c == '\\' && c2 == '\n') return lex(l);
		else {
			/* TODO, detect invalid operators */
			ungetch(l, c2);
			if(c == '#' && l->nl) {
				l->indirective = 1;
				return mktok(l, TOKDIRSTART);
			}
			return mktok(l, c);
		}
		panic("internal error\n");
	}
}


#define MAXINCLUDE 128

typedef enum {
	OBJMACRO,
	FUNCMACRO,
	BUILTINMACRO
} MacroKind;

typedef struct Macro Macro;
struct Macro {
	MacroKind k;
	union {
		struct {
			Vec  *toks;
		} Obj;
		struct {
			Vec *argnames; /* vec of char * */
			Vec *toks;
		} Func;
	};
};

int	nlexers;
Lexer *lexers[MAXINCLUDE];

static Vec *includedirs;

/* List of tokens inserted into the stream */
static List *toks;
static Map  *macros;

static Tok *ppnoexpand();
static int64 ifexpr();

static void
pushlex(char *path)
{
	Lexer *l;

	if(nlexers == MAXINCLUDE)
		panic("include depth limit reached!");
	l = xmalloc(sizeof(Lexer));
	l->pos.file = path;
	l->prevpos.file = path;
	l->markpos.file = path;
	l->pos.line = 1;
	l->pos.col = 1;
	l->nl = 1;
	l->indirective = 0;
	l->includestate = INCLUDEBEGIN;
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

static int
fileexists(char *fname)
{
	FILE *f;

	f = fopen(fname, "r");
	if(!f)
		return 0;
	fclose(f);
	return 1;
}

static char *
findinclude(char *path, int issysinclude)
{
	int i;
	char buf[4096];
	
	/* XXX check current dir */
	/* XXX proper path join */
	for(i = 0; i < includedirs->len; i++) {
		snprintf(buf, sizeof(buf), "%s/%s", (char*)vecget(includedirs, i), path);
		if(fileexists(buf))
			return xstrdup(buf);
	}
	return 0;
}

static void
include()
{
	int  n;
	Tok  *t;
	char *path, *fullpath;
	SrcPos *pos;
	int  sysinclude;

	t = ppnoexpand();
	pos = &t->pos;
	if(t->k != TOKSTR && t->k != TOKHEADER)
		errorposf(pos, "#include expects a string or '<>' style header");
	sysinclude = t->k == TOKHEADER;
	path = xstrdup(t->v);
	n = strlen(path);
	path[n - 1] = 0;
	path++;
	t = ppnoexpand();
	if(t->k != TOKDIREND)
		errorposf(&t->pos, "garbage at end of #include (%s)", tokktostr(t->k));
	fullpath = findinclude(path, sysinclude);
	if(!fullpath)
		errorposf(pos, "could not find header %s", path);
	pushlex(fullpath);
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
	m = xmalloc(sizeof(Macro));
	t = ppnoexpand();
	if(t->k == '(' && !t->ws) {
		m->k = FUNCMACRO;
		m->Func.toks = vec();
		m->Func.argnames = vec();
		for(;;) {
			t = ppnoexpand();
			if(t->k != TOKIDENT)
				errorposf(&t->pos, "invalid macro argname");
			vecappend(m->Func.argnames, t->v);
			t = ppnoexpand();
			if(t->k == ')')
				break;
			if(t->k != ',')
				errorposf(&t->pos, "preprocessor expected ','");
		}
		while(1) {
			t = ppnoexpand();
			if(t->k == TOKDIREND)
				break;
			vecappend(m->Func.toks, t);
		}
	} else {
		m->k = OBJMACRO;
		m->Obj.toks = vec();
		while(t->k != TOKDIREND) {
			vecappend(m->Obj.toks, t);
			t = ppnoexpand();
		}
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

/* TODO, inline? it isn't really too clear by itself */
static void
expandfunclike(SrcPos *pos, Macro *m, Vec *params, StrSet *hs)
{
	int i, j, k, argfound;
	Tok *t1, *t2;
	Vec *v, *expanded;

	if(m->k != FUNCMACRO)
		panic("internal error");
	if(m->Func.argnames->len != params->len)
		panic("internal error");
	expanded = vec();
	for(i = 0; i < m->Func.toks->len; i++) {
		t1 = vecget(m->Func.toks, i);
		argfound = 0;
		for(j = 0; j < m->Func.argnames->len; j++) {
			if(strcmp(t1->v, vecget(m->Func.argnames, j)) == 0) {
				v = vecget(params, j);
				for(k = 0; k < v->len; k++) {
					t2 = xmalloc(sizeof(Tok));
					*t2 = *(Tok*)(vecget(v, k));
					t2->pos = *pos;
					t2->hs = hs;
					vecappend(expanded, t2);
				}
				argfound = 1;
				break;
			}
		}
		if(!argfound) {
			t2 = xmalloc(sizeof(Tok));
			*t2 = *t1;
			t2->pos = *pos;
			t2->hs = hs;
			vecappend(expanded, t2);
		}
	}
	for(i = 0; i < expanded->len; i++)
		listprepend(toks, vecget(expanded, expanded->len - i - 1));
}

Tok *
pp()
{
	int	i, depth;
	Macro  *m;
	Vec	*params, *curparam;
	Tok	*t1, *t2, *expanded;
	StrSet *hsparen, *hsmacro;

	t1 = ppnoexpand();
	if(t1->k == TOKDIRSTART && toks->len == 0) {
		directive();
		return pp();
	}
	m = lookupmacro(t1);
	if(!m)
		return t1;
	hsmacro = t1->hs;
	switch(m->k) {
	case FUNCMACRO:
		t2 = pp();
		if(t2->k != '(') {
			listprepend(toks, t2);
			return t1;
		}
		params = vec();
		depth = 1;
		for(;;) {
			t2 = pp();
			if(t2->k == TOKEOF)
				errorposf(&t2->pos, "end of file in macro arguments");
			if(t2->k == ')' && depth == 1) {
				hsparen = t2->hs;
				break;
			}
			if(params->len == 0 || (t2->k == ',' && depth == 1)) {
				curparam = vec();
				vecappend(params, curparam);
			}
			if(t2->k != ',')
				vecappend(curparam, t2);
			if(t2->k == '(')
				depth++;
			if(t2->k == ')')
				depth--;
		}
		if(m->Func.argnames->len != params->len)
			errorposf(&t1->pos, "macro invoked with incorrect number of args");
		expandfunclike(&t1->pos, m, params, strsetadd(strsetintersect(hsmacro, hsparen), t1->v));
		return pp();
	case OBJMACRO:
		if(strsethas(t1->hs, t1->v))
			return t1;
		for(i = 0; i < m->Obj.toks->len; i++) {
			expanded = xmalloc(sizeof(Tok));
			*expanded = *(Tok*)vecget(m->Obj.toks, m->Obj.toks->len - i - 1);
			expanded->hs = strsetadd(expanded->hs, t1->v);
			listprepend(toks, expanded);
		}
		return pp();
	default:
		;
	}
	panic("unimplemented");
	return 0;
}

void
cppinit(char *path, Vec *includes)
{
	includedirs = includes;
	nlexers = 0;
	toks = list();
	macros = map();
	pushlex(path);
}

