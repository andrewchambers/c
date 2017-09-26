set -e

for t in test/execute/*.c
do
	echo -n "$t "
	
	if !  ./cc -I./test/execute/include $t > $t.ssa
	then
		echo "cc failed"
		exit 1
	fi
	
	if !  qbe $t.ssa > $t.s
	then
		echo "qbe failed"
		exit 1
	fi
	
	if !  cc -fno-pie -static $t.s -o $t.bin
	then
		echo "assembling failed"
		exit 1
	fi
	
	if !  $t.bin 
	then
		echo "test returned non zero"
		exit 1
	fi
	
	echo "ok"
done

for t in test/error/*.c
do
	echo -n "$t "
	
	if  ./cc $t > /dev/null 2> $t.stderr
	then
		echo "fail cc returned zero"
		exit 1
	fi
	
	for p in `sed -n '/^PATTERN:/s/PATTERN://gp' $t`
	do
		if ! grep -q $p $t.stderr
		then
			echo "fail pattern $p not found"
			exit 1
		fi
	done
	
done
