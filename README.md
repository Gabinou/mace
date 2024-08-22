
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
- Compatible with `gcc`, `clang`, `tcc` toolchains.

## How to: Single-header build 
1. Get `mace.h`
2. Write your own macefile e.g. [`macefile.c`](https://github.com/Gabinou/mace/blob/master/example_macefile.c)
3. Bootstrapping: `gcc macefile.c -o builder`
4. Building: `./builder` 
    1. Reserved targets: `./builder clean`, `./builder all`
    2. Configs: `./builder -g release`

Use the MACEFLAGS environment variable to set default flags.
 
### How to: Make-like build
1. Install `mace` convenience executable
    1. Bootstrapping: `gcc installer.c -o installer`
    2. Run installer: `sudo ./installer`. 
2. Write your own macefile e.g. `macefile.c`
3. Bootstrapping & building: `mace`

Use these macro definitions when compiling `installer` to customize `mace`:
- `-DPREFIX=<path>` to change install path. Defaults to `/usr/local`.
- `-DDEFAULT_MACEFILE=<file>` to change default macefile name. Defaults to `macefile.c`.
- `-DBUILDER=<file>` to change builder executable path.
- `-DCC=<compiler>` to change compiler used by `mace`. Defaults to `gcc`.
- `-DAR=<archiver>` to change archiver used by `mace`. Defaults to `ar`.
- `-DZSH_COMPLETION=<path>` to set path for `mace` zsh tab completion. Defaults to `/usr/share/zsh/site-functions`.

## Why?
- Too many build complex, slow build systems.
- Using C to build C gets you free lunches.
    - Learning C is learning `mace`.
    - Full C functionality in your macefile.
- Using C to build C gets me free lunches.
    - No weird syntax to create.
    - No bespoke parser to implement.

## Limitations
- POSIX glob.h required.
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
2. Compile test: `gcc test.c -o test -I..`
3. Run tests `./test`

### Running benchmarks
1. `cd` into benchmarks folder
2. Compile test: `gcc benchmarks.c -o bench`
3. Run benchmarks `./benchmarks`

### Known issues
- Can't compile targets if there are no headers?
    - Cannot reproduce in tests

## Credits
- Inspiration for this project: [mage](https://github.com/magefile/mage)
- API inspired by [Premake5](https://premake.github.io/).
- Checksum sha1dc algorithm: [sha1collisiondetection](https://github.com/cr-marcstevens/sha1collisiondetection)
- Argument parser: [parg](https://github.com/jibsen/parg)
 
