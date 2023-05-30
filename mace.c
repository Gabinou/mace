
#include "mace.h"

#define CC gcc

struct Target tnecs = { /* Unitialized values guaranteed to be 0 / NULL */
    .includes           = "",
    .sources            = "",
    .sources_exclude    = "",
    .base_dir           = "",
    .dependencies       = "",
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


/* ========== WARNING ========== */
// 1. main is defined in mace.h
// 2. arguments from main are passed to mace directly
// 3. mace is declared in mace.h, MUST be implemented by user
int mace(int argc, char *argv[]) {
    MACE_SET_COMPILER(CC);

    assert(tnecs.links == NULL);
    printf("%d \n", strlen(CodenameFiresaga.links));
    printf("%d \n", sizeof(CodenameFiresaga.links));
    printf("%d \n", sizeof(*CodenameFiresaga.links));
    printf("%s \n", cc);

    MACE_ADD_TARGET(tnecs);
    printf("%s \n", tnecs._name);
    MACE_ADD_TARGET(CodenameFiresaga);
}
