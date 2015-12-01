#include <u.h>
#include <mem/mem.h>
#include "ds.h"

typedef struct MapEnt MapEnt;
struct MapEnt {
	char *k;
	void *v;
};

Map *map()
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

	for(e = m->l->head; e != 0; e = e->next) {
		me = e->v;
		if(strcmp(me->k, k) == 0)
			return me->v;
	}
	return 0;
}

void
mapdel(Map *m, char *k)
{
	mapset(m, k, 0);
}

