
#include "mwc.h"

#define CC gcc

struct Target tnecs = { /* Unitialized values guaranteed to be 0 / NULL */
    .includes           = "",
    .sources            = "",
    .sources_exclude    = "",
    .base_dir           = "",
    .dependencies       = "",
    .kind               = MWC_LIBRARY,
};

struct Target CodenameFiresaga = { /* Unitialized values guaranteed to be 0 / NULL */
    .includes           = "",
    .sources            = "",
    .sources_exclude    = "",
    .base_dir           = "",
    .links              = "tnecs SDL2",
    .kind               = MWC_EXECUTABLE,
};


/* ========== WARNING ========== */
// 1. main is defined in mwc.h
// 2. arguments from main are passed to mwc directly
// 3. mwc is declared in mwc.h, MUST be implemented by user
int mwc(int argc, char *argv[]) {

    assert(tnecs.links == NULL);
    printf("%d \n", strlen(CodenameFiresaga.links));
    printf("%d \n", sizeof(CodenameFiresaga.links));
    printf("%d \n", sizeof(*CodenameFiresaga.links));

    MWC_SET_COMPILER(CC)
    MWC_ADD_TARGET(tnecs);
    MWC_ADD_TARGET(CodenameFiresaga);
}
