
#include "mace.h"


int mace(int argc, char *argv[]) {
    printf("Testing mace\n"); 
    assert(mace_isSource("test.c"));
    assert(!mace_isDir("test.c"));
    assert(mace_isDir("../mace"));
    assert(mace_isWildcard("src/*"));
    assert(mace_isWildcard("src/**"));
    assert(!mace_isWildcard("src/"));
}
