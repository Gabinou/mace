
## About

Mace is a build system for C, that uses C.

Specificity, reduced scope, all in service of *simplicity*. 

## Features
- As simple as possible.
- C syntax.
    - Macefiles are `.c` files.
    - Targets are `structs`. 
    - Function `mace` is user entry point.
- Single header build system.
    - Compiling macefile as easy as compiling `hello_world.c`, or easier!
    - Build project by running resulting executable.
- Easy to learn
    - Familiar syntax.
    - Simple API.

## Usage
1. Get `mace.h`
2. Write your own macefile e.g. `macefile.c`
3. Compile builder executable `gcc macefile.c -o build`
4. Build `./build`

### Example macefile
```c
#include "mace.h"

#define CC gcc

/******************************* WARNING ********************************/
// 1. main is defined in mace.h                                         //
// 2. arguments from main are passed to mace directly                   //
// 3. mace function is declared in mace.h, MUST be implemented by user  //
/*======================================================================*/
int mace(int argc, char *argv[]) {
    MACE_SET_COMPILER(CC)
    struct Target tnecs = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .kind               = MACE_STATIC_LIBRARY,
    };
    MACE_ADD_TARGET(tnecs);
}

```

### Convenience executable
1. Install `mace` convenience executable
    1. Compile `installer.c` macefile: `gcc installer.c -o installer`
    2. Run installer: `sudo ./installer`
2. Write your own macefile e.g. `macefile.c`
3. Build `mace macefile.c`

The convenience executable `mace` will compile the `macefile.c` into the `build` executable, and run it.
Nothing more, nothing less.

You can modify the default install path `/usr/local` with `-DPREFIX=\"<path>\"` when compiling `installer`.

Some default behaviour of the convenience executable can be customized, see `installer.c`:
- Default compiler with the target flag `-DCC=gcc`
- Builder executable name with the target flag `-DBUILDER=build`

## Why?
- No build systems for C is truly good.
    - Proof 1: Ostensibly general-purpose build systems (`make`) are never used to build anything other than C/C++ projects.
    - Proof 2: Modern programming languages devs implement their own build systems.
    - Proof 3: Ask random C/C++ developers about `make`, `Cmake`, etc.
- Most if not all build systems have obtuse syntax, scale terribly to larger projects.
    - Makefiles makers exist (premake, autoconf/autotools) and compound this issue.
    - Mix of imperative and declarative style, terrible.
    - Personal experience: homemade `makefile` need constant updates and break in unexpected ways if the project structure changes slightly.
- Build systems are general-purpose and complex.
    - Complexity bad. Simplicity good.
- Build system perfomance bottleneck is compilation.
    - Modern compilers (`gcc`, `clang`) are slow, maybe except `tcc`.

## Limitations
- Linux only.
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
- sha1dc checksum algorithm: [sha1collisiondetection](https://github.com/cr-marcstevens/sha1collisiondetection)

