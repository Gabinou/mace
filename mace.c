
#include "mace.h"

#define CC gcc

struct Target tnecs = { /* Unitialized values guaranteed to be 0 / NULL */
    .includes           = "tnecs.h",
    .sources            = "tnecs.c",
    .base_dir           = "second_party/tnecs",
    .kind               = MACE_LIBRARY,
};


struct Target CodenameFiresaga = { /* Unitialized values guaranteed to be 0 / NULL */
    .includes           = ". include  include/bars  include/menu include/popup include/systems "
    "names names/popup names/menu "
    "second_party/nstr second_party/noursmath second_party/tnecs "
    "third_party/physfs third_party/tinymt third_party/stb third_party/cJson ",
    .sources            = "src/ src/bars/ src/menu/ src/popup/ src/systems/ src/game/",
    .sources_exclude    = "",
    .base_dir           = "",
    .links              = "tnecs",
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
    MACE_ADD_TARGET(tnecs);
    // MACE_ADD_TARGET(CodenameFiresaga);
    // MACE_ADD_TARGET(tnecs_dir);
    // MACE_ADD_TARGET(tnecs_glob);
    // MACE_ADD_TARGET(physfs);
}
