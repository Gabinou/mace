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

/* -- Name of the builder executable to compile macefile into. -- */
#ifndef BUILDER
    #define BUILDER build
#endif

int main(int argc, char *argv[]) {
    /* -- Inputs only one argument: a macefile -- */
    printf("CC %s\n", MACE_STRINGIFY(CC));
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file> -CC <compiler>\n", argv[0]);
        return EPERM;
    }

    /* -- TODO: Get argv compiler -- */
    /* -- TODO: Get argv target -- */

    /* -- Compile the macefile -- */
    char *argv_compile[] = {MACE_STRINGIFY(CC), argv[1], "-o", MACE_STRINGIFY(BUILDER)};
    pid_t pid = mace_exec(MACE_STRINGIFY(CC), argv_compile);
    mace_wait_pid(pid);

    /* -- Run the resulting executable -- */
    char *argv_run[] = {MACE_STRINGIFY(BUILDER)};
    pid = mace_exec(MACE_STRINGIFY(BUILDER), argv_run);
    mace_wait_pid(pid);

}