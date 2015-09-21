#include <u.h>
#include <gc/gc.h>
#include "ds.h"

List *
list()
{
	List *l;

	l = gcmalloc(sizeof(List));
	return l;
}

void
listappend(List *l, void *v)
{
	ListEnt *e, *ne;

	l->len++;
	ne = gcmalloc(sizeof(ListEnt));
	ne->v = v;
	if(l->head == 0) {
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
	e = gcmalloc(sizeof(ListEnt));
	e->v = v;
	e->next = l->head;
	l->head = e;
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
