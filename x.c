#include "cc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void error(char *s)
{
	fputs(s, stderr);
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