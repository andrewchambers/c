#include <unistd.h>
#include <stdlib.h>
#include "cc.h"

void
usage(void)
{
    puts("Usage: cc file.c");
    exit(1);
}

int
main(int argc, char *argv[])
{
    if (argc != 2) 
        usage();
    cppinit(argv[1]);
    parse();
    emit();
    exit(0);
}