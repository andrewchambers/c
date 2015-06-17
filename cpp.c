#include "c.h"
#include <stdio.h>
#include <string.h>

static FILE * f;
static char *fname;
static int line;
static int col;
static int mline;
static int mcol;

void
cppinit(char *p)
{
	line = 1;
	col = 1;
	fname = p;
	f = fopen(p, "r");
	if (!f)
		error("error opening file %s.\n", p);
}

char *
tokktostr(int t) 
{
	switch(t) {
	case TOKIDENT:   return "ident";
	case TOKRETURN:  return "return";
	case TOKVOID:    return "void";
	case TOKCHAR:    return "char";
	case TOKINT:     return "int";
	case TOKNUM:     return "number";
	case TOKTYPEDEF: return "typedef";
	case '(':        return "(";
	case ')':		 return ")";	
	case '{':		 return "{";
	case '}':		 return "}";
	case ';':		 return ";";
	case '+':		 return "+";
	case '-':		 return "-";
	case '/':		 return "/";
	case '*':		 return "*";
	case '#':		 return "#";
	}
	error("tokktostr: unknown tok %d %c\n", t, t);
	return 0;
}

static void 
markstart()
{
	mline = line;
	mcol = col;
}

/* makes a token, copies v */
static Tok *
mktok(int kind, char *v) {
	Tok* r;

	r = ccmalloc(sizeof(Tok));
	r->pos.line = mline;
	r->pos.col = mcol;
	r->pos.file = fname;
	r->k = kind;
	if (v)
		r->v = ccstrdup(v);
	else
		r->v = tokktostr(kind);
	return r;
}

static int 
nextc(void)
{
	int c;

	c = fgetc(f);
	if(c == '\n') {
		line += 1;
		col = 1;
	} else if(c == '\t')
		col += 4;
	else 
		col += 1;
	return c;
}

static void
ungetch(int c) /* avoid name conflict */
{
	ungetc(c, f);
}

static struct {char *kw; int t;} keywordlut[] = {
	{"void", TOKVOID},
	{"signed", TOKSIGNED},
	{"unsigned", TOKUNSIGNED},
	{"char", TOKCHAR},
	{"short", TOKSHORT},
	{"int", TOKINT},
	{"long", TOKLONG},
	{"float", TOKFLOAT},
	{"double", TOKDOUBLE},
	{"return", TOKRETURN},
	{"typedef", TOKTYPEDEF},
	{"struct", TOKSTRUCT},
	{"union", TOKUNION},
	{"goto", TOKGOTO},
	{"switch", TOKSWITCH},
	{"for", TOKFOR},
	{"while", TOKWHILE},
	{"if", TOKIF},
	{"else", TOKELSE},
	{"register", TOKREGISTER},
	{"static", TOKSTATIC},
	{"extern", TOKEXTERN},
	{"auto", TOKAUTO},
	{0, 0}
};

/* check if ident like token is a keyword, typename, or ident. */
static int 
identkind(char *s) {
	int i;

	/* TODO: improve on linear lookup. */
	i = 0;
	while(keywordlut[i].kw) {
		if(strcmp(keywordlut[i].kw, s) == 0)
			return keywordlut[i].t;
		i++;
	}
	return TOKIDENT;
}

static int
numberc(int c)
{
	return c >= '0' && c <= '9';  
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

Tok *
lex(void) 
{
	/* TODO: find max length of ident in standard. */
	char tokval[4096];
	char *p;
	int c,c2;

  again:
	p = tokval;
	markstart();
	c = nextc();
	if(c == EOF) {
		return mktok(TOKEOF, 0);
	} else if(wsc(c)) {
		do {
			c = nextc();
			if(c == EOF) {
				return mktok(TOKEOF, 0);
			}
		} while(wsc(c));
		ungetch(c);
		goto again;
	} else if(identfirstc(c)) {
		*p++ = c;
		for(;;) {
			c = nextc();
			if (!identtailc(c)) {
				*p = 0;
				ungetch(c);
				return mktok(identkind(tokval), tokval);
			}
			*p++ = c;
		}
	} else if(numberc(c)) {
		*p++ = c;
		for(;;) {
			c = nextc();
			if(!numberc(c)) {
				*p = 0;
				ungetch(c);
				return mktok(TOKNUM, tokval);
			}
			*p++ = c;
		}
	} else {
		c2 = nextc();
		if(c == '/' && c2 == '*') {
			for(;;) {
				do {
					c = nextc();
					if(c == EOF) {
						return mktok(TOKEOF, 0);
					}
				} while(c != '*');
				c = nextc();
				if(c == EOF) {
					return mktok(TOKEOF, 0);
				}
				if(c == '/') {
					goto again;
				}
			}
		} else if(c == '/' && c2 == '/') {
			do {
				c = nextc();
				if (c == EOF) {
					return mktok(TOKEOF, 0);
				}
			} while(c != '\n');
			goto again;
		} else if(c == '+' && c2 == '=') return mktok(TOKADDASS, 0);
		  else if(c == '-' && c2 == '=') return mktok(TOKSUBASS, 0);
		  else if(c == '*' && c2 == '=') return mktok(TOKMULASS, 0);
		  else if(c == '/' && c2 == '=') return mktok(TOKDIVASS, 0);
		  else if(c == '%' && c2 == '=') return mktok(TOKMODASS, 0);
		  else if(c == '>' && c2 == '=') return mktok(TOKGTEQL, 0);
		  else if(c == '<' && c2 == '=') return mktok(TOKLTEQL, 0);
		  else if(c == '!' && c2 == '=') return mktok(TOKNEQL, 0);
		  else if(c == '=' && c2 == '=') return mktok(TOKEQL, 0);
		  else if(c == '+' && c2 == '+') return mktok(TOKINC, 0);
		  else if(c == '-' && c2 == '-') return mktok(TOKDEC, 0);
		else {
			ungetch(c2);
			return mktok(c, 0);
		}
		error("internal error\n");
	}
}