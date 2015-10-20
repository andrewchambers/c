#include <u.h>
#include <ds/ds.h>
#include "cc.h"

static void
printpos(SrcPos *p)
{
	FILE *f;
	int line;
	int c;
	int i;

	line = 1;
	f = fopen(p->file, "r");
	if (!f)
		return;
	while(1) {
		if(p->line == line)
			break;
		c = fgetc(f);
		if(c == EOF)
			goto cleanup;
		if(c == '\n')
			line += 1;
	}
	while(1) {
		c = fgetc(f);
		if(c == EOF || c == '\n')
			break;
		if(c == '\t')
		    fputs("    ", stderr);
		else
		    fputc(c, stderr);
	}
	fputc('\n', stderr);
	for(i = 0; i < p->col-1; i++)
		fputc(' ', stderr);
	fputs("^\n", stderr);
	cleanup:
	fclose(f);
}

void
errorf(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	exit(1);
}

void
errorposf(SrcPos *p, char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	fprintf(stderr, " at %s:%d:%d\n", p->file, p->line, p->col);
	printpos(p);
	exit(1);
}
