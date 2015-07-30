# The smaller, faster C compiler suite.

## Design choices

- Be small, fast, just work.
- Maintain small size, high quality, low complexity in code base.

## Building

```
$ make
```

## Quotes

"Gotta go fast" - Sonic
"Mo code Mo problems" - Notorious B.I.G.
"One of my most productive days was throwing away 1000 lines of code." - Ken Thompson

## Style Guidelines

Follow plan9 conventions.

- http://www.lysator.liu.se/c/pikestyle.html
- http://plan9.bell-labs.com/magic/man2html/6/style
- http://aiju.de/b/style

## Tests

The only testing in this repo is the self bootstrap.
All other tests are part of https://github.com/clean-c/cspec

## Useful Links

- C11 standard final draft http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf
- Dave Prosser's C Preprocessing Algorithm http://www.spinellis.gr/blog/20060626/
- The x86-64 ABI http://www.x86-64.org/documentation/abi.pdf
- http://aiju.de/rant/cross-compiling
- http://bellard.org/tcc/
- https://github.com/rui314/8cc
- http://harmful.cat-v.org/software/
