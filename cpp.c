#include "c.h"
#include <stdio.h>
#include <string.h>

#define MAXTOKSZ 4096

typedef struct Lexer Lexer;
struct Lexer {
    FILE *f;
    SrcPos pos;
    SrcPos prevpos;
    SrcPos markpos;
    int  nchars;
    char tokval[MAXTOKSZ+1];
};

Lexer *l;

void
cppinit(char *p)
{
    l = ccmalloc(sizeof(Lexer));
    l->pos.file = p;
    l->prevpos.file = p;
    l->markpos.file = p;
    l->pos.line = 1;
    l->pos.col = 1;
	l->f = fopen(p, "r");
	if (!l->f)
		error("error opening file %s.\n", p);
}

static char *tok2strab[TOKEOF+1] = {
    [TOKNUM]      = "number",
    [TOKIDENT]    = "ident",
    [TOKIF]       = "if",
    [TOKDO]       = "do",
    [TOKFOR]      = "for",
    [TOKWHILE]    = "while",
    [TOKRETURN]   = "return",
    [TOKELSE]     = "else",
    [TOKVOLATILE] = "volatile",
    [TOKCONST]    = "const",
    [TOKSTRUCT]   = "struct",
    [TOKCASE]     = "case",
    [TOKDEFAULT]  = "default",
    [TOKUNION]    = "union",
    [TOKGOTO]     = "goto",
    [TOKSWITCH]   = "switch",
    [TOKREGISTER] = "register",
    [TOKEXTERN]   = "extern",
    [TOKSTATIC]   = "static",
    [TOKAUTO]     = "auto",
    [TOKENUM]     = "enum",
    [TOKSTR]      = "string",
    [TOKTYPEDEF]  = "typedef",
    [TOKSIGNED]   = "signed",
    [TOKUNSIGNED] = "unsigned",
    [TOKDEFAULT]  = "default",
    [TOKSIZEOF]   = "sizeof",
    [TOKVOID]     = "void",
    [TOKCHAR]     = "char",
    [TOKINT]      = "int",
    [TOKSHORT]    = "short",
    [TOKLONG]     = "long",
    [TOKFLOAT]    = "float",
    [TOKDOUBLE]   = "double",
    [TOKEOF]      = "end of file",
    [TOKINC]      = "++",
    [TOKDEC]      = "--",
    [TOKADDASS]   = "+=",
    [TOKSUBASS]   = "-=",
    [TOKMULASS]   = "*=",
    [TOKDIVASS]   = "/=",
    [TOKMODASS]   = "%=",
    [TOKGTEQL]    = ">=",
    [TOKLTEQL]    = "<=",
    [TOKEQL]      = "==",
    [TOKLOR]      = "||",
    [TOKLAND]     = "&&",
    [TOKNEQ]      = "!=",
    [TOKLEQ]      = "<=",
    [TOKGEQ]      = ">=",
    [TOKSHL]      = "<<",
    [TOKSHR]      = ">>",
    [TOKARROW]    = "->",
    [TOKELLIPSIS] = "...",
    ['=']         = "=",
    ['!']         = "!",
    ['~']         = "~",
    ['+']         = "+",
    ['-']         = "-",
    ['/']         = "/",
    ['*']         = "*",
    ['%']         = "%",
    ['&']         = "&",
    ['|']         = "|",
    ['^']         = "^",
	['(']         = "(",
	[')']         = ")",
	['[']         = "[",
	[']']         = "]",
	['{']         = "{",
	['}']         = "}",
	[',']         = ",",
	[';']         = ";",
	[':']         = ":",
};

