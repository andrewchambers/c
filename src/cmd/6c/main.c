#include <u.h>
#include <ds/ds.h>
#include <cc/cc.h>

void
usage()
{
	puts("Usage: 6c file.c");
	exit(1);
}

int
main(int argc, char *argv[])
{
	int   i;
	Vec  *includedirs;
	char *cfile;
	
	cfile = 0;
	includedirs = vec();
	for(i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-I") == 0) {
			i++;
			if(i >= argc)
				errorf("-I requires an include path\n");
			vecappend(includedirs, argv[i]);
		} else if(strncmp(argv[i], "-I", 2) == 0) {
			vecappend(includedirs, argv[i]+2);
		} else {
			if(argv[i][0] == '-')
				errorf("unknown flag %s\n", argv[i]);
			if(cfile)
				errorf("please specify a single c file\n");
			cfile = argv[i];
		}
	}
	if(!cfile)
		usage();
	cppinit(cfile, includedirs);
	emitinit(stdout);
	parse();
	emitend();
	return 0;
}
