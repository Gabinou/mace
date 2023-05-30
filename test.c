
#include "mace.h"

/* --- Testing library --- */
#ifndef __NOURSTEST_H__
#define __NOURSTEST_H__
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

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
    nourstest_true(globbed.gl_pathc == 2);
    nourstest_true(strcmp(globbed.gl_pathv[0], "../mace/mace.c") == 0);
    nourstest_true(strcmp(globbed.gl_pathv[1], "../mace/test.c") == 0);
    globfree(&globbed);

    globbed = mace_glob_sources("../mace/*.h");
    nourstest_true(globbed.gl_pathc == 1);
    nourstest_true(strcmp(globbed.gl_pathv[0], "../mace/mace.h") == 0);
    globfree(&globbed);
}

void test_object() {
    if ((object_len == 0) || (object == NULL)) {
        object_len = 24;
        object = malloc(object_len * sizeof(*object));
    }

    mace_object_path("/mace.c");
    nourstest_true(strcmp(object, "/home/gabinours/Sync/mace/obj/mace.o") == 0);
    nourstest_true(mace_isObject(object));
    mace_object_path("mace.c");
    nourstest_true(strcmp(object, "/home/gabinours/Sync/mace/obj/mace.o") == 0);
    nourstest_true(mace_isObject(object));
}

void test_target() {
    struct Target tnecs = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .kind               = MACE_LIBRARY,
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
        .links              = "SDL2 SDL2_image SDL2_ttf m GLEW cJSON nmath physfs tinymt tnecs nstr parg",
        .kind               = MACE_EXECUTABLE,
    };
    MACE_ADD_TARGET(firesaga);
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
    mace_free();
    target_num = 0;
    mace_init();

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

    mace_target_build_order(targets, target_num);
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

    mace_free();
    target_num = 0; /* cleanup so that mace doesn't build targets */
}

void test_circular() {
    mace_init();
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
    mace_free();
    target_num = 0; /* cleanup so that mace doesn't build targets */
}

void test_self_dependency() {
    mace_init();
    struct Target H = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = "tnecs.h",
        .sources            = "tnecs.c",
        .base_dir           = "tnecs",
        .links              = "H",
        .kind               = MACE_EXECUTABLE,
    };
    MACE_ADD_TARGET(H);
    mace_circular_deps(targets, target_num); /* Should print a warning*/

    mace_free();
    target_num = 0; /* cleanup so that mace doesn't build targets */
}

void test_argv() {
    const char *includes = "A B C D";
    const char *links = "ta mere putain de merde";
    const char *sources = "a.c bd.c efg.c hijk.c lmnop.c";
    int len = 8;
    int argc = 0;
    char **argv = calloc(8, sizeof(*argv));

    argv = mace_argv_flags(&len, &argc, argv, includes, "-I");
    nourstest_true(argc == 4);
    nourstest_true(len == 8);
    nourstest_true(strcmp(argv[0], "-IA") == 0);
    nourstest_true(strcmp(argv[1], "-IB") == 0);
    nourstest_true(strcmp(argv[2], "-IC") == 0);
    nourstest_true(strcmp(argv[3], "-ID") == 0);

    argv = mace_argv_flags(&len, &argc, argv, links, "-l");
    nourstest_true(argc == 9);
    nourstest_true(len == 16);
    nourstest_true(strcmp(argv[4], "-lta") == 0);
    nourstest_true(strcmp(argv[5], "-lmere") == 0);
    nourstest_true(strcmp(argv[6], "-lputain") == 0);
    nourstest_true(strcmp(argv[7], "-lde") == 0);
    nourstest_true(strcmp(argv[8], "-lmerde") == 0);

    argv = mace_argv_flags(&len, &argc, argv, sources, NULL);
    nourstest_true(argc == 14);
    nourstest_true(len == 16);
    nourstest_true(strcmp(argv[9], "a.c") == 0);
    nourstest_true(strcmp(argv[10], "bd.c") == 0);
    nourstest_true(strcmp(argv[11], "efg.c") == 0);
    nourstest_true(strcmp(argv[12], "hijk.c") == 0);
    nourstest_true(strcmp(argv[13], "lmnop.c") == 0);

    struct Target CodenameFiresaga = { /* Unitialized values guaranteed to be 0 / NULL */
        .includes           = ". include  include/bars  include/menu include/popup include/systems "
        "names names/popup names/menu "
        "second_party/nstr second_party/noursmath second_party/tnecs "
        "third_party/physfs third_party/tinymt third_party/stb third_party/cJson ",
        .sources            = "src/ src/bars/ src/menu/ src/popup/ src/systems/ src/game/",
        .flags              = "",
        .links              = "tnecs",
        .kind               = MACE_EXECUTABLE,
    };

    Target_Parse_User(&CodenameFiresaga);
    nourstest_true(CodenameFiresaga._argc_includes == 16);
    nourstest_true(CodenameFiresaga._argc_flags == 0);
    nourstest_true(CodenameFiresaga._argc_links == 1);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[0], "-I.") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[1], "-Iinclude") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[2], "-Iinclude/bars") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[3], "-Iinclude/menu") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[4], "-Iinclude/popup") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[5], "-Iinclude/systems") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[6], "-Inames") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[7], "-Inames/popup") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[8], "-Inames/menu") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[9], "-Isecond_party/nstr") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[10], "-Isecond_party/noursmath") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[11], "-Isecond_party/tnecs") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[12], "-Ithird_party/physfs") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[13], "-Ithird_party/tinymt") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[14], "-Ithird_party/stb") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv_includes[15], "-Ithird_party/cJson") == 0);
    nourstest_true(CodenameFiresaga._argv_flags == NULL);

    MACE_SET_COMPILER(gcc);
    Target_argv_init(&CodenameFiresaga);
    assert(CodenameFiresaga._argv != NULL);
    nourstest_true(CodenameFiresaga._arg_len == 64);
    nourstest_true(strcmp(CodenameFiresaga._argv[MACE_ARGV_CC], "gcc") == 0);
    nourstest_true(CodenameFiresaga._argv[MACE_ARGV_SOURCE] == NULL);
    nourstest_true(CodenameFiresaga._argv[MACE_ARGV_OBJECT] == NULL);
    nourstest_true(strcmp(CodenameFiresaga._argv[3], "-I.") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[4], "-Iinclude") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[5], "-Iinclude/bars") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[6], "-Iinclude/menu") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[7], "-Iinclude/popup") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[8], "-Iinclude/systems") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[9], "-Inames") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[10], "-Inames/popup") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[11], "-Inames/menu") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[12], "-Isecond_party/nstr") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[13], "-Isecond_party/noursmath") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[14], "-Isecond_party/tnecs") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[15], "-Ithird_party/physfs") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[16], "-Ithird_party/tinymt") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[17], "-Ithird_party/stb") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[18], "-Ithird_party/cJson") == 0);
    nourstest_true(strcmp(CodenameFiresaga._argv[19], "-ltnecs") == 0);
    nourstest_true(CodenameFiresaga._argv[20] == NULL);
}

int mace(int argc, char *argv[]) {
    printf("Testing mace\n");
    mace_mkdir(obj_dir);
    mace_mkdir(build_dir);

    nourstest_run("isFunc ",    test_isFunc);
    nourstest_run("globbing ",  test_globbing);
    nourstest_run("object ",    test_object);
    nourstest_run("target ",    test_target);
    nourstest_run("circular ",  test_circular);
    nourstest_run("argv ",      test_argv);
    nourstest_results();

    printf("A warning about self dependency should print now:\n \n");
    test_self_dependency();
    printf("\n");
    printf("Tests done\n");
    return (0);
}
