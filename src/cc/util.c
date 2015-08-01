#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ds/ds.h"
#include "c.h"

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

static void
ws(FILE *f, int n)
{
	int i;

	for(i = 0; i < n; i++)
		fputc(' ', f);
}

static void
_dumpast(FILE *f, int depth, Node *n)
{
	switch(n->t) {
	case NLABELED:
		ws(f, depth); fprintf(f, "{\n");
		ws(f, depth); fprintf(f, ":tag :NLABLED\n");
		ws(f, depth); fprintf(f, ":l %s\n", n->Labeled.l);
		ws(f, depth); fprintf(f, ":stmt\n");
		_dumpast(f, depth + 2 ,n->Labeled.stmt);
		ws(f, depth); fprintf(f, "}\n");
		return;
	case NNUM:
		ws(f, depth); fprintf(f, "{\n");
		ws(f, depth); fprintf(f, ":tag :NNUMBER\n");
		ws(f, depth); fprintf(f, ":v %s\n", n->Num.v);
		ws(f, depth); fprintf(f, "}\n");
		return;
	case NSYM:
		ws(f, depth); fprintf(f, "{\n");
		ws(f, depth); fprintf(f, ":tag :NSYM\n");
		ws(f, depth); fprintf(f, ":n %s\n", n->Sym.n);
		ws(f, depth); fprintf(f, "}\n");
		return;
	/*
	case NNUM:
	case NSTR:
	case NIDX:
	case NSEL:
	case NCALL:
	case NSIZEOF:
	case NIF:
	case NBINOP:
	case NUNOP:
	case NCAST:
	case NINIT:
	case NRETURN:
	case NSWITCH:
	case NGOTO:
	case NWHILE:
	case NDOWHILE:
	case NFOR:
	*/
	default:
		errorf("unimplemented dumpast - %d", n->t);
	}
}

void
dumpast(FILE *f, Node *n)
{
	_dumpast(f, 0, n);
}