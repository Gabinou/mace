# Mace

### My ideal C build system:
- Uses C syntax.
- Is specific to C.
   - No C++ support. It sucks.
- Is as simple as possible.
- Is a single header file:
    - No additional binaries required, just a C compiler!

### Usage
1. Get `mace.h`
2. Write your own macefile e.g. `mace.c`
3. Compile `gcc mace.c -o mace`
4. Build `./mace`

## Context

All C build systems suck. I have used:
- Make
- Cmake
- Please
- ninja

And they all suck, because...
- Obtuse, Incomprehensible syntax.
    - Why do I need to learn that to build C?
- Does not scale to bigger project.
    - Proof: Makefile makers exist.
- Additional executable necessary for compilation.
    - I rarely see this point brought up. 
    C is a compiled language, the compiler can build executables. 
    Why not use the compiler to build the builder? One less dependency!
- They are general purpose.
    - Generality brings complexity. Complexity bad.
    - Simplicity Good. No simpler than single purpose tool.
- Lie about being fast
    - Nothing faster than `make -j`
    - Bottleneck is compilation
    - Not hard to compile in order

### Make
Can be used to build any compiled language.
- Only used by C/C++
- Terrible documentation, very hard to find good tutorials, examples.
- Makefiles suck so much makefile makers exist
    - `autoconf/autotools`. Regularly recommended AGAINST using it online(!)
    - `premake`. Least terrible option.

No modern compiled languages use `make`!
Many devs make their own!
- Rust:     `cargo-make`
- Zig:      `zig build`
- Go:       `mage` (make, with go syntax)
Even for C:
- C:        `bake` 

### Please
- Weird syntax
- Terrible documentation
- Can't build into specific folder
- Can't change directory before building
- NOT FASTER THAN MAKE! -> `make -j<corenumber>` is FASTER


### Features:
- Can be header only build system
    - Write macefile (mace.c, but could be anything)
    - Compile `gcc mace.c -o mace`
    - Run `mace`
    - Build is done!
- NECESSARY
    - Include/exclude header files, folders
    - Include/exclude source files, folders
    - Link executables
    - Link static libraries
    - Define compiler flags
    - Define Linker flags
    - Add static libraries
- PERFORMANCE
    - split into many jobs like `make`

### PROBLEMS:
- How to know which files gets recompiled?
    - Make: target must be recompiled if prerequisite is newer
    - <sys/types.h> (GNU) <sys/stat.h> (GNU) <unistd.h> (POSIX)
- Must read all files in folders
    - <glob.h> is a GNU header
    - Make my own globber.
- Must read files for dependencies! 
    - And make a dependency graph!
    - Graph stored as adjacency list
    - Make my own parser.
- Targets
    - Structs. 
        - Designated init, C99 feature. Automatic init of omitted elements to 0/NULL
    - How to compile a single target on runtime?
        - Have to find all funcs in build.c and put them in a list somehow.
        - -> ADD_TARGET
- How to build targets declared in C file?
　　- have users define targets and put them in an array
- How to build only specific targets?
    - hash the target names, keep in array, build only target with same hash + deps
- `main()` should be in `mwc.h` and parse `build.c`

## Credits
- mace uses the same naming convention as (mage)[]
- mace contains a modified version of `parg` for argument parsing
