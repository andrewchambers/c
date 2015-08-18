
CFLAGS  = -g -Wall
LDFLAGS = -static

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

bin/6c: $(_6CO) $(LIBO)
	@ mkdir -p bin
	$(CC) $(LDFLAGS) $(_6CO) $(LIBO) -o $@

bin/6a: $(_6AO) $(LIBO)
	@ mkdir -p bin
	$(CC) $(LDFLAGS) $(_6AO) $(LIBO) -o $@

bin/cpp:  $(CPPO) $(LIBO)
	@ mkdir -p bin
	$(CC) $(LDFLAGS) $(CPPO) $(LIBO) -o $@

clean:
	rm -rf $(LIBO) $(CPPO) $(_6CO) $(_6AO) bin 
