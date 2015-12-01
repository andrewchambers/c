#include <u.h>
#include <mem/mem.h>
#include "ds.h"

Vec *
vec()
{
	Vec *v;

	v = xmalloc(sizeof(Vec));
	v->len = 0;
	v->cap = 16;
	v->d = xmalloc(sizeof(void*) * v->cap);
	return v;
}

void *
vecget(Vec *v, int idx)
{
	if(idx >= v->len)
		panic("vec get out of bounds");
	return v->d[idx];
}

void 
vecset(Vec *v, int idx, void *x)
{
	if(idx >= v->len)
		panic("vec set out of bounds");
	v->d[idx] = x;
}

static void
vecresize(Vec *v, int cap)
{
	int i;
	void **nd;

	if(v->cap >= cap)
		return;
	nd = xmalloc(cap*sizeof(void*));
	for(i = 0; i < v->len; i++)
		nd[i] = v->d[i];
	v->d = nd;
	v->cap = cap;
}

void
vecappend(Vec *v, void *x)
{
	if(v->len == v->cap)
		vecresize(v, v->len + 16);
	v->d[v->len] = x;
	v->len++;
}

int
vecremove(Vec *v, void *x)
{
	int i;

	for(i = 0; i < v->len; i++) {
		if(v->d[i] == x)
			break;
	}
	if(i == v->len)
		return 0;
	for(; i < v->len - 1; i++) {
		v->d[i] = v->d[i+1];
	}
	v->len -= 1;
	return 1;
}
