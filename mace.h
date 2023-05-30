
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>

/*----------------------------------------------------------------------------*/
/*                                 PUBLIC API                                 */
/*----------------------------------------------------------------------------*/

/************************************ mace ************************************/
// User entry point.
//   1- Set compiler
//   2- Add targets
extern int mace(int argc, char *argv[]);

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
    char   *_name;
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

/******************************* MACE_ADD_TARGET *******************************/
// Adds user-defined target to internal array of targets.
// Also Saves target name hash.
#define MACE_ADD_TARGET(target) do {\
        targets[target_num] = target;\
        if (++target_num == target_len) {\
            target_len *= 2;\
            targets = realloc(targets, target_len * sizeof(*targets));\
        }\
        target._name = #target;\
    }while(0)

/******************************** Phony struct ********************************/
// Builds dependencies, then runs command.
struct PHONY {
    char *command;
    char *dependencies;      /* targets,               ' ' separated */

    /******************************** MACE_ADD_PHONY *******************************/
    // Phony targets are only built when called explicitely e.g. <./build> install
    // Default phony: 'clean' removes all targets.
#define MACE_ADD_PHONY(a)
};

/****************************** MACE_SET_COMPILER ******************************/
char *cc;
char *ar = "ar";
// 1- Save compiler name string
#define MACE_SET_COMPILER(compiler) _MACE_SET_COMPILER(compiler)
#define _MACE_SET_COMPILER(compiler) cc = #compiler

#define MACE_SET_BUILDDIR(dir)

/******************************* MACE_SET_OBJDIR *******************************/
// Sets where the object files will be placed during build.
#define MACE_SET_OBJDIR(a)

/**************************** parg ***********************************/
// Slightly pruned version of parg for arguments parsing.






/******************************* MACE_TARGET_KIND ******************************/
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
int globerr(const char *path, int eerrno) {
    fprintf(stderr, "%s: %s\n", path, strerror(eerrno));
    exit(ENOENT);
}

glob_t mace_glob_sources(const char *path) {
    /* If source is a folder, get all .c files in it */
    glob_t  globbed;
    int     flags = 0;
    int     ret = glob(path, flags, globerr, &globbed);
    if (ret != 0) {
        fprintf(stderr, "problem with %s (%s), quitting\n", path,
                (ret == GLOB_ABORTED ? "filesystem problem" :
                 ret == GLOB_NOMATCH ? "no match of pattern" :
                 ret == GLOB_NOSPACE ? "no dynamic memory" :
                 "unknown problem"));
        exit(ENOENT);
    }

    return (globbed);
}

/* Replaces spaces with -I */
void mace_include_flags(struct Target *target) {

}

void mace_parse_sources(struct Target *target) {
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
    return ((strchr(str, '*') != NULL));
}

int mace_isSource(const char *path) {
    size_t len  = strlen(path);
    int out     = path[len - 1] == 'c';      /* C source extension: .c */
    out        &= path[len - 2] == '.';      /* C source extension: .c */
    out        &= (access(path, F_OK) == 0); /* file exists */
    return (out);
}

int mace_isObject(const char *path) {
    size_t len  = strlen(path);
    int out     = path[len - 1] == 'o';      /* C object extension: .o */
    out        &= path[len - 2] == '.';      /* C object extension: .o */
    return (out);
}

int mace_isDir(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}

char *objdir        = "build/";
char *object        = NULL;
size_t object_len   = 0;

void mace_grow_obj() {
    object_len *= 2;
    object      = realloc(object, object_len * sizeof(*object));
}

void mace_object_path(char *source) {
    size_t objdir_len   = strlen(objdir);
    size_t source_len   = strlen(source);
    size_t obj_len      = objdir_len + source_len + 1;
    if (obj_len > object_len)
        mace_grow_obj();
    strncpy(object,              objdir, obj_len);
    strncpy(object + objdir_len, source, source_len);
    object[obj_len - 2] = 'o';
}

void mace_build_target(struct Target *target) {
    /* --- Parse sources, put into array --- */
    assert(target->kind != 0);
    /* --- Compile sources --- */
    /* --- Preliminaries --- */

    /* --- Split sources into tokens --- */
    char *token = strtok(target->sources, " ");
    while (token != NULL) {
        char *dest = target->_sources[target->_sources_num++];
        strncpy(dest, token, strlen(token));
        if (mace_isDir(token)) {
            // token is a directory
            // glob_t globbed = mace_glob_sources(token);

        } else if (mace_isSource(token)) {
            // token is a source file
            // mace_object_path(globbed.gl_pathv[i]);
            // mace_compile(globbed.gl_pathv[i], object, char *flags);

        } else if (mace_isWildcard(token)) {
            // token has a wildcard in it
            glob_t globbed = mace_glob_sources(token);
            for (int i = 0; i < globbed.gl_pathc; i++) {
                assert(mace_isSource(globbed.gl_pathv[i]));
                mace_object_path(globbed.gl_pathv[i]);
                mace_compile(globbed.gl_pathv[i], object, target->flags);
            }
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




/*----------------------------------------------------------------------------*/
/*                               MACE INTERNALS                               */
/*----------------------------------------------------------------------------*/

/************************************ main ************************************/
// 1- Runs mace function, get all info from user:
//   a- Get compiler
//   b- Get targets
// 2- Builds dependency graph from targets
// 3- Determine which targets need to be recompiled
// 4- Build the targetskee
// if `mace clean` is called (clean target), rm all targets

struct Target *targets = NULL;
size_t target_num = 0;
size_t target_len = 2;


void Target_Free(struct Target *target) {
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
