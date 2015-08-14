# The smaller, faster C compiler suite.


## Goals

- Small size.
- High quality.
- Low complexity.
- No external dependencies.
- No fussy configuration.
- Just work.
- Fast.

The inspiration for this compiler came after dealing with building LLVM and GCC in
many configurations. Each version taking 30 minutes between attempts.

We should aim to compile as much real software as possible, and not prematurely
focus on optimizations. Building real software asap is an early goal, because all other goals
will rely on having a real user base to sustain development.

## Plan

### Stage 1.

First, develop the C compiler and C preprocessor to the point it can compile itself, using
gnu binutils as its assembler/linker. At this stage it will use a tcc/8cc style 
backend and won't do any optimization at all.

### Stage 2.

Focus on building as much real world BSD+Linux software. The OpenBSD community might
be the most welcoming and appreciative of the efforts, so building OpenBSD packages should
be a strong focus. This stage ends when we can build hundreds of open source packages.

During this stage ports to more architectures can be started. The style of ports and
cross compiling will be inspired by the plan9 compilers. A gcc compatible driver can be added
as a utility.

### Stage 3.

Replace the assembler with an in tree assembler. This assembler will have to follow
binutils syntax simply because we want to support inline assembly which is already
written.

### Stage 4.

Rewrite backend to be a simplified SSA or Three address IR backend. Focus on good
but small/fast algorithms.

## Building

```
$ make
```

## Quotes

- "Gotta go fast" - Sonic
- "Mo code Mo problems" - Notorious B.I.G.
- "One of my most productive days was throwing away 1000 lines of code." - Ken Thompson

## Style

Follow plan9 conventions. Headers are not allowed to include
other headers. src/u.h is the only exception to this rule.

- http://www.lysator.liu.se/c/pikestyle.html
- http://plan9.bell-labs.com/magic/man2html/6/style
- http://aiju.de/b/style

## Memory management

The compiler assumes a conservative garbage collector.
For now, the header is stubbed out, but this isn't a
problem for now.

## Useful Links

- C11 standard final draft http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf
- Dave Prosser's C Preprocessing Algorithm http://www.spinellis.gr/blog/20060626/
- The x86-64 ABI http://www.x86-64.org/documentation/abi.pdf
- http://aiju.de/rant/cross-compiling
- http://bellard.org/tcc/
- https://github.com/rui314/8cc
- http://harmful.cat-v.org/software/
- http://suckless.org/philosophy
