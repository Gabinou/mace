
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>

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
    size_t  _sources_len;
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


/**************************** mace_target_dependency ***************************/
// Build target dependency graph from target links
void mace_target_dependency(struct Target *targets, size_t len) {

}

/******************************* mace_find_sources *****************************/
// 1- if glob pattern, find all matches, add to list
// 2- if file add to list
void mace_glob_sources(struct Target * target) {
    /* If source is a folder, get all .c files in it */

    /* If source has a * in it, expland it */
}

/* Replaces spaces with -I */
void mace_include_flags(struct Target * target) {

}

void mace_parse_sources(struct Target * target) {
    target->_sources = malloc(sizeof(*target->_sources));
}


/********************************* mace_build **********************************/
/* Build all sources from target to object */
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

int mace_isWildcard(const char *str) {
    int out = strchr(str, '*') != NULL;
    return(out);
}

int mace_isSource(const char *path) {
    size_t len  = strlen(path);
    int out     = path[len - 1] == 'c';
    out        &= path[len - 2] == '.';
    struct stat path_stat;
    stat(path, &path_stat);
    out        &= S_ISREG(path_stat.st_mode);
    return(out);
}

int mace_isDir(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

void mace_build_target(struct Target *target) {
    /* --- Parse sources, put into array --- */
    assert(target->kind != 0);
    /* --- Compile sources --- */
    /* --- Preliminaries --- */

    /* --- Split sources into tokens --- */
    char *token = strtok(target->sources, " ");
    while( token != NULL ) {
        char * dest = target->_sources[target->_sources_num++]; 
        strncpy(dest, token, strlen(token));
        if (mace_isDir(token)) {
            // token is a directory

        } else if (mace_isSource(token)) {
            // token is a source file

        } else if (mace_isWildcard(token)) {
            // token has a wildcard in it
        
        } else {
            printf("Error: source is neither a .c file, a folder nor has a wildcard in it");
            exit(ENOENT);
        }
        token = strtok(NULL, " ");
   }

    /* --- Linking --- */
    if (target->kind == MACE_LIBRARY) {
        // mace_link(objects, target);
    } else if (target->kind == MACE_EXECUTABLE) {
        // mace_compile(source, object, flags);
    }

}

void mace_build_targets(struct Target *targets, size_t len) {
    for (int i = 0; i < len; i++) {
        mace_build_target(&targets[i]);
    }
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


void Target_Free(struct Target * target) {
    if (target->_sources != NULL) {
        free(target->_sources);
        target->_sources = NULL;
    }
}

void mace_free() {
    for (int i = 0; i < target_num; i++) {
        Target_Free(&targets[i]);
    }
    free(targets);
}

int main(int argc, char *argv[]) {
    /* --- Preliminaries --- */
    targets = malloc(target_len * sizeof(*targets));

    mace(argc, argv);
    size_t len = 0;
    // mace_target_dependency(targets, len);
    // mace_compile("mace.c", "baka.out", NULL);
    mace_build_targets(targets, target_num);
    mace_free();
    return (0);
}
