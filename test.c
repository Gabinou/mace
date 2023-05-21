/*
* test.c
*
* Copyright (C) Gabriel Taillon, 2023
*
* Tests for mace C-based build system.
*
*/

#include "mace.h"
#include <fcntl.h>

/* --- Testing library --- */
#ifndef __NOURSTEST_H__
#define __NOURSTEST_H__
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define MACE_TEST_OBJ_DIR   "obj"
#define MACE_TEST_BUILD_DIR "build"
#define MACE_TEST_BUFFER_SIZE 128

static int test_num = 0, fail_num = 0;

static void nourstest_results() {
    char message[4] = "FAIL";
    if (fail_num == 0)
        strncpy(message, "PASS", 4);
    printf("%s (%d/%d)\n", message, test_num - fail_num, test_num);
}
#define nourstest_true(test) do {\
        if ((test_num++, !(test))) \
            printf("\n %s:%d error #%d", __FILE__, __LINE__, ++fail_num); \
    } while (0)

static void nourstest_run(char *name, void (*test)()) {
    const int ts = test_num, fs = fail_num;
    clock_t start = clock();
    printf("\t%-14s", name), test();
    printf("\t pass: %5d \t fail: %2d \t %4dms\n",
           (test_num - ts) - (fail_num - fs), fail_num - fs,
           (int)((clock() - start) * 1e3 / CLOCKS_PER_SEC));
}
#endif /*__NOURSTEST_H__ */

#define BUILDDIR "build/"

void test_isFunc() {
    nourstest_true(mace_isSource("test.c"));
    nourstest_true(mace_isSource("doesnotexist.c"));
    nourstest_true(!mace_isDir("test.c"));
    nourstest_true(mace_isDir("../mace"));
    nourstest_true(mace_isWildcard("src/*"));
    nourstest_true(mace_isWildcard("src/**"));
    nourstest_true(!mace_isWildcard("src/"));
}

void test_globbing() {
    glob_t globbed;
    globbed = mace_glob_sources("../mace/*.c");
    nourstest_true(globbed.gl_pathc == 3);
    nourstest_true(strcmp(globbed.gl_pathv[0], "../mace/installer.c") == 0);
    nourstest_true(strcmp(globbed.gl_pathv[1], "../mace/mace.c") == 0);
    nourstest_true(strcmp(globbed.gl_pathv[2], "../mace/test.c") == 0);
    globfree(&globbed);

    globbed = mace_glob_sources("../mace/*.h");
    nourstest_true(globbed.gl_pathc == 1);
    nourstest_true(strcmp(globbed.gl_pathv[0], "../mace/mace.h") == 0);
    globfree(&globbed);
}

void test_object() {
    if ((object_len == 0) || (object == NULL)) {
        object_len  = 24;
        object      = malloc(object_len * sizeof(*object));
    }

    char buffer[MACE_TEST_BUFFER_SIZE] = {0};

    size_t cwd_len   = strlen(cwd);
    size_t obj_len   = strlen(MACE_TEST_OBJ_DIR);
    size_t file_len  = strlen("//mace.o");
    size_t total_len = 0;

    strncpy(buffer,             cwd,               cwd_len);
    total_len += cwd_len;
    strncpy(buffer + total_len, "/",               1);
    total_len += 1;
    strncpy(buffer + total_len, MACE_TEST_OBJ_DIR, obj_len);
    total_len += obj_len;
    strncpy(buffer + total_len, "//mace.o",        file_len);

    mace_object_path("/mace.c");
    nourstest_true(strcmp(object, buffer) == 0);
    nourstest_true(mace_isObject(object));

    mace_object_path("mace.c");
    nourstest_true(strcmp(object, buffer) == 0);
    nourstest_true(mace_isObject(object));
}

void test_target() {
    mace_pre_user();
    nourstest_true(target_num                           == 0);
    struct Target tnecs = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .kind               = MACE_STATIC_LIBRARY,
    };
    MACE_ADD_TARGET(tnecs);

    nourstest_true(target_num                           == 1);
    nourstest_true(targets[0]._hash                     == mace_hash("tnecs"));
    nourstest_true(targets[0]._order                    == 0);
    nourstest_true(strcmp(targets[0]._name, "tnecs")    == 0);

    struct Target firesaga = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .links              = "SDL2 SDL2_image SDL2_ttf m GLEW cJSON nmath "
                              "physfs tinymt tnecs nstr parg",
        .kind               = MACE_EXECUTABLE,
    };
    MACE_ADD_TARGET(firesaga);

    nourstest_true(target_num                           == 2);
    nourstest_true(targets[1]._hash                     == mace_hash("firesaga"));
    nourstest_true(targets[1]._deps_links_num           == 12);
    nourstest_true(targets[1]._deps_links[0]            == mace_hash("SDL2"));
    nourstest_true(targets[1]._deps_links[1]            == mace_hash("SDL2_image"));
    nourstest_true(targets[1]._deps_links[2]            == mace_hash("SDL2_ttf"));
    nourstest_true(targets[1]._deps_links[3]            == mace_hash("m"));
    nourstest_true(targets[1]._deps_links[4]            == mace_hash("GLEW"));
    nourstest_true(targets[1]._deps_links[5]            == mace_hash("cJSON"));
    nourstest_true(targets[1]._deps_links[6]            == mace_hash("nmath"));
    nourstest_true(targets[1]._deps_links[7]            == mace_hash("physfs"));
    nourstest_true(targets[1]._deps_links[8]            == mace_hash("tinymt"));
    nourstest_true(targets[1]._deps_links[9]            == mace_hash("tnecs"));
    nourstest_true(targets[1]._deps_links[10]           == mace_hash("nstr"));
    nourstest_true(targets[1]._deps_links[11]           == mace_hash("parg"));

    /* cleanup so that mace doesn't build targets */
    mace_finish(NULL);
    mace_pre_user();

    /* mace computing build order as a function of linking dependencies */
    struct Target A = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .links              = "B C D",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target B = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .links              = "D E",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target C = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target D = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .links              = "F G",
        .kind               = MACE_EXECUTABLE,
    };


    struct Target E = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .links              = "G",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target F = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target G = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .kind               = MACE_EXECUTABLE,
    };

    MACE_ADD_TARGET(B);
    MACE_ADD_TARGET(C);
    MACE_ADD_TARGET(E);
    MACE_ADD_TARGET(A);
    MACE_ADD_TARGET(G);
    MACE_ADD_TARGET(D);
    MACE_ADD_TARGET(F);
    nourstest_true(target_num == 7);
    nourstest_true(strcmp(targets[0]._name, "B") == 0);
    nourstest_true(strcmp(targets[1]._name, "C") == 0);
    nourstest_true(strcmp(targets[2]._name, "E") == 0);
    nourstest_true(strcmp(targets[3]._name, "A") == 0);
    nourstest_true(strcmp(targets[4]._name, "G") == 0);
    nourstest_true(strcmp(targets[5]._name, "D") == 0);
    nourstest_true(strcmp(targets[6]._name, "F") == 0);

    nourstest_true(targets[0]._hash == mace_hash("B"));
    nourstest_true(targets[1]._hash == mace_hash("C"));
    nourstest_true(targets[2]._hash == mace_hash("E"));
    nourstest_true(targets[3]._hash == mace_hash("A"));
    nourstest_true(targets[4]._hash == mace_hash("G"));
    nourstest_true(targets[5]._hash == mace_hash("D"));
    nourstest_true(targets[6]._hash == mace_hash("F"));

    mace_build_order_targets(targets, target_num);
    assert(build_order != NULL);

    // /* Print build order names */
    // for (int i = 0; i < target_num; ++i) {
    //     printf("%s ",  targets[build_order[i]]._name);
    // }
    // printf("\n");

    /* A should be compiled last, has the most dependencies */
    uint64_t A_hash = mace_hash("A");
    int A_order     = mace_hash_order(A_hash);
    assert(A_order >= 0);

    nourstest_true(target_num == 7);
    nourstest_true(build_order[0]);
    nourstest_true(build_order[target_num - 1] == A_order);

    mace_finish(NULL);
    mace_pre_user();

    /* mace computing build order as a function of linking dependencies */
    struct Target AA = { /* Unitialized values guaranteed to be 0 / NULL */
        .links              = "DD",
        .dependencies       = "EE",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target BB = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .links              = "AA CC DD",
        .dependencies       = "AA CC DD",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target CC = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target DD = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .dependencies       = "FF GG",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target EE = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .links              = "GG",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target FF = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target GG = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .kind               = MACE_EXECUTABLE,
    };

    MACE_ADD_TARGET(BB);
    MACE_ADD_TARGET(EE);
    MACE_ADD_TARGET(GG);
    MACE_ADD_TARGET(CC);
    MACE_ADD_TARGET(FF);
    MACE_ADD_TARGET(AA);
    MACE_ADD_TARGET(DD);
    nourstest_true(target_num == 7);
    nourstest_true(strcmp(targets[0]._name, "BB") == 0);
    nourstest_true(strcmp(targets[1]._name, "EE") == 0);
    nourstest_true(strcmp(targets[2]._name, "GG") == 0);
    nourstest_true(strcmp(targets[3]._name, "CC") == 0);
    nourstest_true(strcmp(targets[4]._name, "FF") == 0);
    nourstest_true(strcmp(targets[5]._name, "AA") == 0);
    nourstest_true(strcmp(targets[6]._name, "DD") == 0);

    nourstest_true(targets[0]._hash == mace_hash("BB"));
    nourstest_true(targets[1]._hash == mace_hash("EE"));
    nourstest_true(targets[2]._hash == mace_hash("GG"));
    nourstest_true(targets[3]._hash == mace_hash("CC"));
    nourstest_true(targets[4]._hash == mace_hash("FF"));
    nourstest_true(targets[5]._hash == mace_hash("AA"));
    nourstest_true(targets[6]._hash == mace_hash("DD"));

    nourstest_true(targets[0]._deps_links[0] == mace_hash("AA"));
    nourstest_true(targets[0]._deps_links[1] == mace_hash("CC"));
    nourstest_true(targets[0]._deps_links[2] == mace_hash("DD"));

    mace_build_order_targets(targets, target_num);
    assert(build_order != NULL);

    // /* Print build order names */
    // for (int i = 0; i < target_num; ++i) {
    //     printf("%s ",  targets[build_order[i]]._name);
    // }
    // printf("\n");

    /* A should be compiled last, has the most dependencies */
    uint64_t BB_hash = mace_hash("BB");
    int BB_order     = mace_hash_order(BB_hash);
    assert(BB_order >= 0);

    nourstest_true(target_num == 7);
    nourstest_true(build_order[0]);
    nourstest_true(build_order[target_num - 1] == BB_order);

    mace_finish(NULL);
}

