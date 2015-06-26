#include "buff.h"
#include "mem/mem.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

Buff *
buffnew()
{
    Buff *b;
    b = zmalloc(sizeof(buff));
    b->sz = 64;
    b->d = zmalloc(b->64);
}

void
bufffree(Buff *b)
{
    free(buff->d);
    free(buff);
}

Buff *
buffprintf(Buff *b, char *fmt, ...)
{
    exit(1);
    /*
	va_list va
	va_list vb;
	int w;
	int avail;
    
    avail = b->sz - b->n;
	va_start(va, fmt);
	va_copy(vb, va);
	w = vsnprintf(stderr, avail, b->d, va);
	if(w > avail) {
	    b->d = realloc(b->d, b->sz + w);
	}
	va_end(va);
	va_end(vb);
	*/
}

