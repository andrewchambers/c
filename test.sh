set -e
mkdir -p ./test/
for CFILE in *.c
do
    echo "int __builtin_frame_address(int);" > ./test/$CFILE
    tcc -D'__asm__(x)=' -E $CFILE | grep -v "^#" >> ./test/$CFILE
    echo ./test/$CFILE
    ./c ./test/$CFILE
done
