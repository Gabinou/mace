/*
* installer.c
*
* Copyright (C) Gabriel Taillon, 2023
*
* Macefile for 'mace' convenience executable.
*   - Builds 'mace' executable.
*   - Installs: 'mace' to PREFIX/bin, mace.h PREFIX/include
*
*/

#include "mace.h"

/* Compiler used to compile by 'mace' */
#ifndef CC
    #define CC gcc
#endif /* CC */
#ifndef BUILD_DIR
    #define BUILD_DIR bin
#endif /* BUILD_DIR */
#ifndef OBJ_DIR
    #define OBJ_DIR obj
#endif /* OBJ_DIR */
#ifndef PREFIX
    #define PREFIX /usr/local
#endif /* PREFIX */
/* -- Default macefile used by convenience executable -- */
#ifndef DEFAULT_MACEFILE
    #define DEFAULT_MACEFILE macefile.c
#endif /* DEFAULT_MACEFILE */
/* -- Name of the builder executable to compile macefile into. -- */
#ifndef BUILDER
    #define BUILDER build
#endif



/******************************* WARNING ********************************/
// 1. main is defined in mace.h                                         //
// 2. arguments from main are passed to mace directly                   //
// 3. mace function is declared in mace.h, MUST be implemented by user  //
/*======================================================================*/
int mace(int argc, char *argv[]) {
    MACE_SET_COMPILER(CC);
    MACE_SET_BUILD_DIR(BUILD_DIR);
    MACE_SET_OBJ_DIR(OBJ_DIR);

    /* -- mace convenience executable -- */
    // Note: "mace" token is reserved for user entry point.
    struct Target MACE      = { /* Unitialized values guaranteed to be 0 / NULL */
        .sources            = "mace.c",
        .kind               = MACE_EXECUTABLE,
        // Overrides main in mace.h with custom main.
        .flags              = "-DMACE_OVERRIDE_MAIN -DCC="CC" -DBUILDER="BUILDER" -DDEFAULT_MACEFILE="DEFAULT_MACEFILE,
        .command_post_build =
                "install -T " MACE_STRINGIFY(BUILD_DIR) "/mace " MACE_STRINGIFY(PREFIX) "/bin/mace &&"
                "install -T mace.h"          " " MACE_STRINGIFY(PREFIX) "/include/mace.h",

    };
    // Add target with different name, i.e. "mace"
    mace_add_target(&MACE, "mace");
}
