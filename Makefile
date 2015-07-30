override CFLAGS += -Isrc/ -g -Wall

.PHONY: all

all: bin/c bin/cpp

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

LDSC=$(wildcard src/ds/*.c)
LDSH=$(wildcard src/ds/*.h)
LDSO=$(patsubst %.c, %.o, $(LDSC))
src/ds/ds.a: $(LDSO) $(LDSH)
	ar rs $@ $(LDSO)

LMEMC=$(wildcard src/mem/*.c)
LMEMH=$(wildcard src/mem/*.h)
LMEMO=$(patsubst %.c, %.o, $(LMEMC))
src/mem/mem.a: $(LMEMO) $(LMEMH)
	ar rs $@ $(LMEMO)

LCCC=$(wildcard src/cc/*.c)
LCCH=$(wildcard src/cc/*.h)
LCCO=$(patsubst %.c, %.o, $(LCCC))
src/cc/cc.a: $(LCCO) $(LCCH)
	ar rs $@ $(LCCO)

BCC=$(wildcard src/cmd/c/*.c)
BCH=$(wildcard src/cmd/c/*.h)
BCO=$(patsubst %.c, %.o, $(BCC))
bin/c: $(BCO) $(BCH) src/cc/cc.a src/ds/ds.a src/mem/mem.a
	mkdir -p bin
	$(CC) $(BCO) src/cc/cc.a src/ds/ds.a src/mem/mem.a -o $@

BCPPC=$(wildcard src/cmd/cpp/*.c)
BCPPH=$(wildcard src/cmd/cpp/*.h)
BCPPO=$(patsubst %.c, %.o, $(BCPPC))
bin/cpp: $(BCPPO) $(BCPPH) src/cc/cc.a src/ds/ds.a src/mem/mem.a
	mkdir -p bin
	$(CC) $(BCPPO) src/cc/cc.a src/ds/ds.a src/mem/mem.a -o $@
