set -e
mkdir -p ./test/
for CFILE in *.c
do
    tcc -E $CFILE | grep -v "^#" > ./test/$CFILE
    echo ./test/$CFILE
    ./c ./test/$CFILE
done
