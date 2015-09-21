#include <u.h>
#include <gc/gc.h>
#include "ds.h"

StrSet *
strsetadd(StrSet *ss, char *v)
{
	StrSet *r;

	if(strsethas(ss,v))
		return ss;
	r = gcmalloc(sizeof(StrSet));
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
