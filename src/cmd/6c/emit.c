#include <u.h>
#include <ds/ds.h>
#include <cc/cc.h>
#include <gc/gc.h>

static void expr(Node *);
static void stmt(Node *);
static void store(CTy *);

char    *intargregs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
int      stackoffset;

typedef enum {
	DSTR,
	DOBJ,
	DNUM,
	DZERO,
} DataKind;

typedef struct {
	DataKind k;
	char  *label;
	union {
		struct {
			char *v;
		} Str;
		struct {
			int   sz;
			int64 v;
		} Num;
		struct {
			Vec *vals;
		} Obj;
		struct {
			int sz;
		} Zero;
	};
} Data;

Vec *pendingdata;

static FILE *o;

void
emitinit(FILE *out)
{
	o = out;
	pendingdata = vec();
}

static void
penddata(Data *d)
{
	vecappend(pendingdata, d);
}

static void
out(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	if(vfprintf(o, fmt, va) < 0)
		errorf("Error printing\n");
	va_end(va);
}

static void
outi(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(o, "  ");
	if(vfprintf(o, fmt, va) < 0)
		errorf("Error printing\n");
	va_end(va);
}

static void
block(Node *n)
{
	Vec *v;
	int  i;

	v = n->Block.stmts;
	for(i = 0; i < v->len ; i++) {
		stmt(vecget(v, i));
	}
}

static void
calcslotoffsets(Node *f)
{
	int i, tsz, curoffset;
	StkSlot *s;

	curoffset = 0;
	for(i = 0; i < f->Func.stkslots->len; i++) {
		s = vecget(f->Func.stkslots, i);
		tsz = s->size;
		/* This allows us to just copy params which have had the size rounded up. */
		if(tsz < 8)
			tsz = 8;
		else if(tsz < 16)
			tsz = 16;
		curoffset += tsz;
		if(curoffset % s->align)
			curoffset = (curoffset - (curoffset % s->align) + s->align);
		s->offset = -curoffset;
	}
	if(curoffset % 16)
		curoffset = (curoffset - (curoffset % 16) + 16);
	f->Func.localsz = curoffset;
}

static void
pushq(char *reg)
{
	stackoffset += 8;
	outi("pushq %%%s\n", reg);
}

static void
popq(char *reg)
{
	stackoffset -= 8;
	outi("popq %%%s\n", reg);
}

static void
func(Node *f)
{
	Vec *v;
	Sym *sym;
	int  i;
	
	calcslotoffsets(f);
	out("\n");
	out(".text\n");
	out(".globl %s\n", f->Func.name);
	out("%s:\n", f->Func.name);
	pushq("rbp");
	outi("movq %%rsp, %%rbp\n");
	if (f->Func.localsz) {
		outi("sub $%d, %%rsp\n", f->Func.localsz);
		stackoffset += f->Func.localsz;
	}
	v = f->Func.params;
	for(i = 0; i < v->len; i++) {
		sym = vecget(v, i);
		if(!isitype(sym->type) && !isptr(sym->type))
			errorposf(&f->pos, "unimplemented arg type");
		if(i < 6) {
			outi("movq %%%s, %d(%%rbp)\n", intargregs[i], sym->Local.slot->offset);
		} else {
			outi("movq %d(%%rbp), %%rcx\n", 16 + 8 * (i - 6));
			outi("leaq %d(%%rbp), %%rax\n", sym->Local.slot->offset);
			store(sym->type);
		}
	}
	block(f->Func.body);
	outi("leave\n");
	outi("ret\n");
}


