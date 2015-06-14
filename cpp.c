#include "c.h"
#include <stdio.h>
#include <string.h>

int line;
int col;
static FILE * f;

char *
tokktostr(int t) 
{
	switch(t) {
	case TOKIDENT:  return "ident";
	case TOKRETURN: return "return";
	case TOKVOID:   return "void";
	case TOKCHAR:   return "char";
	case TOKINT:    return "int";
	case TOKNUMBER: return "number";
	case '(':       return "(";
	case ')':		return ")";	
	case '{':		return "{";
	case '}':		return "}";
	case ';':		return ";";
	}
	error("tokktostr: internal error\n");
	return 0;
}

/* makes a token, copies v */
static Tok *
mktok(int kind, char *v) {
	Tok* r;

	r = ccmalloc(sizeof(Tok));
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
	return fgetc(f);
}

static void
ungetch(int c) /* avoid name conflict */
{
	ungetc(c, f);
}

void
cppinit(char *p)
{
	f = fopen(p, "r");
	if (!f)
		error("error opening file %s.\n", p);
}

static struct {char *kw; int t;} keywordlut[] = {
	{"void", TOKVOID},
	{"char", TOKCHAR},
	{"int", TOKINT},
	{"return", TOKRETURN},
	{0, 0}
};

/* check if ident like token is a keyword, typename, or ident. */
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

static int
numberc(int c)
{
	return c >= '0' && c <= '9';  
}

static int
identc(int c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');  
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
	} else if(identc(c)) {
		*p++ = c;
		for(;;) {
			c = nextc();
			if (!identc(c)) {
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
				return mktok(TOKNUMBER, tokval);
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