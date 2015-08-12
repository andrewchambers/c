#include "ds.h"
#include "mem/mem.h"

List *
list()
{
	List *l;

	l = zmalloc(sizeof(List));
	return l;
}

void
listappend(List *l, void *v)
{
	ListEnt *e, *ne;

	ne = zmalloc(sizeof(ListEnt));
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

	e = zmalloc(sizeof(ListEnt));
	e->v = v;
	e->next = l->head;
	l->head = e;
}
