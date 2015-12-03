#!/bin/sh

set -e

SELFHOSTSRC="src/cc/cpp.c
	src/cc/error.c
	src/cc/foldexpr.c
	src/cc/lex.c
	src/cc/parse.c
	src/cc/types.c
	src/cmd/6c/emit.c
	src/cmd/6c/frontend.c
	src/cmd/6c/main.c
	src/ds/list.c
	src/ds/map.c
	src/ds/strset.c
	src/ds/vec.c
	src/mem/mem.c
	src/panic.c"
SELFHOSTOBJDIR=lib/selfhostobj
mkdir -p lib/selfhostobj

for C in $SELFHOSTSRC
do
	O=$SELFHOSTOBJDIR/`basename $C .c`.o
	S=$SELFHOSTOBJDIR/`basename $C .c`.s
	bin/6c -I src -I src/selfhost -I `dirname $C`  $C > $S
	gcc -c $S -o $O
done

mkdir -p bin/
gcc $SELFHOSTOBJDIR/*.o -o bin/selfhosted
