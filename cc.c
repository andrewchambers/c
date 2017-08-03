#include <stdint.h>
#include <stdio.h>
#include "util.h"
#include "cpp.h"
#include "ir.h"
#include "cc.h"


Tok *tok;
Tok *nexttok;

#define MAXSCOPES 1024
static int nscopes;
static Map *tags[MAXSCOPES];
static Map *syms[MAXSCOPES];

typedef struct Switch {

} Switch;

#define MAXLABELDEPTH 2048
static int     switchdepth;
static int     brkdepth;
static int     contdepth;
static char   *breaks[MAXLABELDEPTH];
static char   *conts[MAXLABELDEPTH];
static Switch *switches[MAXLABELDEPTH];

Map  *labels;
Vec  *gotos;
Vec  *tentativesyms;

static void
pushswitch(Switch *s)
{
        switches[switchdepth] = s;
        switchdepth += 1;
}

static void
popswitch(void)
{
        switchdepth -= 1;
        if (switchdepth < 0)
                panic("internal error");
}

static Switch *
curswitch(void)
{
        if (switchdepth == 0)
                return 0;
        return switches[switchdepth - 1];
}

static void
pushcontbrk(char *lcont, char *lbreak)
{
        conts[contdepth] = lcont;
        breaks[brkdepth] = lbreak;
        brkdepth += 1;
        contdepth += 1;
}

static void
popcontbrk(void)
{
        brkdepth -= 1;
        contdepth -= 1;
        if (brkdepth < 0 || contdepth < 0)
                panic("internal error");
}

static char *
curcont()
{
        if (contdepth == 0)
                return 0;
        return conts[contdepth - 1];
}

static char *
curbrk()
{
        if (brkdepth == 0)
                return 0;
        return breaks[brkdepth - 1];
}

static void
pushbrk(char *lbreak)
{
        breaks[brkdepth] = lbreak;
        brkdepth += 1;
}

static void
popscope(void)
{
        nscopes -= 1;
        if (nscopes < 0)
                errorf("bug: scope underflow\n");
        syms[nscopes] = 0;
        tags[nscopes] = 0;
}

static void
pushscope(void)
{
        syms[nscopes] = map();
        tags[nscopes] = map();
        nscopes += 1;
        if (nscopes > MAXSCOPES)
                errorf("scope depth exceeded maximum\n");
}

static int
isglobal(void)
{
        return nscopes == 1;
}

void
compile()
{
	beginmodule();
	endmodule();
}
