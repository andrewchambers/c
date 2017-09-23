#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "util.h"
#include "cpp.h"
#include "ctypes.h"
#include "ir.h"
#include "cc.h"

static char  *newbblabel();
static char  *newdatalabel();
static CTy   *declspecs(int *);
static CTy   *ptag(void);
static CTy   *pstruct(int);
static CTy   *penum(void);
static CTy   *typename(void);
static CTy   *declarator(CTy *, char **, Node **);
static CTy   *directdeclarator(CTy *, char **);
static CTy   *declaratortail(CTy *);
static void   funcbody(void);
static void   block(void);
static void   stmt(void);
static void   pif (void);
static void   pfor(void);
static void   dowhile(void);
static void   pwhile(void);
static void   block(void);
static void   preturn(void);
static void   pswitch(void);
static void   pdefault(void);
static void   pcase(void);
static void   pcontinue(void);
static void   pbreak(void);
static void   stmt(void);
static void   exprstmt(void);
static Const *constexpr(void);
static Node  *expr(void);
static Node  *assignexpr(void);
static Node  *condexpr(void);
static Node  *logorexpr(void);
static Node  *logandexpr(void);
static Node  *orexpr(void);
static Node  *xorexpr(void);
static Node  *andexpr(void);
static Node  *eqlexpr(void);
static Node  *relexpr(void);
static Node  *shiftexpr(void);
static Node  *addexpr(void);
static Node  *mulexpr(void);
static Node  *castexpr(void);
static Node  *unaryexpr(void);
static Node  *postexpr(void);
static Node  *primaryexpr(void);
static Node  *vastart(void);
static Node  *ipromote(Node *);
static Node  *declinit(CTy *);
static CTy   *usualarithconv(Node **, Node **);
static Node  *mkcast(SrcPos *, Node *, CTy *);
static Node  *mknode(int type, SrcPos *p);
static Const *foldexpr(Node *);
static char  *ctype2irtype(CTy *ty);
static IRVal  compileexpr(Node *n);
static IRVal  compilestr(Node *n);
static IRVal  compilecall(Node *n);
static IRVal  compilebinop(Node *n);
static IRVal  compileunop(Node *n);
static IRVal  compileidx(Node *n);
static IRVal  compilesel(Node *n);
static IRVal  compileincdec(Node *n);
static IRVal  compileident(Node *n);
static IRVal  compileaddr(Node *n);
static IRVal  compileload(IRVal v, CTy *t);
static IRVal  compileassign(Node *n);
static IRVal  compilecast(Node *n);
static void   compilestore(IRVal src, IRVal dest, CTy *t);
static void   compilesym(Sym *sym);
static void   outdata(Data *d);
static void   outfuncstart();
static void   outfuncend();


static Tok *tok;
static Tok *nexttok;

#define MAXSCOPES 1024
static int nscopes;
static Map *tags[MAXSCOPES];
static Map *syms[MAXSCOPES];

#define MAXLABELDEPTH 2048
static int     switchdepth;
static int     brkdepth;
static int     contdepth;
static char   *breaks[MAXLABELDEPTH];
static char   *conts[MAXLABELDEPTH];
static Switch *switches[MAXLABELDEPTH];

static Vec *tentativesyms;
static Vec *pendingdata;
static Sym *curfunc;

static Map  *gotolabels;
static Vec  *gotos;

static BasicBlock *preludebb;
static BasicBlock *entrybb;
static BasicBlock *currentbb;
static Vec        *basicblocks;

static int datalabelcount;
static int bblabelcount;
static int vregcount;

static FILE *outf;

void
setoutput(FILE *f)
{
	outf = f;
}

static void
out(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	if(vfprintf(outf, fmt, va) < 0)
		errorf("error printing\n");
	va_end(va);
}

static void
next(void)
{
	tok = nexttok;
	nexttok = pp();
}

static void
expect(Tokkind kind)
{
	if (tok->k != kind)
		errorposf(&tok->pos,"expected %s, got %s", 
			tokktostr(kind), tokktostr(tok->k));
	next();
}

static GotoLabel *
lookupgotolabel(char *name)
{
	GotoLabel *label;

	label = mapget(gotolabels, name);
	if (!label) {
		label = xmalloc(sizeof(GotoLabel));
		label->backendlabel = newbblabel();
		label->defined = 0;
		mapset(gotolabels, name, label);
	}
	return label;
}

static char *
newbblabel()
{
	char *s;
	int   n;

	n = snprintf(0, 0, ".L%d", bblabelcount);
	if(n < 0)
		panic("internal error");
	n += 1;
	s = xmalloc(n);
	if(snprintf(s, n, ".L%d", bblabelcount) < 0)
		panic("internal error");
	bblabelcount++;
	return s;
}

static char *
newdatalabel()
{
	char *s;
	int   n;

	n = snprintf(0, 0, ".D%d", datalabelcount);
	if(n < 0)
		panic("internal error");
	n += 1;
	s = xmalloc(n);
	if(snprintf(s, n, ".D%d", datalabelcount) < 0)
		panic("internal error");
	datalabelcount++;
	return s;
}

static void
setcurbb(BasicBlock *bb)
{
	currentbb = bb;
	vecappend(basicblocks, bb);
}

static IRVal
nextvreg(char *irtype)
{
	return (IRVal){.kind=IRVReg, .irtype=irtype, .v=vregcount++};
}

static IRVal
alloclocal(SrcPos *pos, CTy *ty)
{
	IRVal v;

	if (ty->incomplete)
		errorposf(pos, "cannot alloc a local of incomplete type");

	v = nextvreg("l");
	bbappend(preludebb, (Instruction){.op=Opalloca, .a=v, .b=(IRVal){.kind=IRConst, .irtype="l", .v=ty->size}});
	return v;
}

static void
pushswitch (Switch *s)
{
        switches[switchdepth] = s;
        switchdepth += 1;
}

static void
popswitch (void)
{
        switchdepth -= 1;
        if (switchdepth < 0)
                panic("internal error");
}

static Switch *
curswitch (void)
{
        if (switchdepth == 0)
                return 0;
        return switches[switchdepth - 1];
}

static void
pushcontbrk(char *lcont, char *lbreak)
{
        conts[contdepth] = lcont;
        breaks[brkdepth] = lbreak;
        brkdepth += 1;
        contdepth += 1;
}

static void
popcontbrk(void)
{
        brkdepth -= 1;
        contdepth -= 1;
        if (brkdepth < 0 || contdepth < 0)
                panic("internal error");
}

static char *
curcont()
{
        if (contdepth == 0)
                return 0;
        return conts[contdepth - 1];
}

static char *
curbrk()
{
        if (brkdepth == 0)
                return 0;
        return breaks[brkdepth - 1];
}

static void
pushbrk(char *lbreak)
{
        breaks[brkdepth] = lbreak;
        brkdepth += 1;
}

static void
popbrk(void)
{
	brkdepth -= 1;
	if(brkdepth < 0)
		panic("internal error");
}

static void
popscope(void)
{
        nscopes -= 1;
        if (nscopes < 0)
                errorf("bug: scope underflow\n");
        syms[nscopes] = 0;
        tags[nscopes] = 0;
}

static void
pushscope(void)
{
        syms[nscopes] = map();
        tags[nscopes] = map();
        nscopes += 1;
        if (nscopes > MAXSCOPES)
                errorf("scope depth exceeded maximum\n");
}

static int
isglobal(void)
{
        return nscopes == 1;
}

static int
define(Map *scope[], char *k, void *v)
{
	Map *m;
	
	m = scope[nscopes - 1];
	if (mapget(m, k))
		return 0;
	mapset(m, k, v);
	return 1; 
}

static void *
lookup(Map *scope[], char *k)
{
	int   i;
	void *v;
	
	i = nscopes;
	while (i--) {
		v = mapget(scope[i], k);
		if (v)
			return v;
	}
	return 0;
}

/* TODO: proper efficient set for tentative syms */
static void
removetentativesym(Sym *sym)
{
	int i;
	Vec *newv;
	Sym *s;

	newv = vec();
	for (i = 0; i < tentativesyms->len; i++) {
		s = vecget(tentativesyms, i);
		if (s == sym)
			continue;
		vecappend(newv, s);
	}
	tentativesyms = newv;
}

static void
addtentativesym(Sym *sym)
{
	int i;
	Sym *s;

	for (i = 0; i < tentativesyms->len; i++) {
		s = vecget(tentativesyms, i);
		if (s == sym)
			return;
	}
	vecappend(tentativesyms, sym);
}

static Sym *
defineenum(SrcPos *p, char *name, CTy *type, int64 v)
{
	Sym *sym;

	sym = xmalloc(sizeof(Sym));
	sym->pos = p;
	sym->name = name;
	sym->type = type;
	sym->k = SYMENUM;
	sym->Enum.v = v;
	if (!define(syms, name, sym))
		errorposf(p, "redefinition of %s", name);
	return sym;
}

static Sym *
definesym(SrcPos *p, int sclass, char *name, CTy *type, Node *n)
{
	Sym *sym;

	if (sclass == SCAUTO || n != 0)
		if (type->incomplete)
			errorposf(p, "cannot use incomplete type in this context");
	if (sclass == SCAUTO && isglobal())
		errorposf(p, "defining local symbol in global scope");
	sym = mapget(syms[nscopes - 1], name);
	if (sym) {
		switch (sym->k) {
		case SYMTYPE:
			if (sclass != SCTYPEDEF || !sametype(sym->type, type))
				errorposf(p, "incompatible redefinition of typedef %s", name);
			break;
		case SYMGLOBAL:
			if (sym->Global.sclass == SCEXTERN && sclass == SCGLOBAL)
				sym->Global.sclass = SCGLOBAL;
			if (sym->Global.sclass != sclass)
				errorposf(p, "redefinition of %s with differing storage class", name);
			if (sym->init && n)
				errorposf(p, "%s already initialized", name);
			if (!sym->init && n) {
				sym->init = n;
				if (!isfunc(sym->type))
					compilesym(sym);
				removetentativesym(sym);
			}
			break;
		default:
			errorposf(p, "redefinition of %s", name);
		}
		return sym;
	}
	
	sym = xmalloc(sizeof(Sym));
	sym->pos = p;
	sym->name = name;
	sym->type = type;
	sym->init = n;
	
	switch (sclass) {
	case SCAUTO:
		sym->k = SYMLOCAL;
		sym->Local.addr = alloclocal(p, sym->type);
		break;
	case SCTYPEDEF:
		sym->k = SYMTYPE;
		break;
	case SCEXTERN:
		sym->k = SYMGLOBAL;
		sym->Global.label = name;
		sym->Global.sclass = SCEXTERN;
		break;
	case SCGLOBAL:
		sym->k = SYMGLOBAL;
		sym->Global.label = name;
		sym->Global.sclass = SCGLOBAL;
		break;
	case SCSTATIC:
		sym->k = SYMGLOBAL;
		sym->Global.label = newbblabel();
		sym->Global.sclass = SCSTATIC;
		break;
	default:
		panic("internal error");
	}
	if (!isfunc(sym->type)) {
		if (sym->k == SYMGLOBAL) {
			if (sym->init || sym->Global.sclass == SCEXTERN)
				compilesym(sym);
			else
				addtentativesym(sym);
		}
	}
	if (!define(syms, name, sym))
		panic("internal error");
	
	return sym;
}