static void
call(Node *n)
{
	int i, nargs, nintargs, cleanup;
	Vec  *args;
	Node *arg;

	args = n->Call.args;
	i = nargs = args->len;
	/* Push args in reverse order */
	while(i-- != 0) {
		arg = vecget(args, i);
		if(!isitype(arg->type) && !isptr(arg->type))
			errorf("unimplemented arg type.");
		expr(arg);
		pushq("rax");
	}
	nintargs = nargs;
	if(nintargs > 6)
		nintargs = 6;
	for(i = 0; i < nintargs; i++)
		popq(intargregs[i]);
	expr(n->Call.funclike);
	outi("call *%%rax\n");
	cleanup = 8 * (nargs - nintargs);
	if(cleanup) {
		outi("add $%d, %%rsp\n", cleanup);
		stackoffset -= cleanup;
	}
}

static void
ereturn(Node *r)
{
	CTy *ty;
	
	ty = r->Return.expr->type;
	if(!isitype(ty) && !isptr(ty))
		errorposf(&r->pos, "unimplemented return type");
	expr(r->Return.expr);
	/* No need to cleanup with leave */
	outi("leave\n");
	outi("ret\n");
}


static void
load(CTy *t)
{
	if(isitype(t) || isptr(t)) {
		switch(t->size) {
		case 8:
			outi("movq (%%rax), %%rax\n");
			break;
		case 4:
			outi("movslq (%%rax), %%rax\n");
			break;
		case 2:
			outi("movswq (%%rax), %%rax\n");
			break;
		case 1:
			outi("movsbq (%%rax), %%rax\n");
			break;
		default:
			panic("internal error\n");
		}
		return;
	}
	if(isstruct(t)) {
		return;
	}
	if(isarray(t)) {
		return;
	}
	if(isfunc(t)) {
		return;
	}
	errorf("unimplemented load %d\n", t->t);
}

static void
store(CTy *t)
{
	if(isitype(t) || isptr(t)) {
		switch(t->size) {
		case 8:
			outi("movq %%rcx, (%%rax)\n");
			break;
		case 4:
			outi("movl %%ecx, (%%rax)\n");
			break;
		case 2:
			outi("movw %%cx, (%%rax)\n");
			break;
		case 1:
			outi("movb %%cl, (%%rax)\n");
			break;
		default:
			panic("internal error\n");
		}
		return;
	}
	if(isstruct(t)) {
		pushq("rdi");
		pushq("rsi");
		pushq("rcx");
		outi("movq %%rcx, %%rsi\n");
		outi("movq %%rax, %%rdi\n");
		outi("movq $%d, %%rcx\n", t->size);
		outi("rep movsb\n");
		popq("rcx");
		popq("rsi");
		popq("rdi");
		return;
	}
	errorf("unimplemented store\n");
}

static void
decl(Node *n)
{
	int  i;
	Sym *sym;

	for(i = 0; i < n->Decl.syms->len; i++) {
		sym = vecget(n->Decl.syms, i);
		emitsym(sym);
	}
}

static void
addr(Node *n)
{
	int sz;
	Sym *sym;
	StructMember *sm;
	
	switch(n->t) {
	case NUNOP:
		expr(n->Unop.operand);
		break;
	case NSEL:
		expr(n->Sel.operand);
		sm = getstructmember(n->Sel.operand->type, n->Sel.name);
		if(!sm)
			panic("internal error");
		outi("addq $%d, %%rax\n", sm->offset);
		break;
	case NIDENT:
		sym = n->Ident.sym;
		switch(sym->k) {
		case SYMGLOBAL:
			outi("leaq %s(%%rip), %%rax\n", sym->Global.label);
			break;
		case SYMLOCAL:
			outi("leaq %d(%%rbp), %%rax\n", sym->Local.slot->offset);
			break;
		default:
			panic("internal error");
		}
		break;
	case NIDX:
		expr(n->Idx.idx);
		sz = n->type->size;
		if(sz != 1) {
			outi("imul $%d, %%rax\n", sz);
		}
		pushq("rax");
		expr(n->Idx.operand);
		popq("rcx");
		outi("addq %%rcx, %%rax\n");
		break;
	default:
		errorf("unimplemented addr\n");
	}
}

