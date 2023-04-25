
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**************************** parg ***********************************/
// Slightly pruned version of parg for arguments parsing.

/**************************** Target struct ***********************************/
// Contains all information necessary to compile target
struct Target {
    /* User-set public members */
    char *includes;          /* directories,           ' ' separated */
    char *sources;           /* files, glob patterns,  ' ' separated */
    char *sources_exclude;   /* files, glob patterns,  ' ' separated */
    char *base_dir;          /* directory,                           */
    char *dependencies;      /* targets,               ' ' separated */
    char *links;             /* libraries,             ' ' separated */
    char *flags;             /* passed as is to compiler             */
    char *message_pre_build;
    char *command_pre_build;
    char *message_post_build;
    char *command_post_build;
    int   kind;

    /* Private members */
    char  **_sources;         /* filenames */
    size_t  _sources_num;
    char  *_name;
};

/* --- EXAMPLE TARGET --- */
// Use struct Designated Initializer, guaranteeing unitialized values to 0/NULL.
/*
* struct Target tnecs = {
*     .includes           = "",
*     .sources            = "",
*     .sources_exclude    = "",
*     .base_dir           = "",
*     .dependencies       = "",
*     .kind               = MACE_LIBRARY,
* };
*/

/******************************** Phony struct ********************************/
// Builds dependencies, then runs command.
struct PHONY {
    char *command;
    char *dependencies;
};

/******************************* mace_add_target *******************************/
// 1- Saves target name hash
// 2- Builds list of sources
// All added targets are built
#define MACE_ADD_TARGET(target) do {\
targets[target_num] = target;\
if (++target_num == target_len) {\
    target_len *= 2;\
    targets = realloc(targets, target_len * sizeof(*targets));\
}\
target._name = #target;\
}while(0)

/******************************** MACE_ADD_PHONY *******************************/
// Phony targets are only built when called explicitely e.g. <./build> install
// Default phony: 'clean' removes all targets.
#define MACE_ADD_PHONY(a)

/****************************** MACE_SET_COMPILER ******************************/
char *cc;
char *ar = "ar";
// 1- Save compiler name string
#define MACE_SET_COMPILER(compiler) _MACE_SET_COMPILER(compiler)
#define _MACE_SET_COMPILER(compiler) cc = #compiler

/******************************* MACE_SET_OBJDIR *******************************/
// Sets where the object files will be placed during build.
#define MACE_SET_OBJDIR(a)

enum MACE_TARGET_KIND {
    MACE_EXECUTABLE      = 1,
    MACE_LIBRARY         = 2,
};

// build include flags from target.include
void mace_flags_include(struct Target targets) {

}

// build linker flags from target.links
void mace_flags_link(struct Target targets) {

}

/******************************* mace_find_sources *****************************/
// 1- if glob pattern, find all matches, add to list
// 2- if file add to list
void mace_find_sources(struct Target *targets, size_t len) {

}

/**************************** mace_target_dependency ***************************/
// Build target dependency graph from target links
void mace_target_dependency(struct Target *targets, size_t len) {

}

/********************************* mace_build **********************************/
/* Cuild all sources from target to object */
void mace_build_targets(struct Target *targets, size_t len) {

}

void mace_build_target(struct Target *target) {


    /* --- Linking --- */
    printf("Linking %s", target->_name);
    if (target->kind == MACE_LIBRARY) {
        // mace_link(objects, target);
    } else if (target->kind == MACE_EXECUTABLE) {
        // mace_compile(char *source, char *object, char *flags);
    }

}

void mace_link(char *objects, char *target) {
    char *arguments[] = {ar, "-rcs", target, objects};
    printf("Linking  %d\n", target);
    execvp(ar, arguments);
}


/* Compile a single source file to object */
void mace_compile(char *source, char *object, char *flags) {
    char *arguments[] = {cc, source, "-o", object, flags};
    printf("%s\n", source);
    execvp(cc, arguments);
}

/************************************ mace *************************************/
// User-implemented function.
// SHOULD:
// 1- Set compiler
// 2- Add targets
extern int mace(int argc, char *argv[]);

/************************************ main ************************************/
// 1- Run mace, get all info from user:
//   a- Get compiler
//   a- Get targets
// 2- Build dependency graph from targets
// 3- Determine which targets need to be recompiled
// 4- Build the targets

// if `mace clean` is called (clean target), rm all targets

struct Target *targets = NULL;
size_t target_num = 0;
size_t target_len = 2;

int main(int argc, char *argv[]) {
    /* --- Preliminaries --- */
    targets = malloc(target_len * sizeof(*targets));

    mace(argc, argv);
    size_t len = 0;
    mace_target_dependency(targets, len);
    // mace_compile(arguments);
    mace_compile("mace.c", "baka.out", NULL);
    mace_build_targets(targets, target_num);
    return (0);
}
