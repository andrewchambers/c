#include <stdint.h>
#include <stdio.h>
#include "util.h"
#include "cpp.h"
#include "ctypes.h"
#include "cc.h"
#include "ir.h"

static CTy   *declspecs(int *);
static CTy   *ptag(void);
static CTy   *pstruct(int);
static CTy   *penum(void);
static CTy   *typename(void);
static CTy   *declarator(CTy *, char **, Node **);
static CTy   *directdeclarator(CTy *, char **);
static CTy   *declaratortail(CTy *);

static void   funcbody();
static void   block();
static Const *constexpr(void);


Tok *tok;
Tok *nexttok;

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


#define MAXSCOPES 1024
static int nscopes;
static Map *tags[MAXSCOPES];
static Map *syms[MAXSCOPES];

typedef struct Switch {

} Switch;

#define MAXLABELDEPTH 2048
static int     switchdepth;
static int     brkdepth;
static int     contdepth;
static char   *breaks[MAXLABELDEPTH];
static char   *conts[MAXLABELDEPTH];
static Switch *switches[MAXLABELDEPTH];

Sym  *curfunc;

/* source label -> backend label */
Map  *labels;

typedef struct Goto {
	SrcPos pos;
	char *label;
} Goto;

Vec  *gotos;
Vec  *tentativesyms;

static void
pushswitch(Switch *s)
{
        switches[switchdepth] = s;
        switchdepth += 1;
}

static void
popswitch(void)
{
        switchdepth -= 1;
        if (switchdepth < 0)
                panic("internal error");
}

static Switch *
curswitch(void)
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
					emitsym(sym);
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
		/* XXX This should be changed for new backend.
		sym->Local.slot = xmalloc(sizeof(StkSlot));
		sym->Local.slot->size = sym->type->size;
		sym->Local.slot->align = sym->type->align;
		vecappend(curfunc->Func.stkslots, sym->Local.slot);
		*/
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
		sym->Global.label = newlabel();
		sym->Global.sclass = SCSTATIC;
		break;
	default:
		panic("internal error");
	}
	if (!isfunc(sym->type)) {
		if (sym->k == SYMGLOBAL) {
			if (sym->init || sym->Global.sclass == SCEXTERN)
				emitsym(sym);
			else
				addtentativesym(sym);
		}
	}
	if (!define(syms, name, sym))
		panic("internal error");
	
	return sym;
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
		switch(sclass){
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
			emitfuncstart(sym);
			funcbody();
			emitfuncend();
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
	switch(t->k) {
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
			switch(tok->k) {
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
		switch(tok->k) {
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
	switch(bits){
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
	switch(tok->k) {
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
			*init = NULL; //XXX declinit(t);
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
	switch(tok->k) {
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
	switch(tok->k) {
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
			switch(tkind) {
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
			switch(tkind) {
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
		switch(tkind) {
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

	pushscope();
	
	labels = map();
	gotos = vec();

	for (i = 0; i < curfunc->type->Func.params->len; i++) {
		nt = vecget(curfunc->type->Func.params, i);
		if (nt->name) {
			sym = definesym(curfunc->pos, SCAUTO, nt->name, nt->type, 0);
			sym->Local.isparam = 1;
			sym->Local.paramidx = i;
		}
	}

	block();
	popscope();

	for (i = 0 ; i < gotos->len ; i++) {
		go = vecget(gotos, i);
		if (!mapget(labels, go->label))
			errorposf(&go->pos, "goto target not defined");
	}
}

static void
block()
{
	expect('{');
	pushscope();

	popscope();
	expect('}');
}

static Const *
constexpr(void)
{
	/*
	Node  *n;
	*/
	Const *c;

	c = xmalloc(sizeof(constexpr));
	c->p = 0;
	c->v = 0;

	/*
	n = condexpr();
	c = foldexpr(n);
	if (!c)
		errorposf(&n->pos, "not a constant expression");
	*/
	return c;
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
	tentativesyms = vec();
	pushscope();
	next();
	next();

	beginmodule();
	
	while (tok->k != TOKEOF)
		decl();
	
	for(i = 0; i < tentativesyms->len; i++) {
		sym = vecget(tentativesyms, i);
		emitsym(sym);
	}
	
	endmodule();
}
