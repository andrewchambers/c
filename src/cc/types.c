#include <stdio.h>
#include <string.h>
#include "ds/ds.h"
#include "c.h"

int
convrank(CTy *t)
{
	if(t->t != CPRIM)
		errorf("internal error\n");
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
	}
	errorf("internal error\n");
	return -1;
}

int 
compatiblestruct(CTy *l, CTy *r)
{
	/* TODO */
	return 0;
}

int 
sametype(CTy *l, CTy *r)
{
	/* TODO */
	switch(l->t) {
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
isitype(CTy *t)
{
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

StructMember *
getstructmember(CTy *t, char *n)
{
	int     i;
	StructMember *sm;
	
	if(isptr(t))
		t = t->Ptr.subty;
	if(!isstruct(t))
		errorf("internal error\n");
	for(i = 0; i < t->Struct.members->len; i++) {
		sm = vecget(t->Struct.members, i);
		if(strcmp(n, sm->name) == 0)
			return sm;
	}
	return 0;
}

CTy *
structmemberty(CTy *t, char *n)
{
	StructMember *sm;
	
	sm = getstructmember(t, n);
	if(!sm)
		return 0;
	return sm->type;
}

void
fillstructsz(CTy *t)
{
	int i, sz, align;
	StructMember *sm;
	
	sz = 0;
	if(t->t != CSTRUCT)
		errorf("internal error");
	if(t->Struct.isunion)
		errorf("unimplemented calcstructsz\n");
	if(t->Struct.unspecified) {
		t->size = -1;
		t->align = -1;
		return;
	}
	for(i = 0; i < t->Struct.members->len; i++) {
		sm = vecget(t->Struct.members, i);
		align = sm->type->align;
		if(sz % align)
			sz = sz + align - (sz % align);
		sm->offset = sz;
		sz += sm->type->size;
	}
	t->size = sz;
	t->align = 8; /* TODO: what struct alignment? */
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

unsigned long long int
getmaxval(CTy *l)
{
	switch(l->Prim.type) {
	case PRIMCHAR:
		if(l->Prim.issigned)
			return 0x7f;
		else
			return 0xff;
	case PRIMSHORT:
		if(l->Prim.issigned)
			return 0x7fff;
		else
			return  0xffff;
	case PRIMINT:
	case PRIMLONG:
		if(l->Prim.issigned)
			return 0x7fffffff;
		else
			return 0xffffffff;
	case PRIMLLONG:
		if(l->Prim.issigned)
			return 0x7fffffffffffffff;
		else
			return 0xffffffffffffffff;
	}
	errorf("internal error\n");
	return 0;
}

signed long long int
getminval(CTy *l)
{
	if(!l->Prim.issigned)
		return 0;
	switch(l->Prim.type) {
	case PRIMCHAR:
		return 0xff;
	case PRIMSHORT:
		return 0xffff;
	case PRIMINT:
	case PRIMLONG:
		return 0xffffffffl;
	case PRIMLLONG:
		return 0xffffffffffffffff;
	}
	errorf("internal error\n");
	return 0;
}

int
canrepresent(CTy *l, CTy *r)
{
	if(!isitype(l) || !isitype(r))
		errorf("internal error");
	return getmaxval(l) <= getmaxval(r) && getminval(l) >= getminval(r);
}