static void
penddata(char *label, CTy *ty, Node *init, int isglobal)
{
	Data *d;

	d = xmalloc(sizeof(Data));
	d->label = label;
	d->type = ty;
	d->init = init;
	d->isglobal = isglobal;
	vecappend(pendingdata, d);
}

static void
params(CTy *fty)
{
	int     sclass;
	CTy    *t;
	char   *name;
	SrcPos *pos;

	fty->Func.isvararg = 0;
	if (tok->k == ')')
		return;
	if (tok->k == TOKVOID && nexttok->k == ')') {
		next();
		return;
	}
	for (;;) {
		pos = &tok->pos;
		t = declspecs(&sclass);
		t = declarator(t, &name, 0);
		if (sclass != SCNONE)
			errorposf(pos, "storage class not allowed in parameter decl");
		vecappend(fty->Func.params, newnamety(name, t));
		if (tok->k != ',')
			break;
		next();
		if (tok->k == TOKELLIPSIS) {
			fty->Func.isvararg = 1;
			next();
			break;
		}
	}
}

static void
decl()
{
	Node   *init;
	char   *name;
	CTy    *type, *basety;
	SrcPos *pos;
	Sym    *sym;
	Vec    *syms;
	int     sclass;

	pos = &tok->pos;
	syms  = vec();
	basety = declspecs(&sclass);
	while (tok->k != ';' && tok->k != TOKEOF) {
		type = declarator(basety, &name, &init);
		switch (sclass){
		case SCNONE:
			if (isglobal()) {
				sclass = SCGLOBAL;
			} else {
				sclass = SCAUTO;
			}
			break;
		case SCTYPEDEF:
			if (init)
				errorposf(pos, "typedef cannot have an initializer");
			break;
		}
		if (!name)
			errorposf(pos, "decl needs to specify a name");
		sym = definesym(pos, sclass, name, type, init);
		vecappend(syms, sym);
		if (isglobal() && tok->k == '{') {
			if (init)
				errorposf(pos, "function declaration has an initializer");
			if (type->t != CFUNC)
				errorposf(pos, "expected a function");
			curfunc = sym;
			outfuncstart();
			funcbody();
			outfuncend();
			definesym(pos, sclass, name, type, (Node*)1);
			curfunc = 0;
			return;
		}
		if (tok->k == ',')
			next();
		else
			break;
	}
	expect(';');

}

static int
issclasstok(Tok *t) {
	switch (t->k) {
	case TOKEXTERN:
	case TOKSTATIC:
	case TOKREGISTER:
	case TOKTYPEDEF:
	case TOKAUTO:
		return 1;
	default:
		return 0;
	}
}

static CTy *
declspecs(int *sclass)
{
	CTy    *t;
	SrcPos *pos;
	Sym    *sym;
	int     bits;

	enum {
		BITCHAR = 1<<0,
		BITSHORT = 1<<1,
		BITINT = 1<<2,
		BITLONG = 1<<3,
		BITLONGLONG = 1<<4,
		BITSIGNED = 1<<5,
		BITUNSIGNED = 1<<6,
		BITFLOAT = 1<<7,
		BITDOUBLE = 1<<8,
		BITENUM = 1<<9,
		BITSTRUCT = 1<<10,
		BITVOID = 1<<11,
		BITIDENT = 1<<12,
	};

	t = 0;
	bits = 0;
	pos = &tok->pos;
	*sclass = SCNONE;

	for(;;) {
		if (issclasstok(tok)) {
			if (*sclass != SCNONE)
				errorposf(pos, "multiple storage classes in declaration specifiers.");
			switch (tok->k) {
			case TOKEXTERN:
				*sclass = SCEXTERN;
				break;
			case TOKSTATIC:
				*sclass = SCSTATIC;
				break;
			case TOKREGISTER:
				*sclass = SCREGISTER;
				break;
			case TOKAUTO:
				*sclass = SCAUTO;
				break;
			case TOKTYPEDEF:
				*sclass = SCTYPEDEF;
				break;
			default:
				panic("internal error");
			}
			next();
			continue;
		}
		switch (tok->k) {
		case TOKCONST:
		case TOKVOLATILE:
			next();
			break;
		case TOKSTRUCT:
		case TOKUNION:
			if (bits)
				goto err;
			bits |= BITSTRUCT;
			t = ptag();
			goto done;
		case TOKENUM:
			if (bits)
				goto err;
			bits |= BITENUM;
			t = ptag();
			goto done;
		case TOKVOID:
			if (bits&BITVOID)
				goto err;
			bits |= BITVOID;
			next();
			goto done;
		case TOKCHAR:
			if (bits&BITCHAR)
				goto err;
			bits |= BITCHAR;
			next();
			break;
		case TOKSHORT:
			if (bits&BITSHORT)
				goto err;
			bits |= BITSHORT;
			next();
			break;
		case TOKINT:
			if (bits&BITINT)
				goto err;
			bits |= BITINT;
			next();
			break;
		case TOKLONG:
			if (bits&BITLONGLONG)
				goto err;
			if (bits&BITLONG) {
				bits &= ~BITLONG;
				bits |= BITLONGLONG;
			} else {
				bits |= BITLONG;
			}
			next();
			break;
		case TOKFLOAT:
			if (bits&BITFLOAT)
				goto err;
			bits |= BITFLOAT;
			next();
			break;
		case TOKDOUBLE:
			if (bits&BITDOUBLE)
				goto err;
			bits |= BITDOUBLE;
			next();
			break;
		case TOKSIGNED:
			if (bits&BITSIGNED)
				goto err;
			bits |= BITSIGNED;
			next();
			break;
		case TOKUNSIGNED:
			if (bits&BITUNSIGNED)
				goto err;
			bits |= BITUNSIGNED;
			next();
			break;
		case TOKIDENT:
			sym = lookup(syms, tok->v);
			if (sym && sym->k == SYMTYPE)
				t = sym->type;
			if (t && !bits) {
				bits |= BITIDENT;
				next();
				goto done;
			}
			/* fallthrough */
		default:
			goto done;
		}
	}
	done:
	switch (bits){
	case BITFLOAT:
		return cfloat;
	case BITDOUBLE:
		return cdouble;
	case BITLONG|BITDOUBLE:
		return cldouble;
	case BITSIGNED|BITCHAR:
	case BITCHAR:
		return cchar;
	case BITUNSIGNED|BITCHAR:
		return cuchar;
	case BITSIGNED|BITSHORT|BITINT:
	case BITSHORT|BITINT:
	case BITSHORT:
		return cshort;
	case BITUNSIGNED|BITSHORT|BITINT:
	case BITUNSIGNED|BITSHORT:
		return cushort;
	case BITSIGNED|BITINT:
	case BITSIGNED:
	case BITINT:
	case 0:
		return cint;
	case BITUNSIGNED|BITINT:
	case BITUNSIGNED:
		return cuint;
	case BITSIGNED|BITLONG|BITINT:
	case BITSIGNED|BITLONG:
	case BITLONG|BITINT:
	case BITLONG:
		return clong;
	case BITUNSIGNED|BITLONG|BITINT:
	case BITUNSIGNED|BITLONG:
		return culong;
	case BITSIGNED|BITLONGLONG|BITINT:
	case BITSIGNED|BITLONGLONG:
	case BITLONGLONG|BITINT:
	case BITLONGLONG:
		return cllong;
	case BITUNSIGNED|BITLONGLONG|BITINT:
	case BITUNSIGNED|BITLONGLONG:
		return cullong;
	case BITVOID:
		t = cvoid;
		return t;
	case BITENUM:
	case BITSTRUCT:
	case BITIDENT:
		return t;
	default:
		goto err;
	}
	err:
	errorposf(pos, "invalid declaration specifiers");
	return 0;
}

/* Declarator is what introduces names into the program. */
static CTy *
declarator(CTy *basety, char **name, Node **init) 
{
	CTy *t;

	while (tok->k == TOKCONST || tok->k == TOKVOLATILE)
		next();
	switch (tok->k) {
	case '*':
		next();
		basety = mkptr(basety);
		t = declarator(basety, name, init);
		return t;
	default:
		t = directdeclarator(basety, name);
		if (tok->k == '=') {
			if (!init)
				errorposf(&tok->pos, "unexpected initializer");
			next();
			*init = declinit(t);
		} else {
			if (init)
				*init = 0;
		}
		return t; 
	}

}

static CTy *
directdeclarator(CTy *basety, char **name) 
{
	CTy *ty, *stub;

	*name = 0;
	switch (tok->k) {
	case '(':
		expect('(');
		stub = xmalloc(sizeof(CTy));
		*stub = *basety;
		ty = declarator(stub, name, 0);
		expect(')');
		*stub = *declaratortail(basety);
		return ty;
	case TOKIDENT:
		if (name)
			*name = tok->v;
		next();
		return declaratortail(basety);
	default:
		if (!name)
			errorposf(&tok->pos, "expected ident or ( but got %s", tokktostr(tok->k));
		return declaratortail(basety);
	}
	errorf("unreachable");
	return 0;
}

static CTy *
declaratortail(CTy *basety)
{
	SrcPos *p;
	CTy    *t, *newt;
	Const  *c;
	
	t = basety;
	for(;;) {
		c = 0;
		switch (tok->k) {
		case '[':
			newt = newtype(CARR);
			newt->Arr.subty = t;
			newt->Arr.dim = -1;
			next();
			if (tok->k != ']') {
				p = &tok->pos;
				/* XXX check for int type? */
				c = constexpr();
				if (c->p)
					errorposf(p, "pointer derived constant in array size");
				newt->Arr.dim = c->v;
				newt->size = newt->Arr.dim * newt->Arr.subty->size;
			}
			newt->align = newt->Arr.subty->align;
			expect(']');
			t = newt;
			break;
		case '(':
			newt = newtype(CFUNC);
			newt->Func.rtype = basety;
			newt->Func.params = vec();
			next();
			params(newt);
			if (tok->k != ')')
				errorposf(&tok->pos, "expected valid parameter or )");
			next();
			t = newt;
			break;
		default:
			return t;
		}
	}
}

static CTy *
ptag()
{
	SrcPos *pos;
	char   *name;
	int     tkind;
	CTy    *namety, *bodyty;

	pos = &tok->pos;
	namety = 0;
	bodyty = 0;
	name = 0;
	switch (tok->k) {
	case TOKUNION:
	case TOKSTRUCT:
	case TOKENUM:
		tkind = tok->k;
		next();
		break;
	default:
		errorposf(pos, "expected a tag");
	}
	if (tok->k == TOKIDENT) {
		name = tok->v;
		next();
		namety = lookup(tags, name);
		if (namety) {
			switch (tkind) {
			case TOKUNION:
			case TOKSTRUCT:
				if (namety->t != CSTRUCT)
					errorposf(pos, "struct/union accessed by enum tag");
				if (namety->Struct.isunion != (tkind == TOKUNION))
					errorposf(pos, "struct/union accessed by wrong tag type");
				break;
			case TOKENUM:
				if (namety->t != CENUM)
					errorposf(pos, "enum tag accessed by struct or union");
				break;
			default:
				panic("internal error");
			}
		} else {
			switch (tkind) {
			case TOKUNION:
				namety = newtype(CSTRUCT);
				namety->Struct.isunion = 1;
				namety->incomplete = 1;
				break;
			case TOKSTRUCT:
				namety = newtype(CSTRUCT);
				namety->incomplete = 1;
				break;
			case TOKENUM:
				namety = newtype(CENUM);
				namety->incomplete = 1;
				break;
			default:
				panic("unreachable");
			}
			mapset(tags[nscopes - 1], name, namety);
		}
	}
	if (tok->k == '{' || !name) {
		switch (tkind) {
		case TOKUNION:
			bodyty = pstruct(1);
			break;
		case TOKSTRUCT:
			bodyty = pstruct(0);
			break;
		case TOKENUM:
			bodyty = penum();
			break;
		default:
			panic("unreachable");
		}
	}
	if (!name) {
		if (!bodyty)
			panic("internal error");
		return bodyty;
	}
	if (bodyty) {
		namety = mapget(tags[nscopes - 1], name);
		if (!namety) {
			mapset(tags[nscopes - 1], name, bodyty);
			return bodyty;
		}
		if (!namety->incomplete)
			errorposf(pos, "redefinition of tag %s", name);
		*namety = *bodyty;
		return namety;
	}
	return namety;
}

