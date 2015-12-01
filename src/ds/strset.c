#include <u.h>
#include <mem/mem.h>
#include "ds.h"

StrSet *
strsetadd(StrSet *ss, char *v)
{
	StrSet *r;

	if(strsethas(ss,v))
		return ss;
	r = xmalloc(sizeof(StrSet));
	r->v = v;
	r->next = ss;
	return r;
}

int
strsethas(StrSet *ss, char *v)
{
	while(ss) {
		if(strcmp(v, ss->v) == 0)
			return 1;
		ss = ss->next;
	}
	return 0;
}

StrSet *
strsetintersect(StrSet *ss, StrSet *other)
{
	char *v;
	StrSet *r;

	r = 0;
	while(ss) {
		v = ss->v;
		if(strsethas(other, v))
			r = strsetadd(r, v);
		ss = ss->next;
	}
	return r;
}
