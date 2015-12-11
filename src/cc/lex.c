#include <u.h>
#include <mem/mem.h>
#include <ds/ds.h>
#include "cc.h"

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
	INCLUDEHEADER,
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

Tok *
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
