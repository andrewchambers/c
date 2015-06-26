override CFLAGS += -Isrc/

all: bin/c

%.o: %.c
	tcc $(CFLAGS) -o $@ -c $<

LDSC=$(wildcard src/ds/*.c)
LDSH=$(wildcard src/ds/*.h)
LDSO=$(patsubst %.c, %.o, $(LDSC))
src/ds/ds.a: $(LDSO) $(LDSH)
	ar rvs $@ $(LDSO)

LMEMC=$(wildcard src/mem/*.c)
LMEMH=$(wildcard src/mem/*.h)
LMEMO=$(patsubst %.c, %.o, $(LMEMC))
src/mem/mem.a: $(LMEMO) $(LMEMH)
	ar rvs $@ $(LMEMO)

LCC=$(wildcard src/cc/*.c)
LCCH=$(wildcard src/cc/*.h)
LCCO=$(patsubst %.c, %.o, $(LMEMC))
src/cc/cc.a: $(LMEMO) $(LMEMH)
	ar rvs $@ $(LMEMO)

BCC=$(wildcard src/cmd/c/*.c)
BCH=$(wildcard src/cmd/c/*.h)
BCO=$(patsubst %.c, %.o, $(BCC))
bin/c: $(BCO) $(BCH) src/ds/ds.a src/mem/mem.a src/cc/cc.a
	mkdir -p bin
	tcc $(BCO) src/ds/ds.a src/mem/mem.a src/cc/cc.a -o $@
