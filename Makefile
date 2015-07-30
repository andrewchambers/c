HFILES=src/buff/buff.h src/cc/c.h src/ds/ds.h src/mem/mem.h
override CFLAGS += -Isrc/ -g -Wall

all:  bin/c bin/cpp

.PHONY: all

%.o: %.c $(HFILES)
	$(CC) $(CFLAGS) -o $@ -c $<
src/cc/cc.a:  src/cc/cpp.o src/cc/lex.o src/cc/parse.o src/cc/util.o
	$(AR) rs $@  src/cc/cpp.o src/cc/lex.o src/cc/parse.o src/cc/util.o
src/ds/ds.a:  src/ds/list.o src/ds/map.o src/ds/strset.o
	$(AR) rs $@  src/ds/list.o src/ds/map.o src/ds/strset.o
src/mem/mem.a:  src/mem/mem.o
	$(AR) rs $@  src/mem/mem.o
bin/c:  src/cmd/c/emit.o src/cmd/c/main.o  src/cc/cc.a src/ds/ds.a src/mem/mem.a
	mkdir -p bin
	$(CC)  src/cmd/c/emit.o src/cmd/c/main.o  src/cc/cc.a src/ds/ds.a src/mem/mem.a -o $@
bin/cpp:  src/cmd/cpp/main.o  src/cc/cc.a src/ds/ds.a src/mem/mem.a
	mkdir -p bin
	$(CC)  src/cmd/cpp/main.o  src/cc/cc.a src/ds/ds.a src/mem/mem.a -o $@
