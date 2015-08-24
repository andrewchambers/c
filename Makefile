
CFLAGS  = -g -Wall
LDFLAGS = -static

# NOTE if one of these headers does not exist, the wildcard rule fails.
HFILES = src/u.h src/cc/c.h src/ds/ds.h src/gc/gc.h
CCO    = src/cc/cpp.o \
         src/cc/dbg.o \
         src/cc/lex.o \
         src/cc/parse.o \
         src/cc/types.o \
         src/cc/foldexpr.o \
         src/cc/error.o
GCO    = src/gc/gc.o
DSO    = src/ds/list.o \
         src/ds/map.o \
         src/ds/vec.o \
         src/ds/strset.o
LIBO   = src/u.o $(CCO) $(GCO) $(DSO)
LIBA   = lib/libcompiler.a
CPPO   = src/cmd/cpp/main.o 
_6CO   = src/cmd/6c/emit.o \
         src/cmd/6c/main.o 
_6AO   = src/cmd/6a/main.o 

all:  bin/6c \
      bin/6a \
      bin/cpp

.PHONY: all clean

%.o: %.c $(HFILES)
	$(CC) $(CFLAGS) -Isrc/ -o $@ -c $<

bin/6c: $(_6CO) $(LIBA)
	@ mkdir -p bin
	$(CC) $(LDFLAGS) $(_6CO) $(LIBA) -o $@

bin/6a: $(_6AO) $(LIBA)
	@ mkdir -p bin
	$(CC) $(LDFLAGS) $(_6AO) $(LIBA) -o $@

bin/cpp:  $(CPPO) $(LIBA)
	@ mkdir -p bin
	$(CC) $(LDFLAGS) $(CPPO) $(LIBA) -o $@

$(LIBA): $(LIBO)
	@ mkdir -p lib
	ar rcs $(LIBA) $(LIBO) 

clean:
	rm -rf $(LIBA) $(LIBO) $(CPPO) $(_6CO) $(_6AO) bin 
