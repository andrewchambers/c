
all: bin/c

LCC=$(wildcard src/cmd/c/*.c)
LCH=$(wildcard src/cmd/c/*.h)
LCO=$(patsubst %.c, %.o, $(LCC))
bin/c: $(LCO) $(LCH) src/ds/ds.a
	mkdir -p bin
	gcc src/ds/ds.a $(LCO) -o $@

LDSC=$(wildcard src/ds/*.c)
LDSH=$(wildcard src/ds/*.h)
LDSO=$(patsubst %.c, %.o, $(DSC))
src/ds/ds.a: $(DSO) $(DSH)
	ar rvs $@ $(DSO)

