#include <u.h>
#include "gc.h"

char *
gcstrdup(char *s)
{
	int  l;
	char *r;

	l = strlen(s);
	r = gcmalloc(l + 1);
	strncpy(r, s, l);
	return r;
}

void *
gcmalloc(int n)
{
	char *v;
	int  i;

	v = malloc(n);
	if(!v)
		gc();
	for(i = 0; i < n; i++)
		v[i] = 0;
	return v;
}

void
gc()
{
	panic("gc unimplemented");
}
