
HFILES = src/buff/buff.h src/cc/c.h src/ds/ds.h src/mem/mem.h
CCO    = src/cc/cpp.o \
         src/cc/lex.o \
         src/cc/parse.o \
         src/cc/util.o 
MEMO   = src/mem/mem.o
DSO    = src/ds/list.o \
         src/ds/map.o \
         src/ds/strset.o
LIBO   = $(CCO) $(MEMO) $(DSO)
CO     = src/cmd/c/emit.o \
         src/cmd/c/main.o 
CPPO   = src/cmd/cpp/main.o 

all:  bin/c bin/cpp

.PHONY: all clean

%.o: %.c $(HFILES)
	$(CC) -Isrc/ $(CFLAGS) -o $@ -c $<

bin/c: $(CO) $(LIBO)
	@ mkdir -p bin
	$(CC) $(CO) $(LIBO) -o $@

bin/cpp:  $(CPPO) $(LIBO)
	@ mkdir -p bin
	$(CC) $(CPPO) $(LIBO) -o $@

clean:
	rm -rf $(LIBO) $(CPPO) $(CO) bin 
