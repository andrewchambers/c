#include <u.h>
#include <ds/ds.h>
#include <cc/cc.h>

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
	cppinit(argv[1], vec());
	while(1) {
		t = pp();
		if(t->k == TOKEOF)
			break;
		printf("%s %s\n", tokktostr(t->k), t->v);
	}
	return 0;
}
