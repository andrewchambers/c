#include "u.h"
#include "ds/ds.h"
#include "cc/c.h"

uint64
getmaxval(CTy *l)
{
	if(l->t == CENUM)
		return 0xffffffff;
	switch(l->Prim.type) {
	case PRIMCHAR:
		if(l->Prim.issigned)
			return 0x7f;
		else
			return 0xff;
	case PRIMSHORT:
		if(l->Prim.issigned)
			return 0x7fff;
		else
			return  0xffff;
	case PRIMINT:
	case PRIMLONG:
		if(l->Prim.issigned)
			return 0x7fffffff;
		else
			return 0xffffffff;
	case PRIMLLONG:
		if(l->Prim.issigned)
			return 0x7fffffffffffffff;
		else
			return 0xffffffffffffffff;
	}
	panic("internal error");
	return 0;
}

int64
getminval(CTy *l)
{
	if(l->t == CENUM)
		return -2147483648LL;
	if(!l->Prim.issigned)
		return 0;
	switch(l->Prim.type) {
	case PRIMCHAR:
		return -128LL;
	case PRIMSHORT:
		return -32768LL;
	case PRIMINT:
	case PRIMLONG:
		return -2147483648LL;
	case PRIMLLONG:
		return (int64)1 << 63;
	}
	panic("internal error");
	return 0;
}

CTy *cvoid    = &(CTy){ .t = CVOID, .incomplete = 1};
CTy *cchar    = &(CTy){ .t = CPRIM, .size = 1, .align = 1, .Prim = {.type = PRIMCHAR, .issigned = 1}};
CTy *cshort   = &(CTy){ .t = CPRIM, .size = 2, .align = 2, .Prim = {.type = PRIMSHORT, .issigned = 1}};
CTy *cint     = &(CTy){ .t = CPRIM, .size = 4, .align = 4, .Prim = {.type = PRIMINT, .issigned = 1}};
CTy *clong    = &(CTy){ .t = CPRIM, .size = 8, .align = 8, .Prim = {.type = PRIMLONG, .issigned = 1}};
CTy *cllong   = &(CTy){ .t = CPRIM, .size = 8, .align = 8, .Prim = {.type = PRIMLLONG, .issigned = 1}};
CTy *cuchar   = &(CTy){ .t = CPRIM, .size = 1, .align = 1, .Prim = {.type = PRIMCHAR}};
CTy *cushort  = &(CTy){ .t = CPRIM, .size = 2, .align = 2, .Prim = {.type = PRIMSHORT}};
CTy *cuint    = &(CTy){ .t = CPRIM, .size = 4, .align = 4, .Prim = {.type = PRIMINT}};
CTy *culong   = &(CTy){ .t = CPRIM, .size = 8, .align = 8, .Prim = {.type = PRIMLONG}};
CTy *cullong  = &(CTy){ .t = CPRIM, .size = 8, .align = 8, .Prim = {.type = PRIMLLONG}};
CTy *cfloat   = &(CTy){ .t = CPRIM, .size = 8, .align = 8, .Prim = {.type = PRIMFLOAT}};
CTy *cdouble  = &(CTy){ .t = CPRIM, .size = 8, .align = 8, .Prim = {.type = PRIMDOUBLE}};
CTy *cldouble = &(CTy){ .t = CPRIM, .size = 8, .align = 8, .Prim = {.type = PRIMLDOUBLE}};

