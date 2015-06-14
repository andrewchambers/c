#include "c.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
error(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	exit(1);
}

void errorpos(SrcPos *p, char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	fprintf(stderr, " at %s:%d:%d\n", p->file, p->line, p->col);
	exit(1);
}


void *ccmalloc(int n) 
{
	void *v;

	v = malloc(n);
	if(!v)
		error("out of memory\n");
	memset(v, 0, n);
	return v;
}

char *ccstrdup(char *s)
{
	int l;
	char *r;

	l = strlen(s) + 1;
	r = ccmalloc(l);
	strcpy(r, s);
	return r;
}

List *addlist(List *l, void *v)
{
	List *nl;

	nl = ccmalloc(sizeof(List));
	nl->rest = l;
	nl->v = v;
	return nl;
}