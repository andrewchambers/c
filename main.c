#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "cpp.h"
#include "ctypes.h"
#include "ir.h"
#include "cc.h"

void
usage()
{
	puts("Usage: cc file.c");
	exit(1);
}

int
main(int argc, char *argv[])
{
	int   i;
	char *cfile;
	Vec  *includedirs;
	
	cfile = 0;
	includedirs = vec();
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-I") == 0) {
			i++;
			if (i >= argc)
				errorf("-I requires an include path\n");
			vecappend(includedirs, argv[i]);
		} else if (strncmp(argv[i], "-I", 2) == 0) {
			vecappend(includedirs, argv[i]+2);
		} else {
			if (argv[i][0] == '-')
				errorf("unknown flag %s\n", argv[i]);
			if (cfile)
				errorf("please specify a single c file\n");
			cfile = argv[i];
		}
	}
	if (!cfile)
		usage();
	cppinit(cfile, includedirs);
	setoutput(stdout);
	compile();
	return 0;
}
