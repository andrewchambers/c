# A small, fast C compiler suite.

NOTE - This project is not being actively developed. Please direct yourself to https://github.com/michaelforney/cc which
is a successor that is more complete. I direct my own fixes to that project instead now.

[![Join the chat at https://gitter.im/andrewchambers/c](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/andrewchambers/c?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

- Small.
- Fast.
- Consistent.
- High quality.
- Low complexity.
- No dependencies.
- No fussy configuration.
- Painless cross compiling.
- Just work.

You should be able to get a C compiler, assembler, linker and libc for any
supported target in less than 30 seconds.

## Building

Requires an external C compiler and gnu binutils (for now), and I have only tested
it on linux 64 bit so far.

The code does use anonymous union extensions, so your compiler will need to support them too.

```
$ make
```

## Testing
```
$ make test
$ make selfhost # self hosting
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
- SSA backend.

## Status

Pre stage 2. Self hosting with lots of missing common cases. Though technically these
bugs can be fixed with the compiler itself :). It uses it's own stubbed out headers
and cannot correctly process system headers yet (Help wanted). 


## Contributing

Project on hold. See https://github.com/michaelforney/cc for a new compiler project that is more complete.

### Code layout

- Libraries are in src/*
- Commands are in src/cmd/*

If you are unsure about the purpose of a library, check the header which
should give a short description.

### Code style

Follow Plan9 style conventions. Headers are not allowed to include
other headers to eliminate circular dependencies and increase build speed.
src/u.h is the only exception to this rule.

- http://www.lysator.liu.se/c/pikestyle.html
- http://plan9.bell-labs.com/magic/man2html/6/style
- http://aiju.de/b/style

### Bug fixes and issues

Try and attach a single source file which exibits your issue. If possible
reduce the test case by hand until it is as small as possible.

Try and follow the general template changed where needed:
```
What are you trying to do:
...
What you expected to happen:
...
What actually hapened:
...
```
Try and add a small self contained file which reproduces the issue.

In general each bug fix or change should add a test file which triggers the bug.

### Memory management

The compiler does not explicitly free memory. Peak memory usage while self hosting
is approximately 2Mb, so it should not be an issue, even for planned targets/hosts like
the raspberry pi.

 This actually simplifies the code and probably makes it faster because allocations can be pointer bumps.

## Useful Links

- C11 standard final draft http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf
- Dave Prosser's C Preprocessing Algorithm http://www.spinellis.gr/blog/20060626/
- The x86-64 ABI http://www.x86-64.org/documentation/abi.pdf
- http://aiju.de/rant/cross-compiling
- http://bellard.org/tcc/
- https://github.com/rui314/8cc
- http://harmful.cat-v.org/software/
- http://suckless.org/philosophy

