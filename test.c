
#include "mace.h"

#ifndef __NOURSTEST_H__
#define __NOURSTEST_H__
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* Testing library */
static int test_num = 0, fail_num = 0;

static void nourstest_results() {
    char message[4] = "FAIL";
    if (fail_num == 0) {
        strncpy(message, "PASS", 4);
    }
    printf("%s (%d/%d)\n", message, test_num - fail_num, test_num);
}
#define nourstest_true(test) do {\
    if ((test_num++, !(test))) \
        printf("\n %s:%d error #%d", __FILE__, __LINE__, ++fail_num); \
} while (0)

static void nourstest_run(char * name, void (*test)()) {
    const int ts = test_num, fs = fail_num;
    clock_t start = clock();
    printf("\t%-14s", name), test();
    printf("\t pass: %5d \t fail: %2d \t %4dms\n",
           (test_num - ts) - (fail_num - fs), fail_num - fs,
           (int)((clock() - start) * 1e3 / CLOCKS_PER_SEC));
}
#endif /*__NOURSTEST_H__ */

void test_isFunc() {
    nourstest_true(mace_isSource("test.c"));
    nourstest_true(!mace_isSource("doesnotexist.c"));
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

    globbed = mace_glob_sources("../mace/*.h");
    nourstest_true(globbed.gl_pathc == 1);
    nourstest_true(strcmp(globbed.gl_pathv[0], "../mace/mace.h") == 0);

}

int mace(int argc, char *argv[]) {
    printf("Testing mace\n");    
    nourstest_run("isFunc ",    test_isFunc);
    nourstest_run("globbing ",  test_globbing);
    nourstest_results();
    printf("Tests done\n");
}
