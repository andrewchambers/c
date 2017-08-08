set -e



for t in test/execute/*.c
do
	echo -n "$t "
	
	if ! timeout 2s ./cc -I./test/execute/include $t > $t.ssa
	then
		echo "cc failed"
		continue
	fi
	
	if ! timeout 2s qbe $t.ssa > $t.s
	then
		echo "qbe failed"
		continue
	fi
	
	if ! timeout 2s cc -fno-pie -static $t.s -o $t.bin
	then
		echo "assembling failed"
		continue
	fi
	
	if ! timeout 2s $t.bin 
	then
		echo "test returned non zero"
		continue
	fi
	
	echo "ok"
done

for t in test/error/*.c
do
	echo -n "$t "
	
	if timeout 2s ./cc $t > /dev/null 2> $t.stderr
	then
		echo "fail cc returned zero"
		continue
	fi
	
	status=pass
	for p in `sed -n '/^PATTERN:/s/PATTERN://gp' $t`
	do
		if ! grep -q $p $t.stderr
		then
			echo "fail pattern $p not found"
			status=fail
			break
		fi
	done
	
	if [ $status = pass ]
	then
		echo "ok"
	fi
done
