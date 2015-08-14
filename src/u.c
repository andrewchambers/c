#include <u.h>

void panic(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	fputs("\n", stderr);
	va_end(va);
	exit(1);
}
