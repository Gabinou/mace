
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
    // /* Print names */
    // for (int i = 0; i < target_num; ++i) {
    //     printf("%s ", targets[i]._name);
    // }
    // printf("\n");

    // /* Print hashes */
    // for (int i = 0; i < target_num; ++i) {
    //     printf("%d ", targets[i]._hash);
    // }
    // printf("\n");
    // nourstest_true(!mace_circular_deps(targets, target_num));

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

void test_includes() {
    const char *includes = "A B C D";
    char *include_flags = mace_flags(includes, "-I");
    nourstest_true(strcmp(include_flags, "-IA -IB -IC -ID") == 0);
    free(include_flags);
}

void test_links() {
    const char *includes = "A B C D";
    char *link_flags = mace_flags(includes, "-l");
    nourstest_true(strcmp(link_flags, "-lA -lB -lC -lD") == 0);
    free(link_flags);
}

int mace(int argc, char *argv[]) {
    printf("Testing mace\n");
    nourstest_run("isFunc ",    test_isFunc);
    nourstest_run("globbing ",  test_globbing);
    nourstest_run("object ",    test_object);
    nourstest_run("target ",    test_target);
    nourstest_run("circular ",  test_circular);
    nourstest_run("includes ",  test_includes);
    nourstest_results();

    printf("A warning about self dependency should print now:\n \n");
    test_self_dependency();
    printf("\n");
    printf("Tests done\n");
}
