#include <u.h>
#include <ds/ds.h>
#include <cc/c.h>

void
usage(void)
{
	puts("Usage: 6c file.c");
	exit(1);
}

int
main(int argc, char *argv[])
{
	if (argc != 2)
		usage();
	cppinit(argv[1]);
	parseinit();
	emitinit(stdout);
	emit();
	return 0;
}
