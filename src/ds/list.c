#include "ds.h"

List *
listnew()
{
	List *l;

	l = ccmalloc(sizeof(List));
	return l;
}

void
listappend(List *l, void *v)
{
    ListEnt *e;
    ListEnt *ne;
    
    ne = ccmalloc(sizeof(ListEnt));
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
    
	e = ccmalloc(sizeof(ListEnt));
	e->v = v;
	e->next = l->head;
	l->head = e;
}
