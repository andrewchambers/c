set -e
make -Bs

for T in test/c/*.c
do
	if ! ( bin/c $T > $T.s && gcc $T.s -o $T.bin && timeout 1s $T.bin )
	then
		echo $T FAIL
		exit 1
	fi
	echo $T PASS
done
