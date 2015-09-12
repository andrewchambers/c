#include "u.h"
#include "ds/ds.h"
#include "gc/gc.h"
#include <time.h>

#define MAXPARAMS  10
#define MAXSTRUCTM 6
#define MAXNEST 3

enum {
	CHAR,
	INT,
	STRUCT,
	TEND,
} Type;

typedef struct {
	int t;
	union {
		struct {
			char v;
		} Cchar;
		struct {
			int v;
		} Cint;
		struct {
			char *name;
			Vec  *members;
		} Cstruct;
	};
} Param;

static Param *randparam();

void
printparamtype(Param *p)
{
	switch(p->t) {
	case CHAR:
		printf("char");
		break;
	case INT:
		printf("int");
		break;
	case STRUCT:
		printf("struct %s", p->Cstruct.name);
		break;
	default:
		panic("internal error");
	}
}

int structcount = 0;

static Param *
randstruct(int depth)
{
	Param *r, *p;
	int i, n; 
	char buff[1024];

	r = gcmalloc(sizeof(Param));
	r->t = STRUCT;
	r->Cstruct.members = vec();
	n = rand() % MAXSTRUCTM;
	for(i = 0; i < n; i++)
		vecappend(r->Cstruct.members, randparam(depth + 1));
	snprintf(buff, sizeof buff, "s%d", structcount++);
	r->Cstruct.name = gcstrdup(buff);
	printf("struct %s {\n", r->Cstruct.name);
	for(i = 0; i < r->Cstruct.members->len; i++) {
		p = vecget(r->Cstruct.members, i);
		printf("\t");
		printparamtype(p);
		printf(" m%d;\n", i);
	}
	printf("};\n");
	return r;
}

static Param *
randparam(int depth)
{
	Param *r;

	again:
	switch(rand() % TEND) {
	case CHAR:
		r = gcmalloc(sizeof(Param));
		r->t = CHAR;
		r->Cchar.v = rand();
		break;	
	case INT:
		r = gcmalloc(sizeof(Param));
		r->t = INT;
		r->Cint.v = rand();
		break;
	case STRUCT:
		if(depth > MAXNEST)
			goto again;
		r = randstruct(depth);
		break;
	default:
		panic("internal error randparam");
	}
	return r;
}

static Vec *
randparams()
{
	Vec *r;
	int n, i;
	
	r = vec();
	n = rand() % MAXPARAMS;
	for(i = 0; i < n; i++)
		vecappend(r, randparam(0));
	return r;
}


static void
printcheck(char *sel, Param *p)
{
	Param *mem;
	char buf[128];
	int i;

	switch(p->t) {
	case CHAR:
		printf("\tif(%s != %d) return 1;\n", sel, p->Cchar.v);
		break;
	case INT:
		printf("\tif(%s != %d) return 1;\n", sel, p->Cint.v);
		break;
	case STRUCT:
		for(i = 0; i < p->Cstruct.members->len; i++) {
			mem = vecget(p->Cstruct.members, i);
			snprintf(buf, sizeof(buf), "%s.m%d", sel, i);
			printcheck(buf, mem);
		}
		break;
	default:
		panic("internal error");
	}
}

static void
printfunc(Vec *v)
{
	int i;
	char buf[64];
	Param *p;

	printf("int\n");
	printf("f(");
	for(i = 0; i < v->len; i++) {
		p = vecget(v, i);
		printparamtype(p);
		printf(" p%d%s", i, (i == v->len - 1) ? "" : ", ");
	}
	printf(")\n");
	printf("{\n");
	for(i = 0; i < v->len; i++) {
		p = vecget(v, i);
		snprintf(buf, sizeof(buf), "p%d", i);
		printcheck(buf, p);
	}
	printf("	return 0;\n");
	printf("}\n");
}

static void
printinit(char *sel, Param *p)
{
	Param *mem;
	char buf[128];
	int i;

	switch(p->t) {
	case CHAR:
		printf("\t%s = %d;\n", sel, p->Cchar.v);
		break;
	case INT:
		printf("\t%s = %d;\n", sel, p->Cint.v);
		break;
	case STRUCT:
		for(i = 0; i < p->Cstruct.members->len; i++) {
			mem = vecget(p->Cstruct.members, i);
			snprintf(buf, sizeof(buf), "%s.m%d", sel, i);
			printinit(buf, mem);
		}
		break;
	default:
		panic("internal error");
	}
}

static void
printmain(Vec *v)
{
	Param *p;
	char buf[64];
	int i;

	printf("int\n");
	printf("main()\n");
	printf("{\n");
	for(i = 0; i < v->len; i++) {
		p = vecget(v, i);
		printf("\t");
		printparamtype(p);
		printf(" p%d;\n", i);
	}
	for(i = 0; i < v->len; i++) {
		p = vecget(v, i);
		snprintf(buf, sizeof(buf), "p%d", i);
		printinit(buf, p);
	}
	printf("\treturn f(");
	for(i = 0; i < v->len; i++) {
		printf("p%d%s", i, (i == v->len - 1) ? "" : ", ");
	}
	printf(");\n");
	printf("}\n");
}

static int
getseed()
{
	int i, c, r;
	FILE *f;

	f = fopen("/dev/urandom", "r");
	if(!f)
		panic("get seed failed");
	r = 0;
	for(i = 0; i < 4; i++) {
		c = fgetc(f);
		if(c == EOF)
			panic("get seed read failed");
		r = (r << 8) | c;
	}
	if(fclose(f) != 0)
		panic("get seed close failed");
	return r;
}

int main(int argc, char *argv[])
{
	Vec *v;
	int seed;

	if(argc == 1)
		seed = getseed();
	else
		seed = strtoul(argv[1], 0, 10);
	printf("/* seed %u */\n", seed);
	srand(seed);
	v = randparams();
	printfunc(v);
	printmain(v);
	return 0;
}
