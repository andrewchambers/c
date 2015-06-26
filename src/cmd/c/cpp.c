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
		errorf("error opening file %s.\n", p);
}

char *
tokktostr(int t) 
{
    switch(t) {
    case TOKWHILE:    return "while";
    case TOKVOLATILE: return "volatile";
    case TOKVOID:     return "void";
    case TOKUNSIGNED: return "unsigned";
    case TOKUNION:    return "union";
    case TOKTYPEDEF:  return "typedef";
    case TOKSWITCH:   return "switch";
    case TOKSUBASS:   return "-=";
    case TOKSTRUCT:   return "struct";
    case TOKSTR:      return "string";
    case TOKSTATIC:   return "static";
    case TOKSIZEOF:   return "sizeof";
    case TOKSIGNED:   return "signed";
    case TOKSHR:      return ">>";
    case TOKSHORT:    return "short";
    case TOKSHL:      return "<<";
    case TOKRETURN:   return "return";
    case TOKREGISTER: return "register";
    case TOKORASS:    return "|=";
    case TOKNUM:      return "number";
    case TOKNEQ:      return "!=";
    case TOKMULASS:   return "*=";
    case TOKMODASS:   return "%=";
    case TOKLOR:      return "||";
    case TOKLONG:     return "long";
    case TOKLEQ:      return "<=";
    case TOKLAND:     return "&&";
    case TOKINT:      return "int";
    case TOKINC:      return "++";
    case TOKIF:       return "if";
    case TOKIDENT:    return "ident";
    case TOKGOTO:     return "goto";
    case TOKGEQ:      return ">=";
    case TOKFOR:      return "for";
    case TOKFLOAT:    return "float";
    case TOKEXTERN:   return "extern";
    case TOKEQL:      return "==";
    case TOKEOF:      return "end of file";
    case TOKENUM:     return "enum";
    case TOKELSE:     return "else";
    case TOKELLIPSIS: return "...";
    case TOKDOUBLE:   return "double";
    case TOKDO:       return "do";
    case TOKDIVASS:   return "/=";
    case TOKDEFAULT:  return "default";
    case TOKDEC:      return "--";
    case TOKCONTINUE: return "continue";
    case TOKCONST:    return "const";
    case TOKCHAR:     return "char";
    case TOKCASE:     return "case";
    case TOKBREAK:     return "break";
    case TOKAUTO:     return "auto";
    case TOKARROW:    return "->";
    case TOKANDASS:   return "&=";
    case TOKADDASS:   return "+=";
    case '[':         return "[";
    case '+':         return "+";
    case '%':         return "%";
    case '&':         return "&";
    case '*':         return "*";
    case '}':         return "}";
    case '{':         return "{";
    case ']':         return "]";
    case ')':         return ")";
    case '(':         return "(";
    case '.':         return ".";
    case '/':         return "/";
    case '!':         return "!";
    case ':':         return ":";
    case ';':         return ";";
    case '<':         return "<";
    case '>':         return ">";
    case ',':         return ",";
    case '-':         return "-";
    case '|':         return "|";
    case '=':         return "=";
    case '~':         return "~";
    case '^':         return "^";
    }
    errorf("unknown token %d\n", t);
	return 0;
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
			if (c == '"') { /* TODO: escape chars */ 
				return mktok(l, TOKSTR);
			}
		}
	} else if (c == '\'') {
	    accept(l, c);
	    for(;;) {
			c = nextc(l);
			if(c == EOF)
			    errorf("unclosed char\n"); /* TODO error pos */
			accept(l, c);
			if(c == '\\') {
			    c = nextc(l);
			    if(c == EOF || c == '\n')
			        errorf("EOF or newline in char literal");
			    accept(l, c);
			    continue;
			}
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
		  else if(c == '|' && c2 == '|') return mktok(l, TOKLOR);
		  else if(c == '&' && c2 == '&') return mktok(l, TOKLAND);
		else {
		    /* TODO, detect invalid operators */
			ungetch(l, c2);
			return mktok(l, c);
		}
		errorf("internal error\n");
	}
}
