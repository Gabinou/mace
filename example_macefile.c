
#include "mace.h"

#ifndef CC
    #define CC "tcc"
#endif
#ifndef AR
    #define AR "tcc -ar"
#endif

struct Config debug         = {
    .flags = "-g3 -rdynamic -O0"
};

struct Target A     = {
    .includes = ". include include/unit",
    .sources  = "src src/unit",
    .links    = "A B",
    .flags    = "-fno-strict-overflow -fno-strict-aliasing "
                "-fwrapv -fno-delete-null-pointer-checks "
                "-std=iso9899:1999",
    .cmd_pre  = "astyle --recursive src/* include/*",
    .kind     = MACE_EXECUTABLE,
};

struct Target B     = {
    .base_dir  = "third_party/A",
    .flags     = "-std=iso9899:1999",
    .sources   = ".",
    .kind      = MACE_STATIC_LIBRARY,
};

struct Target C  = {
    .base_dir  = "third_party/B",
    .flags     = "-std=iso9899:1999",
    .sources   = ".",
    .kind      = MACE_STATIC_LIBRARY,
};


int mace(int argc, char *argv[]) {
    /* -- Setting compiler, directories -- */
    mace_set_compiler(CC);
    mace_set_archiver(AR);
    mace_set_build_dir("build");
    mace_set_obj_dir("obj");

    /* -- Configs -- */
    MACE_ADD_CONFIG(debug);

    /* -- Targets -- */
    MACE_ADD_TARGET(A);
    MACE_ADD_TARGET(B);
    MACE_ADD_TARGET(C);

    MACE_SET_DEFAULT_TARGET(A);

    /* - Target configs - */
    MACE_TARGET_CONFIG(A, debug);

    return (0);
}
