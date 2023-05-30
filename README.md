
## About

Single-header build system for C.
Uses C to exclusively build C.

Specificity, reduced scope, everything in service of *simplicity*. 

## Features
- C syntax.
    - Macefiles are `.c` files.
    - Targets are `structs`. 
    - Function `mace` is user entry point.
- Single header build system : `mace.h` is all you need.
    - Compiling macefile as easy as compiling `hello_world.c`, or easier!
    - Build project by running resulting executable.
- Simple API
    - Add target with `MACE_ADD_TARGET`
    - Set compiler with `MACE_SET_COMPILER`
    - Set output directories with `MACE_SET_BUILD_DIR`, `MACE_SET_OBJ_DIR`
    - Set default target with `MACE_DEFAULT_TARGET`
- Convenience executable for make-like behavior
    - Build and run macefile `installer.c` to install `mace` and `mace.h`
    - `mace macefile.c` to build!

## Usage
1. Get `mace.h`
2. Write your own macefile e.g. `macefile.c`
3. Compile builder executable `gcc macefile.c -o build`
4. Build `./build` 
    1. Same as `./build all` by default.
    2. Remove all objects and targets: `./build clean`
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
        /* Default separator is ' ', but can be set with MACE_SET_SEPARATOR */
        .includes           = "include include/sub/a.h",
        .sources            = "src src/sub/*",
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
- To build my personal C projects with.
    - Tried: make, premake, please, cmake...
    - Couldn't find anything that uses C to build C.
- I want it simple to use, with simple internals.
    - Complexity bad. Simplicity good.
- No well known build system for C is truly good.
    - Proof 1: Ostensibly general-purpose build systems (`make`) are never used to build anything other than C/C++ projects.
    - Proof 2: Modern programming languages devs implement their own build systems: Rust, Zig, Go, etc.
    - Proof 3: Ask random C/C++ developers about `make`, `CMake`, etc.
- Most build systems have obtuse syntax, scale terribly to larger projects.
    - Makefiles makers exist (`premake`, `autoconf`/`autotools`) and compound this issue.
    - Mix of imperative and declarative style.
    - Personal experience: homemade `makefile` need constant updates and break in unexpected ways if the project structure changes slightly.

## Limitations
- Tested on Linux only.
- Cannot deal with circular dependencies in linked libraries.
- C only, C99 and above, C++ not supported.

## Under the hood
- Compiler spits out object file dependecies (headers)
- User inputs target dependencies with `target.links` (libraries or other targets)
    - Build order determined by depth first search through dependencies.
- Mace saves file and flag checksums in `.mace_checksums`
    - Recompile if checksum changed

## Credits
- Inspiration for this project: [mage](https://github.com/magefile/mage)
- Checksum sha1dc algorithm: [sha1collisiondetection](https://github.com/cr-marcstevens/sha1collisiondetection)
- Argument parser [parg](https://github.com/jibsen/parg)
