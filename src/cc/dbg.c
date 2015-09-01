#include <u.h>
#include <ds/ds.h>
#include "c.h"

static void
_dumpty(CTy *ty)
{
	int i, issigned;
	StructMember *sm;
	CTy *subt;

	fprintf(stderr, "(type ");
	switch(ty->t) {
	case CPRIM:
		issigned = ty->Prim.issigned;
		switch(ty->Prim.type){
		case PRIMCHAR:
			fprintf(stderr, issigned ? "char" : "uchar");
			break;
		case PRIMSHORT:
			fprintf(stderr, issigned ? "short" : "ushort");
			break;
		case PRIMINT:
			fprintf(stderr, issigned ? "int" : "uint");
			break;
		case PRIMLONG:
			fprintf(stderr, issigned ? "long" : "ulong");
			break;
		case PRIMLLONG:
			fprintf(stderr, issigned ? "llong" : "ullong");
			break;
		case PRIMFLOAT:
			fprintf(stderr, "float");
			break;
		case PRIMDOUBLE:
			fprintf(stderr, "double");
			break;
		case PRIMLDOUBLE:
			fprintf(stderr, "ldouble");
			break;
		default:
			panic("_dumpty");
		}
		break;
	case CPTR:
		fprintf(stderr, "(ptr ");
		_dumpty(ty->Ptr.subty);
		fprintf(stderr, ")");
		break;
	case CFUNC:
		fprintf(stderr, "(func");
		fprintf(stderr, " (");
		for(i = 0; i < ty->Func.params->len; i++) {
			subt = vecget(ty->Func.params, i);
			fprintf(stderr, " ");
			_dumpty(subt);
		}
		fprintf(stderr, ")");
		if(ty->Func.isvararg)
			fprintf(stderr, " ...");
		fprintf(stderr, " -> ");
		_dumpty(ty->Func.rtype);
		fprintf(stderr, ")");
		break;
	case CSTRUCT:
		fprintf(stderr, "(%s ", ty->Struct.isunion ? "union" : "struct");
		for(i = 0; i < ty->Struct.members->len; i++) {
			sm = vecget(ty->Struct.members, i);
			fprintf(stderr, "(");
			fprintf(stderr, " %s ", sm->name);
			_dumpty(sm->type);
			fprintf(stderr, ")");
		}
		_dumpty(ty->Ptr.subty);
		fprintf(stderr, ")");
		break;
	case CARR:
		fprintf(stderr, "carray");
		break;
	default:
		panic("_dumpty");
	}
	fprintf(stderr, ")");
}

void
dumpty(CTy *ty)
{
	_dumpty(ty);
	fprintf(stderr, "\n");
}
