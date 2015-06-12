#include "cc.h"
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
	if (v) {
		error("out of memory!");
		exit(1);
	}
	memset(v, 0, n);
	return v;
}