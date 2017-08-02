#include <stdio.h>
#include "lex.h"
#include "cpp.h"

int
main()
{
	initlexer("<stdin>", stdin);
	initcpp();

	while (cpptok->kind != TOKEOF) {
		cppnext();
	}
	
	return 0;
}
