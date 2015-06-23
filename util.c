#include "c.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	while(1) {
		if(p->line == line)
			break;
		c = fgetc(f);
		if(c == EOF)
			goto cleanup;
		if(c == '\n')
			line += 1;
	}
	while(1) {
		c = fgetc(f);
		if(c == EOF || c == '\n')
			break;
		if(c == '\t')
		    fputs("    ", stderr);
		else
		    fputc(c, stderr);
	}
	fputc('\n', stderr);
	for(i = 0; i < p->col-1; i++)
		fputc(' ', stderr);
	fputs("^\n", stderr);
	cleanup:
	fclose(f);
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


void *
ccmalloc(int n) 
{
	void *v;

	v = malloc(n);
	if(!v)
		errorf("out of memory\n");
	memset(v, 0, n);
	return v;
}

char *
ccstrdup(char *s)
{
	int l;
	char *r;

	l = strlen(s) + 1;
	r = ccmalloc(l);
	strcpy(r, s);
	return r;
}

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