static void
obinop(int op, CTy *t)
{
	char *lset;
	char *lafter;
	char *opc;
	
	if(!isitype(t))
		errorf("unimplemented binary operator type");
	switch(op) {
	case '+':
		outi("addq %%rcx, %%rax\n");
		break;
	case '-':
		outi("subq %%rcx, %%rax\n");
		break;
	case '*':
		outi("imul %%rcx, %%rax\n");
		break;
	case '/':
		outi("cqto\n");
		outi("idiv %%rcx\n");
		break;
	case '%':
		outi("cqto\n");
		outi("idiv %%rcx\n");
		outi("mov %%rdx, %%rax\n");
		break;
	case '|':
		outi("or %%rcx, %%rax\n");
		break;
	case '&':
		outi("and %%rcx, %%rax\n");
		break;
	case '^':
		outi("xor %%rcx, %%rax\n");
		break;
	case TOKSHR:
		outi("sar %%cl, %%rax\n");
		break;
	case TOKSHL:
		outi("sal %%cl, %%rax\n");
		break;
	case TOKEQL:
	case TOKNEQ:
	case TOKGEQ:
	case TOKLEQ:
	case '>':
	case '<':
		lset = newlabel();
		lafter = newlabel();
		switch(op) {
		case TOKEQL:
			opc = "jz";
			break;
		case TOKNEQ:
			opc = "jnz";
			break;
		case '<':
			opc = "jl";
			break;
		case '>':
			opc = "jg";
			break;
		case TOKGEQ:
			opc = "jge";
			break;
		case TOKLEQ:
			opc = "jle";
			break;
		}
		outi("cmp %%rcx, %%rax\n");
		outi("%s .%s\n", opc, lset);
		outi("movq $0, %%rax\n");
		outi("jmp .%s\n", lafter);
		out(".%s:\n", lset);
		outi("movq $1, %%rax\n");
		out(".%s:\n", lafter);
		break;
	default:
		errorf("unimplemented binop %d\n", op);
	}
}

static void
assign(Node *n)
{
	Node *l, *r;
	int op;

	op = n->Assign.op;
	l = n->Assign.l;
	r = n->Assign.r;
	if(op == '=') {
		expr(r);
		pushq("rax");
		addr(l);
		popq("rcx");
		if(!isptr(l->type) && !isitype(l->type) && !isstruct(l->type))
			errorf("unimplemented assign\n");
		store(l->type);
		outi("movq %%rcx, %%rax\n");
		return;
	}
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
}

static void
shortcircuit(Node *n)
{
	char *t, *f, *e;

	t = newlabel();
	f = newlabel();
	e = newlabel();

	expr(n->Binop.l);
	if(n->Binop.op == TOKLAND) {
		outi("testq %%rax, %%rax\n");
		outi("jz .%s\n", f);
	} else if(n->Binop.op == TOKLOR) {
		outi("testq %%rax, %%rax\n");
		outi("jnz .%s\n", t);
	} else {
		panic("internal error");
	}
	expr(n->Binop.r);
	if(n->Binop.op == TOKLAND) {
		outi("testq %%rax, %%rax\n");
		outi("jz .%s\n", f);
		outi("jmp .%s\n", t);
	} else if(n->Binop.op == TOKLOR) {
		outi("testq %%rax, %%rax\n");
		outi("jnz .%s\n", t);
		outi("jmp .%s\n", f);
	} else {
		panic("internal error");
	}
	out(".%s:\n", t);
	outi("mov $1, %%rax\n");
	outi("jmp .%s\n", e);
	out(".%s:\n", f);
	outi("xor %%rax, %%rax\n");
	outi("jmp .%s\n", e);
	out(".%s:\n", e);
}

static void
binop(Node *n)
{
	if(n->Binop.op == TOKLAND || n->Binop.op == TOKLOR) {
		shortcircuit(n);
		return;
	}
	expr(n->Binop.l);
	pushq("rax");
	expr(n->Binop.r);
	outi("movq %%rax, %%rcx\n");
	popq("rax");
	obinop(n->Binop.op, n->type);
}