void test_circular() {
    mace_pre_user();
    /* mace detect circular dependency */
    struct Target A = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .links              = "B C D",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target B = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .links              = "D E",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target D = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .links              = "F G",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target C = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target E = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .links              = "G",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target F = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .links              = "D",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target G = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .kind               = MACE_EXECUTABLE,
    };

    MACE_ADD_TARGET(B);
    MACE_ADD_TARGET(C);
    MACE_ADD_TARGET(E);
    MACE_ADD_TARGET(A);
    MACE_ADD_TARGET(G);
    MACE_ADD_TARGET(D);
    MACE_ADD_TARGET(F);
    nourstest_true(target_num == 7);

    nourstest_true(mace_circular_deps(targets, target_num));
    mace_finish(NULL);
    target_num = 0; /* cleanup so that mace doesn't build targets */
}

void test_self_dependency() {
    mace_pre_user();
    struct Target H = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .links              = "H",
        .kind               = MACE_EXECUTABLE,
    };
    MACE_ADD_TARGET(H);
    mace_circular_deps(targets, target_num); /* Should print a warning*/

    mace_finish(NULL);
    target_num = 0; /* cleanup so that mace doesn't build targets */
}

void test_argv() {
    const char *includes    = "A B C D";
    const char *links       = "ta mere putain de merde";
    const char *sources     = "a.c bd.c efg.c hijk.c lmnop.c";
    int len                 = 8;
    int argc                = 0;
    char **argv             = calloc(8, sizeof(*argv));

    mace_set_obj_dir(MACE_TEST_OBJ_DIR);
    mace_set_build_dir(MACE_TEST_BUILD_DIR);
    mace_mkdir(obj_dir);
    mace_mkdir(build_dir);

    argv = mace_argv_flags(&len, &argc, argv, includes, "-I", false);
    nourstest_true(argc == 4);
    nourstest_true(len == 8);
    nourstest_true(strcmp(argv[0], "-IA") == 0);
    nourstest_true(strcmp(argv[1], "-IB") == 0);
    nourstest_true(strcmp(argv[2], "-IC") == 0);
    nourstest_true(strcmp(argv[3], "-ID") == 0);

    argv = mace_argv_flags(&len, &argc, argv, links, "-l", false);
    nourstest_true(argc == 9);
    nourstest_true(len == 16);
    nourstest_true(strcmp(argv[4], "-lta")     == 0);
    nourstest_true(strcmp(argv[5], "-lmere")   == 0);
    nourstest_true(strcmp(argv[6], "-lputain") == 0);
    nourstest_true(strcmp(argv[7], "-lde")     == 0);
    nourstest_true(strcmp(argv[8], "-lmerde")  == 0);

    argv = mace_argv_flags(&len, &argc, argv, sources, NULL, false);
    nourstest_true(argc == 14);
    nourstest_true(len == 16);
    nourstest_true(strcmp(argv[9],  "a.c")     == 0);
    nourstest_true(strcmp(argv[10], "bd.c")    == 0);
    nourstest_true(strcmp(argv[11], "efg.c")   == 0);
    nourstest_true(strcmp(argv[12], "hijk.c")  == 0);
    nourstest_true(strcmp(argv[13], "lmnop.c") == 0);
    mace_argv_free(argv, argc);

    struct Target CodenameFiresaga = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = ". include  include/bars  include/menu include/popup "
                              "include/systems names names/popup names/menu "
                              "second_party/nstr second_party/noursmath second_party/tnecs "
                              "third_party/physfs third_party/tinymt third_party/stb "
                              "third_party/cJson",
        .sources            = "src/ src/bars/ src/menu/ src/popup/ src/systems/ src/game/",
        .links              = "tnecs",
        .kind               = MACE_EXECUTABLE,
    };

    char buffer[PATH_MAX] = {0};
    mace_Target_Parse_User(&CodenameFiresaga);
    nourstest_true(CodenameFiresaga._argc_includes == 16);
    nourstest_true(CodenameFiresaga._argc_flags == 0);
    nourstest_true(CodenameFiresaga._argc_links == 1);
    strncpy(buffer, "-I", 2);
    strncpy(buffer + 2, cwd, strlen(cwd));

    nourstest_true(strcmp(CodenameFiresaga._argv_includes[0],  buffer)                     == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[1],  "-Iinclude")                == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[2],  "-Iinclude/bars")           == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[3],  "-Iinclude/menu")           == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[4],  "-Iinclude/popup")          == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[5],  "-Iinclude/systems")        == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[6],  "-Inames")                  == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[7],  "-Inames/popup")            == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[8],  "-Inames/menu")             == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[9],  "-Isecond_party/nstr")      == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[10], "-Isecond_party/noursmath") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[11], "-Isecond_party/tnecs")     == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[12], "-Ithird_party/physfs")     == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[13], "-Ithird_party/tinymt")     == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[14], "-Ithird_party/stb")        == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[15], "-Ithird_party/cJson")      == 0);
    nourstest_true(CodenameFiresaga._argv_flags == NULL);

    // MACE_SET_COMPILER(gcc);
    mace_Target_argv_init(&CodenameFiresaga);
    assert(CodenameFiresaga._argv != NULL);
    nourstest_true(CodenameFiresaga._arg_len == 32);
    nourstest_true(strcmp(CodenameFiresaga._argv[MACE_ARGV_CC], "gcc") == 0);
    nourstest_true(CodenameFiresaga._argv[MACE_ARGV_SOURCE] == NULL);
    nourstest_true(CodenameFiresaga._argv[MACE_ARGV_OBJECT] == NULL);
    nourstest_true(strcmp(CodenameFiresaga._argv[3],  buffer)                     == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[4],  "-Iinclude")                == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[5],  "-Iinclude/bars")           == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[6],  "-Iinclude/menu")           == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[7],  "-Iinclude/popup")          == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[8],  "-Iinclude/systems")        == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[9],  "-Inames")                  == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[10], "-Inames/popup")            == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[11], "-Inames/menu")             == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[12], "-Isecond_party/nstr")      == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[13], "-Isecond_party/noursmath") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[14], "-Isecond_party/tnecs")     == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[15], "-Ithird_party/physfs")     == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[16], "-Ithird_party/tinymt")     == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[17], "-Ithird_party/stb")        == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[18], "-Ithird_party/cJson")      == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[19], "-ltnecs")                  == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[20], "-Lbuild")                  == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[21], "-c")                       == 0);
    nourstest_true(CodenameFiresaga._argv[22] == NULL);

    mace_Target_Free(&CodenameFiresaga);
    mace_finish(NULL);
}

