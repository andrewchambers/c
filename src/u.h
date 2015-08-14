/* <u.h> is named after the plan9 u.h.
   This is the only header in the compiler suite
   which is allowed to include other headers.
   Keep the list small for performance reasons. */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

typedef int64_t  int64;
typedef int32_t  int32;
typedef uint64_t uint64;
typedef uint32_t uint32;
typedef unsigned int uint;

void panic(char *fmt, ...);

