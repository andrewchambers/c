#include <u.h>
#include <mem/mem.h>
#include "ds.h"

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
	if(!e) {
		listappend(l, v);
		return;
	}
	while(1) {
		if(i == idx)
			break;
		if(!e->next)
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
