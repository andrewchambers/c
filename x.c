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


void *ccmalloc(int n) 
{
	void *v;

	v = malloc(n);
	if(!v)
		error("out of memory\n");
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