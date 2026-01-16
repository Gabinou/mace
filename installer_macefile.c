/*
**  Copyright 2023-2026 Gabriel Taillon
**  Licensed under GPLv3
**
**      Éloigne de moi l'esprit d'oisiveté, de
**          découragement, de domination et de
**          vaines paroles.
**      Accorde-moi l'esprit d'intégrité,
**          d'humilité, de patience et de charité.
**      Donne-moi de voir mes fautes.
**
***************************************************
**  Macefile for 'mace' convenience executable
**  
**  1. Builds 'mace' exe
**  2. Installs: 
**      1. mace         to PREFIX/bin
**      2. mace.h       to PREFIX/include
**      3. _mace.zsh    to ZSH_COMPLETION
**
*/

#include "mace.h"

/* Compiler used to compile by 'mace' */
#ifndef CC
    #define CC gcc
#endif /* CC */
#ifndef AR
    #define AR ar
#endif /* AR */

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

struct Target MACE  = {
    .sources        = "convenience_executable.c",
    .kind           = MACE_EXECUTABLE,
    /* Flags:
    **  1. Override main in mace.h with custom main in convenience_executable.c
    **  2. Set compiler, names */
    .flags          = "-DMACE_OVERRIDE_MAIN -DCC="STRINGIFY(CC)" "
    "-DBUILDER="STRINGIFY(BUILDER)" "
    "-DDEFAULT_MACEFILE="STRINGIFY(DEFAULT_MACEFILE),
    "-O2",
    .cmd_post       =
    /* Install mace convenience executable*/
    "install -T " STRINGIFY(BUILD_DIR) "/mace " STRINGIFY(PREFIX) "/bin/mace &&"
    /* Install mace.h header*/
    "install -T mace.h"  " " STRINGIFY(PREFIX) "/include/mace.h &&"
    /* Install zsh completion */
    "cp _mace.zsh _mace.temp &&"
    "sed -i s/macefile.c/" STRINGIFY(DEFAULT_MACEFILE) "/ _mace.temp &&"
    "sed -i s/builder/" STRINGIFY(BUILDER) "/ _mace.temp &&"
    "install -T _mace.temp"   " " STRINGIFY(ZSH_COMPLETION) "/_mace &&"
    "rm _mace.temp"
};

int mace(int argc, char *argv[]) {
    MACE_SET_COMPILER(CC);
    MACE_SET_BUILD_DIR(BUILD_DIR);
    MACE_SET_OBJ_DIR(OBJ_DIR);

    mace_add_target(&MACE, "mace");
    MACE_SET_DEFAULT_TARGET(mace);
    return (0);
}
