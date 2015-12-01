#include <u.h>
#include "mem.h"

char *
xstrdup(char *s)
{
	int  l;
	char *r;

	l = strlen(s);
	r = xmalloc(l + 1);
	strncpy(r, s, l);
	return r;
}

void *
xmalloc(int n)
{
	char *v;
	int  i;

	v = malloc(n);
	if(!v)
		panic("out of memory!");
	for(i = 0; i < n; i++)
		v[i] = 0;
	return v;
}


