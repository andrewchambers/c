# A smaller, faster C compiler suite.

- Fast.
- Consistent.
- Small.
- High quality.
- Low complexity.
- No dependencies.
- No fussy configuration.
- Painless cross compiling.
- Just work.

## Building

Requires an external C compiler and gnu binutils (for now).

```
$ make
```

## Testing
```
$ bash test.sh
```

## Plan

### Stage 1.

Self hosting x86_64, dumb backend.

### Stage 2.

Self hosting arm, something like raspberry pi/android.

### Stage 3.

Build small clean C code bases like 8cc, tcc, sbase.

### Stage 4.

Build musl libc.

### Beyond. 

- Build more programs.
- Replace gnu as with our own assembler.
- Replace ld with our own static linker.
- Build OS kernels.
- Advanced SSA backend.

## Status

Pre stage 1.

See tests for what works.

## Code layout

- Libraries are in src/*
- Commands are in src/cmd/*

If you are unsure about the purpose of a library, check the header which
should give a short description.

## Code style

Follow Plan9 style conventions. Headers are not allowed to include
other headers to eliminate circular dependencies and increase build speed.
src/u.h is the only exception to this rule.

- http://www.lysator.liu.se/c/pikestyle.html
- http://plan9.bell-labs.com/magic/man2html/6/style
- http://aiju.de/b/style

## Bug fixes and issues.

Try and attach a single source file which exibits your issue. If possible
reduce the test case by hand until it is as small as possible.

Try and follow the general template changed where needed:
```
What are you trying to do:

I am trying to compile foo.c and your compiler has ruined my life by deleting all 
my files.

What you expected to happen:

The program works in many other C compilers, so should compile with no issue.

What actually hapened:

/ was deleted while the compiler printed "hahaha" many thousands of times.

Here is a small self contained file which reproduces the issue.
```

In general each bug fix or change should add a test file which triggers the bug.

### Commit messages

General style: 

```
Fix issue #3 post increment.

Post increment had a bug where it decremented instead of incremented.
```

Commits can be squashed to increase clarity.

### Memory management

The compiler assumes a conservative garbage collector.
For now, the header is stubbed out. This simplifies the code.

## Useful Links

- C11 standard final draft http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf
- Dave Prosser's C Preprocessing Algorithm http://www.spinellis.gr/blog/20060626/
- The x86-64 ABI http://www.x86-64.org/documentation/abi.pdf
- http://aiju.de/rant/cross-compiling
- http://bellard.org/tcc/
- https://github.com/rui314/8cc
- http://harmful.cat-v.org/software/
- http://suckless.org/philosophy

