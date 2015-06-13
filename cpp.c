#include "cc.h"
#include <stdio.h>

int tok;
/* TODO: limit len - this can be standard max. */
char tokval[4096];

static FILE * f;

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
	{"return", TOKRETURN},
	{0, 0}
};

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

void
next(void) 
{
	char *p;
	int c,c2;

  again:
	p = tokval;
	while(*p++)
		*p = 0;
	p = tokval;
	c = nextc();
	*p++ = c;
	if(c == EOF) {
		tok = TOKEOF;
		return;
	} else if(wsc(c)) {
		do {
			c = nextc();
			if(c == EOF) {
				tok = TOKEOF;
				return;
			}
		} while(wsc(c));
		ungetch(c);
		goto again;
	} else if(identc(c)) {
		for(;;) {
			c = nextc();
			if (!identc(c)) {
				*p = 0;
				ungetch(c);
				tok = TOKIDENT;
				return;
			}
			*p++ = c;
		}
	} else if(numberc(c)) {
		for(;;) {
			c = nextc();
			if(!numberc(c)) {
				*p = 0;
				ungetch(c);
				tok = TOKNUMBER;
				return;
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
						tok = TOKEOF;
						return;
					}
				} while(c != '*');
				c = nextc();
				if(c == EOF) {
					tok = TOKEOF;
					return;
				}
				if(c == '/') {
					goto again;
				}
			}
		} else if(c == '/' && c2 == '/') {
			do {
				c = nextc();
				if (c == EOF) {
					tok = TOKEOF;
					return;
				}
			} while(c != '\n');
			goto again;
		} else if(c == '+' && c2 == '+') tok = TOKPOSINC;
		  else if(c == '+' && c2 == '=') tok = TOKADDASS;
		  else if(c == '-' && c2 == '-') tok = TOKPOSDEC;
		  else if(c == '-' && c2 == '=') tok = TOKSUBASS;
		  else if(c == '*' && c2 == '=') tok = TOKMULASS;
		  else if(c == '/' && c2 == '=') tok = TOKDIVASS;
		  else if(c == '%' && c2 == '=') tok = TOKMODASS;
		  else if(c == '>' && c2 == '=') tok = TOKGTEQL;
		  else if(c == '<' && c2 == '=') tok = TOKLTEQL;
		  else if(c == '!' && c2 == '=') tok = TOKNEQL;
		  else if(c == '=' && c2 == '=') tok = TOKEQL;
		else {
			ungetch(c2);
			tok = c;
		}
		return;
	}
}
