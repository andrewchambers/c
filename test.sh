set -e
mkdir -p ./test/
for CFILE in *.c
do
    tcc -D'__asm__(x)=' -E $CFILE | grep -v "^#" > ./test/$CFILE
    echo ./test/$CFILE
    ./c ./test/$CFILE
done