void test_post_user() {
    pid_t pid;
    int status;

    // mace does not exit if nothing is wrong
    mace_pre_user();
    struct Target tnecs = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .kind               = MACE_STATIC_LIBRARY,
    };
    MACE_ADD_TARGET(tnecs);
    struct Mace_Arguments args = Mace_Arguments_default;

    pid = fork();
    if (pid < 0) {
        perror("Error: forking issue. \n");
        exit(ENOENT);
    } else if (pid == 0) {
        /* -- redirect stderr and stdout to /dev/null -- */
        int fd = open("/dev/null", O_WRONLY | O_CREAT, 0666);
        dup2(fd, fileno(stderr));
        dup2(fd, fileno(stdout));
        mace_post_user(args);
        close(fd);
        exit(0);
    }
    nourstest_true(waitpid(pid, &status, 0) > 0);
    nourstest_true(WEXITSTATUS(status) == 0);

    // mace exits as expected if CC is NULL
    pid = fork();
    if (pid < 0) {
        perror("Error: forking issue. \n");
        exit(ENOENT);
    } else if (pid == 0) {
        cc = NULL;
        /* -- redirect stderr and stdout to /dev/null -- */
        int fd = open("/dev/null", O_WRONLY | O_CREAT, 0666);  // open the file /dev/null
        dup2(fd, fileno(stderr));
        dup2(fd, fileno(stdout));
        mace_post_user(args);
        close(fd);
        exit(0);
    }

    nourstest_true(waitpid(pid, &status, 0) > 0);
    nourstest_true(WEXITSTATUS(status) == ENXIO);

    // MACE_SET_COMPILER(gcc);
    mace_finish(NULL);
}