static void
unop(Node *n)
{
	switch(n->Unop.op) {
	case '*':
		expr(n->Unop.operand);
		load(n->type);
		break;
	case '&':
		addr(n->Unop.operand);
		break;
	case '~':
		expr(n->Unop.operand);
		out("notq %%rax\n");
		break;
	case '!':
		expr(n->Unop.operand);
		outi("xorq %%rcx, %%rcx\n");
		outi("testq %%rax, %%rax\n");
		outi("setz %%cl\n");
		outi("movq %%rcx, %%rax\n");
		break;
	case '-':
		expr(n->Unop.operand);
		outi("neg %%rax\n");
		break;
	case TOKINC:
	default:
		errorf("unimplemented unop %d\n", n->Unop.op);
	}
}

static void
incdec(Node *n)
{
	if(!isitype(n->type) && !isptr(n->type))
		panic("unimplemented incdec");
	addr(n->Incdec.operand);
	pushq("rax");
	load(n->type);
	if(isptr(n->type)) {
		if(n->Incdec.op == TOKINC)
			outi("add $%d, %%rax\n", n->type->Ptr.subty->size);
		else
			outi("add $%d, %%rax\n", -n->type->Ptr.subty->size);
	} else {
		if(n->Incdec.op == TOKINC)
			outi("inc %%rax\n");
		else
			outi("dec %%rax\n");
	}
	outi("movq %%rax, %%rcx\n");
	popq("rax");
	store(n->type);
	outi("movq %%rcx, %%rax\n");
	if(n->Incdec.post == 1) {
		if(n->Incdec.op == TOKINC)
			outi("dec %%rax\n");
		else
			outi("inc %%rax\n");
	}
}

static void
ident(Node *n)
{
	Sym *sym;

	sym = n->Ident.sym;
	if(sym->k == SYMENUM) {
		outi("movq $%d, %%rax\n", sym->Enum.v);
		return;
	}
	addr(n);
	load(n->type);
}

static void
eif(Node *n)
{
	char *end;

	end = newlabel();
	expr(n->If.expr);
	outi("test %%rax, %%rax\n");
	outi("jz .%s\n", n->If.lelse);
	stmt(n->If.iftrue);
	outi("jmp .%s\n", end);
	out(".%s:\n", n->If.lelse);
	if(n->If.iffalse)
		stmt(n->If.iffalse);
	out(".%s:\n", end);
}

static void
efor(Node *n)
{
	if(n->For.init)
		expr(n->For.init);
	out(".%s:\n", n->For.lstart);
	if(n->For.cond) {
		expr(n->For.cond);
		outi("test %%rax, %%rax\n");
		outi("jz .%s\n", n->For.lend);
	}
	stmt(n->For.stmt);
	if(n->For.step)
		expr(n->For.step);
	outi("jmp .%s\n", n->For.lstart);
	out(".%s:\n", n->For.lend);
}

static void
ewhile(Node *n)
{
	out(".%s:\n", n->While.lstart);
	expr(n->While.expr);
	outi("test %%rax, %%rax\n");
	outi("jz .%s\n", n->While.lend);
	stmt(n->While.stmt);
	outi("jmp .%s\n", n->While.lstart);
	out(".%s:\n", n->While.lend);
}

static void
dowhile(Node *n)
{
	out(".%s:\n", n->DoWhile.lstart);
	stmt(n->DoWhile.stmt);
	out(".%s:\n", n->DoWhile.lcond);
	expr(n->DoWhile.expr);
	outi("test %%rax, %%rax\n");
	outi("jz .%s\n", n->DoWhile.lend);
	outi("jmp .%s\n", n->DoWhile.lstart);
	out(".%s:\n", n->DoWhile.lend);
}

