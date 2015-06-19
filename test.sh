set -e
mkdir -p ./test/
for CFILE in *.c
do
	echo "extern int __builtin_bswap32(int);" > ./test/$CFILE
    gcc -D__extension__="" -D__inline="" -D__restrict="" -D'__attribute__(x)=' -D__const=const -E -P $CFILE >> ./test/$CFILE
    echo ./test/$CFILE
    ./c ./test/$CFILE
done
