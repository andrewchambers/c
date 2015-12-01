
CFLAGS  = -std=c89 -g -Wfatal-errors -Wno-unused-parameter -Wall -Wextra -D_DEFAULT_SOURCE

# NOTE if one of these headers does not exist, the wildcard rule fails.
HFILES = src/u.h src/cc/cc.h src/ds/ds.h src/mem/mem.h
CCO    = src/cc/cpp.o \
         src/cc/lex.o \
         src/cc/parse.o \
         src/cc/types.o \
         src/cc/foldexpr.o \
         src/cc/error.o
GCO    = src/mem/mem.o
DSO    = src/ds/list.o \
         src/ds/map.o \
         src/ds/vec.o \
         src/ds/strset.o
LIBO   = src/panic.o $(CCO) $(GCO) $(DSO)
LIBA   = lib/libcompiler.a
CPPO   = src/cmd/cpp/main.o 
_6CO   = src/cmd/6c/emit.o \
         src/cmd/6c/frontend.o \
         src/cmd/6c/main.o 
ABIFZO = src/cmd/abifuzz/main.o
all:  bin/6c \
      bin/cpp \
      bin/abifuzz

.PHONY: all clean test selfhost

test:
	make clean
	make all
	./test.sh
	
selfhost:
	make clean
	make all
	./test.sh
	./selfhost.sh
	mv ./bin/selfhosted ./bin/6c
	./test.sh
	./selfhost.sh
	mv ./bin/selfhosted ./bin/6c
	./test.sh
	./selfhost.sh
	mv ./bin/selfhosted ./bin/6c
	./test.sh

.c.o: $(HFILES)
	$(CC) $(CFLAGS) -Isrc/ -o $@ -c $<

bin/6c: $(_6CO) $(LIBA)
	@ mkdir -p bin
	$(CC) $(LDFLAGS) $(_6CO) $(LIBA) -o $@

bin/cpp:  $(CPPO) $(LIBA)
	@ mkdir -p bin
	$(CC) $(LDFLAGS) $(CPPO) $(LIBA) -o $@

bin/abifuzz:  $(ABIFZO) $(LIBA)
	@ mkdir -p bin
	$(CC) $(LDFLAGS) $(ABIFZO) $(LIBA) -o $@

$(LIBA): $(LIBO)
	@ mkdir -p lib
	$(AR) rcs $(LIBA) $(LIBO)

clean:
	rm -rf $(LIBO) $(CPPO) $(_6CO) $(ABIFZO) lib bin

