#!/bin/sh

set -e

GCCSRC="src/cc/cpp.c
	src/cc/error.c
	src/cc/types.c
	src/cmd/6c/emit.c
	src/cmd/6c/frontend.c
	src/panic.c
	src/cc/foldexpr.c
	src/cc/parse.c"

QCSRC="src/ds/list.c
	src/ds/strset.c
	src/ds/map.c
	src/mem/mem.c
	src/ds/vec.c
	src/cc/lex.c
	src/cmd/6c/main.c"

OBJDIR=lib/qcobj
mkdir -p $OBJDIR

for C in $GCCSRC
do
	O=$OBJDIR/`basename $C .c`.o
	gcc -c -I src $C -o $O
done

for C in $QCSRC
do
	SSA=$OBJDIR/`basename $C .c`.ssa
	S=$OBJDIR/`basename $C .c`.s
	O=$OBJDIR/`basename $C .c`.o
	/home/ac/src/qc/qc -I src -I src/selfhost -I `dirname $C` $C > $SSA
	qbe < $SSA > $S
	gcc -c $S -o $O
done

mkdir -p bin/
gcc $OBJDIR/*.o -o bin/qchosted