void test_separator() {
    mace_pre_user();
    mace_set_separator(",");
    nourstest_true(strcmp(mace_separator, ",") == 0);
    struct Target tnecs = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .links              = "tnecs,baka,ta,mere",
        .kind               = MACE_STATIC_LIBRARY,
    };
    MACE_ADD_TARGET(tnecs);

    mace_Target_Parse_User(&targets[0]);
    nourstest_true(targets[0]._argc_links == 4);
    nourstest_true(strcmp(targets[0]._argv_links[0], "-ltnecs") == 0);
    nourstest_true(strcmp(targets[0]._argv_links[1], "-lbaka") == 0);
    nourstest_true(strcmp(targets[0]._argv_links[2], "-lta") == 0);
    nourstest_true(strcmp(targets[0]._argv_links[3], "-lmere") == 0);

    mace_set_separator(" ");
    struct Target tnecs2 = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .links              = "tnecs,baka,ta,mere",
        .kind               = MACE_STATIC_LIBRARY,
    };
    MACE_ADD_TARGET(tnecs2);

    mace_Target_Parse_User(&targets[1]);
    nourstest_true(targets[1]._argc_links == 1);
    nourstest_true(strcmp(targets[1]._argv_links[0], "-ltnecs,baka,ta,mere") == 0);

    mace_finish(NULL);

    int pid, status;
    // mace exits as expected if separator is NULL
    pid = fork();
    status;
    if (pid < 0) {
        perror("Error: forking issue. \n");
        exit(ENOENT);
    } else if (pid == 0) {
        cc = NULL;
        /* -- redirect stderr and stdout to /dev/null -- */
        int fd = open("/dev/null", O_WRONLY | O_CREAT, 0666);  // open the file /dev/null
        dup2(fd, fileno(stderr));
        dup2(fd, fileno(stdout));
        mace_set_separator(NULL);
        close(fd);
        exit(0);
    }
    nourstest_true(waitpid(pid, &status, 0) > 0);
    nourstest_true(WEXITSTATUS(status) == EPERM);

    // mace exits as expected if separator is too long
    pid = fork();
    status;
    if (pid < 0) {
        perror("Error: forking issue. \n");
        exit(ENOENT);
    } else if (pid == 0) {
        cc = NULL;
        /* -- redirect stderr and stdout to /dev/null -- */
        int fd = open("/dev/null", O_WRONLY | O_CREAT, 0666);  // open the file /dev/null
        dup2(fd, fileno(stderr));
        dup2(fd, fileno(stdout));
        mace_set_separator("Aaaa");
        close(fd);
        exit(0);
    }
    nourstest_true(waitpid(pid, &status, 0) > 0);
    nourstest_true(WEXITSTATUS(status) == EPERM);

    // mace exits as expected if separator is too short
    pid = fork();
    status;
    if (pid < 0) {
        perror("Error: forking issue. \n");
        exit(ENOENT);
    } else if (pid == 0) {
        cc = NULL;
        /* -- redirect stderr and stdout to /dev/null -- */
        int fd = open("/dev/null", O_WRONLY | O_CREAT, 0666);  // open the file /dev/null
        dup2(fd, fileno(stderr));
        dup2(fd, fileno(stdout));
        mace_set_separator("");
        close(fd);
        exit(0);
    }
    nourstest_true(waitpid(pid, &status, 0) > 0);
    nourstest_true(WEXITSTATUS(status) == EPERM);

    // No issues for " "
    pid = fork();
    status;
    if (pid < 0) {
        perror("Error: forking issue. \n");
        exit(ENOENT);
    } else if (pid == 0) {
        cc = NULL;
        /* -- redirect stderr and stdout to /dev/null -- */
        int fd = open("/dev/null", O_WRONLY | O_CREAT, 0666);  // open the file /dev/null
        dup2(fd, fileno(stderr));
        dup2(fd, fileno(stdout));
        mace_set_separator(" ");
        close(fd);
        exit(0);
    }
    nourstest_true(waitpid(pid, &status, 0) > 0);
    nourstest_true(WEXITSTATUS(status) == 0);

    // No issues for ","
    pid = fork();
    status;
    if (pid < 0) {
        perror("Error: forking issue. \n");
        exit(ENOENT);
    } else if (pid == 0) {
        cc = NULL;
        /* -- redirect stderr and stdout to /dev/null -- */
        int fd = open("/dev/null", O_WRONLY | O_CREAT, 0666);  // open the file /dev/null
        dup2(fd, fileno(stderr));
        dup2(fd, fileno(stdout));
        mace_set_separator(",");
        close(fd);
        exit(0);
    }
    nourstest_true(waitpid(pid, &status, 0) > 0);
    nourstest_true(WEXITSTATUS(status) == 0);
}
void test_parse_args() {
    struct Mace_Arguments args;
    int len                 = 8;
    int argc                = 0;
    char **argv             = calloc(len, sizeof(*argv));

    const char *command_1     = "mace clean";
    argv = mace_argv_flags(&len, &argc, argv, command_1, NULL, false);
    args = mace_parse_args(argc, argv);
    nourstest_true(args.user_target_hash == mace_hash(MACE_CLEAN));
    nourstest_true(strcmp(args.user_target, MACE_CLEAN) == 0);
    nourstest_true(args.jobs             == 1);
    nourstest_true(args.macefile         == NULL);
    nourstest_true(args.dir              == NULL);
    nourstest_true(args.debug            == false);
    nourstest_true(args.silent           == false);
    nourstest_true(args.dry_run          == false);
    nourstest_true(args.skip_checksum    == false);
    Mace_Arguments_Free(&args);
    mace_argv_free(argv, argc);
    argc = 0;

    const char *command_2     = "mace all";
    argv = calloc(len, sizeof(*argv));
    argv = mace_argv_flags(&len, &argc, argv, command_2, NULL, false);
    args = mace_parse_args(argc, argv);
    nourstest_true(args.user_target_hash == mace_hash(MACE_ALL));
    nourstest_true(args.jobs             == 1);
    nourstest_true(args.macefile         == NULL);
    nourstest_true(args.dir              == NULL);
    nourstest_true(args.debug            == false);
    nourstest_true(args.silent           == false);
    nourstest_true(args.dry_run          == false);
    nourstest_true(args.skip_checksum    == false);
    Mace_Arguments_Free(&args);
    mace_argv_free(argv, argc);
    argc = 0;

#define TARGET "AAAAA"
    const char *command_3     = "mace " TARGET;
    argv = calloc(len, sizeof(*argv));
    argv = mace_argv_flags(&len, &argc, argv, command_3, NULL, false);
    args = mace_parse_args(argc, argv);
    nourstest_true(args.user_target_hash == mace_hash(TARGET));
    nourstest_true(args.jobs             == 1);
    nourstest_true(args.macefile         == NULL);
    nourstest_true(args.dir              == NULL);
    nourstest_true(args.debug            == false);
    nourstest_true(args.silent           == false);
    nourstest_true(args.dry_run          == false);
    nourstest_true(args.skip_checksum    == false);
    Mace_Arguments_Free(&args);
    mace_argv_free(argv, argc);
    argc = 0;
#undef TARGET

#define TARGET "baka"
    const char *command_4     = "mace -B " TARGET ;
    argv = calloc(len, sizeof(*argv));
    argv = mace_argv_flags(&len, &argc, argv, command_4, NULL, false);
    args =  mace_parse_args(argc, argv);
    nourstest_true(args.user_target_hash == mace_hash(TARGET));
    nourstest_true(args.skip_checksum    == true);
    nourstest_true(args.jobs             == 1);
    nourstest_true(args.macefile         == NULL);
    nourstest_true(args.dir              == NULL);
    nourstest_true(args.debug            == false);
    nourstest_true(args.silent           == false);
    nourstest_true(args.dry_run          == false);
    Mace_Arguments_Free(&args);
    mace_argv_free(argv, argc);
    argc = 0;
#undef TARGET

    const char *command_5     = "mace -B";
    argv = calloc(len, sizeof(*argv));
    argv = mace_argv_flags(&len, &argc, argv, command_5, NULL, false);
    args = mace_parse_args(argc, argv);
    nourstest_true(args.user_target_hash == 0);
    nourstest_true(args.skip_checksum    == true);
    nourstest_true(args.jobs             == 1);
    nourstest_true(args.macefile         == NULL);
    nourstest_true(args.dir              == NULL);
    nourstest_true(args.debug            == false);
    nourstest_true(args.silent           == false);
    nourstest_true(args.dry_run          == false);
    Mace_Arguments_Free(&args);
    mace_argv_free(argv, argc);
    argc = 0;

    const char *command_6     = "mace -Cmydir";
    argv = calloc(len, sizeof(*argv));
    argv = mace_argv_flags(&len, &argc, argv, command_6, NULL, false);
    args = mace_parse_args(argc, argv);
    nourstest_true(args.user_target_hash == 0);
    nourstest_true(args.skip_checksum    == false);
    nourstest_true(args.jobs             == 1);
    nourstest_true(strcmp(args.dir, "mydir") == 0);
    nourstest_true(args.macefile         == NULL);
    nourstest_true(args.debug            == false);
    nourstest_true(args.silent           == false);
    nourstest_true(args.dry_run          == false);
    Mace_Arguments_Free(&args);
    mace_argv_free(argv, argc);
    argc = 0;

    const char *command_7     = "mace -d";
    argv = calloc(len, sizeof(*argv));
    argv = mace_argv_flags(&len, &argc, argv, command_7, NULL, false);
    args = mace_parse_args(argc, argv);
    nourstest_true(args.user_target_hash == 0);
    nourstest_true(args.skip_checksum    == false);
    nourstest_true(args.jobs             == 1);
    nourstest_true(args.dir              == NULL);
    nourstest_true(args.macefile         == NULL);
    nourstest_true(args.debug            == true);
    nourstest_true(args.silent           == false);
    nourstest_true(args.dry_run          == false);
    Mace_Arguments_Free(&args);
    mace_argv_free(argv, argc);
    argc = 0;

    const char *command_8     = "mace -fmymacefile.c";
    argv = calloc(len, sizeof(*argv));
    argv = mace_argv_flags(&len, &argc, argv, command_8, NULL, false);
    args = mace_parse_args(argc, argv);
    nourstest_true(args.user_target_hash == 0);
    nourstest_true(args.skip_checksum    == false);
    nourstest_true(args.jobs             == 1);
    nourstest_true(strcmp(args.macefile, "mymacefile.c") == 0);
    nourstest_true(args.dir              == NULL);
    nourstest_true(args.debug            == false);
    nourstest_true(args.silent           == false);
    nourstest_true(args.dry_run          == false);
    Mace_Arguments_Free(&args);
    mace_argv_free(argv, argc);
    argc = 0;

    const char *command_9     = "mace -j2";
    argv = calloc(len, sizeof(*argv));
    argv = mace_argv_flags(&len, &argc, argv, command_9, NULL, false);
    args = mace_parse_args(argc, argv);
    nourstest_true(args.user_target_hash == 0);
    nourstest_true(args.skip_checksum    == false);
    nourstest_true(args.jobs             == 2);
    nourstest_true(args.macefile         == NULL);
    nourstest_true(args.dir              == NULL);
    nourstest_true(args.debug            == false);
    nourstest_true(args.silent           == false);
    nourstest_true(args.dry_run          == false);
    Mace_Arguments_Free(&args);
    mace_argv_free(argv, argc);
    argc = 0;

    const char *command_10     = "mace -n";
    argv = calloc(len, sizeof(*argv));
    argv = mace_argv_flags(&len, &argc, argv, command_10, NULL, false);
    args = mace_parse_args(argc, argv);
    nourstest_true(args.user_target_hash == 0);
    nourstest_true(args.skip_checksum    == false);
    nourstest_true(args.jobs             == 1);
    nourstest_true(args.macefile         == NULL);
    nourstest_true(args.dir              == NULL);
    nourstest_true(args.debug            == false);
    nourstest_true(args.silent           == false);
    nourstest_true(args.dry_run          == true);
    Mace_Arguments_Free(&args);
    mace_argv_free(argv, argc);
    argc = 0;

    const char *command_11     = "mace -n -otnecs";
    argv = calloc(len, sizeof(*argv));
    argv = mace_argv_flags(&len, &argc, argv, command_11, NULL, false);
    args = mace_parse_args(argc, argv);
    nourstest_true(args.user_target_hash == 0);
    nourstest_true(args.skip_checksum    == false);
    nourstest_true(args.skip             == mace_hash("tnecs"));
    nourstest_true(args.jobs             == 1);
    nourstest_true(args.macefile         == NULL);
    nourstest_true(args.dir              == NULL);
    nourstest_true(args.debug            == false);
    nourstest_true(args.silent           == false);
    nourstest_true(args.dry_run          == true);
    Mace_Arguments_Free(&args);
    mace_argv_free(argv, argc);
    argc = 0;

    const char *command_12     = "mace allo -s -d -n -otnecs";
    argv = calloc(len, sizeof(*argv));
    argv = mace_argv_flags(&len, &argc, argv, command_12, NULL, false);
    args = mace_parse_args(argc, argv);
    nourstest_true(args.user_target_hash == mace_hash("allo"));
    nourstest_true(strcmp(args.user_target, "allo") == 0);
    nourstest_true(args.skip_checksum    == false);
    nourstest_true(args.skip             == mace_hash("tnecs"));
    nourstest_true(args.jobs             == 1);
    nourstest_true(args.macefile         == NULL);
    nourstest_true(args.dir              == NULL);
    nourstest_true(args.debug            == true);
    nourstest_true(args.silent           == true);
    nourstest_true(args.dry_run          == true);
    Mace_Arguments_Free(&args);
    mace_argv_free(argv, argc);
    argc = 0;
}

