#include <u.h>
#include <ds/ds.h>
#include <mem/mem.h>
#include "cc.h"

static int
align(int v, int a)
{
	if(v % a)
		return v + a - (v % a);
	return v;
}

CTy *
newtype(int type)
{
	CTy *t;

	t = xmalloc(sizeof(CTy));
	t->t = type;
	return t;
}

CTy *
mkptr(CTy *t)
{
	CTy *p;

	p = newtype(CPTR);
	p->Ptr.subty = t;
	p->size = 8;
	p->align = 8;
	return p;
}


int
convrank(CTy *t)
{
	if(t->t != CPRIM && t->t != CENUM)
		panic("internal error");
	if(t->t == CENUM)
		return 2;
	switch(t->Prim.type){
	case PRIMCHAR:
		return 0;
	case PRIMSHORT:
		return 1;
	case PRIMINT:
		return 2;
	case PRIMLONG:
		return 3;
	case PRIMLLONG:
		return 4;
	case PRIMFLOAT:
		return 5;
	case PRIMDOUBLE:
		return 6;
	case PRIMLDOUBLE:
		return 7;
	default:
		panic("internal error");
		return -1;
	}
}

int 
compatiblestruct(CTy *l, CTy *r)
{
	if(l || r)
		errorf("unimplemented compatiblestruct");
	/* TODO */
	return 0;
}

int 
sametype(CTy *l, CTy *r)
{
	int     i;
	NameTy *lnt, *rnt;
	StructMember *lsm, *rsm;

	if(l == r)
		return 1;
	switch(l->t) {
	case CENUM:
		if(r->t != CENUM)
			return 0;
		return 1;
	case CVOID:
		if(r->t != CVOID)
			return 0;
		return 1;
	case CPRIM:
		if(r->t != CPRIM)
			return 0;
		if(l->Prim.issigned != r->Prim.issigned)
			return 0;
		if(l->Prim.type != r->Prim.type)
			return 0;
		return 1;
	case CPTR:
		if(r->t != CPTR)
			return 0;
		return sametype(l->Ptr.subty, r->Ptr.subty);
	case CFUNC:
		if(r->t != CFUNC)
			return 0;
		if(!sametype(l->Func.rtype, r->Func.rtype))
			return 0;
		if(l->Func.isvararg != r->Func.isvararg)
			return 0;
		if(l->Func.params->len != r->Func.params->len)
			return 0;
		for(i = 0; i < l->Func.params->len; i++) {
			lnt = vecget(l->Func.params, i);
			rnt = vecget(r->Func.params, i);
			if(!sametype(lnt->type, rnt->type))
				return 0;
		}
		return 1;
	case CSTRUCT:
		if(r->t != CSTRUCT)
			return 0;
		if(l->incomplete || r->incomplete)
			return l->incomplete == r->incomplete;
		if(l->Struct.members->len != r->Struct.members->len)
			return 0;
		for(i = 0; i < l->Struct.members->len; i++) {
			lsm = vecget(l->Struct.members, i);
			rsm = vecget(r->Struct.members, i);
			if(!sametype(lsm->type, rsm->type))
				return 0;
		}
		return 1;
	case CARR:
		if(r->t != CARR)
			return 0;
		if(r->Arr.dim != l->Arr.dim)
			return 0;
		return 1;
	default:
		panic("unimplemented same type");
	}
	return 0;
}

int
isftype(CTy *t)
{
	if(t->t != CPRIM)
		return 0;
	switch(t->Prim.type){
	case PRIMFLOAT:
	case PRIMDOUBLE:
	case PRIMLDOUBLE:
		return 1;
	}
	return 0;
}

int
isvoid(CTy *t)
{
	return t->t == CVOID;
}

int
isitype(CTy *t)
{
	if(t->t == CENUM)
		return 1;
	if(t->t != CPRIM)
		return 0;
	switch(t->Prim.type){
	case PRIMCHAR:
	case PRIMSHORT:
	case PRIMINT:
	case PRIMLONG:
	case PRIMLLONG:
		return 1;
	}
	return 0;
}

int
isarithtype(CTy *t)
{
	return isftype(t) || isitype(t);
}

int
isptr(CTy *t)
{
	return t->t == CPTR;
}

int
ischarptr(CTy *t)
{
	if(!isptr(t))
		return 0;
	if(t->Ptr.subty->t != CPRIM)
		return 0;
	return t->Ptr.subty->Prim.type == PRIMCHAR;
}

int
ischararray(CTy *t)
{
	if(!isarray(t))
		return 0;
	if(t->Ptr.subty->t != CPRIM)
		return 0;
	return t->Ptr.subty->Prim.type == PRIMCHAR;
}

int
isfunc(CTy *t)
{
	return t->t == CFUNC;
}

int
isfuncptr(CTy *t)
{
	return isptr(t) && isfunc(t->Ptr.subty);
}

int
isstruct(CTy *t)
{
	return t->t == CSTRUCT;
}

int
isarray(CTy *t)
{
	return t->t == CARR;
}

int
structoffsetfromname(CTy *t, char *name)
{
	int offset;
	StructMember *sm;
	StructIter it;
	
	if(!getstructiter(&it, t, name))
		return -1;
	structwalk(&it, &sm, &offset);
	return offset;
}

CTy *
structtypefromname(CTy *t, char *name)
{
	int offset;
	StructMember *sm;
	StructIter it;
	
	if(!getstructiter(&it, t, name))
		return 0;
	structwalk(&it, &sm, &offset);
	return sm->type;
}