char *
tokktostr(int t) 
{
	return tok2strab[t];
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
	{"enum", TOKENUM},
	{"goto", TOKGOTO},
	{"continue", TOKCONTINUE},
	{"break", TOKBREAK},
	{"default", TOKDEFAULT},
	{"sizeof", TOKSIZEOF},
	{"switch", TOKSWITCH},
	{"for", TOKFOR},
	{"while", TOKWHILE},
	{"if", TOKIF},
	{"else", TOKELSE},
	{"const", TOKCONST},
	{"volatile", TOKVOLATILE},
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

/* makes a token, copies v */
static Tok *
mktok(Lexer *l, int kind) {
	Tok* r;
    
    l->tokval[l->nchars] = 0;
	if(kind == TOKIDENT)
	    kind = identkind(l->tokval);
	r = ccmalloc(sizeof(Tok));
	r->pos.line = l->markpos.line;
	r->pos.col = l->markpos.col;
	r->pos.file = l->markpos.file;
	r->k = kind;
	switch(kind){
	case TOKSTR:
    case TOKNUM:
    case TOKIDENT:
        r->v = ccstrdup(l->tokval);
        break;
	default:
	    r->v = tokktostr(kind);
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
	if(c >= 'F' && c <= 'F')
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
        errorpos(&l->markpos, "token too large");
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

Tok *
lex() 
{
	int c,c2;
    
	mark(l);
	c = nextc(l);
	if(c == EOF) {
		return mktok(l, TOKEOF);
	} else if(wsc(c)) {
		do {
			c = nextc(l);
			if(c == EOF) {
				return mktok(l, TOKEOF);
			}
		} while(wsc(c));
		ungetch(l, c);
		return lex();
	} else if (c == '"') {
	     accept(l, c);;
	    for(;;) {
			c = nextc(l);
			if(c == EOF)
			    error("unclosed string\n"); /* TODO error pos */
			accept(l, c);
			if (c == '"') { /* TODO: escape chars */ 
				return mktok(l, TOKSTR);
			}
		}
	} else if (c == '\'') {
	    accept(l, c);
	    for(;;) {
			c = nextc(l);
			if(c == EOF)
			    error("unclosed string\n"); /* TODO error pos */
			accept(l, c);
			if (c == '\'') { /* TODO: escape chars */ 
				return mktok(l, TOKNUM);
			}
		}
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
				ungetch(l, c);
				return mktok(l, TOKNUM);
			}
			accept(l, c);
		}
	} else {
		c2 = nextc(l);
		if(c == '/' && c2 == '*') {
			for(;;) {
				do {
					c = nextc(l);
					if(c == EOF) {
						return mktok(l, TOKEOF);
					}
				} while(c != '*');
				c = nextc(l);
				if(c == EOF) {
					return mktok(l, TOKEOF);
				}
				if(c == '/') {
					return lex();
				}
			}
		} else if(c == '/' && c2 == '/') {
			do {
				c = nextc(l);
				if (c == EOF) {
					return mktok(l, TOKEOF);
				}
			} while(c != '\n');
			return lex();
		} else if(c == '.' && c2 == '.') {
			/* TODO, errorpos? */
			c = nextc(l);
			if(c != '.')
				error("expected ...\n");
			return mktok(l, TOKELLIPSIS);
		} else if(c == '+' && c2 == '=') return mktok(l, TOKADDASS);
		  else if(c == '-' && c2 == '=') return mktok(l, TOKSUBASS);
		  else if(c == '*' && c2 == '=') return mktok(l, TOKMULASS);
		  else if(c == '/' && c2 == '=') return mktok(l, TOKDIVASS);
		  else if(c == '%' && c2 == '=') return mktok(l, TOKMODASS);
		  else if(c == '>' && c2 == '=') return mktok(l, TOKGTEQL);
		  else if(c == '<' && c2 == '=') return mktok(l, TOKLTEQL);
		  else if(c == '!' && c2 == '=') return mktok(l, TOKNEQ);
		  else if(c == '=' && c2 == '=') return mktok(l, TOKEQL);
		  else if(c == '+' && c2 == '+') return mktok(l, TOKINC);
		  else if(c == '-' && c2 == '-') return mktok(l, TOKDEC);
		  else if(c == '<' && c2 == '<') return mktok(l, TOKSHL);
		  else if(c == '>' && c2 == '>') return mktok(l, TOKSHR);
		else {
		    /* TODO, detect invalid operators */
			ungetch(l, c2);
			return mktok(l, c);
		}
		error("internal error\n");
	}
}