static CTy *
pstruct(int isunion)
{
	SrcPos *startpos, *p;
	CTy    *strct;
	char   *name;
	int     sclass;
	CTy    *t, *basety;

	strct = newtype(CSTRUCT);
	strct->align = 1; /* XXX zero sized structs? */
	strct->Struct.isunion = isunion;
	strct->Struct.exports = vec();
	strct->Struct.members = vec();
	
	startpos = &tok->pos;
	expect('{');
	while (tok->k != '}') {
		basety = declspecs(&sclass);
		for(;;) {
			p = &tok->pos;
			t = declarator(basety, &name, 0);
			if (tok->k == ':') {
				next();
				constexpr();
			}
			if (t->incomplete)
				errorposf(p, "incomplete type inside struct/union");
			addtostruct(strct, name, t);
			if (tok->k != ',')
				break;
			next();
		}
		expect(';');
	}
	expect('}');
	finalizestruct(startpos, strct);
	return strct;
}

static CTy *
penum()
{
	SrcPos *p;
	char   *name;
	CTy    *t;
	Const  *c;
	Sym    *s;
	int64   v;

	v = 0;
	t = newtype(CENUM);
	/* TODO: backend specific? */
	t->size = 4;
	t->align = 4;
	t->Enum.members = vec();
	expect('{');
	for(;;) {
		if (tok->k == '}')
			break;
		p = &tok->pos;
		name = tok->v;
		expect(TOKIDENT);
		if (tok->k == '=') {
			next();
			c = constexpr();
			if (c->p)
				errorposf(p, "pointer derived constant in enum");
			v = c->v;
		}
		s = defineenum(p, name, t, v);
		vecappend(t->Enum.members, s);
		if (tok->k != ',')
			break;
		next();
		v += 1;
	}
	expect('}');
	
	return t;
}


static void
funcbody()
{
	NameTy *nt;
	Sym    *sym;
	Goto   *go;
	int     i;

	vregcount = 0;
	bblabelcount = 0;
	basicblocks = vec();
	preludebb = mkbasicblock(newbblabel());
	entrybb = mkbasicblock(newbblabel());
	setcurbb(preludebb);
	setcurbb(entrybb);

	pushscope();
	
	gotolabels = map();
	gotos = vec();

	for (i = 0; i < curfunc->type->Func.params->len; i++) {
		nt = vecget(curfunc->type->Func.params, i);
		if (nt->name) {
			sym = definesym(curfunc->pos, SCAUTO, nt->name, nt->type, 0);
			sym->Local.isparam = 1;
			sym->Local.paramidx = i;
			compilestore((IRVal){.kind=IRNamedVReg, .irtype=ctype2irtype(nt->type), .label=nt->name}, sym->Local.addr, nt->type);
		}
	}

	block();
	popscope();

	for (i = 0 ; i < gotos->len ; i++) {
		go = vecget(gotos, i);
		if (!go->label->defined)
			errorposf(&go->pos, "goto target not defined");
	}

	bbterminate(preludebb, (Terminator){.op=Opjmp, .label1=bbgetlabel(entrybb)});
}


static void
pif(void)
{
	SrcPos *p;
	Node   *e;
	IRVal   cond;
	BasicBlock *iftruebb;
	BasicBlock *iffalsebb;
	BasicBlock *donebb;
	
	p = &tok->pos;
	expect(TOKIF);
	expect('(');
	e = expr();
	cond = compileexpr(e);
	expect(')');

	iftruebb = mkbasicblock(newbblabel());
	iffalsebb = mkbasicblock(newbblabel());
	donebb = mkbasicblock(newbblabel());

	bbterminate(currentbb, (Terminator){.op=Opcond, .v=cond, .label1=bbgetlabel(iftruebb), .label2=bbgetlabel(iffalsebb)});
	setcurbb(iftruebb);
	stmt();
	bbterminate(currentbb, (Terminator){.op=Opjmp, .label1=bbgetlabel(donebb)});
	setcurbb(iffalsebb);

	if (tok->k == TOKELSE) {
		expect(TOKELSE);
		stmt();
	}
	bbterminate(currentbb, (Terminator){.op=Opjmp, .label1=bbgetlabel(donebb)});
	setcurbb(donebb);
}

static void
pfor(void)
{
	Node   *i, *c, *s;
	BasicBlock *condbb, *stepbb, *bodybb, *stopbb;
	IRVal cond;

	condbb = mkbasicblock(newbblabel());
	stepbb = mkbasicblock(newbblabel());
	bodybb = mkbasicblock(newbblabel());
	stopbb = mkbasicblock(newbblabel());
	
	i = 0;
	c = 0;
	s = 0;
	
	expect(TOKFOR);
	expect('(');
	if (tok->k == ';') {
		next();
	} else {
		i = expr();
		expect(';');
	}
	if (tok->k == ';') {
		next();
	} else {
		c = expr();
		expect(';');
	}
	if (tok->k != ')')
		s = expr();
	expect(')');
	
	if (i)
		compileexpr(i);
	bbterminate(currentbb, (Terminator){.op=Opjmp, .label1=bbgetlabel(condbb)});
	setcurbb(condbb);
	if (c)
		cond = compileexpr(c);
	else
		cond = (IRVal){.kind=IRConst, .v=1};
	bbterminate(currentbb, (Terminator){.op=Opcond, .v=cond, .label1=bbgetlabel(bodybb), .label2=bbgetlabel(stopbb)});
	setcurbb(bodybb);
	pushcontbrk(bbgetlabel(stepbb), bbgetlabel(stopbb));
	stmt();
	popcontbrk();

	bbterminate(currentbb, (Terminator){.op=Opjmp, .label1=bbgetlabel(stepbb)});
	setcurbb(stepbb);
	if(s)
		compileexpr(s);
	
	bbterminate(currentbb, (Terminator){.op=Opjmp, .label1=bbgetlabel(condbb)});
	setcurbb(stopbb);
}

static void
pwhile(void)
{
	IRVal cond;
	BasicBlock *condbb, *bodybb, *stopbb;

	condbb = mkbasicblock(newbblabel());
	stopbb = mkbasicblock(newbblabel());
	bodybb = mkbasicblock(newbblabel());
	
	bbterminate(currentbb, (Terminator){.op=Opjmp, .label1=bbgetlabel(condbb)});
	setcurbb(condbb);

	expect(TOKWHILE);
	expect('(');
	cond = compileexpr(expr());
	expect(')');

	bbterminate(currentbb, (Terminator){.op=Opcond, .v=cond, .label1=bbgetlabel(bodybb), .label2=bbgetlabel(stopbb)});
	setcurbb(bodybb);

	pushcontbrk(bbgetlabel(condbb), bbgetlabel(stopbb));
	stmt();
	popcontbrk();

	bbterminate(currentbb, (Terminator){.op=Opjmp, .label1=bbgetlabel(condbb)});
	setcurbb(stopbb);
}

static void
dowhile(void)
{
	IRVal cond;
	BasicBlock *condbb, *bodybb, *stopbb;

	condbb = mkbasicblock(newbblabel());
	stopbb = mkbasicblock(newbblabel());
	bodybb = mkbasicblock(newbblabel());

	expect(TOKDO);
	pushcontbrk(bbgetlabel(condbb), bbgetlabel(stopbb));
	bbterminate(currentbb, (Terminator){.op=Opjmp, .label1=bbgetlabel(bodybb)});
	setcurbb(bodybb);
	stmt();
	popcontbrk();
	bbterminate(currentbb, (Terminator){.op=Opjmp, .label1=bbgetlabel(condbb)});
	setcurbb(condbb);
	expect(TOKWHILE);
	expect('(');
	cond = compileexpr(expr());
	expect(')');
	expect(';');
	bbterminate(currentbb, (Terminator){.op=Opcond, .v=cond, .label1=bbgetlabel(bodybb), .label2=bbgetlabel(stopbb)});
	setcurbb(stopbb);
}

static void
pgoto()
{
	Goto *go;

	go = xmalloc(sizeof(Goto));
	go->pos = tok->pos;
	expect(TOKGOTO);
	go->label = lookupgotolabel(tok->v);
	expect(TOKIDENT);
	expect(';');
	vecappend(gotos, go);
	bbterminate(currentbb, (Terminator){.op=Opjmp, .label1=go->label->backendlabel});
}

static int
istypename(char *n)
{
	Sym *sym;

	sym = lookup(syms, n);
	if (sym && sym->k == SYMTYPE)
		return 1;
	return 0;
}

static int
istypestart(Tok *t)
{
	switch(t->k) {
	case TOKENUM:
	case TOKSTRUCT:
	case TOKUNION:
	case TOKVOID:
	case TOKCHAR:
	case TOKSHORT:
	case TOKINT:
	case TOKLONG:
	case TOKSIGNED:
	case TOKUNSIGNED:
		return 1;
	case TOKIDENT:	
		return istypename(t->v);
	default:
		return 0;
	}
}

static int
isdeclstart(Tok *t)
{
	if (istypestart(t))
		return 1;
	switch (tok->k) {
	case TOKEXTERN:
	case TOKREGISTER:
	case TOKSTATIC:
	case TOKAUTO:
	case TOKCONST:
	case TOKVOLATILE:
		return 1;
	case TOKIDENT:
		return istypename(t->v);
	default:
		return 0;
	}
}

static void
declorstmt()
{
	if (isdeclstart(tok))
		decl();
	else
		stmt();
}

static void
stmt(void)
{
	Tok  *t;
	GotoLabel *label;

	if (tok->k == TOKIDENT && nexttok->k == ':') {
		t = tok;
		label = lookupgotolabel(t->v);
		next();
		next();
		if (label->defined)
			errorposf(&t->pos, "redefinition of label %s", t->v);
		label->defined = 1;
		bbterminate(currentbb, (Terminator){.op=Opjmp, .label1=label->backendlabel});
		setcurbb(mkbasicblock(newbblabel()));
		vecappend(currentbb->labels, label->backendlabel);
		return;
	}
	switch (tok->k) {
	case TOKIF:
		pif ();
		return;
	case TOKFOR:
		pfor();
		return;
	case TOKWHILE:
		pwhile();
		return;
	case TOKDO:
		dowhile();
		return;
	case TOKRETURN:
		preturn();
		return;
	case TOKSWITCH:
		pswitch();
		return;
	case TOKCASE:
		pcase();
		return;
	case TOKDEFAULT:
		pdefault();
		return;
	case TOKBREAK:
		pbreak();
		return;
	case TOKCONTINUE:
		pcontinue();
		return;
	case TOKGOTO:
		pgoto();
		return;
	case '{':
		block();
		return;
	default:
		exprstmt();
		return;
	}
}

