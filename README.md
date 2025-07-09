
# mace

Single-header build system for C.
Uses C to exclusively build C.

Specificity, reduced scope, everything in service of *simplicity*. 

## Features
- C syntax build file.
- Simple API.
- Single header: `mace.h`.
- `make`-like usage, flags with `mace` convenience executable.
- Tab completion (`zsh` only), see `_mace.zsh`
- Compatible with `gcc`, `clang`, `tcc` toolchains.

## How to
### Write a macefile 
1. See [`example_macefile.c`](example_macefile.c)
2. Read `PUBLIC API` section in [`mace.h`](mace.h)  

### Two step build (single-header build)
1. Bootstrap: `gcc macefile.c -o builder`
2. Build: `./builder` 

Use the `MACEFLAGS` environment variable to set default flags.
Same as `MAKEFLAGS` e.g. `export MACEFLAGS=-j12`

### One step build (with mace convenience executable)
0. Install `mace` convenience executable
    1. Bootstrap: `gcc installer.c -o installer`
    2. Install: `./installer`. 
1. Build: `mace`

Flags for `installer` to customize `mace`:
- `-DPREFIX=<path>` to change install path. Defaults to `/usr/local`.
- `-DDEFAULT_MACEFILE=<file>` to change default macefile name. Defaults to `macefile.c`.
- `-DBUILDER=<file>` to change builder executable name. Defaults to `builder`.
- `-DCC=<compiler>` to change compiler used by `mace`. Defaults to `gcc`.
- `-DAR=<archiver>` to change archiver used by `mace`. Defaults to `ar`.
- `-DZSH_COMPLETION=<path>` to set path for `mace` zsh tab completion. Defaults to `/usr/share/zsh/site-functions`.

### Common options
1. Reserved targets: 
    - `<./builder or mace> clean`
    - `<./builder or mace> all`
2. Configs: `<./builder or mace> -g release`
3. Compiler: `<./builder or mace> -c gcc`
4. Macefile: `<./builder or mace> -f my_macefile.c`

## Why?
- Too many complex, slow build systems.
- Using C to build C gets you free lunches.
    - Learning C is learning `mace`.
    - Full C functionality in your macefile.
- Using C to build C gets me free lunches.
    - No weird syntax to create.
    - No bespoke parser to implement.

## Limitations
- POSIX `glob.h` required.
    - On Windows, `Cygwin` or `MSYS2` shells might work. Untested.
- Circular dependencies unsupported.

## Under the hood
- User inputs target dependencies with `target.links` and `target.dependencies`
    - Build order determined by depth first search through all target dependencies.
- Mace saves file checksums to `.sha1` files in `<obj_dir>/src`, `<obj_dir>/include`
    - Uses checksum to check if sources, headers change for recompilation.
- Compiler computes object file dependencies, saved to `.d` files in `<obj_dir>`
    - Parsed into binary `.ho` file for faster reading.

### Running tests
1. `cd` into test folder
2. Compile test:
```bash
gcc --std=iso9899:1999 -O0 -fsanitize=undefined,address -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks -g test.c -o test -I..
```
3. Run tests `./test`

### Running benchmarks
1. `cd` into benchmarks folder
2. Compile test: `gcc benchmarks.c -o bench`
3. Run benchmarks `./benchmarks`

### Known issues
- Can't compile targets if there are no headers?
    - Cannot reproduce in tests

## Credits
Copyright (c) 2025 Gabriel Taillon

- Checksum sha1dc algorithm: [sha1collisiondetection](https://github.com/cr-marcstevens/sha1collisiondetection)
- Argument parser: [parg](https://github.com/jibsen/parg)

Originally created for use in a game I am developing: [Codename: Firesaga](https://gitlab.com/Gabinou/firesagamaker).