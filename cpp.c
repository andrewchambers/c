#include "cc.h"

int tok;
char tokval[4096];
static char *tokend;
static char *tokp;

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
	f = fopen(p);
	if (!f)
		error("error opening file %s.\n", p);
}

static struct {char *kw, int t} keywordlut[] = {
	{"return", TOKRETURN},
	{0, 0},
}

static int
isnumeric(int c)
{
	return c >= '0' && c <= '9';  
}

static int
ialpha(int c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');  
}

void
next(void) 
{
	char *p;
	int c,c2;

	p = tokval;
	while(*p++)
		*p = 0;
	p = tokval;
	c = nextc();
	*p++ = c;
	if(isalpha(c)) {
		for(;;) {
			c = nextc();
			if (!isalpha(c)) {
				*p = 0;
				ungetch();
				if (issym) {
					tok = TOKIDENT;
					return
				}
			}
			*p++ = c;
		}
	} else if(isnumeric(c)) {
		for(;;) {
			c = nextc();
			if (!isnumber(c)) {
				*p = 0;
				ungetch();
				tok = TOKNUMBER;
				return
			}
			*p++ = c;
		}
	} else {
		c2 = nextc();
		if(c == '/' && c2 == '*') {
			for (;;) {
				while(nextc() != '*');
				if (nextc() == '/') {
					next();
					return;
				}
			}
			next();
			return;
		} else if(c == '/' && c2 == '/') {
			while(nextc() != '\n');
			next();
			return;
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
