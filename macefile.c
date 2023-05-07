
#include "mace.h"

#define CC gcc

/* - second_party - */
struct Target noursmath = { /* Unitialized values guaranteed to be 0 / NULL */
    .sources            = ".",
    .base_dir           = "second_party/noursmath",
    .kind               = MACE_STATIC_LIBRARY,
};

struct Target nstr = { /* Unitialized values guaranteed to be 0 / NULL */
    .sources            = ".",
    .base_dir           = "second_party/nstr",
    .kind               = MACE_STATIC_LIBRARY,
};

struct Target parg = { /* Unitialized values guaranteed to be 0 / NULL */
    .sources            = ".",
    .base_dir           = "second_party/parg",
    .kind               = MACE_STATIC_LIBRARY,
};

struct Target tnecs = { /* Unitialized values guaranteed to be 0 / NULL */
    .sources            = ".",
    .base_dir           = "second_party/tnecs",
    .kind               = MACE_STATIC_LIBRARY,
};

/* - third_party - */
struct Target cJson = { /* Unitialized values guaranteed to be 0 / NULL */
    .sources            = ".",
    .base_dir           = "third_party/cJson",
    .kind               = MACE_STATIC_LIBRARY,
};

struct Target physfs = { /* Unitialized values guaranteed to be 0 / NULL */
    .sources            = ".",
    .base_dir           = "third_party/physfs",
    .allatonce          = false,
    .kind               = MACE_STATIC_LIBRARY,
};

struct Target tinymt = { /* Unitialized values guaranteed to be 0 / NULL */
    .sources            = ".",
    .base_dir           = "third_party/tinymt",
    .kind               = MACE_STATIC_LIBRARY,
};

struct Target SotA = { /* Unitialized values guaranteed to be 0 / NULL */
    .includes           = ". include include/bars include/menu include/popup "
                          "include/systems names names/popup names/menu "
                          "second_party/nstr second_party/noursmath second_party/tnecs "
                          "second_party/parg second_party/nourstest "
                          "third_party/physfs third_party/tinymt third_party/stb "
                          "third_party/cJson",
    .sources            = "src/*.c src/bars/ src/menu/ src/popup/ src/systems/ src/game/",
    .links              = "SDL2 SDL2_image SDL2_ttf m GLEW cJson noursmath physfs "
                          "tinymt tnecs nstr parg",
    .kind               = MACE_EXECUTABLE,
};

/******************************* WARNING ********************************/
// 1. main is defined in mace.h                                         //
// 2. arguments from main are passed to mace directly                   //
// 3. mace function is declared in mace.h, MUST be implemented by user  //
/*======================================================================*/
int mace(int argc, char *argv[]) {
    MACE_SET_COMPILER(CC);
    mace_set_build_dir("A_build");
    mace_set_obj_dir("B_obj");
    /* - second_party - */
    // MACE_ADD_TARGET(noursmath);
    // MACE_ADD_TARGET(nstr);
    // MACE_ADD_TARGET(parg);
    // MACE_ADD_TARGET(tnecs);
    /* - third_party - */
    // MACE_ADD_TARGET(cJson);
    // MACE_ADD_TARGET(physfs);
    // MACE_ADD_TARGET(tinymt);

    /* - SotA - */
    MACE_ADD_TARGET(SotA);
}
