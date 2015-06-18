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
error(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	exit(1);
}

void
errorpos(SrcPos *p, char *fmt, ...)
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
		error("out of memory\n");
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
listadd(List *l, void *v)
{
	List *nl;

	nl = ccmalloc(sizeof(List));
	nl->rest = l;
	nl->v = v;
	return nl;
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
    return m;
}

void 
mapset(Map *m, char *k, void *v)
{
    MapEnt *me;

    me = ccmalloc(sizeof(MapEnt));
    me->k = k;
    me->v = v;
    m->l = listadd(m->l, me);
}

void *
mapget(Map *m, char *k)
{
    List *l;
    MapEnt *me;
    
    l = m->l;
    while(l) {
        me = l->v;
        if(strcmp(me->k, k) == 0)
            return me->v;
        l = l->rest;
    }
    return 0;
}


