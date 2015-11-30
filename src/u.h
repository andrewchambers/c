/* Provides the core types and core functions like panic */

/* This is the only header in the code base
   which is allowed to include other headers.
   Keep the list small. */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* XXX uncomment to silence clang static analysis
#define NORETURN __attribute__((__noreturn__)); 
*/
#define NORETURN 

typedef int64_t  int64;
typedef int32_t  int32;
typedef uint64_t uint64;
typedef uint32_t uint32;
typedef unsigned int uint;

void panic(char *fmt, ...) NORETURN;

