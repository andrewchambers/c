#include "u.h"
#include "ds/ds.h"
#include "gc/gc.h"
#include <time.h>

#define MAXPARAMS  10
#define MAXSTRUCTM 10
#define MAXNEST 4

typedef enum {
	CHAR,
	SHORT,
	INT,
	LONG,
	LLONG,
	FLOAT,
	DOUBLE,
	STRUCT,
	TEND,
} Type;

typedef struct {
	Type t;
	union {
		struct {
			char v;
		} Cchar;
		struct {
			short v;
		} Cshort;
		struct {
			int v;
		} Cint;
		struct {
			long v;
		} Clong;
		struct {
			long long v;
		} Cllong;
		struct {
			float v;
		} Cfloat;
		struct {
			double v;
		} Cdouble;
		struct {
			char *name;
			Vec  *members;
		} Cstruct;
	};
} Val;

typedef struct {
	Val *ret;
	Vec *vals;
} Testcase;

static Val *randval();

void
printvaltype(Val *v)
{
	switch(v->t) {
	case CHAR:
		printf("char");
		break;
	case SHORT:
		printf("short");
		break;
	case INT:
		printf("int");
		break;
	case LONG:
		printf("long");
		break;
	case LLONG:
		printf("long long");
		break;
	case FLOAT:
		printf("float");
		break;
	case DOUBLE:
		printf("double");
		break;
	case STRUCT:
		printf("struct %s", v->Cstruct.name);
		break;
	default:
		panic("internal error");
	}
}

int structcount = 0;

static Val *
randstruct(int depth)
{
	Val *r, *p;
	int i, n; 
	char buff[1024];

	r = gcmalloc(sizeof(Val));
	r->t = STRUCT;
	r->Cstruct.members = vec();
	n = rand() % MAXSTRUCTM;
	for(i = 0; i < n; i++)
		vecappend(r->Cstruct.members, randval(depth + 1));
	snprintf(buff, sizeof buff, "s%d", structcount++);
	r->Cstruct.name = gcstrdup(buff);
	printf("struct %s {\n", r->Cstruct.name);
	for(i = 0; i < r->Cstruct.members->len; i++) {
		p = vecget(r->Cstruct.members, i);
		printf("\t");
		printvaltype(p);
		printf(" m%d;\n", i);
	}
	printf("};\n");
	return r;
}

static Val *
randval(int depth)
{
	Val *r;

	again:
	switch(rand() % TEND) {
	case CHAR:
		r = gcmalloc(sizeof(Val));
		r->t = CHAR;
		r->Cchar.v = rand();
		break;
	case SHORT:
		r = gcmalloc(sizeof(Val));
		r->t = SHORT;
		r->Cshort.v = rand();
		break;
	case INT:
		r = gcmalloc(sizeof(Val));
		r->t = INT;
		r->Cint.v = rand();
		break;
	case LONG:
		r = gcmalloc(sizeof(Val));
		r->t = LONG;
		r->Clong.v = rand();
		break;
	case LLONG:
		r = gcmalloc(sizeof(Val));
		r->t = LLONG;
		r->Cllong.v = rand();
		break;
	case FLOAT:
		r = gcmalloc(sizeof(Val));
		r->t = FLOAT;
		r->Cfloat.v = (float)rand();
		break;
	case DOUBLE:
		r = gcmalloc(sizeof(Val));
		r->t = DOUBLE;
		r->Cdouble.v = (double)rand();
		break;
	case STRUCT:
		if(depth > MAXNEST)
			goto again;
		r = randstruct(depth);
		break;
	default:
		panic("internal error randval");
	}
	return r;
}

static Testcase *
randtestcase()
{
	Testcase *t;
	int n, i;
	
	t = gcmalloc(sizeof(Testcase));
	t->vals = vec();
	t->ret = randval(0);
	n = rand() % MAXPARAMS;
	for(i = 0; i < n; i++)
		vecappend(t->vals, randval(0));
	return t;
}

