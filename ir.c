#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include "util.h"
#include "ctypes.h"
#include "cc.h"
#include "ir.h"

int labelcount;

char *
newlabel(void)
{
	char *s;
	int   n;

	n = snprintf(0, 0, ".L%d", labelcount);
	if(n < 0)
		panic("internal error");
	n += 1;
	s = xmalloc(n);
	if(snprintf(s, n, ".L%d", labelcount) < 0)
		panic("internal error");
	labelcount++;
	return s;
}

static FILE *outf;

void
setiroutput(FILE *f)
{
	outf = f;
}

static void
out(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	if(vfprintf(outf, fmt, va) < 0)
		errorf("error printing\n");
	va_end(va);
}

void
beginmodule()
{
	out("; Compiled with care...\n");
}

void
emitsym(Sym *sym)
{
	out("; %s:%d:%d %s\n", sym->pos->file, sym->pos->line, sym->pos->col, sym->name);
	switch(sym->k){
	case SYMGLOBAL:
		break;
	case SYMLOCAL:
		break;
	case SYMENUM:
		break;
	case SYMTYPE:
		break;
	}
	out("\n");
}

void
beginfunc()
{

}

void
emitins()
{

}

void
emitterm()
{

}

void
endfunc()
{

}

void
endmodule()
{
	out("; Until we meet again.\n");
}