static int
compareinits(const void *lvoid, const void *rvoid)
{
	const InitMember *l, *r;
	
	l = *(void**)lvoid;
	r = *(void**)rvoid;
	if (l->offset < r->offset)
		return -1;
	if (l->offset > r->offset)
		return 1;
	return 0;
}

static void
checkinitoverlap(Node *n)
{
	InitMember *init, *nextinit;
	int i;

	qsort(n->Init.inits->d, n->Init.inits->len, sizeof(void*), compareinits);
	for(i = 0; i < n->Init.inits->len - 1; i++) {
		init = vecget(n->Init.inits, i);
		nextinit = vecget(n->Init.inits, i + 1);
		if (nextinit->offset < init->offset + init->n->type->size)
			errorposf(&init->n->pos, "fields in init overlaps with another field");
	}
}

static Node *
declarrayinit(CTy *t)
{
	InitMember *initmemb;
	Node   *n, *subinit;
	CTy    *subty;
	SrcPos *initpos, *selpos;
	Const  *arrayidx;
	int     i;
	int     idx;
	int     largestidx;
	
	subty = t->Arr.subty;
	initpos = &tok->pos;
	n = mknode(NINIT, initpos);
	n->type = t;
	n->Init.inits = vec();
	idx = 0;
	largestidx = 0;
	expect('{');
	for(;;) {
		if (tok->k == '}')
			break;
		if (tok->k == '[') {
			selpos = &tok->pos;
			expect('[');
			arrayidx = constexpr();
			expect(']');
			expect('=');
			if (arrayidx->p != 0)
				errorposf(selpos, "pointer derived constants not allowed in initializer selector");
			if (arrayidx->v < 0)
				errorposf(selpos, "negative initializer index not allowed");
			idx = arrayidx->v;
			if (largestidx < idx)
				largestidx = idx;
		}
		subinit = declinit(subty);
		/* Flatten nested inits */
		if (subinit->t == NINIT) {
			for(i = 0; i < subinit->Init.inits->len; i++) {
				initmemb = vecget(subinit->Init.inits, i);
				initmemb->offset = subty->size * idx + initmemb->offset;
				vecappend(n->Init.inits, initmemb);
			}
		} else {
			initmemb = xmalloc(sizeof(InitMember));
			initmemb->offset = subty->size * idx;
			initmemb->n = subinit;
			vecappend(n->Init.inits, initmemb);
		}
		idx += 1;
		if (largestidx < idx)
			largestidx = idx;
		if (tok->k != ',')
			break;
		next();
	}
	checkinitoverlap(n);
	expect('}');
	if (t->Arr.dim == -1)
		t->Arr.dim = largestidx;
	if (largestidx != t->Arr.dim)
		errorposf(initpos, "array initializer wrong size for type");
	return n;
}

static Node *
declstructinit(CTy *t)
{
	StructMember *structmember;
	InitMember   *initmemb;
	StructIter   it;
	Node         *n, *subinit;
	CTy          *subty;
	SrcPos       *initpos, *selpos;
	char         *selname;
	int          i, offset, neednext;
	
	initpos = &tok->pos;
	if (t->incomplete)
		errorposf(initpos, "cannot initialize an incomplete struct/union");
		
	n = mknode(NINIT, initpos);
	n->type = t;
	n->Init.inits = vec();
	
	initstructiter(&it, t);
	expect('{');
	neednext = 0;
	for(;;) {
		if (tok->k == '}')
			break;
		if (tok->k == '.') {
			neednext = 0;
			selpos = &tok->pos;
			expect('.');
			selname = tok->v;
			expect(TOKIDENT);
			expect('=');
			if (!getstructiter(&it, t, selname))
				errorposf(selpos, "struct has no member '%s'", selname);
		}
		if (neednext) {
			if (!structnext(&it))
				errorposf(&tok->pos, "end of struct already reached");
		}
		structwalk(&it, &structmember, &offset);
		if (!structmember)
			errorposf(initpos, "too many elements in struct initializer");
		subty = structmember->type;
		subinit = declinit(subty);
		/* Flatten nested inits */
		if (subinit->t == NINIT) {
			for(i = 0; i < subinit->Init.inits->len; i++) {
				initmemb = vecget(subinit->Init.inits, i);
				initmemb->offset += offset;
				vecappend(n->Init.inits, initmemb);
			}
		} else {
			initmemb = xmalloc(sizeof(InitMember));
			initmemb->offset = offset;
			initmemb->n = subinit;
			vecappend(n->Init.inits, initmemb);
		}
		if (tok->k != ',')
			break;
		next();
		neednext = 1;
	}
	checkinitoverlap(n);
	expect('}');
	return n;
}

static Node *
declinit(CTy *t)
{
	if (isarray(t) && tok->k == '{') 
		return declarrayinit(t);
	if (isstruct(t)  && tok->k == '{') 
		return declstructinit(t);
	return assignexpr();
}

static void
exprstmt(void)
{
	Node *e;

	if (tok->k == ';') {
		next();
		return;
	}
	e = expr();
	compileexpr(e);
	expect(';');
}

static void
preturn(void)
{   
	Node *e;
	IRVal ret;

	expect(TOKRETURN);
	if (tok->k != ';') {
		e = expr();
		e = mkcast(&e->pos, e, curfunc->type->Func.rtype);
		ret = compileexpr(e);
		bbterminate(currentbb, (Terminator){.op=Opret, .v=ret});
	} else {
		bbterminate(currentbb, (Terminator){.op=Opret, .v=(IRVal){.kind=IRConst, .v=0}});
	}
	expect(';');
}

static void
pcontinue(void)
{
	SrcPos *pos;
	char   *l;
	
	pos = &tok->pos;
	l = curcont();
	if (!l)
		errorposf(pos, "continue without parent statement");
	expect(TOKCONTINUE);
	expect(';');
}

static void
pbreak(void)
{
	SrcPos *pos;
	char   *l;
	
	pos = &tok->pos;
	l = curbrk();
	if (!l)
		errorposf(pos, "break without parent statement");
	expect(TOKBREAK);
	expect(';');
}

static int
switchcasecmp(const void *a, const void *b)
{
	Case *ca, *cb;
	ca = (Case *)a;
	cb = (Case *)b;

	if (ca->v < cb->v)
		return -1;
	if (ca->v > cb->v)
		return 1;
	
	return 0;
}

static void
pswitch(void)
{
	int i;
	Node   *e;
	Switch *sw;
	Case   *cs1, *cs2;
	IRVal   swval;
	IRVal   cmpval;
	BasicBlock *startbb;
	BasicBlock *bodybb;
	BasicBlock *nextbb;
	BasicBlock *donebb;

	startbb = currentbb;
	bodybb = mkbasicblock(newbblabel());
	donebb = mkbasicblock(newbblabel());

	sw = xmalloc(sizeof(Switch));
	sw->pos = &tok->pos;
	sw->defaultlabel = 0;
	sw->cases = vec();

	expect(TOKSWITCH);
	expect('(');
	e = expr();
	expect(')');
	swval = compileexpr(e);

	pushbrk(bbgetlabel(donebb));
	pushswitch(sw);
	setcurbb(bodybb);
	stmt();
	popswitch();
	popbrk();

	vecsort(sw->cases, switchcasecmp);
	for (i = 0; i < sw->cases->len - 1; i++) {
		cs1 = vecget(sw->cases, i);
		cs2 = vecget(sw->cases, i + 1);
		if (cs1->v == cs2->v)
			errorposf(cs1->pos, "duplicate case");
	}


	bbterminate(currentbb, (Terminator){.op=Opjmp, .label1=bbgetlabel(donebb)});
	/* we are backtracking to write out the switch cases, which we didn't know before. */
	currentbb = startbb;

	for (i = 0; i < sw->cases->len; i++) {
		cs1 = vecget(sw->cases, i);
		nextbb = mkbasicblock(newbblabel());
		cmpval = nextvreg("w");
		/* XXX: compare op depends on type of something? */
		bbappend(currentbb, (Instruction){.op=Opceql, .a=cmpval, .b=swval, .c=(IRVal){.kind=IRConst, .v=cs1->v}});
		bbterminate(currentbb, (Terminator){.op=Opcond, .v=cmpval, .label1=cs1->label, .label2=bbgetlabel(nextbb)});
		setcurbb(nextbb);
	}
	bbterminate(currentbb, (Terminator){.op=Opjmp, .label1=bbgetlabel(donebb)});
	setcurbb(donebb);
}

static void
pdefault(void)
{
	SrcPos *pos;
	Switch *s;
	BasicBlock *defaultbb;

	pos = &tok->pos;
	expect(TOKDEFAULT);
	expect(':');
	s = curswitch();
	if (s->defaultlabel)
		errorposf(pos, "switch already has a default case");
	defaultbb = mkbasicblock(newbblabel());
	bbterminate(currentbb, (Terminator){.op=Opjmp, .label1=bbgetlabel(defaultbb)});
	setcurbb(defaultbb);
	stmt();
}

static void
pcase(void)
{
	SrcPos *pos;
	Const  *c;
	Switch *sw;
	Case   *cs;
	BasicBlock *casebb;

	pos = &tok->pos;
	sw = curswitch();
	expect(TOKCASE);
	c = constexpr();
	if (c->p)
		errorposf(pos, "case cannot have pointer derived constant");
	expect(':');

	casebb = mkbasicblock(newbblabel());
	bbterminate(currentbb, (Terminator){.op=Opjmp, .label1=bbgetlabel(casebb)});
	setcurbb(casebb);

	cs = xmalloc(sizeof(Case));
	cs->v = c->v;
	cs->label = bbgetlabel(casebb);
	cs->pos = pos;

	vecappend(sw->cases, cs);

	stmt();
}

static void
block(void)
{
	pushscope();
	expect('{');
	while(tok->k != '}' && tok->k != TOKEOF)
		declorstmt();
	expect('}');
	popscope();
}


static Node *
mknode(int type, SrcPos *p)
{
	Node *n;

	n = xmalloc(sizeof(Node));
	n->pos = *p;
	n->t = type;
	return n;
}

static Node *
mkincdec(SrcPos *p, int op, int post, Node *operand)
{
	Node *n;

	n = mknode(NINCDEC, p);
	n->Incdec.op = op;
	n->Incdec.post = post;
	n->Incdec.operand = operand;
	n->type = operand->type;
	return n;
}

static Node *
mkptradd(SrcPos *p, Node *ptr, Node *offset)
{
	Node *n;
	if (!isptr(ptr->type))
		panic("internal error");
	if (!isitype(offset->type))
		errorposf(&offset->pos, "addition with a pointer requires an integer type");
	n = mknode(NPTRADD, p);
	n->Ptradd.ptr = ptr;
	n->Ptradd.offset = offset;
	n->type = ptr->type;
	return n;
}

