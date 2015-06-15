# A better C compiler

How? Do the opposite of gcc and clang.

- Be fast.
- Be lightweight.
- Be tidy.
- Fit in your head.
- Bring joy to people.

Why? Lets build a linux system from scratch and find out.

i7 workstation with 16 gigs of ram:
```
$ time make linux
real	4m25.821s
user	3m53.863s
sys	0m38.541s
$ time make gcc
real	7m49.989s
user	13m34.639s
sys	1m54.663s
```

Raspberry Pi 2 Model B:
```
$ time make linux
HAHAHAHAHHAHAH.
$ time make gcc
HAHAHAH. P.S. OUT OF RAM.
```

Qemu System Emulator:
```
$ time make linux
infinity.
$ time make gcc
2 x infinity.
```

Multiply all of these by the number of cpu architectures you want and by 2 if you use a laptop.

## Design choices

- We should be able to be *MUCH* faster than gcc, 
but still generate decent code. Choose fast algorithms. They will also make the code simpler.
- Die on error, I don't need to see 1 good error and 50 junk errors caused by the first.
- Few/No warnings. Building is different to checking, if you want warnings, use something like clang static checker.

## Style Guidelines

- http://www.lysator.liu.se/c/pikestyle.html
- http://plan9.bell-labs.com/magic/man2html/6/style
- http://aiju.de/b/style

## Reference

- http://aiju.de/rant/cross-compiling
- http://harmful.cat-v.org/software/GCC
- http://bellard.org/tcc/
- https://github.com/rui314/8cc
- C11 standard final draft http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf
- Dave Prosser's C Preprocessing Algorithm http://www.spinellis.gr/blog/20060626/
- The x86-64 ABI http://www.x86-64.org/documentation/abi.pdf
