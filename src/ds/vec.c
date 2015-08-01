#include <stdlib.h>
#include <stdio.h>
#include "mem/mem.h"
#include "ds.h"

Vec *
vec()
{
	Vec *v;

	v = zmalloc(sizeof(Vec));
	v->len = 0;
	v->cap = 16;
	v->d = zmalloc(sizeof(void*) * v->cap);
	return v;
}

void *
vecget(Vec *v, int idx)
{
	if(idx >= v->len) {
		fprintf(stderr, "vec get out of bounds\n");
		abort();
	}
	return v->d[idx];
}

void 
vecset(Vec *v, int idx, void *x)
{
	if(idx >= v->len) {
		fprintf(stderr, "vec set out of bounds\n");
		abort();
	}
	v->d[idx] = x;
}

static void
vecresize(Vec *v, int cap)
{
	int i;
	void **nd;

	if(v->cap >= cap)
		return;
	nd = zmalloc(cap*sizeof(void*));
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