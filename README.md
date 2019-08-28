# An Implementation of the Synacor Challenge -- In C

Link to challenge: https://challenge.synacor.com/

## Build instructions

```
meson bld
ninja -C bld
./bld/syn-run challenge.bin
```

## Playing with the code

Define `DEBUG` in  `main.c` to enable debug output

Also see my other repo in which I implemented a synacor disassembler: https://github.com/pdietl/synacor-disass
