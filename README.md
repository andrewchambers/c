# A better C compiler

How? Unlike gcc and clang...

- Be fast.
- Be lightweight.
- Be tidy.
- Fit in your head.

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