static void
eswitch(Node *n)
{
	int   i;
	Node *c;

	expr(n->Switch.expr);
	for(i = 0; i < n->Switch.cases->len; i++) {
		c = vecget(n->Switch.cases, i);
		outi("mov $%lld, %%rcx\n", c->Case.cond);
		outi("cmp %%rax, %%rcx\n");
		outi("je .%s\n", c->Case.l);
	}
	if(n->Switch.ldefault) {
		outi("jmp .%s\n", n->Switch.ldefault);
	} else {
		outi("jmp .%s\n", n->Switch.lend);
	}
	stmt(n->Switch.stmt);
	out(".%s:\n", n->Switch.lend);
}

static void
cond(Node *n)
{
	char *lfalse, *lend;

	if(!isitype(n->type) && !isptr(n->type))
		panic("unimplemented emit cond");
	expr(n->Cond.cond);
	lfalse = newlabel();
	lend = newlabel();
	outi("test %%rax, %%rax\n");
	outi("jz .%s\n", lfalse);
	expr(n->Cond.iftrue);
	outi("jmp .%s\n", lend);
	out(".%s:\n", lfalse);
	expr(n->Cond.iffalse);
	out(".%s:\n", lend);
}

static void
cast(Node *n)
{
	CTy *from;
	CTy *to;
	
	expr(n->Cast.operand);
	from = n->Cast.operand->type;
	to = n->type;
	if(isptr(from) && isptr(to))
		return;
	if(isptr(to) && isitype(from))
		return;
	if(isptr(from) && isitype(to))
		return;
	if(isitype(from) && isitype(to))
		return;
	if(isfunc(from) && isptr(to))
		return;
	if(isarray(from) && isptr(to))
		return;
	errorf("unimplemented cast %d %d\n", from->t, to->t);
}

static void
sel(Node *n)
{
	CTy *t;
	int offset;

	expr(n->Sel.operand);
	t = n->Sel.operand->type;
	offset = getstructmember(t, n->Sel.name)->offset;
	if(offset != 0) {
		outi("add $%d, %%rax\n", offset);
	}
	load(n->type);
}

static void
idx(Node *n)
{
	int sz;

	expr(n->Idx.idx);
	sz = n->type->size;
	if(sz != 1) {
		outi("imul $%d, %%rax\n", sz);
	}
	outi("push %%rax\n");
	expr(n->Idx.operand);
	outi("pop %%rcx\n");
	outi("addq %%rcx, %%rax\n");
	load(n->type);
}

static void
comma(Node *n)
{
	int i;

	for(i = 0; i < n->Comma.exprs->len; i++) {
		expr(vecget(n->Comma.exprs, i));
	}
}

static void
str(Node *n)
{
	char *l;
	Data *d;

	l = newlabel();
	d = gcmalloc(sizeof(Data));
	d->label = l;
	d->k = DSTR;
	d->Str.v = n->Str.v;
	penddata(d);
	outi("leaq .%s(%%rip), %%rax\n", l);
}

static void
expr(Node *n)
{
	switch(n->t){
	case NCOMMA:
		comma(n);
		break;
	case NCAST:
		cast(n);
		break;
	case NSTR:
		str(n);
		break;
	case NSIZEOF:
		outi("movq $%lld, %%rax\n", n->Sizeof.type->size);
		break;
	case NNUM:
		outi("movq $%lld, %%rax\n", n->Num.v);
		break;
	case NIDENT:
		ident(n);
		break;
	case NUNOP:
		unop(n);
		break;
	case NASSIGN:
		assign(n);
		break;
	case NBINOP:
		binop(n);
		break;
	case NIDX:
		idx(n);
		break;
	case NSEL:
		sel(n);
		break;
	case NCOND:
		cond(n);
		break;
	case NCALL:
		call(n);
		break;
	case NINCDEC:
		incdec(n);
		break;
	default:
		errorf("unimplemented emit expr %d\n", n->t);
	}
}

