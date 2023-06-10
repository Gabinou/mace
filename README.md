
## About

Single-header build system for C.
Uses C to exclusively build C.

Specificity, reduced scope, everything in service of *simplicity*. 

## Features
- C syntax: `macefile.c`.
- Simple API.
- Single header build system: `mace.h`.
- `make`-like usage and flags with `mace` convenience executable.
- Tab completion (`zsh` only), see `_mace.zsh`
- Compatible with gcc, clang, tcc toolchains.

## How to
1. Get `mace.h`
2. Write your own macefile e.g. `macefile.c` ([example](https://github.com/Gabinou/mace/blob/master/example_macefile.c))
3. Compile builder executable `gcc macefile.c -o builder`
4. Build `./builder` 
    1. Reserved targets: `./builder clean`, `./builder all`
    2. Configs: `./builder -g release`

Use the MACEFLAGS environment variable to set default flags.

### Convenience executable
Compile and build with a single `mace` command.

1. Install `mace` convenience executable
    1. Compile `installer.c`: `gcc installer.c -o installer`
    2. Run installer: `sudo ./installer`. 
2. Write your own macefile e.g. `macefile.c`
3. Build `mace`

The `mace` convenience executable also use the environment variable MACEFLAGS.

Use these macro definitions when compiling `installer` to customize `mace`:
- `-DPREFIX=<path>` to change install path. Defaults to `/usr/local`.
- `-DDEFAULT_MACEFILE=<file>` to change default macefile name. Defaults to `macefile.c`.
- `-DBUILDER=<file>` to change builder executable path.
- `-DCC=<compiler>` to compiler used by `mace`. Defaults to `gcc`.
- `-DZSH_COMPLETION=<path>` to set path for `mace` zsh tab completion. Defaults to `/usr/share/zsh/site-functions`.

## Documentation

See the first ~250 lines of `mace.h`
- `Target` and `Config` structs
- Public functions and macros  
- `mace` function

## Why?
- I want a much simpler build system.
    - Complexity bad. Simplicity good.
- I want to build C projects only.
- Using C to build C gets me free lunches.
    - No weird syntax to create.
    - No bespoke parser to implement.
- Using C to build C gets you free lunches.
    - Learning C is learning `mace`.
    - Full C functionality in your macefile.

## Limitations
- Tested on Linux only.
- Cannot deal with circular dependencies.
- C99 and above, C++ not supported.

## Under the hood
- User inputs target dependencies with `target.links` and `target.dependencies`
    - Build order determined by depth first search through all target dependencies.
- Mace saves file checksums to `.sha1` files in `<obj_dir>/src`, `<obj_dir>/include`
    - Uses checksum to check if sources, headers change for recompilation.
- Compiler computes object file dependencies, saved to `.d` files in `<obj_dir>`
    - Parsed into binary `.ho` file for faster reading.

### Lines
- mace.h: ~6400 Lines
    - parg:     ~600 lines
    - sha1dc:   ~3000 lines
    - mace:     ~3800 lines

### Running tests
1. `cd` into test folder
2. Compile test: `gcc test.c -o test -I..`
3. Run tests `./test`

### Running benchmarks
1. `cd` into benchmarks folder
2. Compile test: `gcc benchmarks.c -o bench`
3. Run benchmarks `./benchmarks`

## Credits
- Inspiration for this project: [mage](https://github.com/magefile/mage)
- Checksum sha1dc algorithm: [sha1collisiondetection](https://github.com/cr-marcstevens/sha1collisiondetection)
- Argument parser: [parg](https://github.com/jibsen/parg)
 