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
    #define PREFIX /usr
#endif /* PREFIX */
/* -- Default macefile used by convenience executable -- */
#ifndef DEFAULT_MACEFILE
    #define DEFAULT_MACEFILE macefile.c
#endif /* DEFAULT_MACEFILE */
/* -- Name of the builder executable to compile macefile into. -- */
#ifndef BUILDER
    #define BUILDER builder
#endif
/* -- Path to zsh completion. -- */
#ifndef ZSH_COMPLETION
    #define ZSH_COMPLETION /usr/share/zsh/site-functions
#endif
/* -- Path to bash completion. -- */
#ifndef BASH_COMPLETION
    #define BASH_COMPLETION /usr/share/bash-completion/completions
#endif

#define STRINGIFY(x) _STRINGIFY(x)
#define _STRINGIFY(x) #x

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
        .flags              = "-DMACE_OVERRIDE_MAIN -DCC="STRINGIFY(CC)" "
                              "-DBUILDER="STRINGIFY(BUILDER)" "
                              "-DDEFAULT_MACEFILE="STRINGIFY(DEFAULT_MACEFILE),
        .command_post_build =
                /* Install mace convenience executable*/
                "install -T " STRINGIFY(BUILD_DIR) "/mace " STRINGIFY(PREFIX) "/bin/mace &&"
                /* Install mace.h header*/
                "install -T mace.h"  " " STRINGIFY(PREFIX) "/include/mace.h &&"
                /* Install zsh completion */
                "cp _mace.zsh _mace.temp &&"
                "sed -i s/macefile.c/" STRINGIFY(DEFAULT_MACEFILE) "/ _mace.temp &&"
                "sed -i s/builder/" STRINGIFY(BUILDER) "/ _mace.temp &&"
                "install -T _mace.temp"   " " STRINGIFY(ZSH_COMPLETION) "/_mace &&"
                "rm _mace.temp &&"
                /* Install bash completion */
                // TODO: replace macefile.c/Macefile.c with default macefile
                "cp mace.bash mace.temp &&"
                "sed -i s/macefile.c/" STRINGIFY(DEFAULT_MACEFILE) "/ mace.temp &&"
                "sed -i s/builder/" STRINGIFY(BUILDER) "/ mace.temp &&"
                "install -T mace.temp"   " " STRINGIFY(BASH_COMPLETION) "/mace &&",
                "rm mace.temp"
    };

    // Add target with different name, i.e. "mace"
    mace_add_target(&MACE, "mace");
}
