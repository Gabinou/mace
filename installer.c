
#include "mace.h"

// Compiler used to compile installer executable
#ifndef CC
#define CC gcc
#endif /* CC */
#define BUILD_DIR "bin"
#define OBJ_DIR "obj"
#define EXECUTABLE_NAME "mace"
#define HEADER_NAME "mace.h"
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
    mace_set_build_dir(BUILD_DIR);
    mace_set_obj_dir(OBJ_DIR);

    /* -- mace convenience executable -- */
    // Note: "mace" token is reserved for user entry point.
    struct Target MACE      = { /* Unitialized values guaranteed to be 0 / NULL */
        .sources            = "mace.c",
        .kind               = MACE_EXECUTABLE,
        // Overrides main in mace.h with custom main.
        .flags              = "-DMACE_OVERRIDE_MAIN -DCC=gcc -DBUILDER=build",
    };
    // Add target with different name, i.e. "mace"
    mace_add_target(&MACE, EXECUTABLE_NAME);

    /* - mace install - */
    // 1. Copies mace convenience executable to `/usr/local/bin`
    // 2. Copies mace header                 to `/usr/local/include`
    struct Command install_mace = {
        .commands               = "install" 
                                  " " BUILD_DIR "/" EXECUTABLE_NAME
                                  " " PREFIX "/bin/" EXECUTABLE_NAME
                                  " " "&& install"
                                  " " HEADER_NAME 
                                  " " PREFIX "/include/" HEADER_NAME,
    };
    MACE_ADD_COMMAND(install_mace);
}