void test_build_order() {
    /* cleanup so that mace doesn't build targets */
    mace_finish(NULL);
    mace_pre_user();

    /* mace computing build order as a function of linking dependencies */
    struct Target A = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .links              = "B C D",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target B = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .links              = "D E",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target C = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target D = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .links              = "F G",
        .kind               = MACE_EXECUTABLE,
    };


    struct Target E = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .links              = "G",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target F = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .kind               = MACE_EXECUTABLE,
    };

    struct Target G = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .kind               = MACE_EXECUTABLE,
    };

    MACE_ADD_TARGET(B);
    MACE_ADD_TARGET(C);
    MACE_ADD_TARGET(E);
    MACE_ADD_TARGET(A);
    MACE_ADD_TARGET(G);
    MACE_ADD_TARGET(D);
    MACE_ADD_TARGET(F);
    nourstest_true(target_num == 7);
    nourstest_true(strcmp(targets[0]._name, "B") == 0);
    nourstest_true(strcmp(targets[1]._name, "C") == 0);
    nourstest_true(strcmp(targets[2]._name, "E") == 0);
    nourstest_true(strcmp(targets[3]._name, "A") == 0);
    nourstest_true(strcmp(targets[4]._name, "G") == 0);
    nourstest_true(strcmp(targets[5]._name, "D") == 0);
    nourstest_true(strcmp(targets[6]._name, "F") == 0);

    nourstest_true(targets[0]._hash == mace_hash("B"));
    nourstest_true(targets[1]._hash == mace_hash("C"));
    nourstest_true(targets[2]._hash == mace_hash("E"));
    nourstest_true(targets[3]._hash == mace_hash("A"));
    nourstest_true(targets[4]._hash == mace_hash("G"));
    nourstest_true(targets[5]._hash == mace_hash("D"));
    nourstest_true(targets[6]._hash == mace_hash("F"));

    mace_build_order_targets(targets, target_num);
    assert(build_order != NULL);
    nourstest_true(build_order[0] == mace_hash_order(mace_hash("F")));
    nourstest_true(build_order[1] == mace_hash_order(mace_hash("G")));
    nourstest_true(build_order[2] == mace_hash_order(mace_hash("D")));
    nourstest_true(build_order[3] == mace_hash_order(mace_hash("E")));
    nourstest_true(build_order[4] == mace_hash_order(mace_hash("B")));
    nourstest_true(build_order[5] == mace_hash_order(mace_hash("C")));
    nourstest_true(build_order[6] == mace_hash_order(mace_hash("A")));
    build_order_num = 0;

    // set default_target to "D" check build_order
    mace_default_target = mace_hash_order(mace_hash("D"));
    mace_user_target    = MACE_NULL_ORDER;  /* order */
    mace_build_order_targets(targets, target_num);
    nourstest_true(build_order_num == 3);

    nourstest_true(build_order[0] == mace_hash_order(mace_hash("F")));
    nourstest_true(build_order[1] == mace_hash_order(mace_hash("G")));
    nourstest_true(build_order[2] == mace_hash_order(mace_hash("D")));
    build_order_num = 0;

    // set user_target to E check build_order
    // user_target should override mace_default_target
    mace_user_target    = mace_hash_order(mace_hash("E"));
    mace_default_target = mace_hash_order(mace_hash("D"));
    mace_build_order_targets(targets, target_num);
    nourstest_true(build_order_num == 2);

    nourstest_true(build_order[0] == mace_hash_order(mace_hash("G")));
    nourstest_true(build_order[1] == mace_hash_order(mace_hash("E")));


    // set user_target to A check build_order
    // user_target should override mace_default_target
    mace_user_target    = mace_hash_order(mace_hash("A"));
    mace_default_target = mace_hash_order(mace_hash("D"));
    mace_build_order_targets(targets, target_num);
    nourstest_true(build_order_num == 7);

    nourstest_true(build_order[0] == mace_hash_order(mace_hash("G")));
    nourstest_true(build_order[1] == mace_hash_order(mace_hash("E")));
    nourstest_true(build_order[2] == mace_hash_order(mace_hash("F")));
    nourstest_true(build_order[3] == mace_hash_order(mace_hash("D")));
    nourstest_true(build_order[4] == mace_hash_order(mace_hash("B")));
    nourstest_true(build_order[5] == mace_hash_order(mace_hash("C")));
    nourstest_true(build_order[6] == mace_hash_order(mace_hash("A")));

    mace_default_target = MACE_ALL_ORDER;   /* order */
    mace_user_target    = MACE_NULL_ORDER;  /* order */
}

