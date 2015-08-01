#include <stdlib.h>
#include <stdio.h>
#include "ds/ds.h"
#include "cc/c.h"

void
usage(void)
{
	puts("Usage: c file.c");
	exit(1);
}

int
main(int argc, char *argv[])
{
	Node *n;

	if (argc != 2)
		usage();
	cppinit(argv[1]);
	parseinit();
	emitinit(stdout);
	emit();
	return 0;
}
