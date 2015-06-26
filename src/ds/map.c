#include "ds.h"

typedef struct MapEnt MapEnt;
struct MapEnt {
    char *k;
    void *v;
};

Map *map()
{
    Map *m;
    
    m = ccmalloc(sizeof(Map));
    m->l = listnew();
    return m;
}

void 
mapset(Map *m, char *k, void *v)
{
    MapEnt *me;

    me = ccmalloc(sizeof(MapEnt));
    me->k = k;
    me->v = v;
    listprepend(m->l, me);
}

void *
mapget(Map *m, char *k)
{
    ListEnt *e;
    MapEnt *me;
    
    for(e = m->l->head; e != 0; e = e->next) {
        me = e->v;
        if(strcmp(me->k, k) == 0)
            return me->v;
    }
    return 0;
}