static void
stmt(Node *n)
{
	switch(n->t){
	case NDECL:
		decl(n);
		out(".text\n");
		break;
	case NRETURN:
		ereturn(n);
		break;
	case NIF:
		eif(n);
		break;
	case NWHILE:
		ewhile(n);
		break;
	case NFOR:
		efor(n);
		break;
	case NDOWHILE:
		dowhile(n);
		break;
	case NBLOCK:
		block(n);
		break;
	case NSWITCH:
		eswitch(n);
		break;
	case NGOTO:
		outi("jmp .%s\n", n->Goto.l);
		break;
	case NCASE:
		out(".%s:\n", n->Case.l);
		stmt(n->Case.stmt);
		break;
	case NLABELED:
		out(".%s:\n", n->Labeled.l);
		stmt(n->Labeled.stmt);
		break;
	case NEXPRSTMT:
		if(n->ExprStmt.expr)
			expr(n->ExprStmt.expr);
		break;
	default:
		errorf("unimplemented emit stmt %d\n", n->t);
	}	
}

static void
emititype(Node *prim)
{
	if(!isitype(prim->type))
		panic("internal error %d");
	if(prim->t != NNUM)
		errorposf(&prim->pos, "invalid initializer for integer type");
	switch(prim->type->size) {
	case 8:
		out(".quad %d\n", prim->Num.v);
		return;
	case 4:
		out(".dword %d\n", prim->Num.v);
		return;
	default:
		panic("unimplemented");
	}
	panic("internal error");
}

static void
emitglobal(char *name, Node *init)
{
	InitMember *initmemb;
	int i;

	out(".data\n");
	out("_%s:\n", name);
	if(isitype(init->type)) {
		emititype(init);
		return;
	}
	if(isarray(init->type)) {
		for(i = 0; i < init->Init.inits->len ; i++) {
			initmemb = vecget(init->Init.inits, i);
			emititype(initmemb->n);
		}
		return;
	}
}


void
emitsym(Sym *sym)
{
	out("# emit sym %s\n", sym->name);
	switch(sym->k){
	case SYMGLOBAL:
		if(isfunc(sym->type)) {
			func(sym->init);
			break;
		}
		if(sym->init) {
			emitglobal(sym->name, sym->init);
			break;
		}
		out(".data\n");
		out(".comm %s, %d, %d\n", sym->name, sym->type->size, sym->type->align);
		break;
	case SYMLOCAL:
		if(sym->init) {
			expr(sym->init);
			pushq("rax");
			outi("leaq %d(%%rbp), %%rax\n", sym->Local.slot->offset);
			popq("rcx");
			if(!isptr(sym->type) && !isitype(sym->type) && !isstruct(sym->type))
				errorf("unimplemented init\n");
			store(sym->type);
		}
		break;
	case SYMENUM:
	case SYMTYPE:
		panic("internal error");
	}
	out("\n");
}

static void
data(Data *d)
{
	int i;

	if(d->label)
		out(".%s:\n", d->label);
	switch(d->k) {
	case DSTR:
		out(".string %s\n", d->Str.v);
		break;
	case DNUM:
		switch(d->Num.sz) {
		case 8:
			out(".qword");
			break;
		case 4:
			out(".dword");
			break;
		case 2:
			out(".word");
			break;
		case 1:
			out(".byte");
			break;
		default:
			panic("bad number size\n");
		}
		out("%lld\n", d->Num.v);
		break;
	case DOBJ:
		for(i = 0; i < d->Obj.vals->len; i++)
			data(vecget(d->Obj.vals, i));
		break;
	case DZERO:
	default:
		panic("Unknown data type");
	}
	out("\n");
}

void
emitend()
{
	int  i;
	
	out("# Emit Data\n");
	out(".data\n\n");
	for(i = 0; i < pendingdata->len; i++)
		data(vecget(pendingdata, i));
}

