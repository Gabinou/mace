/*
* mace.c
*
* Copyright (C) Gabriel Taillon, 2023
*
* Convenience executable for mace C-based build system.
*   - Compiles input macefile to builder executable.
*   - Runs builder executable.
*
*/

#include "mace.h"

/* -- Default compiler used by mace executable to compile macefiles. -- */
#ifndef CC
    #define CC gcc
#endif

/* -- Default macefile. -- */
#ifndef DEFAULT_MACEFILE
    #define DEFAULT_MACEFILE "macefile.c"
#endif /* DEFAULT_MACEFILE */

/* -- Name of the builder executable to compile macefile into. -- */
#ifndef BUILDER
    #define BUILDER build
#endif

int main(int argc, char *argv[]) {
    /* -- Parse inputs -- */
    struct Mace_Arguments args = mace_parse_args(argc, argv);

    /* -- Build argv -- */
    
    
    /* -- Compile the macefile -- */
    pid_t pid = mace_exec(MACE_STRINGIFY(CC), argv_compile);
    mace_wait_pid(pid);

    /* -- Run the resulting executable -- */
    char *argv_run[] = {MACE_STRINGIFY(BUILDER)};
    pid = mace_exec(MACE_STRINGIFY(BUILDER), argv_run);
    mace_wait_pid(pid);

}