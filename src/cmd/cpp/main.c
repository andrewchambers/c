#include <stdlib.h>
#include <stdio.h>
#include "ds/ds.h"
#include "cc/c.h"

void
usage(void)
{
	puts("Usage: cpp file.c");
	exit(1);
}

int
main(int argc, char *argv[])
{
	Tok *t;

	if(argc != 2)
		usage();
	cppinit(argv[1]);
	while(1) {
		t = lex();
		if(t->k == TOKEOF)
			break;
		puts(tokktostr(t->k));
	}
	return 0;
}
