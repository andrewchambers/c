set -e
mkdir -p ./test/
for CFILE in *.c
do
    gcc -D'__attribute__(x)=' -D__const=const -E -P $CFILE > ./test/$CFILE
    ./c ./test/$CFILE
done