static void
printinit(char *sel, Val *p)
{
	Val *mem;
	char   buf[128];
	int    i;

	switch(p->t) {
	case CHAR:
		printf("\t%s = %d;\n", sel, p->Cchar.v);
		break;
	case SHORT:
		printf("\t%s = %d;\n", sel, p->Cshort.v);
		break;
	case INT:
		printf("\t%s = %d;\n", sel, p->Cint.v);
		break;
	case LONG:
		printf("\t%s = %ld;\n", sel, p->Clong.v);
		break;
	case LLONG:
		printf("\t%s = %lld;\n", sel, p->Cllong.v);
		break;
	case FLOAT:
		printf("\t%s = %f;\n", sel, p->Cfloat.v);
		break;
	case DOUBLE:
		printf("\t%s = %f;\n", sel, p->Cdouble.v);
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
printcheck(char *sel, Val *p)
{
	Val *mem;
	char buf[128];
	int i;

	switch(p->t) {
	case CHAR:
		printf("\tif(%s != %d) abort();\n", sel, p->Cchar.v);
		break;
	case SHORT:
		printf("\tif(%s != %d) abort();\n", sel, p->Cshort.v);
		break;
	case INT:
		printf("\tif(%s != %d) abort();\n", sel, p->Cint.v);
		break;
	case LONG:
		printf("\tif(%s != %ld) abort();\n", sel, p->Clong.v);
		break;
	case LLONG:
		printf("\tif(%s != %lld) abort();\n", sel, p->Cllong.v);
		break;
	case FLOAT:
		printf("\tif(%s != %f) abort();\n", sel, p->Cfloat.v);
		break;
	case DOUBLE:
		printf("\tif(%s != %f) abort();\n", sel, p->Cdouble.v);
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
printfunc(Testcase *t)
{
	int i;
	char buf[64];
	Vec *v;
	Val *p;

	v = t->vals;
	printvaltype(t->ret);
	printf("\n");
	printf("f(");
	for(i = 0; i < v->len; i++) {
		p = vecget(v, i);
		printvaltype(p);
		printf(" p%d%s", i, (i == v->len - 1) ? "" : ", ");
	}
	printf(")\n");
	printf("{\n");
	printf("\t");
	printvaltype(t->ret);
	printf(" r;\n");
	printinit("r", t->ret);
	for(i = 0; i < v->len; i++) {
		p = vecget(v, i);
		snprintf(buf, sizeof(buf), "p%d", i);
		printcheck(buf, p);
	}
	printf("\treturn r;\n");
	printf("}\n");
}

static void
printmain(Testcase *t)
{
	Vec *v;
	Val *p;
	char   buf[64];
	int    i;
	
	v = t->vals;
	printf("int\n");
	printf("main()\n");
	printf("{\n");
	printf("\t");
	printvaltype(t->ret);
	printf(" r;\n");
	for(i = 0; i < v->len; i++) {
		p = vecget(v, i);
		printf("\t");
		printvaltype(p);
		printf(" p%d;\n", i);
	}
	for(i = 0; i < v->len; i++) {
		p = vecget(v, i);
		snprintf(buf, sizeof(buf), "p%d", i);
		printinit(buf, p);
	}
	printf("\tr = f(");
	for(i = 0; i < v->len; i++) {
		printf("p%d%s", i, (i == v->len - 1) ? "" : ", ");
	}
	printf(");\n");
	printcheck("r", t->ret);
	printf("\treturn 0;\n");
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
	for(i = 0; i < sizeof(int); i++) {
		c = fgetc(f);
		if(c == EOF)
			panic("get seed read failed");
		r = (r << 8) | c;
	}
	if(fclose(f) != 0)
		panic("get seed close failed");
	return r;
}

int
main(int argc, char *argv[])
{
	Testcase *t;
	int seed;

	if(argc == 1)
		seed = getseed();
	else
		seed = strtoul(argv[1], 0, 10);
	printf("/* seed %u */\n", seed);
	srand(seed);
	t = randtestcase();
	printf("void abort(void);\n");
	printfunc(t);
	printmain(t);
	return 0;
}