void test_checksum() {
    mace_pre_user();
    mace_set_obj_dir("obj");
    char *allo = mace_checksum_filename("allo.c", MACE_CHECKSUM_MODE_NULL);
    nourstest_true(strcmp(allo, "obj/allo.sha1") == 0);
    free(allo);
    allo = mace_checksum_filename("allo.h", MACE_CHECKSUM_MODE_NULL);
    nourstest_true(strcmp(allo, "obj/allo.sha1") == 0);
    free(allo);
    allo = mace_checksum_filename("src/allo.h", MACE_CHECKSUM_MODE_NULL);
    nourstest_true(strcmp(allo, "obj/allo.sha1") == 0);
    free(allo);

    char *header_objpath = mace_checksum_filename("/home/gabinours/Sync/Firesaga/include/combat.h",
                                                  MACE_CHECKSUM_MODE_NULL);
    nourstest_true(strcmp(header_objpath, "obj/combat.sha1") == 0);
    free(header_objpath);

    header_objpath = mace_checksum_filename("/home/gabinours/Sync/Firesaga/include/combat.h",
                                            MACE_CHECKSUM_MODE_INCLUDE);
    nourstest_true(strcmp(header_objpath, "obj/include/combat.sha1") == 0);
    free(header_objpath);
    header_objpath = mace_checksum_filename("/home/gabinours/Sync/Firesaga/include/combat.h",
                                            MACE_CHECKSUM_MODE_SRC);
    nourstest_true(strcmp(header_objpath, "obj/src/combat.sha1") == 0);
    free(header_objpath);

    mace_finish(NULL);
}

void test_excludes() {
    mace_pre_user();
    char *rpath = calloc(PATH_MAX, sizeof(*rpath));
    char *token = "tnecs.c";
    realpath(token, rpath);
    FILE *fd = fopen(rpath, "w");
    fclose(fd);
    struct Target tnecs = { /* Unitialized values guaranteed to be 0 / NULL */
        .excludes           = "tnecs.c",
    };

    mace_Target_excludes(&tnecs);
    nourstest_true(tnecs._excludes_num == 1);
    nourstest_true(tnecs._excludes[0] == mace_hash(rpath));
    remove(rpath);

    mace_Target_Free(&tnecs);
    free(rpath);
    mace_finish(NULL);
}

