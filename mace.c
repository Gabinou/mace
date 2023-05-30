
#include "mace.h"

#define CC gcc

struct Target tnecs = { /* Unitialized values guaranteed to be 0 / NULL */
    .includes           = "tnecs.h",
    .sources            = "tnecs.c",
    .base_dir           = "tnecs",
    .kind               = MACE_LIBRARY,
};

struct Target tnecs_dir = { /* Unitialized values guaranteed to be 0 / NULL */
    .includes           = "tnecs",
    .sources            = "tnecs",
    .base_dir           = "tnecs",
    .kind               = MACE_LIBRARY,
};

struct Target tnecs_glob = { /* Unitialized values guaranteed to be 0 / NULL */
    .includes           = "tnecs",
    .sources            = "tnecs/*.c",
    .base_dir           = "tnecs",
    .kind               = MACE_LIBRARY,
};


struct Target physfs = { /* Unitialized values guaranteed to be 0 / NULL */
    .includes           = "physfs",
    .sources            = "physfs",
    .base_dir           = "physfs",
    .kind               = MACE_LIBRARY,
};


struct Target CodenameFiresaga = { /* Unitialized values guaranteed to be 0 / NULL */
    .includes           = "",
    .sources            = "",
    .sources_exclude    = "",
    .base_dir           = "",
    .links              = "tnecs SDL2",
    .kind               = MACE_EXECUTABLE,
};


/*========================= WARNING =========================*/
// 1. main is defined in mace.h
// 2. main arguments are passed to mace directly
// 3. mace is declared in mace.h, MUST be implemented by user
int mace(int argc, char *argv[]) {
    MACE_SET_COMPILER(CC);
    MACE_ADD_TARGET(tnecs);
    // MACE_ADD_TARGET(tnecs_dir);
    // MACE_ADD_TARGET(tnecs_glob);
    // MACE_ADD_TARGET(physfs);
}