static Node *
mkbinop(SrcPos *p, int op, Node *l, Node *r)
{
	Node *n;
	CTy  *t;
	
	t = 0;
	if (op == '+') {
		if (isptr(l->type))
			return mkptradd(p, l, r);
		if (isptr(r->type))
			return mkptradd(p, r, l);
	}
	
	if (!isptr(l->type))
		l = ipromote(l);
	if (!isptr(r->type))
		r = ipromote(r);
	if (!isptr(l->type) && !isptr(r->type))
		t = usualarithconv(&l, &r);
	/* Other relationals? */
	if (op == TOKEQL || op == TOKNEQ)
		t = cint;
	n = mknode(NBINOP, p);
	n->Binop.op = op;
	n->Binop.l = l;
	n->Binop.r = r;
	n->type = t;
	return n;
}

static Node *
mkassign(SrcPos *p, int op, Node *l, Node *r)
{
	Node *n;
	CTy  *t;

	r = mkcast(p, r, l->type);
	t = l->type;
	n = mknode(NASSIGN, p);
	switch (op) {
	case '=':
		n->Assign.op = '=';
		break;
	case TOKADDASS:
		n->Assign.op = '+';
		break;
	case TOKSUBASS:
		n->Assign.op = '-';
		break;
	case TOKORASS:
		n->Assign.op = '|';
		break;
	case TOKANDASS:
		n->Assign.op = '&';
		break;
	case TOKMULASS:
		n->Assign.op = '*';
		break;
	default:
		panic("mkassign");
	}
	n->Assign.l = l;
	n->Assign.r = r;
	n->type = t;
	return n;
}

static Node *
mkunop(SrcPos *p, int op, Node *o)
{
	Node *n;
	
	n = mknode(NUNOP, p);
	n->Unop.op = op;
	switch (op) {
	case '&':
		n->type = mkptr(o->type);
		break;
	case '*':
		if (!isptr(o->type))
			errorposf(&o->pos, "cannot deref non pointer");
		n->type = o->type->Ptr.subty;
		break;
	default:
		if (isitype(o->type))
			o = ipromote(o);
		n->type = o->type;
		break;
	}
	n->Unop.operand = o;
	return n;
}

static Node *
mkcast(SrcPos *p, Node *o, CTy *to)
{
	Node *n;
	
	if (sametype(o->type, to))
		return o;
	n = mknode(NCAST, p);
	n->type = to;
	n->Cast.operand = o;
	return n;
}

static Node *
ipromote(Node *n)
{
	if (!isitype(n->type))
		errorposf(&n->pos, "internal error - ipromote expects itype got %d", n->type->t);
	switch (n->type->Prim.type) {
	case PRIMCHAR:
	case PRIMSHORT:
		if (n->type->Prim.issigned)
			return mkcast(&n->pos, n, cint);
		else
			return mkcast(&n->pos, n, cuint);
	}
	return n;
}

static CTy *
usualarithconv(Node **a, Node **b)
{   
	Node **large, **small;
	CTy   *t;

	if (!isarithtype((*a)->type) || !isarithtype((*b)->type))
		panic("internal error\n");
	if (convrank((*a)->type) < convrank((*b)->type)) {
		large = a;
		small = b;
	} else {
		large = b;
		small = a;
	}
	if (isftype((*large)->type)) {
		*small = mkcast(&(*small)->pos, *small, (*large)->type);
		return (*large)->type;
	}
	*large = ipromote(*large);
	*small = ipromote(*small);
	if (sametype((*large)->type, (*small)->type))
		return (*large)->type;
	if ((*large)->type->Prim.issigned == (*small)->type->Prim.issigned ) {
		*small = mkcast(&(*small)->pos, *small, (*large)->type);
		return (*large)->type;
	}
	if (!(*large)->type->Prim.issigned) {
		*small = mkcast(&(*small)->pos, *small, (*large)->type);
		return (*large)->type;
	}
	if ((*large)->type->Prim.issigned && canrepresent((*large)->type, (*small)->type)) {
		*small = mkcast(&(*small)->pos, *small, (*large)->type);
		return (*large)->type;
	}
	t = xmalloc(sizeof(CTy));
	*t = *((*large)->type);
	t->Prim.issigned = 0;
	*large = mkcast(&(*large)->pos, *large, t);
	*small = mkcast(&(*small)->pos, *small, t);
	return t;
}

static Node *
expr(void)
{
	SrcPos *p;
	Vec    *v;
	Node   *n, *last;

	p = &tok->pos;
	n = assignexpr();
	last = n;
	if (tok->k == ',') {
		v = vec();
		vecappend(v, n);
		while(tok->k == ',') {
			next();
			last = assignexpr();
			vecappend(v, last);
		}
		n = mknode(NCOMMA, p);
		n->Comma.exprs = v;
		n->type = last->type;
	}
	return n;
}

static int
isassignop(Tokkind k)
{
	switch (k) {
	case '=':
	case TOKADDASS:
	case TOKSUBASS:
	case TOKMULASS:
	case TOKDIVASS:
	case TOKMODASS:
	case TOKORASS:
	case TOKANDASS:
		return 1;
	default:
		return 0;
	}
}

static Node *
assignexpr(void)
{
	Tok  *t;
	Node *l, *r;

	l = condexpr();
	if (isassignop(tok->k)) {
		t = tok;
		next();
		r = assignexpr();
		l = mkassign(&t->pos, t->k, l, r);
	}
	return l;
}

static Const *
constexpr(void)
{
	Node  *n;
	Const *c;

	n = condexpr();
	c = foldexpr(n);
	if (!c)
		errorposf(&n->pos, "not a constant expression");

	return c;
}

/* Aka Ternary operator. */
static Node *
condexpr(void)
{
	Node *n, *c, *t, *f;

	c = logorexpr();
	if (tok->k != '?')
		return c;
	next();
	t = expr();
	expect(':');
	f = condexpr();
	n = mknode(NCOND, &tok->pos);
	n->Cond.cond = c;
	n->Cond.iftrue = t;
	n->Cond.iffalse = f;
	/* TODO: what are the limitations? */
	if (!sametype(t->type, f->type))
		errorposf(&n->pos, "both cases of ? must be same type.");
	n->type = t->type;
	return n;
}

