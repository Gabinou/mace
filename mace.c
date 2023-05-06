
#include "mace.h"

#define CC gcc

// Source file for mace convenience executable

int main(int argc, char *argv[]) {
    // Inputs only one argument: a macefile
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return EPERM;
    }

    // How to get the compiler?
    //  - Compiler is set in the macefile...
    //  - mace executable uses a different compiler variable?
    //      - YES! use tcc to compile mace
    // Compile the macefile

    // Run the resulting executable
    // mace_exec(const char *exec, char *const arguments[]);
}