void
initstructiter(StructIter *it, CTy *t)
{
	if(!isstruct(t))
		panic("internal error");
	it->root = t;
	it->depth = 1;
	it->path[0] = 0;
}

int
getstructiter(StructIter *it, CTy *t, char *name)
{
	StructExport *export;
	ExportPath   *path;
	int i;
	
	it->root = t;	
	it->depth = 0;
	for(i = 0; i < t->Struct.exports->len; i++) {
		export = vecget(t->Struct.exports, i);
		if(strcmp(export->name, name) != 0)
			continue;
		path = export->path;
		while(path) {
			it->path[it->depth++] = path->idx;
			path = path->next;
		}
		return 1;
	}
	return 0;
}

static void
_structwalk(StructIter *it, CTy *cur, int depth, StructMember **smout, int *offout)
{
	*smout = vecget(cur->Struct.members, it->path[depth]);
	*offout = *offout + (*smout)->offset;
	if((depth + 1) == it->depth)
		return;
	_structwalk(it,  (*smout)->type, depth + 1, smout, offout);
}

void
structwalk(StructIter *it, StructMember **smout, int *offout)
{
	*smout = 0;
	*offout = 0;
	_structwalk(it, it->root, 0, smout, offout);
}

int
structnext(StructIter *it)
{
	StructMember *sm;
	CTy *curstruct;
	int offset;

	if(it->depth == 0)
		return 0;
	
	if(it->depth == 1) {
		curstruct = it->root;
		offset = 0;
	} else {
		it->depth--;
		structwalk(it, &sm, &offset);
		curstruct = sm->type;
		it->depth++;
	}
	if(curstruct->Struct.isunion) {
		if(curstruct == it->root)
			return 0;
		it->depth--;
		return structnext(it);
	}
	if(it->path[it->depth-1] + 1 < curstruct->Struct.members->len) {
		it->path[it->depth-1]++;
		while(1) {
			structwalk(it, &sm, &offset);
			if(!sm->name)
			if(isstruct(sm->type)) {
				if(!sm->type->Struct.members->len)
					return structnext(it);
				it->path[it->depth] = 0;
				it->depth++;
				continue;
			}
			break;
		}
		return 1;
	}
	it->depth -= 1;
	return structnext(it);
}

static StructMember *
newstructmember(char *name, int offset, CTy *membt)
{
	StructMember *sm;

	sm = xmalloc(sizeof(StructMember));
	sm->name = name;
	sm->type = membt;
	sm->offset = offset;
	return sm;
}

void
addtostruct(CTy *t, char *name, CTy *membt)
{
	vecappend(t->Struct.members, newstructmember(name, -1, membt));
}

void
finalizestruct(SrcPos *pos, CTy *t)
{
	StructMember *sm;
	int i, j, curoffset;
	StrSet *exportednames;
	StructExport *export, *subexport;
	
	/* calc alignment */
	for(i = 0; i < t->Struct.members->len; i++) {
		sm = vecget(t->Struct.members, i);
		t->align = align(t->align, sm->type->align);
	}
	/* calc member offsets */
	if(t->Struct.isunion) {
		for(i = 0; i < t->Struct.members->len; i++) {
			sm = vecget(t->Struct.members, i);
			sm->offset = 0;
			if(t->size < sm->type->size)
				t->size = sm->type->size;
		}
	} else {
		curoffset = 0;
		for(i = 0; i < t->Struct.members->len; i++) {
			sm = vecget(t->Struct.members, i);
			curoffset = align(curoffset, sm->type->align);
			sm->offset = curoffset;
			curoffset += sm->type->size;
		}
		t->size = curoffset;	
	}
	/* Calc export fields */
	exportednames = 0;
	for(i = 0; i < t->Struct.members->len; i++) {
		sm = vecget(t->Struct.members, i);
		if(sm->name) {
			if(strsethas(exportednames, sm->name))
				errorposf(pos, "field %s duplicated in struct", sm->name);
			export = xmalloc(sizeof(StructExport));
			export->name = sm->name;
			export->path = xmalloc(sizeof(ExportPath));
			export->path->idx = i;
			export->path->next = 0;
			vecappend(t->Struct.exports, export);
			exportednames = strsetadd(exportednames, export->name);
			continue;
		}
		if(!isstruct(sm->type))
			continue;
		for(j = 0; j < sm->type->Struct.exports->len; j++) {
			subexport = vecget(sm->type->Struct.exports, j);
			if(strsethas(exportednames, subexport->name))
				errorposf(pos, "field %s duplicated in struct", subexport->name);
			export = xmalloc(sizeof(StructExport));
			export->name = subexport->name;
			export->path = xmalloc(sizeof(ExportPath));
			export->path->idx = i;
			export->path->next = subexport->path;
			vecappend(t->Struct.exports, export);
			exportednames = strsetadd(exportednames, export->name);
		}
	}
	t->size = align(t->size, t->align);
}

int
isassignable(CTy *to, CTy *from)
{
	if((isarithtype(to) || isptr(to)) &&
		(isarithtype(from) || isptr(from)))
		return 1;
	if(compatiblestruct(to, from))
		return 1;
	return 0;
}

int
canrepresent(CTy *l, CTy *r)
{
	if(!isitype(l) || !isitype(r))
		panic("internal error");
	return getmaxval(l) <= getmaxval(r) && getminval(l) >= getminval(r);
}


