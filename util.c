#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

void panic(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	fputs("\n", stderr);
	va_end(va);
	exit(1);
}

void
errorf(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	exit(1);
}

static void
printpos(SrcPos *p)
{
	FILE *f;
	int line;
	int c;
	int i;

	line = 1;
	f = fopen(p->file, "r");
	if (!f)
		return;
	while (1) {
		if (p->line == line)
			break;
		c = fgetc(f);
		if (c == EOF)
			goto cleanup;
		if (c == '\n')
			line += 1;
	}
	while (1) {
		c = fgetc(f);
		if (c == EOF || c == '\n')
			break;
		if (c == '\t')
		    fputs("    ", stderr);
		else
		    fputc(c, stderr);
	}
	fputc('\n', stderr);
	for (i = 0; i < p->col-1; i++)
		fputc(' ', stderr);
	fputs("^\n", stderr);
	cleanup:
	fclose(f);
}

void
errorposf(SrcPos *p, char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	fprintf(stderr, " at %s:%d:%d\n", p->file, p->line, p->col);
	printpos(p);
	exit(1);
}

char *
xstrdup(char *s)
{
	int  l;
	char *r;

	l = strlen(s);
	r = xmalloc(l + 1);
	strncpy(r, s, l);
	return r;
}

int64 malloctotal;

void *
xmalloc(int n)
{
	char *v;

	malloctotal += n;

	v = malloc(n);
	if(!v)
		panic("out of memory!");
	
	return v;
}

List *
list()
{
	List *l;

	l = xmalloc(sizeof(List));
	return l;
}

void
listappend(List *l, void *v)
{
	ListEnt *e, *ne;

	l->len++;
	ne = xmalloc(sizeof(ListEnt));
	ne->v = v;
	if (l->head == 0) {
		l->head = ne;
		return;
	}
	e = l->head;
	while(e->next)
		e = e->next;
	e->next = ne;
}

void
listprepend(List *l, void *v)
{
	ListEnt *e;

	l->len++;
	e = xmalloc(sizeof(ListEnt));
	e->v = v;
	e->next = l->head;
	l->head = e;
}

void
listinsert(List *l, int idx, void *v)
{
	ListEnt *e, *newe;
	int i;

	i = 0;
	e = l->head;
	if (!e) {
		listappend(l, v);
		return;
	}
	while (1) {
		if (i == idx)
			break;
		if (!e->next)
			break;
		e = e->next;
		i++;
	}
	newe = xmalloc(sizeof(ListEnt));
	newe->v = v;
	newe->next = e->next;
	e->next = newe;
}

void *
listpopfront(List *l)
{
	ListEnt *e;

	if(!l->len)
		panic("pop from empty list");
	if(l->len == 1) {
		e = l->head;
		l->head = 0;
		l->len = 0;
		return e->v;
	}
	l->len--;
	e = l->head;
	l->head = e->next;
	return e->v;
}

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
	if (idx >= v->len)
		panic("vec get out of bounds");
	return v->d[idx];
}

void 
vecset(Vec *v, int idx, void *x)
{
	if (idx >= v->len)
		panic("vec set out of bounds");
	v->d[idx] = x;
}

static void
vecresize(Vec *v, int cap)
{
	int i;
	void **nd;

	if (v->cap >= cap)
		return;
	nd = xmalloc(cap*sizeof(void*));
	for (i = 0; i < v->len; i++)
		nd[i] = v->d[i];
	v->d = nd;
	v->cap = cap;
}

void
vecappend(Vec *v, void *x)
{
	if (v->len == v->cap)
		vecresize(v, v->len + 16);
	v->d[v->len] = x;
	v->len++;
}

int
vecremove(Vec *v, void *x)
{
	int i;

	for (i = 0; i < v->len; i++) {
		if(v->d[i] == x)
			break;
	}
	if (i == v->len)
		return 0;
	for (; i < v->len - 1; i++) {
		v->d[i] = v->d[i+1];
	}
	v->len -= 1;
	return 1;
}

void
vecsort(Vec *v, int (*compar)(const void *, const void *))
{
	qsort(v->d, v->len, sizeof(void*), compar);
}

typedef struct MapEnt MapEnt;
struct MapEnt {
	char *k;
	void *v;
};

Map *
map()
{
	Map *m;

	m = xmalloc(sizeof(Map));
	m->l = list();
	return m;
}

void 
mapset(Map *m, char *k, void *v)
{
	MapEnt *me;

	me = xmalloc(sizeof(MapEnt));
	me->k = k;
	me->v = v;
	listprepend(m->l, me);
}

void *
mapget(Map *m, char *k)
{
	MapEnt *me;
	ListEnt *e;

	for (e = m->l->head; e != 0; e = e->next) {
		me = e->v;
		if (strcmp(me->k, k) == 0)
			return me->v;
	}
	return 0;
}

void
mapdel(Map *m, char *k)
{
	mapset(m, k, 0);
}

StrSet *
strsetadd(StrSet *ss, char *v)
{
	StrSet *r;

	if (strsethas(ss,v))
		return ss;
	r = xmalloc(sizeof(StrSet));
	r->v = v;
	r->next = ss;
	return r;
}

int
strsethas(StrSet *ss, char *v)
{
	while (ss) {
		if (strcmp(v, ss->v) == 0)
			return 1;
		ss = ss->next;
	}
	return 0;
}

StrSet *
strsetintersect(StrSet *ss, StrSet *other)
{
	char *v;
	StrSet *r;

	r = 0;
	while (ss) {
		v = ss->v;
		if (strsethas(other, v))
			r = strsetadd(r, v);
		ss = ss->next;
	}
	return r;
}
