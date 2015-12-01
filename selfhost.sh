#!/bin/sh

set -e

GCCSRC="src/cc/error.c
	src/panic.c
	src/cmd/6c/emit.c"
SELFHOSTSRC="src/ds/list.c
	src/ds/map.c
	src/ds/vec.c
	src/ds/strset.c
	src/gc/gc.c
	src/cc/types.c
	src/cc/foldexpr.c
	src/cc/lex.c
	src/cmd/6c/frontend.c
	src/cc/cpp.c
	src/cmd/6c/main.c
	src/cc/parse.c"
SELFHOSTOBJDIR=lib/selfhostobj

mkdir -p $SELFHOSTOBJDIR
for C in $GCCSRC
do
	O=$SELFHOSTOBJDIR/`basename $C .c`.o
	gcc -c -I./src/ $C -o $O
done
for C in $SELFHOSTSRC
do
	O=$SELFHOSTOBJDIR/`basename $C .c`.o
	S=$SELFHOSTOBJDIR/`basename $C .c`.s
	bin/6c -I src -I src/selfhost -I `dirname $C`  $C > $S
	gcc -c $S -o $O
done

mkdir -p bin/
gcc $SELFHOSTOBJDIR/*.o -o bin/selfhosted