void test_parse_d() {
    mace_pre_user();

    struct Target target1 = {0};
    MACE_ADD_TARGET(target1);
    mace_Target_Source_Add(&target1, "test1.c");
    mace_Target_Object_Add(&target1, "test1.o");
    mace_Target_Parse_Objdep(&target1, 0);
    assert(target1._headers_hash != NULL);
    nourstest_true(target1._headers_num == 1);
    nourstest_true(target1._deps_headers_num[0] == 1);
    nourstest_true(target1._headers_len == 8);
    nourstest_true(target1._headers_hash[0] == mace_hash("tnecs.h"));

    struct Target target = {
        .includes           = "tnecs.h",
    };
    assert(target_num == 1);
    MACE_ADD_TARGET(target);
    assert(target_num == 2);

    mace_Target_Source_Add(&target, "test2.c");
    mace_Target_Object_Add(&target, "test2.o");
    mace_Target_Parse_Objdep(&target, 0);
    assert(target._headers_hash != NULL);
    nourstest_true(target._headers_num == 72);
    nourstest_true(target._deps_headers_num[0] == 72);
    nourstest_true(target._deps_headers_num[0] == 72);
    nourstest_true(target._headers_len == 128);
// *INDENT-OFF*
    nourstest_true(target._headers_hash[0] ==  mace_hash("/home/gabinours/Sync/Firesaga/include/unit.h"));
    nourstest_true(target._headers_hash[1] ==  mace_hash("/home/gabinours/Sync/Firesaga/include/types.h"));
    nourstest_true(target._headers_hash[2] ==  mace_hash("/home/gabinours/Sync/Firesaga/include/enums.h"));
    nourstest_true(target._headers_hash[3] ==  mace_hash("/home/gabinours/Sync/Firesaga/names/mounts_types.h"));
    nourstest_true(target._headers_hash[4] ==  mace_hash("/home/gabinours/Sync/Firesaga/names/mounts.h"));
    nourstest_true(target._headers_hash[5] ==  mace_hash("/home/gabinours/Sync/Firesaga/names/chapters.h"));
    nourstest_true(target._headers_hash[6] ==  mace_hash("/home/gabinours/Sync/Firesaga/names/shops.h"));
    nourstest_true(target._headers_hash[7] ==  mace_hash("/home/gabinours/Sync/Firesaga/names/menu/options.h"));
    nourstest_true(target._headers_hash[8] ==  mace_hash("/home/gabinours/Sync/Firesaga/names/items.h"));
    nourstest_true(target._headers_hash[9] ==  mace_hash("/home/gabinours/Sync/Firesaga/names/units_stats.h"));
    nourstest_true(target._headers_hash[10] == mace_hash("/home/gabinours/Sync/Firesaga/names/skills_passive.h"));
    nourstest_true(target._headers_hash[11] == mace_hash("/home/gabinours/Sync/Firesaga/names/skills_active.h"));
    nourstest_true(target._headers_hash[12] == mace_hash("/home/gabinours/Sync/Firesaga/names/items_types.h"));
    nourstest_true(target._headers_hash[13] == mace_hash("/home/gabinours/Sync/Firesaga/names/items_stats.h"));
    nourstest_true(target._headers_hash[14] == mace_hash("/home/gabinours/Sync/Firesaga/names/weapon_stats.h"));
    nourstest_true(target._headers_hash[15] == mace_hash("/home/gabinours/Sync/Firesaga/names/units_PC.h"));
    nourstest_true(target._headers_hash[16] == mace_hash("/home/gabinours/Sync/Firesaga/names/units_NPC.h"));
    nourstest_true(target._headers_hash[17] == mace_hash("/home/gabinours/Sync/Firesaga/names/items_effects.h"));
    nourstest_true(target._headers_hash[18] == mace_hash("/home/gabinours/Sync/Firesaga/names/camp_jobs.h"));
    nourstest_true(target._headers_hash[19] == mace_hash("/home/gabinours/Sync/Firesaga/names/classes.h"));
    nourstest_true(target._headers_hash[20] == mace_hash("/home/gabinours/Sync/Firesaga/names/units_types.h"));
    nourstest_true(target._headers_hash[21] == mace_hash("/home/gabinours/Sync/Firesaga/names/armies.h"));
    nourstest_true(target._headers_hash[22] == mace_hash("/home/gabinours/Sync/Firesaga/names/units_statuses.h"));
    nourstest_true(target._headers_hash[23] == mace_hash("/home/gabinours/Sync/Firesaga/names/tiles.h"));
    nourstest_true(target._headers_hash[24] == mace_hash("/home/gabinours/Sync/Firesaga/names/game_states.h"));
    nourstest_true(target._headers_hash[25] == mace_hash("/home/gabinours/Sync/Firesaga/names/game_substates.h"));
    nourstest_true(target._headers_hash[26] == mace_hash("/home/gabinours/Sync/Firesaga/names/json_elements.h"));
    nourstest_true(target._headers_hash[27] == mace_hash("/home/gabinours/Sync/Firesaga/names/mvt_types.h"));
    nourstest_true(target._headers_hash[28] == mace_hash("/home/gabinours/Sync/Firesaga/names/buttons.h"));
    nourstest_true(target._headers_hash[29] == mace_hash("/home/gabinours/Sync/Firesaga/names/popup/types.h"));
    nourstest_true(target._headers_hash[30] == mace_hash("/home/gabinours/Sync/Firesaga/names/menu/types.h"));
    nourstest_true(target._headers_hash[31] == mace_hash("/home/gabinours/Sync/Firesaga/names/menu/player_select.h"));
    nourstest_true(target._headers_hash[32] == mace_hash("/home/gabinours/Sync/Firesaga/names/errors.h"));
    nourstest_true(target._headers_hash[33] == mace_hash("/home/gabinours/Sync/Firesaga/names/input_flags.h"));
    nourstest_true(target._headers_hash[34] == mace_hash("/home/gabinours/Sync/Firesaga/names/scene_time.h"));
    nourstest_true(target._headers_hash[35] == mace_hash("/home/gabinours/Sync/Firesaga/include/structs.h"));
    nourstest_true(target._headers_hash[36] == mace_hash("/home/gabinours/Sync/Firesaga/second_party/noursmath/nmath.h"));
    nourstest_true(target._headers_hash[37] == mace_hash("/home/gabinours/Sync/Firesaga/second_party/tnecs/tnecs.h"));
    nourstest_true(target._headers_hash[38] == mace_hash("/home/gabinours/Sync/Firesaga/include/filesystem.h"));
    nourstest_true(target._headers_hash[39] == mace_hash("/home/gabinours/Sync/Firesaga/include/globals.h"));
    nourstest_true(target._headers_hash[40] == mace_hash("/home/gabinours/Sync/Firesaga/third_party/physfs/physfs.h"));
    nourstest_true(target._headers_hash[41] == mace_hash("/home/gabinours/Sync/Firesaga/include/platform.h"));
    nourstest_true(target._headers_hash[42] == mace_hash("/home/gabinours/Sync/Firesaga/third_party/cJson/cJSON.h"));
    nourstest_true(target._headers_hash[43] == mace_hash("/home/gabinours/Sync/Firesaga/second_party/nstr/nstr.h"));
    nourstest_true(target._headers_hash[44] == mace_hash("/home/gabinours/Sync/Firesaga/include/utilities.h"));
    nourstest_true(target._headers_hash[45] == mace_hash("/home/gabinours/Sync/Firesaga/include/palette.h"));
    nourstest_true(target._headers_hash[46] == mace_hash("/home/gabinours/Sync/Firesaga/include/debug.h"));
    nourstest_true(target._headers_hash[47] == mace_hash("/home/gabinours/Sync/Firesaga/include/macros.h"));
    nourstest_true(target._headers_hash[48] == mace_hash("/home/gabinours/Sync/Firesaga/include/names.h"));
    nourstest_true(target._headers_hash[49] == mace_hash("/home/gabinours/Sync/Firesaga/include/hashes.h"));
    nourstest_true(target._headers_hash[50] == mace_hash("/home/gabinours/Sync/Firesaga/include/supports.h"));
    nourstest_true(target._headers_hash[51] == mace_hash("/home/gabinours/Sync/Firesaga/names/support_types.h"));
    nourstest_true(target._headers_hash[52] == mace_hash("/home/gabinours/Sync/Firesaga/include/jsonio.h"));
    nourstest_true(target._headers_hash[53] == mace_hash("/home/gabinours/Sync/Firesaga/include/tile.h"));
    nourstest_true(target._headers_hash[54] == mace_hash("/home/gabinours/Sync/Firesaga/include/weapon.h"));
    nourstest_true(target._headers_hash[55] == mace_hash("/home/gabinours/Sync/Firesaga/include/item.h"));
    nourstest_true(target._headers_hash[56] == mace_hash("/home/gabinours/Sync/Firesaga/third_party/stb/stb_sprintf.h"));
    nourstest_true(target._headers_hash[57] == mace_hash("/home/gabinours/Sync/Firesaga/include/pixelfonts.h"));
    nourstest_true(target._headers_hash[58] == mace_hash("/home/gabinours/Sync/Firesaga/include/convoy.h"));
    nourstest_true(target._headers_hash[59] == mace_hash("/home/gabinours/Sync/Firesaga/include/camp.h"));
    nourstest_true(target._headers_hash[60] == mace_hash("/home/gabinours/Sync/Firesaga/include/narrative.h"));
    nourstest_true(target._headers_hash[61] == mace_hash("/home/gabinours/Sync/Firesaga/include/bitfields.h"));
    nourstest_true(target._headers_hash[62] == mace_hash("/home/gabinours/Sync/Firesaga/include/RNG.h"));
    nourstest_true(target._headers_hash[63] == mace_hash("/home/gabinours/Sync/Firesaga/third_party/tinymt/tinymt32.h"));
    nourstest_true(target._headers_hash[64] == mace_hash("/home/gabinours/Sync/Firesaga/include/sprite.h"));
    nourstest_true(target._headers_hash[65] == mace_hash("/home/gabinours/Sync/Firesaga/include/index_shader.h"));
    nourstest_true(target._headers_hash[66] == mace_hash("/home/gabinours/Sync/Firesaga/include/map.h"));
    nourstest_true(target._headers_hash[67] == mace_hash("/home/gabinours/Sync/Firesaga/include/arrow.h"));
    nourstest_true(target._headers_hash[68] == mace_hash("/home/gabinours/Sync/Firesaga/include/position.h"));
    nourstest_true(target._headers_hash[69] == mace_hash("/home/gabinours/Sync/Firesaga/include/equations.h"));
    nourstest_true(target._headers_hash[70] == mace_hash("/home/gabinours/Sync/Firesaga/include/combat.h"));
    nourstest_true(target._headers_hash[71] == mace_hash("/home/gabinours/Sync/Firesaga/names/units_struct_stats.h"));

    nourstest_true(strcmp(target._headers[0],"/home/gabinours/Sync/Firesaga/include/unit.h") == 0);
    nourstest_true(strcmp(target._headers[71],"/home/gabinours/Sync/Firesaga/names/units_struct_stats.h") == 0);

// *INDENT-ON*
    nourstest_true(target._deps_headers_num[0] == 72);
    for (int i = 0; i < target._deps_headers_num[0]; i++) {
        nourstest_true(target._deps_headers[0][i] == i);
    }

    assert(target._headers_num == 72);
    assert(target._headers_len == 128);

    nourstest_true(target._headers_checksum[0] != NULL);
    nourstest_true(target._headers_checksum[1] != NULL);
    nourstest_true(target._headers_checksum[2] != NULL);
    nourstest_true(target._headers_checksum[3] != NULL);
    nourstest_true(target._headers_checksum[4] != NULL);
    nourstest_true(target._headers_checksum[5] != NULL);
    nourstest_true(target._headers_checksum[6] != NULL);
    nourstest_true(target._headers_checksum[7] != NULL);
    nourstest_true(target._headers_checksum[8] != NULL);
    nourstest_true(target._headers_checksum[9] != NULL);
    nourstest_true(target._headers_checksum[10] != NULL);
    nourstest_true(target._headers_checksum[11] != NULL);
    nourstest_true(target._headers_checksum[12] != NULL);
    nourstest_true(target._headers_checksum[13] != NULL);
    nourstest_true(target._headers_checksum[14] != NULL);
    nourstest_true(target._headers_checksum[15] != NULL);
    nourstest_true(target._headers_checksum[16] != NULL);
    nourstest_true(target._headers_checksum[17] != NULL);
    nourstest_true(target._headers_checksum[18] != NULL);
    nourstest_true(target._headers_checksum[19] != NULL);
    nourstest_true(target._headers_checksum[20] != NULL);
    nourstest_true(target._headers_checksum[21] != NULL);
    nourstest_true(target._headers_checksum[22] != NULL);
    nourstest_true(target._headers_checksum[23] != NULL);
    nourstest_true(target._headers_checksum[24] != NULL);
    nourstest_true(target._headers_checksum[25] != NULL);
    nourstest_true(target._headers_checksum[26] != NULL);
    nourstest_true(target._headers_checksum[27] != NULL);
    nourstest_true(target._headers_checksum[28] != NULL);
    nourstest_true(target._headers_checksum[29] != NULL);
    nourstest_true(target._headers_checksum[30] != NULL);
    nourstest_true(target._headers_checksum[31] != NULL);
    nourstest_true(target._headers_checksum[32] != NULL);
    nourstest_true(target._headers_checksum[33] != NULL);
    nourstest_true(target._headers_checksum[34] != NULL);
    nourstest_true(target._headers_checksum[35] != NULL);
    nourstest_true(target._headers_checksum[36] != NULL);
    nourstest_true(target._headers_checksum[37] != NULL);
    nourstest_true(target._headers_checksum[38] != NULL);
    nourstest_true(target._headers_checksum[39] != NULL);
    nourstest_true(target._headers_checksum[40] != NULL);
    nourstest_true(target._headers_checksum[41] != NULL);
    nourstest_true(target._headers_checksum[42] != NULL);
    nourstest_true(target._headers_checksum[43] != NULL);
    nourstest_true(target._headers_checksum[44] != NULL);
    nourstest_true(target._headers_checksum[45] != NULL);
    nourstest_true(target._headers_checksum[46] != NULL);
    nourstest_true(target._headers_checksum[47] != NULL);
    nourstest_true(target._headers_checksum[48] != NULL);
    nourstest_true(target._headers_checksum[49] != NULL);
    nourstest_true(target._headers_checksum[50] != NULL);
    nourstest_true(target._headers_checksum[51] != NULL);
    nourstest_true(target._headers_checksum[52] != NULL);
    nourstest_true(target._headers_checksum[53] != NULL);
    nourstest_true(target._headers_checksum[54] != NULL);
    nourstest_true(target._headers_checksum[55] != NULL);
    nourstest_true(target._headers_checksum[56] != NULL);
    nourstest_true(target._headers_checksum[57] != NULL);
    nourstest_true(target._headers_checksum[58] != NULL);
    nourstest_true(target._headers_checksum[59] != NULL);
    nourstest_true(target._headers_checksum[60] != NULL);
    nourstest_true(target._headers_checksum[61] != NULL);
    nourstest_true(target._headers_checksum[62] != NULL);
    nourstest_true(target._headers_checksum[63] != NULL);
    nourstest_true(target._headers_checksum[64] != NULL);
    nourstest_true(target._headers_checksum[65] != NULL);
    nourstest_true(target._headers_checksum[66] != NULL);
    nourstest_true(target._headers_checksum[67] != NULL);
    nourstest_true(target._headers_checksum[68] != NULL);
    nourstest_true(target._headers_checksum[69] != NULL);
    nourstest_true(target._headers_checksum[70] != NULL);
    nourstest_true(target._headers_checksum[71] != NULL);

    nourstest_true(strcmp(target._headers_checksum[0], "obj/include/unit.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[1], "obj/include/types.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[2], "obj/include/enums.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[3], "obj/include/mounts_types.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[4], "obj/include/mounts.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[5], "obj/include/chapters.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[6], "obj/include/shops.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[7], "obj/include/options.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[8], "obj/include/items.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[9], "obj/include/units_stats.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[10], "obj/include/skills_passive.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[11], "obj/include/skills_active.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[12], "obj/include/items_types.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[13], "obj/include/items_stats.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[14], "obj/include/weapon_stats.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[15], "obj/include/units_PC.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[16], "obj/include/units_NPC.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[17], "obj/include/items_effects.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[18], "obj/include/camp_jobs.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[19], "obj/include/classes.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[20], "obj/include/units_types.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[21], "obj/include/armies.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[22], "obj/include/units_statuses.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[23], "obj/include/tiles.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[24], "obj/include/game_states.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[25], "obj/include/game_substates.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[26], "obj/include/json_elements.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[27], "obj/include/mvt_types.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[28], "obj/include/buttons.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[29], "obj/include/types0.sha") == 0);
    nourstest_true(strcmp(target._headers_checksum[30], "obj/include/types1.sha") == 0);
    nourstest_true(strcmp(target._headers_checksum[31], "obj/include/player_select.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[32], "obj/include/errors.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[33], "obj/include/input_flags.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[34], "obj/include/scene_time.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[35], "obj/include/structs.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[36], "obj/include/nmath.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[37], "obj/include/tnecs.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[38], "obj/include/filesystem.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[39], "obj/include/globals.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[40], "obj/include/physfs.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[41], "obj/include/platform.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[42], "obj/include/cJSON.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[43], "obj/include/nstr.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[44], "obj/include/utilities.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[45], "obj/include/palette.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[46], "obj/include/debug.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[47], "obj/include/macros.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[48], "obj/include/names.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[49], "obj/include/hashes.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[50], "obj/include/supports.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[51], "obj/include/support_types.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[52], "obj/include/jsonio.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[53], "obj/include/tile.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[54], "obj/include/weapon.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[55], "obj/include/item.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[56], "obj/include/stb_sprintf.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[57], "obj/include/pixelfonts.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[58], "obj/include/convoy.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[59], "obj/include/camp.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[60], "obj/include/narrative.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[61], "obj/include/bitfields.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[62], "obj/include/RNG.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[63], "obj/include/tinymt32.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[64], "obj/include/sprite.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[65], "obj/include/index_shader.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[66], "obj/include/map.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[67], "obj/include/arrow.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[68], "obj/include/position.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[69], "obj/include/equations.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[70], "obj/include/combat.sha1") == 0);
    nourstest_true(strcmp(target._headers_checksum[71], "obj/include/units_struct_stats.sha1") == 0);

    mace_Target_Free(&target);
    mace_Target_Free(&target1);
    mace_finish(NULL);
}

int mace(int argc, char *argv[]) {
    printf("Testing mace\n");
    MACE_SET_COMPILER(gcc);

    nourstest_run("isFunc ",        test_isFunc);
    nourstest_run("globbing ",      test_globbing);
    nourstest_run("object ",        test_object);
    nourstest_run("target ",        test_target);
    nourstest_run("circular ",      test_circular);
    nourstest_run("argv ",          test_argv);
    nourstest_run("post_user ",     test_post_user);
    nourstest_run("separator ",     test_separator);
    nourstest_run("parse_args ",    test_parse_args);
    nourstest_run("build_order ",   test_build_order);
    nourstest_run("checksum ",      test_checksum);
    nourstest_run("excludes ",      test_excludes);
    nourstest_run("parse_d ",       test_parse_d);
    nourstest_results();

    printf("A warning about self dependency should print now:\n \n");
    test_self_dependency();
    printf("\n");
    printf("Tests done\n");
    return (0);
}
