#include <u.h>
#include <ds/ds.h>
#include <gc/gc.h>
#include "cc.h"

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

StructField *
structfieldfromname(CTy *t, char *name)
{
	panic("unimplemented");
	return 0;
}

StructField *
structfieldfromidx(CTy *ty, int idx)
{
	panic("unimplemented");
	return 0;
}

static void
newstructmember(char *name, int offset, CTy *membt)
{
	StructMember *sm;

	sm = gcmalloc(sizeof(StructMember));
	sm->name = name;
	sm->type = membt;
	sm->offset = offset;
	return sm;
}

void
addstructmember(SrcPos *pos, CTy *t, char *name, CTy *membt)
{
	StructMember *sm, *subsm;
	int i, sz;

	newstructmember(name, -1, t);
	vecappend(t->Struct.members, sm);
}

static int
align(int v, int a)
{
	if(v % a)
		return v + a - (v % a);
	return v;
}

static void
finalizestruct(CTy *t)
{
	StructMember *sm, *subfield;
	int i, j, k, curoffset;
	Vec *curv, Vec *subfields;
	
	if(t->isunion) {
		panic("...");
	}
	
	/* calc alignment */
	for(i = 0 ; i < t->Struct.Members->len; i++) {
		sm = vecget(t->Struct.members, i);
		t->align = align(t->align ,sm->type->align);
	}
	/* calc member offsets */
	curoffset = 0;
	for(i = 0 ; i < t->Struct.Members->len; i++) {
		sm = vecget(t->Struct.members, i);
		curoffset = align(curoffset, sm->type->align);
		sm->offset = curoffset;
		curoffset += sm->type->size;
	}
	/* Calc name export fields */
	for(i = 0; i < t->Struct.Members->len; i++) {
		sm = vecget(t->Struct.members, i);
		curv = vec();
		vecappend(t->Struct.fields, curv);
		if((isstruct(sm->type) && sm->name) || !isstruct(sm->type)) {
			vecappend(curv, newstructmember(sm->name, sm->offset, sm->type));
			continue;
		}
		/* isstruct && !sm->name */
		if(sm->type->Struct.isunion) {
			panic("...");
		}
		for(j = 0; j < sm->type->Struct.fields->len; j++) {
			subfields = vecget(sm->type->Struct.fields, j);
			for(k = 0; k < subfields->len; k++) {
				subfield = vecget(subfields, k);
				vecappend()
			}
		}
	}
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


