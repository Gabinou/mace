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

#define MACE_CONVENCIENCE_EXECUTABLE
#include "mace.h"

/* -- Default compiler used by mace executable to compile macefiles. -- */
#ifndef CC
    #define CC gcc
#endif

/* -- Default macefile. -- */
#ifndef DEFAULT_MACEFILE
    #define DEFAULT_MACEFILE macefile.c
#endif /* DEFAULT_MACEFILE */

/* -- Name of the builder executable to compile macefile into. -- */
#ifndef BUILDER
    #define BUILDER build
#endif

int main(int argc, char *argv[]) {
    /* -- Parse inputs -- */
    struct Mace_Arguments args = mace_parse_args(argc, argv);

    /* --- Compile the macefile --- */
    /* - Build argv_compile - */
    size_t len_cc = strlen(MACE_STRINGIFY(CC));
    size_t len_macefile;
    size_t len_flag = strlen("-o");
    size_t len_builder = strlen(MACE_STRINGIFY(BUILDER));
    char *macefile;
    if (args.macefile != NULL) {
        len_macefile = strlen(args.macefile);
        macefile = args.macefile;
    } else {
        len_macefile = strlen(MACE_STRINGIFY(DEFAULT_MACEFILE));
        macefile = MACE_STRINGIFY(DEFAULT_MACEFILE);
    }
    size_t len_total = len_cc + 1 + len_macefile + 1 + len_flag + 1 + len_builder + 1;
    char *compile_cmd = calloc(len_total, sizeof(compile_cmd));
    size_t i = 0;
    strncpy(compile_cmd + i, MACE_STRINGIFY(CC), len_cc);
    i += len_cc;
    strncpy(compile_cmd + i, " ", 1);
    i += 1;
    strncpy(compile_cmd + i, macefile, len_macefile);
    i += len_macefile;
    strncpy(compile_cmd + i, " ", 1);
    i += 1;
    strncpy(compile_cmd + i, "-o", len_flag);
    i += len_flag;
    strncpy(compile_cmd + i, " ", 1);
    i += 1;
    strncpy(compile_cmd + i, MACE_STRINGIFY(BUILDER), len_builder);

    int len = 8;
    int argc_compile = 0;
    char **argv_compile = calloc(len, sizeof(*argv_compile));
    argv_compile = mace_argv_flags(&len, &argc_compile, argv_compile, compile_cmd, NULL, false);

    /* - Compile - */
    pid_t pid = mace_exec(MACE_STRINGIFY(CC), argv_compile);
    mace_wait_pid(pid);

    /* --- Run the resulting executable --- */
    /* - Build argv_run - */
    char *argv_run[] = {"./"MACE_STRINGIFY(BUILDER), NULL};

    /* - Run it - */
    pid = mace_exec("./"MACE_STRINGIFY(BUILDER), argv_run);
    mace_wait_pid(pid);
}