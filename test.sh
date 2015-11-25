#!/bin/sh

set -e

for T in test/execute/*.c test/cpp/*.c test/bugs/*.c
do
	if ! ( bin/6c $T > $T.s &&
           gcc -c $T.s -o $T.o &&
           gcc $T.o -o $T.bin && 
           $T.bin > /dev/null )
	then
		echo $T FAIL
		exit 1
	fi
	echo $T PASS
done

for T in test/error/*.c test/cpperror/*.c
do
	if bin/6c $T > /dev/null 2> $T.stderr
	then
		echo $T FAIL
		exit 1
	fi
	for P in `sed -n '/^PATTERN:/s/PATTERN://gp' $T`
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
