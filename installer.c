
#include "mace.h"

#define CC gcc
#define MACE_CONVENIENCE_EXECUTABLE "mace"
#ifndef PREFIX
    #define PREFIX "/usr/local"
#endif /* PREFIX */

/******************************* WARNING ********************************/
// 1. main is defined in mace.h                                         //
// 2. arguments from main are passed to mace directly                   //
// 3. mace function is declared in mace.h, MUST be implemented by user  //
/*======================================================================*/
int mace(int argc, char *argv[]) {
    MACE_SET_COMPILER(CC);
    mace_set_build_dir("bin");
    mace_set_obj_dir("obj");


    /* - mace convenience executable - */
    // Note: "mace" token is reserved for user entry point.
    struct Target MACE      = { /* Unitialized values guaranteed to be 0 / NULL */
        .sources            = "mace.c",
        .kind               = MACE_EXECUTABLE,
        // Overrides main in mace.h with custom main.
        .flags              = "-DMACE_CONVENIENCE_EXECUTABLE",
    };
    MACE_ADD_TARGET(MACE);
    // Change target name from "MACE" to "mace"
    targets[0]._name = MACE_CONVENIENCE_EXECUTABLE;

    /* - mace install - */
    // 1. Copies mace.h header to `/usr/include`.
    // 2. Copies mace convenient executable to `/usr/bin`
    struct Command install  = {
        .command            = "install " build_dir / MACE_CONVENIENCE_EXECUTABLE
                              PREFIX / MACE_CONVENIENCE_EXECUTABLE,
    };
}

