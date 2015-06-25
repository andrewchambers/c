# The smaller, faster C compiler

Legend says this compiler was written while clang and gcc were bootstrapping themselves. The preprocessor came later, during a linux kernel build.

## Quotes

"Gotta go fast" - Sonic

"One of my most productive days was throwing away 1000 lines of code." - Ken Thompson


## Design choices

- Be very small, very fast, very sane.
- Build real software.
- Cross compiling.
- Use fast algorithms, keep compilation speed and code complexity in mind.
- Die on error. Only the first error means anything anyway.
- Few/No warnings. Use a static checker for warnings.

## Style Guidelines

- http://www.lysator.liu.se/c/pikestyle.html
- http://plan9.bell-labs.com/magic/man2html/6/style
- http://aiju.de/b/style

## Useful Links

- C11 standard final draft http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf
- Dave Prosser's C Preprocessing Algorithm http://www.spinellis.gr/blog/20060626/
- The x86-64 ABI http://www.x86-64.org/documentation/abi.pdf
- http://aiju.de/rant/cross-compiling
- http://bellard.org/tcc/
- https://github.com/rui314/8cc

