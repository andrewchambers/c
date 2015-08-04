#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mem.h"

#if linux
static size_t
strlcpy(char *dst, const char *src, size_t size)
{
	size_t i;

	for(i = 0; i < size - 1; i++) {
		if(src[i])
			dst[i] = src[i];
		else
			break;
	}
	dst[i] = 0;
	return i;
}
#endif

void *
zmalloc(int n) 
{
	void *v;

	v = malloc(n);
	if(!v) {
		fprintf(stderr, "out of memory\n");
		abort();
	}
	memset(v, 0, n);
	return v;
}

char *
zstrdup(char *s)
{
	int l;
	char *r;

	l = strlen(s) + 1;
	r = zmalloc(l);
	strlcpy(r, s, l);
	return r;
}
