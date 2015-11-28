#!/bin/sh

set -e

GCCSRC="src/cc/cpp.c
	src/cc/error.c
	src/panic.c
	src/cmd/6c/emit.c
	src/cmd/6c/frontend.c
	src/cmd/6c/main.c
	src/cc/parse.c"
SELFHOSTSRC="src/ds/list.c
	src/ds/map.c
	src/ds/vec.c
	src/ds/strset.c
	src/gc/gc.c
	src/cc/types.c
	src/cc/foldexpr.c
	src/cc/lex.c"
SELFHOSTOBJDIR=lib/selfhostobj

mkdir -p $SELFHOSTOBJDIR
for F in $GCCSRC
do
	O=$SELFHOSTOBJDIR/`basename $F .c`.o
	gcc -c -I./src/ $F -o $O
done
for F in $SELFHOSTSRC
do
	O=$SELFHOSTOBJDIR/`basename $F .c`.o
	C=$SELFHOSTOBJDIR/`basename $F`
	S=$SELFHOSTOBJDIR/`basename $F .c`.s
	gcc -nostdinc -I./src/ -I./src/selfhost/ -E $F | grep -v "^#" > $C
	bin/6c $C > $S
	gcc -c $S -o $O
done

mkdir -p bin/
gcc $SELFHOSTOBJDIR/*.o -o bin/selfhosted
