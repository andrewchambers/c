set -e
make -Bs

for T in test/c/*
do
	if ! bin/c $T
	then
		echo $T failed
		exit 1
	fi
done
