#include <u.h>
#include <unistd.h>

static void
usage(int rc)
{
	fprintf(stderr, "Usage: 6a in.s out.o\n");
	exit(rc);
}

int
main(int argc, char *argv[])
{
	if (argc != 3)
		usage(1);
	execlp("as", "as", argv[1], "-o", argv[2], NULL);
	panic("exec failed");
	return 1;
}
