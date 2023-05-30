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

#define STRINGIFY(x) _STRINGIFY(x)
#define _STRINGIFY(x) #x

int main(int argc, char *argv[]) {
    /* -- Parse inputs -- */
    struct Mace_Arguments args = mace_parse_args(argc, argv);
    
    /* -- Override c compiler-- */
    if (arg.cc == NULL) {
        cc = args.ccb
    } else {
        cc = STRINGIFY(CC);
    }
  
    /* --- Compile the macefile --- */
    /* - Read macefile name from args - */
    char *builder = STRINGIFY(BUILDER);
    char *macefile;
    size_t len_cc       = strlen(cc);
    size_t len_flag     = strlen("-o");
    size_t len_builder  = strlen(builder);
    size_t len_macefile;
    if (args.macefile != NULL) {
        macefile        = args.macefile;
        len_macefile    = strlen(args.macefile);
    } else {
        macefile        = STRINGIFY(DEFAULT_MACEFILE);
        len_macefile    = strlen(STRINGIFY(DEFAULT_MACEFILE));
    }

    /* - Write space-separated command - */
    size_t len_total    = len_cc + 1 + len_macefile + 1 + len_flag + 1 + len_builder + 1;
    char *compile_cmd   = calloc(len_total, sizeof(compile_cmd));
    size_t i = 0;
    strncpy(compile_cmd + i, STRINGIFY(CC),         len_cc );
    i += len_cc;
    strncpy(compile_cmd + i, " ",         1);
    i += 1;
    strncpy(compile_cmd + i, macefile,                 len_macefile);
    i += len_macefile;
    strncpy(compile_cmd + i, " -o "STRINGIFY(BUILDER), len_builder + len_flag + 2);

    /* - Build argv_compile - */
    int len             = 8;
    int argc_compile    = 0;
    char **argv_compile = calloc(len, sizeof(*argv_compile));
    argv_compile = mace_argv_flags(&len, &argc_compile, argv_compile, compile_cmd, NULL, false);

    /* - Compile it - */
    pid_t pid = mace_exec(cc, argv_compile);
    mace_wait_pid(pid);

    /* --- Run the resulting executable --- */
    /* - Build argv_run - */
    char *argv_run[] = {"./"STRINGIFY(BUILDER), NULL};

    /* - Run it - */
    pid = mace_exec("./"STRINGIFY(BUILDER), argv_run);
    mace_wait_pid(pid);
}