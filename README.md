
## About

Single-header build system for C.
Uses C to exclusively build C.

Specificity, reduced scope, everything in service of *simplicity*. 

## Features
- C syntax.
    - Macefiles are `.c` files.
    - Targets are `structs`. 
    - Function `mace` is user entry point.
- Simple API
    - Add target with `MACE_ADD_TARGET`
    - Set compiler with `MACE_SET_COMPILER`
    - Set output directories with `MACE_SET_BUILD_DIR`, `MACE_SET_OBJ_DIR`
    - Set default target with `MACE_DEFAULT_TARGET`
- Single header build system : `mace.h` is all you need.
    - Compiling macefile as easy as compiling `hello_world.c`, or even easier!
    - Build project by running resulting executable.
- Convenience executable for `make`-like behavior
    - Build and run macefile `installer.c` to install `mace` and `mace.h`
    - Run `mace` to build!
- Similar performance to make
    - Faster at high `-j` count, using fewer resources.

## Usage
1. Get `mace.h`
2. Write your own macefile e.g. `macefile.c`
3. Compile builder executable `gcc macefile.c -o build`
4. Build `./builder` 
    1. Same as `./builder all` by default.
    2. Remove all objects and targets: `./builder clean`
    3. Usage mostly the same as `make`

### Example macefile
```c
#include "mace.h"

/* CC might be set when calling the compiler e.g. with "-DCC=tcc" */
#ifndef CC
    #define CC gcc
#endif

/******************************* WARNING ********************************/
/* 1. main is defined in mace.h                                         */
/* 2. arguments from main are passed to mace                            */
/* 3. mace function is declared in mace.h, MUST be defined by user      */
/*======================================================================*/
int mace(int argc, char *argv[]) {
    MACE_SET_COMPILER(CC)
    MACE_SET_BUILD_DIR(build);
    MACE_SET_OBJ_DIR(obj);

    // Note: 'clean' and 'all' are reserved target names with expected behavior.
    struct Target foo = { /* Unitialized values guaranteed to be 0 / NULL */
        /* Default separator is ',', but can be set with MACE_SET_SEPARATOR */
        .includes           = "include,include/sub/a.h",
        .sources            = "src,src/sub/*",
        .base_dir           = "foo",
        .kind               = MACE_STATIC_LIBRARY,
    };
    MACE_ADD_TARGET(foo);

    // Change default target from 'all' to input.
    MACE_DEFAULT_TARGET(foo);
}

```

### Convenience executable
1. Install `mace` convenience executable
    1. Compile `installer.c` macefile: `gcc installer.c -o installer`
    2. Run installer: `sudo ./installer`. 
2. Write your own macefile e.g. `macefile.c`
3. Build `mace`
    1. Default macefile: `macefile.c`
    2. Usage mostly the same as `make`

Use these macro definitions when compiling `installer` to customize `mace`:
- `-DPREFIX=<path>` to change install path. Defaults to `/usr/local`.
- `-DDEFAULT_MACEFILE=<file>` to change default macefile name. Defaults to `macefile.c`.
- `-DBUILDER=<file>` to change builder executable path.
- `-DCC=<compiler>` to compiler used by `mace`. Defaults to `gcc`.

## Why?
- I want a much simpler build system.
    - Complexity bad. Simplicity good.
    - Simpler to develop, simpler to use, as simple as possible.
- I want to build C projects, and only C projects.
    - Generality breeds complexity.
- Using C to build C gets me free lunches.
    - No weird syntax to create.
    - No bespoke parser to implement: C compilers already exist!
- Using C to build C gets you free lunches! 
    - C syntax is widely known.
    - If you know how to compile one C file, you can use `mace`.

## Limitations
- Tested on Linux only.
- Cannot deal with circular dependencies.
- C only, C99 and above, C++ not supported.

## Under the hood
- User inputs target dependencies with `target.links` and `target.dependencies`
    - Build order determined by depth first search through all target dependencies.
- Mace saves file checksums to `.sha1` files in `obj_dir`
    - Checksum recorded to `.sha1` files in `src` and `include`
- Compiler computes object file dependencies, saved to `.d` files in `obj_dir`
    - Parsed into binary `.ho` file for faster reading.
    - Check if any header file changed to recompile.
### Lines
- mace.h: ~6300 Lines
    - parg:     ~600 lines
    - sha1cd:   ~3000 lines
    - mace:     ~3700 lines

### Running tests
1. `cd` into test folder
2. Compile test: `gcc test.c -o test -I..`
3. Run tests `./test`

## Credits
- Inspiration for this project: [mage](https://github.com/magefile/mage)
- Checksum sha1dc algorithm: [sha1collisiondetection](https://github.com/cr-marcstevens/sha1collisiondetection)
- Argument parser [parg](https://github.com/jibsen/parg)
