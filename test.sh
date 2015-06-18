set -e
mkdir -p ./test/
for CFILE in *.c
do
    gcc -E -P $CFILE > ./test/$CFILE
    ./c ./test/$CFILE
done
