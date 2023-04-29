
## About

Mace is a C99-only build system. 
Specificity, reduced scope, all in service of *simplicity*. 

## Features
- As simple as possible.
- C syntax.
    - Macefiles are .c files.
    - Targets are structs. 
    - Function `mace` is user entry point.
    - C++ not supported.
- Single header build system.
- Easy to learn
    - Familiar syntax.
    - Simple API.
    - Compiling `mace.c` as easy as compiling `hello_world.c`.

## Usage
1. Get `mace.h`
2. Write your own macefile e.g. `mace.c`
3. Compile `gcc mace.c -o build`
4. Build `./build`

### Convenience executable
1. Install mace
    1. `gcc mace.h -o mace`
    2. `install ....`
2. Write your own macefile e.g. `mace.c`
3. Build `mace mace.c`

## Example macefile
```
#include "mace.h"

#define CC gcc

struct Target tnecs = { /* Unitialized values guaranteed to be 0 / NULL */
    .includes           = "tnecs.h",
    .sources            = "tnecs.c",
    .base_dir           = "tnecs",
    .kind               = MACE_LIBRARY,
};

/******************************* WARNING ********************************/
// 1. main is defined in mace.h                                         //
// 2. arguments from main are passed to mace directly                   //
// 3. mace function is declared in mace.h, MUST be implemented by user  //
/*======================================================================*/
int mace(int argc, char *argv[]) {
    MACE_SET_COMPILER(CC)
    MACE_ADD_TARGET(tnecs);
}

```

## Why?
- No build systems for C is truly good.
    - Proof 1: Ostensibly general-purpose build systems (`make`) are never used to build anything other than C/C++ projects.
    - Proof 2: Modern programming languages devs implement their own build systems.
- Most if not all build systems have obtuse syntax, scale terribly to larger projects.
    - Makefiles makers exist (premake, autoconf/autotools) and compound this issue.
    - Make mixes imperative and declarative style.
    - Why not build an API with familiar C syntax?
- Build systems are general-purpose and complex.
    - Complexity bad. Simplicity good.
- Build system perfomance bottleneck is compilation.
    - Modern compilers (`gcc`, `clang`) are slow, except `tcc`

## Under the hood
- Compiler spits out object file  dependecies (headers)
- User inputs targets dependencies with `target.links` (libraries or other targets)
    - Build order determined by depth first search through depedencies.
- Mace saves file and flag checksums in .mace_checksums
    - Recompile if checksum changed

## Credits
- mace uses the same naming convention as [mage](https://github.com/magefile/mage)
- mace contains a modified version of `parg` for argument parsing
