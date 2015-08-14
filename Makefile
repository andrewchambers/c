
CFLAGS = -g -Wall

# NOTE if one of these headers does not exist, the wildcard rule fails.
HFILES = src/u.h src/cc/c.h src/ds/ds.h src/gc/gc.h
CCO    = src/cc/cpp.o \
         src/cc/lex.o \
         src/cc/parse.o \
         src/cc/types.o \
         src/cc/error.o
GCO    = src/gc/gc.o
DSO    = src/ds/list.o \
         src/ds/map.o \
         src/ds/vec.o \
         src/ds/strset.o
LIBO   = src/u.o $(CCO) $(GCO) $(DSO)
CO     = src/cmd/c/emit.o \
         src/cmd/c/main.o 
CPPO   = src/cmd/cpp/main.o 

all:  bin/c bin/cpp

.PHONY: all clean

%.o: %.c $(HFILES)
	$(CC) $(CFLAGS) -Isrc/ -o $@ -c $<

bin/c: $(CO) $(LIBO)
	@ mkdir -p bin
	$(CC) $(CO) $(LIBO) -o $@

bin/cpp:  $(CPPO) $(LIBO)
	@ mkdir -p bin
	$(CC) $(CPPO) $(LIBO) -o $@

clean:
	rm -rf $(LIBO) $(CPPO) $(CO) bin 
