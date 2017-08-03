#include <stdio.h>
#include "util.h"
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

void
setiroutput(FILE *f)
{

}

void
beginmodule()
{

}

void
adddata()
{

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

}
