
#include "mace.h"

#define CC gcc

/* - mace convenience executable - */
// Overrides main in mace.h with custom main.
struct Target mace      = { /* Unitialized values guaranteed to be 0 / NULL */
    .sources            = "mace.c",
    .kind               = MACE_EXECUTABLE,
    .flags              = "-DMACE_EXECUTABLE",
};


/* - mace install - */
// 1. Copies mace.h header to `/usr/include`.
// 2. Copies mace convenient executable to `/usr/bin`
struct Command install  = {
    .command            = " ",
};

/******************************* WARNING ********************************/
// 1. main is defined in mace.h                                         //
// 2. arguments from main are passed to mace directly                   //
// 3. mace function is declared in mace.h, MUST be implemented by user  //
/*======================================================================*/
int mace(int argc, char *argv[]) {
    MACE_SET_COMPILER(CC);
    mace_set_build_dir("bin");
    mace_set_obj_dir("obj");
    
    /* - SotA - */
    MACE_ADD_TARGET(mace);
}

