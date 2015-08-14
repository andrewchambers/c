set -e
make -Bs

for T in test/execute/*.c test/bugs/*.c
do
	if ! ( bin/c $T > $T.s && gcc $T.s -o $T.bin && $T.bin )
	then
		echo $T FAIL
		exit 1
	fi
	echo $T PASS
done

for T in test/error/*.c
do
	PATTERNS=`grep "^PATTERN:" $T | sed s/PATTERN://g`
	if bin/c $T > /dev/null 2> $T.stderr
	then
		echo $T FAIL
		exit 1
	fi
	for P in $PATTERNS
	do
		if ! grep -q $P $T.stderr
		then
			echo pattern: $P failed
			echo $T FAIL
			exit 1
		fi
	done
	echo $T PASS
done