static Node *
logorexpr(void)
{
	Tok  *t;
	Node *l, *r;

	l = logandexpr();
	while(tok->k == TOKLOR) {
		t = tok;
		next();
		r = logandexpr();
		l = mkbinop(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
logandexpr(void)
{
	Tok  *t;
	Node *l, *r;

	l = orexpr();
	while(tok->k == TOKLAND) {
		t = tok;
		next();
		r = orexpr();
		l = mkbinop(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
orexpr(void)
{
	Tok  *t;
	Node *l, *r;

	l = xorexpr();
	while(tok->k == '|') {
		t = tok;
		next();
		r = xorexpr();
		l = mkbinop(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
xorexpr(void)
{
	Tok  *t;
	Node *l, *r;

	l = andexpr();
	while(tok->k == '^') {
		t = tok;
		next();
		r = andexpr();
		l = mkbinop(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
andexpr(void) 
{
	Tok  *t;
	Node *l, *r;

	l = eqlexpr();
	while(tok->k == '&') {
		t = tok;
		next();
		r = eqlexpr();
		l = mkbinop(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
eqlexpr(void)
{
	Tok  *t;
	Node *l, *r;

	l = relexpr();
	while(tok->k == TOKEQL || tok->k == TOKNEQ) {
		t = tok;
		next();
		r = relexpr();
		l = mkbinop(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
relexpr(void)
{
	Tok  *t;
	Node *l, *r;

	l = shiftexpr();
	while(tok->k == '>' || tok->k == '<' 
		  || tok->k == TOKLEQ || tok->k == TOKGEQ) {
		t = tok;
		next();
		r = shiftexpr();
		l = mkbinop(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
shiftexpr(void)
{
	Tok  *t;
	Node *l, *r;

	l = addexpr();
	while(tok->k == TOKSHL || tok->k == TOKSHR) {
		t = tok;
		next();
		r = addexpr();
		l = mkbinop(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
addexpr(void)
{
	Tok  *t;
	Node *l, *r;

	l = mulexpr();
	while(tok->k == '+' || tok->k == '-'	) {
		t = tok;
		next();
		r = mulexpr();
		l = mkbinop(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
mulexpr(void)
{
	Tok  *t;
	Node *l, *r;
	
	l = castexpr();
	while(tok->k == '*' || tok->k == '/' || tok->k == '%') {
		t = tok;
		next();
		r = castexpr();
		l = mkbinop(&t->pos, t->k, l, r);
	}
	return l;
}

static Node *
castexpr(void)
{
	Tok  *t;
	Node *o;
	CTy  *ty;
	
	if (tok->k == '(' && istypestart(nexttok)) {
		t = tok;
		expect('(');
		ty = typename();
		expect(')');
		if (tok->k == '{') {
			o = declinit(ty);
			return o;
		}
		o = unaryexpr();
		return mkcast(&t->pos, o, ty);
	}
	return unaryexpr();
}

static CTy *
typename(void)
{
	int   sclass;
	CTy  *t;
	char *name;
	
	t = declspecs(&sclass);
	t = declarator(t, &name, 0);
	return t;
}

static Node *
unaryexpr(void)
{
	Tok  *t;
	CTy  *ty;
	Node *n;

	switch (tok->k) {
	case TOKINC:
	case TOKDEC:
		t = tok;
		next();
		n = unaryexpr();
		return mkincdec(&t->pos, t->k, 0, n);
	case '*':
	case '&':
	case '-':
	case '!':
	case '~':
		t = tok;
		next();
		return mkunop(&t->pos, t->k, castexpr());
	case TOKSIZEOF:
		n = mknode(NSIZEOF, &tok->pos);
		next();
		if (tok->k == '(' && istypestart(nexttok)) {
			expect('(');
			ty = typename();
			expect(')');
		} else {
			ty = unaryexpr()->type;
		}
		n->Sizeof.type = ty;
		n->type = cint;
		return n;
	default:
		;
	}
	return postexpr();
}

static Node *
call(Node *funclike)
{
	Node   *n;
	CTy    *fty;
	SrcPos *pos ;

	pos = &tok->pos;
	expect('(');
	n = mknode(NCALL, &tok->pos);
	n->Call.funclike = funclike;
	n->Call.args = vec();
	if (isfunc(funclike->type))
		fty = funclike->type;
	else if (isfuncptr(funclike->type))
		fty = funclike->type->Ptr.subty;
	else
		errorposf(pos, "cannot call non function");
	n->type = fty->Func.rtype;
	if (tok->k != ')') {
		for(;;) {
			vecappend(n->Call.args, assignexpr());
			if (tok->k != ',') {
				break;
			}
			next();
		}
	}
	expect(')');
	if (n->Call.args->len < fty->Func.params->len)
		errorposf(pos, "function called with too few args");
	if (n->Call.args->len > fty->Func.params->len && !fty->Func.isvararg)
		errorposf(pos, "function called with too many args");
	return n;
}

static Node *
postexpr(void)
{
	int   done;
	Tok  *t;
	Node *n1, *n2, *n3;

	n1 = primaryexpr();
	done = 0;
	while(!done) {
		switch (tok->k) {
		case '[':
			t = tok;
			next();
			n2 = expr();
			expect(']');
			n3 = mknode(NIDX, &t->pos);
			if (isptr(n1->type))
				n3->type = n1->type->Ptr.subty;
			else if (isarray(n1->type))
				n3->type = n1->type->Arr.subty;
			else
				errorposf(&t->pos, "can only index an array or pointer");
			n3->Idx.idx = n2;
			n3->Idx.operand = n1;
			n1 = n3;
			break;
		case '.':
			if (!isstruct(n1->type))
				errorposf(&tok->pos, "expected a struct");
			if (n1->type->incomplete)
				errorposf(&tok->pos, "selector on incomplete type");
			n2 = mknode(NSEL, &tok->pos);
			next();
			n2->Sel.name = tok->v;
			n2->Sel.operand = n1;
			n2->type = structtypefromname(n1->type, tok->v);
			if (!n2->type)
				errorposf(&tok->pos, "struct has no member %s", tok->v);
			expect(TOKIDENT);
			n1 = n2;
			break;
		case TOKARROW:
			if (!(isptr(n1->type) && isstruct(n1->type->Ptr.subty)))
				errorposf(&tok->pos, "expected a struct pointer");
			if (n1->type->Ptr.subty->incomplete)
				errorposf(&tok->pos, "selector on incomplete type");
			n2 = mknode(NSEL, &tok->pos);
			next();
			n2->Sel.name = tok->v;
			n2->Sel.operand = n1;
			n2->Sel.arrow = 1;
			n2->type = structtypefromname(n1->type->Ptr.subty, tok->v);
			if (!n2->type)
				errorposf(&tok->pos, "struct pointer has no member %s", tok->v);
			expect(TOKIDENT);
			n1 = n2;
			break;
		case '(':
			n1 = call(n1);
			break;
		case TOKINC:
			n1 = mkincdec(&tok->pos, TOKINC, 1, n1);
			next();
			break;
		case TOKDEC:
			n1 = mkincdec(&tok->pos, TOKDEC, 1, n1);
			next();
			break;
		default:
			done = 1;
		}
	}
	return n1;
}

static Node *
primaryexpr(void) 
{
	Sym  *sym;
	Node *n;
	
	switch (tok->k) {
	case TOKIDENT:
		if (strcmp(tok->v, "__builtin_va_start") == 0)
			return vastart();
		sym = lookup(syms, tok->v);
		if (!sym)
			errorposf(&tok->pos, "undefined symbol %s", tok->v);
		n = mknode(NIDENT, &tok->pos);
		n->Ident.sym = sym;
		n->type = sym->type;
		next();
		return n;
	case TOKNUM:
		n = mknode(NNUM, &tok->pos);
		n->Num.v = atoll(tok->v);
		n->type = cint;
		next();
		return n;
	case TOKCHARLIT:
		/* XXX it seems wrong to do this here, also table is better */
		n = mknode(NNUM, &tok->pos);
		if (strcmp(tok->v, "'\\n'") == 0) {
			n->Num.v = '\n';
		} else if (strcmp(tok->v, "'\\\\'") == 0) {
			n->Num.v = '\\';
		} else if (strcmp(tok->v, "'\\''") == 0) {
			n->Num.v = '\'';
		} else if (strcmp(tok->v, "'\\r'") == 0) {
			n->Num.v = '\r';
		} else if (strcmp(tok->v, "'\\t'") == 0) {
			n->Num.v = '\t';
		}  else if (tok->v[1] == '\\') {
			errorposf(&tok->pos, "unknown escape code");
		} else {
			n->Num.v = tok->v[1];
		}
		n->type = cint;
		next();
		return n;
	case TOKSTR:
		n = mknode(NSTR, &tok->pos);
		n->Str.v = tok->v;
		n->type = mkptr(cchar);
		next();
		return n;
	case '(':
		next();
		n = expr();
		expect(')');
		return n;
	default:
		errorposf(&tok->pos, "expected an ident, constant, string or (");
	}
	errorf("unreachable.");
	return 0;
}

static Node *
vastart()
{
	Node *n, *valist, *param;
	
	n = mknode(NBUILTIN, &tok->pos);
	expect(TOKIDENT);
	expect('(');
	valist = assignexpr();
	expect(',');
	param = assignexpr();
	expect(')');
	n->type = cvoid;
	n->Builtin.t = BUILTIN_VASTART;
	n->Builtin.Vastart.param = param;
	n->Builtin.Vastart.valist = valist;
	if (param->t != NIDENT)
		errorposf(&n->pos, "expected an identifer in va_start");
	if (param->Ident.sym->k != SYMLOCAL 
	   || !param->Ident.sym->Local.isparam)
		errorposf(&n->pos, "expected a parameter symbol in va_start");
	return n;
}

static Const *
mkconst(char *p, int64 v)
{
	Const *c;

	c = xmalloc(sizeof(Const));
	c->p = p;
	c->v = v;
	return c;
}

static Const *
foldbinop(Node *n)
{
	Const *l, *r;

	l = foldexpr(n->Binop.l);
	r = foldexpr(n->Binop.r);
	if(!l || !r)
		return 0;
	if(isitype(n->type)) {
		switch(n->Binop.op) {
		case '+':
			if(l->p && r->p)
				return 0;
			if(l->p)
				mkconst(l->p, l->v + r->v);
			if(r->p)
				mkconst(r->p, l->v + r->v);
			return mkconst(0, l->v + r->v);
		case '-':
			if(l->p || r->p) {
				if(l->p && !r->p)
					return mkconst(l->p, l->v - r->v);
				if(!l->p && r->p)
					return 0;
				if(strcmp(l->p, r->p) == 0)
					return mkconst(0, l->v - r->v);
				return 0;
			}
			return mkconst(0, l->v - r->v);
		case '*':
			if(l->p || r->p)
				return 0;
			return mkconst(0, l->v * r->v);
		case '/':
			if(l->p || r->p)
				return 0;
			if(r->v == 0)
				return 0;
			return mkconst(0, l->v / r->v);
		case TOKSHL:
			if(l->p || r->p)
				return 0;
			return mkconst(0, l->v << r->v);
		case '|':
			if(l->p || r->p)
				return 0;
			return mkconst(0, l->v | r->v);
		default:
			errorposf(&n->pos, "unimplemented fold binop %d", n->Binop.op);
		}
	}
	panic("unimplemented fold binop");
	return 0;
}

static Const *
foldaddr(Node *n)
{
	char *l;
	Sym  *sym;

	if(n->Unop.operand->t == NINIT) {
		l = newdatalabel();
		penddata(l, n->Unop.operand->type, n->Unop.operand, 0);
		return mkconst(l, 0);
	}
	if(n->Unop.operand->t != NIDENT)
		return 0;
	sym = n->Unop.operand->Ident.sym;
	if(sym->k != SYMGLOBAL)
		return 0;
	return mkconst(sym->Global.label, 0);
}

static Const *
foldunop(Node *n)
{
	Const *c;

	switch(n->Unop.op) {
	case '&':
		return foldaddr(n);
	case '-':
		c = foldexpr(n->Unop.operand);
		if(!c)
			return 0;
		if(c->p)
			return 0;
		return mkconst(0, -c->v);
	default:
		panic("unimplemented fold unop %d", n->Binop.op);
	}
	panic("unimplemented fold unop");
	return 0;
}

static Const *
foldcast(Node *n)
{
	if(!isitype(n->type))
		return 0;
	if(!isitype(n->Cast.operand->type))
		return 0;
	return foldexpr(n->Cast.operand);
}

static Const *
foldident(Node *n)
{
	Sym *sym;

	sym = n->Ident.sym;
	switch(sym->k) {
	case SYMENUM:
		return mkconst(0, sym->Enum.v);
	default:
		return 0;
	}
}

static Const *
foldexpr(Node *n)
{
	char *l;
	CTy  *ty;

	switch(n->t) {
	case NBINOP:
		return foldbinop(n);
	case NUNOP:
		return foldunop(n);
	case NNUM:
		return mkconst(0, n->Num.v);
	case NSIZEOF:
		return mkconst(0, n->Sizeof.type->size);
	case NCAST:
		return foldcast(n);
	case NIDENT:
		return foldident(n);
	case NSTR:
		l = newdatalabel();
		ty = newtype(CARR);
		ty->Arr.subty = cchar;
		/* XXX DIM? */
		penddata(l, ty, n, 0);
		return mkconst(l, 0);
	default:
		return 0;
	}
}

void
compile()
{
	int i;
	Sym *sym;

	switchdepth = 0;
	brkdepth = 0;
	contdepth = 0;
	nscopes = 0;
	pendingdata = vec();
	tentativesyms = vec();
	pushscope();
	next();
	next();

	while (tok->k != TOKEOF)
		decl();
	
	for(i = 0; i < tentativesyms->len; i++) {
		sym = vecget(tentativesyms, i);
		compilesym(sym);
	}

	out("\n");

	for(i = 0; i < pendingdata->len; i++)
		outdata(vecget(pendingdata, i));
	
	out("\n");

	out("# Compiled with %lld bytes allocated.\n", malloctotal);
	if (fflush(outf) != 0)
		panic("error flushing output");
}

static char *
ctype2irtype(CTy *ty)
{
	switch (ty->t) {
	case CVOID:
		return "";
	case CPRIM:
		switch (ty->Prim.type) {
		case PRIMCHAR:
			return "w";
		case PRIMSHORT:
			return "w";
		case PRIMINT:
			return "w";
		case PRIMLONG:
			return "l";
		case PRIMLLONG:
			return "l";
		default:
			panic("unhandled cprim");
		}
	case CPTR:
		return "l";
	default:
		panic("unhandled ctype");
	}
}

static IRVal
compileexpr(Node *n)
{
	switch(n->t){
	/*
	case NCOMMA:
		comma(n);
		break;
	*/
	case NCAST:
		return compilecast(n);
	case NSTR:
		return compilestr(n);
	case NSIZEOF:
		if (n->Sizeof.type->incomplete)
			errorposf(&n->pos, "cannot use incomplete type in sizeof");

		return (IRVal){.kind=IRConst, .irtype=ctype2irtype(n->type), .v=n->Sizeof.type->size};
	case NNUM:
		return (IRVal){.kind=IRConst, .irtype=ctype2irtype(n->type), .v=n->Num.v};
	case NIDENT:
		return compileident(n);
	case NUNOP:
		return compileunop(n);
	case NASSIGN:
		return compileassign(n);
	case NBINOP:
		return compilebinop(n);
	case NIDX:
		return compileidx(n);
	case NSEL:
		return compilesel(n);
	case NCALL:
		return compilecall(n);
	/*
	case NCOND:
		cond(n);
		break;
	case NPTRADD:
		ptradd(n);
		break;
	*/
	case NINCDEC:
		return compileincdec(n);
	/*
	case NBUILTIN:
		switch(n->Builtin.t) {
		case BUILTIN_VASTART:
			vastart(n);
			break;
		default:
			errorposf(&n->pos, "unimplemented builtin");
		}
		break;
	*/
	default:
		panic("unimplemented compileexpr for node at %s:%d:%d\n", n->pos.file, n->pos.line, n->pos.col);
	}
}

static IRVal
compilecast(Node *n)
{
	IRVal v1, v2;

	v1 = compileexpr(n->Cast.operand);
	v2 = nextvreg(ctype2irtype(n->type));

	// We are assuming the cast was validated during parsing.
	
	switch (n->Cast.operand->type->t) {
	case CPRIM:
		switch (n->Cast.operand->type->size) {
		case 8:
			switch (n->type->size) {
			case 8:
			case 4:
			case 2:
			case 1:
				return v1;
			default:
				break;
			}
			break;
		case 4:
			switch (n->type->size) {
			case 8:
				bbappend(currentbb, (Instruction){.op=issignedtype(n->type) ? Opextsw : Opextuw, .a=v2, .b=v1});
				return v2;
			case 4:
			case 2:
			case 1:
				return v1;
			default:
				break;
			}
			break;
		case 2:
			switch (n->type->size) {
			case 8:
			case 4:
				bbappend(currentbb, (Instruction){.op=issignedtype(n->type) ? Opextsh : Opextuh, .a=v2, .b=v1});
				return v2;
			case 2:
			case 1:
				return v1;
			default:
				break;
			}
			break;
		case 1:
			switch (n->type->size) {
			case 8:
			case 4:
			case 2:
				bbappend(currentbb, (Instruction){.op=issignedtype(n->type) ? Opextsb : Opextub, .a=v2, .b=v1});
				return v2;
			case 1:
				return v1;
			default:
				break;
			}
			break;
		default:
			panic("internal error");
		}
	case CPTR:
		return v1;
	default:
		;
	}

	panic("unimplemented compilecast for node at %s:%d:%d\n", n->pos.file, n->pos.line, n->pos.col);
}

static IRVal
compilestr(Node *n)
{
	char *l;

	l = newdatalabel();
	penddata(l, n->type, n, 0);
	return compileload((IRVal){.kind=IRLabel, .irtype="l", .label=l}, n->type);
}

static IRVal
compilecall(Node *n)
{
	int i;
	IRVal funclike;
	IRVal res;
	IRVal *irargs;
	Vec  *args;
	Node *arg;

	args = n->Call.args;
	irargs = xmalloc(sizeof(IRVal) * args->len);

	for (i = 0; i < args->len; i++) {
		arg = vecget(args, i);
		if(!isitype(arg->type) && !isptr(arg->type) && !isarray(arg->type) && !isfunc(arg->type))
			errorposf(&arg->pos, "unimplemented arg type\n");
		irargs[i] = compileexpr(arg);
	}

	funclike = compileexpr(n->Call.funclike);
	res = nextvreg(ctype2irtype(n->type));
	bbappend(currentbb, (Instruction){.op=Opcall, .a=res, .b=funclike, .args=irargs, .nargs=args->len});
	return res;
}

static IRVal
compilebinop(Node *n)
{
	int irop;
	IRVal  res, l, r;

	if (n->t != NBINOP)
		panic("compilebinop precondition failed");

	if (n->Binop.op == TOKLAND || n->Binop.op == TOKLOR) {
		panic("shortcircuit unimplemented");
		// shortcircuit(n);
		// return;
	}
	l = compileexpr(n->Binop.l);
	r = compileexpr(n->Binop.r);

	res = nextvreg(ctype2irtype(n->type));
	
	switch (n->Binop.op) {
	case '+':
		irop = Opadd;
		break;
	case '-':
		irop = Opsub;
		break;
	case '*':
		irop = Opmul;
		break;
	case '/':
		irop = Opdiv;
		break;
	case '%':
		irop = Oprem;
		break;
	case '|':
		irop = Opbor;
		break;
	case '&':
		irop = Opband;
		break;
	case '^':
		irop = Opbxor;
		break;
	case TOKEQL:
		irop = n->Binop.l->type->size == 4 ? Opceqw : Opceql;
		break;
	case TOKNEQ:
		irop = n->Binop.l->type->size == 4 ? Opcnew : Opcnel;
		break;
	case TOKGEQ:
		if (issignedtype(n->Binop.l->type))
			irop = n->Binop.l->type->size == 4 ? Opcsgew : Opcsgel;
		else
			irop = n->Binop.l->type->size == 4 ? Opcugew : Opcugel;
		break;
	case TOKLEQ:
		if (issignedtype(n->Binop.l->type))
			irop = n->Binop.l->type->size == 4 ? Opcslew : Opcslel;
		else
			irop = n->Binop.l->type->size == 4 ? Opculew : Opculel;
		break;
	case '>':
		if (issignedtype(n->Binop.l->type))
			irop = n->Binop.l->type->size == 4 ? Opcsgtw : Opcsgtl;
		else
			irop = n->Binop.l->type->size == 4 ? Opcugtw : Opcugtl;
		break;
	case '<':
		if (issignedtype(n->Binop.l->type))
			irop = n->Binop.l->type->size == 4 ? Opcsltw : Opcsltl;
		else
			irop = n->Binop.l->type->size == 4 ? Opcultw : Opcultl;
		break;
	case TOKSHR:
	case TOKSHL:
	default:
		errorf("unimplemented binop %d\n", n->Binop.op);
	}

	bbappend(currentbb, (Instruction){.op=irop, .a=res, .b=l, .c=r});
	return res;
}

static IRVal
compileunop(Node *n)
{
	IRVal v;

	if (n->t != NUNOP)
		panic("compileunop precondition failed");

	switch(n->Unop.op) {
	case '*':
		v = compileexpr(n->Unop.operand);
		return compileload(v, n->type);
	case '&':
		return compileaddr(n->Unop.operand);
	case '~':
		v = compileexpr(n->Unop.operand);
		panic("unimplemented unop !");
		break;
	case '!':
		v = compileexpr(n->Unop.operand);
		panic("unimplemented unop !");
		break;
	case '-':
		v = compileexpr(n->Unop.operand);
		panic("unimplemented unop -");
		break;
	default:
		errorf("unimplemented unop %d\n", n->Unop.op);
	}
}

static IRVal
compileidxaddr(Node *n)
{
	IRVal addr, idxv, idxv2, operand;

	idxv = compileexpr(n->Idx.idx);
	idxv2 = nextvreg("l");
	bbappend(currentbb, (Instruction){.op=Opmul, .a=idxv2, .b=idxv, .c=(IRVal){.kind=IRConst, .irtype="l", .v=n->type->size}});
	operand = compileexpr(n->Idx.operand);
	addr = nextvreg("l");
	bbappend(currentbb, (Instruction){.op=Opadd, .a=addr, .b=operand, .c=idxv2});
	return addr;
	
}

static IRVal
compileidx(Node *n)
{
	IRVal addr;

	addr = compileidxaddr(n);
	return compileload(addr, n->type);
}

static IRVal
compileseladdr(Node *n)
{
	CTy *t;
	IRVal v1, v2;
	int offset;

	v1 = compileexpr(n->Sel.operand);
	t = n->Sel.operand->type;
	if(isptr(t))
		offset = structoffsetfromname(t->Ptr.subty, n->Sel.name);
	else if(isstruct(t))
		offset = structoffsetfromname(t, n->Sel.name);
	else
		panic("internal error");
	if(offset < 0)
		panic("internal error");
	if(offset == 0)
		return v1;
	v2 = nextvreg("l");
	bbappend(currentbb, (Instruction){.op=Opadd, .a=v2, .b=v1, .c=(IRVal){.kind=IRConst, .irtype="l", .v=offset}});
	return v2;
}

static IRVal
compilesel(Node *n)
{
	IRVal addr;

	addr = compileseladdr(n);
	return compileload(addr, n->type);
}

static IRVal
compileincdec(Node *n)
{
	IRVal addr;
	IRVal val;
	IRVal newval;
	IRVal offset;

        if(!isitype(n->type) && !isptr(n->type))
                panic("unimplemented incdec");
        
	addr = compileaddr(n->Incdec.operand);
	val = compileload(addr, n->type);
	
	offset = (IRVal){.kind=IRConst, .irtype=val.irtype, .v=1};

	if(isptr(n->type))
		offset.v = n->type->Ptr.subty->size;
        
	if(n->Incdec.op == TOKDEC)
                offset.v = -offset.v;

	newval = nextvreg(val.irtype);        
	bbappend(currentbb, (Instruction){.op=Opadd, .a=newval, .b=val, .c=offset });

	compilestore(newval, addr, n->type);

	if(n->Incdec.post)
		return val;
	else
		return newval;
}

static IRVal
compileident(Node *n)
{
	Sym *sym;
	IRVal v;

	if (n->t != NIDENT)
		panic("compileident precondition failed");

	sym = n->Ident.sym;
	if (sym->k == SYMENUM)
		return (IRVal){.kind=IRConst, .irtype=ctype2irtype(n->type), .v=n->Num.v};

	v = compileaddr(n);
	if (sym->k == SYMLOCAL)
	if (sym->Local.isparam)
	if (isarray(sym->type))
		panic("array param unimplemented...");

	return compileload(v, n->type);
}

static IRVal
compileaddr(Node *n)
{
	Sym *sym;

	switch(n->t) {
	case NUNOP:
		if (n->Unop.op == '*')
			return compileexpr(n->Unop.operand);
		break;
	case NSEL:
		return compileseladdr(n);
	case NIDENT:
		sym = n->Ident.sym;
		switch (sym->k) {
		case SYMGLOBAL:
			return (IRVal){.kind=IRLabel, .irtype="l", .label=sym->Global.label};
		case SYMLOCAL:
			return sym->Local.addr;
		default:
			panic("internal error");
		}
		break;
	case NIDX:
		return compileidxaddr(n);
	default:
		;
	}
	errorposf(&n->pos, "expected an addressable value");
}

static IRVal
compileload(IRVal v, CTy *t)
{
	IRVal res;

	if (isstruct(t))
		return v;
	if (isarray(t))
		return v;
	if (isfunc(t))
		return v;

	if (isptr(t)) {
		res = nextvreg("l");
		bbappend(currentbb, (Instruction){.op=Opload, .a=res, .b=v});
		return res;
	}

	if (isitype(t)) {
		if (t->Prim.issigned) {
			switch (t->size) {
			case 8:
				res = nextvreg("l");
				bbappend(currentbb, (Instruction){.op=Opload, .a=res, .b=v});
				return res;
			case 4:
				res = nextvreg("w");
				bbappend(currentbb, (Instruction){.op=Opload, .a=res, .b=v});
				return res;
			case 2:
				res = nextvreg("w");
				bbappend(currentbb, (Instruction){.op=Oploadsh, .a=res, .b=v});
				return res;
			case 1:
				res = nextvreg("w");
				bbappend(currentbb, (Instruction){.op=Oploadsb, .a=res, .b=v});
				return res;
			default:
				panic("internal error compile load signed int\n");
			}
		} else {
			switch (t->size) {
			case 8:
				res = nextvreg("l");
				bbappend(currentbb, (Instruction){.op=Opload, .a=res, .b=v});
				return res;
			case 4:
				res = nextvreg("w");
				bbappend(currentbb, (Instruction){.op=Opload, .a=res, .b=v});
				return res;
			case 2:
				res = nextvreg("w");
				bbappend(currentbb, (Instruction){.op=Oploaduh, .a=res, .b=v});
				return res;
			case 1:
				res = nextvreg("w");
				bbappend(currentbb, (Instruction){.op=Oploadub, .a=res, .b=v});
				return res;
			default:
				panic("internal error compile load unsigned int\n");
			}
		}
	}
	panic("unimplemented load %d\n", t->t);
}

static void
compilestore(IRVal src, IRVal dest, CTy *t)
{
	if (isitype(t) || isptr(t)) {
		switch(t->size) {
		case 8:
			bbappend(currentbb, (Instruction){.op=Opstorel, .a=src, .b=dest});
			break;
		case 4:
			bbappend(currentbb, (Instruction){.op=Opstorew, .a=src, .b=dest});
			break;
		case 2:
			bbappend(currentbb, (Instruction){.op=Opstoreh, .a=src, .b=dest});
			break;
		case 1:
			bbappend(currentbb, (Instruction){.op=Opstoreb, .a=src, .b=dest});
			break;
		default:
			panic("internal error\n");
		}
		return;
	}

	if (isstruct(t)) {
		panic("unimplemented struct store");
		return;
	}
	errorf("unimplemented store\n");
}

static IRVal
compileassign(Node *n)
{
	Node *ln, *rn;
	IRVal l, r;

	ln = n->Assign.l;
	rn = n->Assign.r;

	if(n->Assign.op == '=') {
		r = compileexpr(rn);
		l = compileaddr(ln);
		if(!isptr(ln->type) && !isitype(ln->type) && !isstruct(ln->type))
			errorf("unimplemented assign\n");
		compilestore(r, l, ln->type);
		return compileload(l, ln->type);
	}
	/*
	addr(l);
	pushq("rax");
	load(l->type);
	pushq("rax");
	expr(r);
	outi("movq %%rax, %%rcx\n");
	popq("rax");
	obinop(op, n->type);
	outi("movq %%rax, %%rcx\n");
	popq("rax");
	store(l->type);
	outi("movq %%rcx, %%rax\n");
	*/
	panic("unimplemented compileassign");
}


static void
compilesym(Sym *sym)
{
	if (isfunc(sym->type))
		panic("compilesym precondition failed");

	switch(sym->k){
	case SYMGLOBAL:
		if (sym->Global.sclass == SCEXTERN)
			break;
		penddata(sym->Global.label, sym->type, sym->init, sym->Global.sclass == SCGLOBAL);
		break;
	case SYMLOCAL:
		break;
	case SYMENUM:
		break;
	case SYMTYPE:
		break;
	}
}

static void
outitypedata(Node *prim)
{
	Const *c;

	if(!isitype(prim->type) && !isptr(prim->type))
		panic("internal error %d");
	c = foldexpr(prim);
	if(!c)
		errorposf(&prim->pos, "not a constant expression");
	if(c->p) {
		switch(prim->type->size) {
		case 8:
			out("l %s + %d", c->p, c->v);
			return;
		case 4:
			out("w %s + %d", c->p, c->v);
			return;
		case 2:
			out("h %s + %d", c->p, c->v);
			return;
		case 1:
			out("b %s + %d", c->p, c->v);
			return;
		default:
			panic("unimplemented");
		}
	}

	switch(prim->type->size) {
	case 8:
		out("l %d", c->v);
		return;
	case 4:
		out("w %d", c->v);
		return;
	case 2:
		out("h %d", c->v);
		return;
	case 1:
		out("b %d", c->v);
		return;
	default:
		panic("unimplemented");
	}
	panic("internal error");
}

static void
outdata(Data *d)
{
	InitMember *initmemb;
	int   i, offset;
	char *l;
	
	if(!d->init) {
		out("data $%s = {z %d}", d->label, d->type->size);
		return;
	}
	if(d->isglobal)
		out("export ");
	out("data $%s = ", d->label);
	
	if(ischararray(d->type))
	if(d->init->t == NSTR) {
		out("%s\n", d->init->Str.v);
		return;
	}
	
	if(ischarptr(d->type))
	if(d->init->t == NSTR) {
		l = newdatalabel();
		out("{l $%s}\n", l);
		out("data $%s = {b %s, b 0}\n", l, d->init->Str.v);
		return;
	}

	if(isitype(d->type) || isptr(d->type)) {
		out("{");
		outitypedata(d->init);
		out("}\n");
		return;
	}

	if(isarray(d->type) || isstruct(d->type)) {
		if(d->init->t != NINIT)
			errorposf(&d->init->pos, "array/struct expects a '{' style initializer");
		offset = 0;
		out("{");
		for(i = 0; i < d->init->Init.inits->len ; i++) {
			initmemb = vecget(d->init->Init.inits, i);
			if(initmemb->offset != offset)
				out("z %d, ", initmemb->offset - offset);
			outitypedata(initmemb->n);
			out(", ");
			offset = initmemb->offset + initmemb->n->type->size;
		}
		if(offset < d->type->size)
			out("z %d", d->type->size - offset);
		out("}\n");
		return;
	}
	
	panic("internal error");
}

static void
outirval(IRVal *val)
{
	switch (val->kind) {
	case IRConst:
		out("%lld", val->v);
		break;
	case IRVReg:
		out("%%.v%d", val->v);
		break;
	case IRNamedVReg:
		out("%%%s", val->label);
		break;
	case IRLabel:
		out("$%s", val->label);
		break;
	default:
		panic("unhandled irval");
	}
}

static void
outterminator(Terminator *term)
{
	out("  ");
	switch (term->op) {
	case Opret:
		out("ret ");
		outirval(&term->v);
		out("\n");
		break;
	case Opjmp:
		out("jmp @%s\n", term->label1);
		break;
	case Opcond:
		out("jnz ");
		outirval(&term->v);
		out(", @%s, @%s\n", term->label1, term->label2);
		break;
	default:
		panic("unhandled terminator");
	}
}

static void
outalloca(Instruction *instr)
{
	if (instr->op != Opalloca)
		panic("internal error, not a valid alloca");

	out("  ");
	outirval(&instr->a);
	out(" =l alloc16 ");
	outirval(&instr->b);
	out("\n");
}

static void
outstore(Instruction *instr)
{
	char *opname;

	switch (instr->op) {
	case Opstorel:
		opname = "storel";
		break;
	case Opstorew:
		opname = "storew";
		break;
	case Opstoreh:
		opname = "storeh";
		break;
	case Opstoreb:
		opname = "storeb";
		break;
	default:
		panic("unhandled store instruction");
	}

	out("  ");
	out("%s ", opname);
	outirval(&instr->a);
	out(", ");
	outirval(&instr->b);
	out("\n");
}

static void
outload(Instruction *instr)
{
	char *opname;

	switch (instr->op) {
	case Opload:
		opname = "load";
		break;
	case Oploadsh:
		opname = "loadsh";
		break;
	case Oploadsb:
		opname = "loadsb";
		break;
	case Oploaduh:
		opname = "loaduh";
		break;
	case Oploadub:
		opname = "loadub";
		break;
	default:
		panic("unhandled load instruction");
	}

	out("  ");
	outirval(&instr->a);
	out(" =%s %s ", instr->a.irtype, opname);
	outirval(&instr->b);
	out("\n");
}

static void
outextend(Instruction *instr)
{
	char *opname;

	switch (instr->op) {
	case Opextsw:
		opname = "extsw";
		break;
	case Opextsh:
		opname = "extsh";
		break;
	case Opextsb:
		opname = "extsb";
		break;
	default:
		panic("unhandled extend instruction");
	}

	out("  ");
	outirval(&instr->a);
	out(" =%s %s ", instr->a.irtype, opname);
	outirval(&instr->b);
	out("\n");
}

static void
outcall(Instruction *instr)
{
	int i;

	out("  ");
	outirval(&instr->a);
	out(" =%s call ", instr->a.irtype);
	outirval(&instr->b);
	out("(");
	for (i = 0; i < instr->nargs; i++) {
		out("%s ", instr->args[i].irtype);
		outirval(&instr->args[i]);
		if (i != instr->nargs - 1)
			out(", ");
	}
	out(")\n");
}

static int
isstoreinstr(Instruction *instr)
{
	return instr->op >= Opstorel && instr->op <= Opstoreb;
}

static int
isloadinstr(Instruction *instr)
{
	return instr->op >= Opload && instr->op <= Oploadsb;
}

static int
isextendinstr(Instruction *instr)
{
	return instr->op >= Opextsw && instr->op <= Opextub;
}

static void
outinstruction(Instruction *instr)
{
	char *opname;

	if (instr->op == Opalloca) {
		outalloca(instr);
		return;
	}

	if (isstoreinstr(instr)) {
		outstore(instr);
		return;
	}

	if (isloadinstr(instr)) {
		outload(instr);
		return;
	}

	if (isextendinstr(instr)) {
		outextend(instr);
		return;
	}

	if (instr->op == Opcall) {
		outcall(instr);
		return;
	}

	switch (instr->op) {
	case Opadd:
		opname = "add";
		break;
	case Opsub:
		opname = "sub";
		break;
	case Opmul:
		opname = "mul";
		break;
	case Opdiv:
		opname = "div";
		break;
	case Oprem:
		opname = "rem";
		break;
	case Opband:
		opname = "and";
		break;
	case Opbor:
		opname = "or";
		break;
	case Opbxor:
		opname = "xor";
		break;
	case Opceqw:
		opname = "ceqw";
		break;
	case Opcnew:
		opname = "cnew";
		break;
	case Opceql:
		opname = "ceql";
		break;
	case Opcnel:
		opname = "cnel";
		break;
	case Opcsgew:
		opname = "csgew";
		break;
	case Opcslew:
		opname = "cslew";
		break;
	case Opcsgtw:
		opname = "csgtw";
		break;
	case Opcsltw:
		opname = "csltw";
		break;
	case Opcsgel:
		opname = "csgel";
		break;
	case Opcslel:
		opname = "cslel";
		break;
	case Opcsgtl:
		opname = "csgtl";
		break;
	case Opcsltl:
		opname = "csltl";
		break;
	case Opcugew:
		opname = "cugew";
		break;
	case Opculew:
		opname = "culew";
		break;
	case Opcugtw:
		opname = "cugtw";
		break;
	case Opcultw:
		opname = "cultw";
		break;
	case Opcugel:
		opname = "cugel";
		break;
	case Opculel:
		opname = "culel";
		break;
	case Opcugtl:
		opname = "cugtl";
		break;
	case Opcultl:
		opname = "cultl";
		break;
	default:
		panic("unhandled instruction");
	}

	out("  ");
	outirval(&instr->a);
	out(" =%s %s ", instr->a.irtype, opname);
	outirval(&instr->b);
	out(", ");
	outirval(&instr->c);
	out("\n");
}

static void
outfuncstart()
{
	int i;
	NameTy *namety;

	if (curfunc->k != SYMGLOBAL || !isfunc(curfunc->type))
		panic("emitfuncstart precondition failed");

	out("export\n");
	out("function %s $%s(", ctype2irtype(curfunc->type->Func.rtype), curfunc->name);

	for (i = 0; i < curfunc->type->Func.params->len; i++) {
		namety = vecget(curfunc->type->Func.params, i);
		out("%s %%%s%s", ctype2irtype(namety->type), namety->name, i == curfunc->type->Func.params->len - 1 ? "" : ", ");
	}
	out(") {\n");
}

static void
outbb(BasicBlock *bb)
{
	int i;

	for (i = 0; i < bb->labels->len; i++) {
		out(" @%s\n", vecget(bb->labels, i));
	}

	for (i = 0; i < bb->ninstructions; i++) {
		outinstruction(&bb->instructions[i]);
	}

	if (bb->terminated) {
		outterminator(&bb->terminator);
	} else {
		out("  ret\n");
	}
}

static void
outfuncend()
{
	int i;

	for (i = 0; i < basicblocks->len; i++) {
		outbb(vecget(basicblocks, i));
	}
	out("}\n\n");
}
