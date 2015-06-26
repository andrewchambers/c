#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "c.h"

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
    n = parse();
    emit(n);
    return 0;
}
