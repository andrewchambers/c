#include <stdlib.h>
#include <stdio.h>
#include "ds/ds.h"
#include "cc/c.h"

void
emit(Node *n)
{
	dumpast(stderr, n);	
}