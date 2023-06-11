#include "mace.h"

/* CC might be set when compiling e.g. with "-DCC=gcc" */
#ifndef CC
    #define CC gcc
#endif
/* AR might be set when compiling e.g. with "-DAR=ar" */
#ifndef AR
    #define AR ar
#endif

struct Config debug     = {.flags = "-g -O0"};
struct Config release   = {.flags = "-O2"};
// Cross-compilation config
// win_debug target overrides default target set with MACE_DEFAULT_TARGET
struct Config win_debug = {.flags = "-g -O0", .target = "win_bar",
    .cc    = "x86_64-w64-mingw32-gcc",  .ar = "x86_64-w64-mingw32-ar"
};

/******************************* WARNING ********************************/
/* 1. main is defined in mace.h                                         */
/* 2. arguments from main are passed to mace                            */
/* 3. mace function is declared in mace.h, MUST be defined by user      */
/*======================================================================*/
int mace(int argc, char *argv[]) {
    MACE_SET_COMPILER(CC)
    MACE_SET_BUILD_DIR(build);
    MACE_SET_OBJ_DIR(obj);

    // Note: 'clean' and 'all' are reserved target names with expected behavior.
    struct Target foo = { /* Unitialized values guaranteed to be 0 / NULL */
        /* Default separator is ',', but can be set with MACE_SET_SEPARATOR */
        .includes           = "include,include/sub/a.h",
        .sources            = "src,src/sub/*",
        .base_dir           = "foo",
        .kind               = MACE_STATIC_LIBRARY,
    };
    struct Target bar = {
        .includes           = "include,include/sub/b.h",
        .sources            = "src,src/sub/*",
        .base_dir           = "bar",
        .links              = "foo",
        .kind               = MACE_EXECUTABLE,
    };
    struct Target win_bar = {
        .includes           = "include,include/sub/b.h",
        .sources            = "src,src/sub/*",
        .base_dir           = "bar",
        .links              = "foo",
        .flags              = "-lmingw32",
        .kind               = MACE_EXECUTABLE,
    };
    struct Target bar_test = {
        .includes           = "include,include/sub/c.h",
        .sources            = "src,src/sub/*,test/test.c",
        .excludes           = "src/main.c",
        .base_dir           = "bar",
        .links              = "foo",
        .kind               = MACE_EXECUTABLE,
    };

    /* -- Targets -- */
    MACE_ADD_TARGET(foo);
    MACE_ADD_TARGET(bar);
    MACE_ADD_TARGET(bar_test);

    // Change default target from 'all' to `foo`, skipping `bar_test`.
    // `bar` depends on `foo`, so `foo` gets built first.
    // NOTE: if user selects win_debug config, this default target is overriden
    MACE_DEFAULT_TARGET(bar);

    /* -- Configs -- */
    MACE_ADD_CONFIG(debug);     /* First config is default config         */
    MACE_ADD_CONFIG(release);   /* To use this config: -g flag            */
    MACE_ADD_CONFIG(win_debug); /* Overrides default 'bar' with 'win_bar' */
}
