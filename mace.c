/* Source file for mace convenience executable */

#include "mace.h"

// Compiler used by mace executable to compile macefiles
#define CC "gcc"
#define MACE_BUILDER "build"

int main(int argc, char *argv[]) {
    /* Inputs only one argument: a macefile */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return EPERM;
    }

    pid_t pid;
    /* Compile the macefile */
    char *argv_compile[] = {CC, argv[1], "-o", MACE_BUILDER};
    pid = mace_exec(CC, argv_compile);
    mace_wait_pid(pid);

    /* Run the resulting executable */
    char *argv_run[] = {MACE_BUILDER};
    pid = mace_exec(MACE_BUILDER, argv_run);
    mace_wait_pid(pid);
}