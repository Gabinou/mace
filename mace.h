
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

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
void Target_Free(struct Target *target);

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
        targets[target_num]       = target;\
        targets[target_num]._name = #target;\
        if (++target_num == target_len) {\
            target_len *= 2;\
            targets     = realloc(targets, target_len * sizeof(*targets));\
        }\
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
    // exit(ENOENT);
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
        // exit(ENOENT);
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


pid_t mace_exec(char *exec, char *arguments[]) {
    pid_t pid = fork();
    if (pid < 0) {
        printf("Error: forking issue. \n");
        exit(ENOENT);
    } else if (pid == 0) {
        execvp(exec, arguments);
        exit(0);
    }
    return (pid);
}

void mace_wait_pid(int pid) {
    int status;
    if (waitpid(pid, &status, 0) > 0) {
        if (WIFEXITED(status)        && !WEXITSTATUS(status)) {
        } else if (WIFEXITED(status) &&  WEXITSTATUS(status)) {
            if (WEXITSTATUS(status) == 127) {
                // execvp failed
                printf("execvp failed\n");
                exit(WEXITSTATUS(status));
            } else {
                printf("program terminated normally, but returned a non-zero status\n");
                exit(WEXITSTATUS(status));
            }
        } else {
            printf("program didn't terminate normally\n");
            exit(WEXITSTATUS(status));
        }
    }
}

void mace_link(char *objects, char *target) {
    char *arguments[] = {ar, "-rcs", target, objects, NULL};
    printf("Linking  %s \n", target);
    pid_t pid = mace_exec(ar, arguments);
    mace_wait_pid(pid);
}

/* Compile a single source file to object */
void mace_compile(char *source, char *object, char *flags, int kind) {
    char libflag[3] = "";
    if (kind == MACE_LIBRARY) {
        strncpy(libflag, "-c", 2);
    }
    char *arguments[] = {cc, source, libflag, "-o", object, flags, NULL};
    pid_t pid = mace_exec(cc, arguments);
    mace_wait_pid(pid);
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


void mace_mkdir(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        mkdir(path, 0700);
    }
}

char   *objdir      = "obj/";
char   *object      = NULL;
size_t  object_len  = 16;
char   *objects     = NULL;
size_t  objects_len = 128;
size_t  objects_num = 0;

char   *builddir    = "build/";
size_t  build_len   = 16;

void mace_grow_obj() {
    object_len *= 2;
    object      = realloc(object, object_len * sizeof(*object));
}

void mace_grow_objs() {
    objects_len *= 2;
    objects      = realloc(objects, objects_len * sizeof(*objects));
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

void mace_add_source(struct Target *target, char *token) {
    size_t i = target->_sources_num++;
    size_t srcdir_len = strlen(target->base_dir);
    size_t source_len = strlen(token);
    size_t full_len   = srcdir_len + source_len + 2;
    target->_sources[i] = malloc(full_len * sizeof(**target->_sources));

    if (target->_sources_num >= target->_sources_len) {
        target->_sources_len *= 2;
        size_t bytesize = target->_sources_len * sizeof(*target->_sources);
        target->_sources = realloc(target->_sources, bytesize);
    }

    strncpy(target->_sources[i],              target->base_dir, full_len);
    strncpy(target->_sources[i] + srcdir_len, "/",              1);
    strncpy(target->_sources[i] + srcdir_len + 1, token,        source_len);
}

char *mace_library_path(char *target_name) {
    assert(target_name != NULL);
    char *lib = calloc((strlen(target_name) + 6), sizeof(*lib));
    strncpy(lib,                            "lib",       3);
    strncpy(lib + 3,                        target_name, strlen(target_name));
    strncpy(lib + 3 + strlen(target_name),  ".a",        2);
    return (lib);
}

void mace_build_target(struct Target *target) {
    /* --- Parse sources, put into array --- */
    printf("mace_build_target\n");

    mace_mkdir(objdir);
    assert(target->kind != 0);
    /* --- Compile sources --- */
    /* --- Preliminaries --- */
    Target_Free(target);
    target->_sources_num    = 0;
    target->_sources_len    = 16;
    target->_sources        = malloc(target->_sources_len * sizeof(*target->_sources));
    memset(objects, 0, objects_len * sizeof(*objects));
    objects_num = 0;
    /* --- Split sources into tokens --- */
    char *token = strtok(target->sources, " ");
    do {
        printf("token %s\n", token);

        mace_add_source(target, token);
        size_t i = target->_sources_num - 1;
        if (mace_isDir(target->_sources[i])) {
            // token is a directory
            // Glob recursively?
            // if no / at end, add it
            char * globstr =target->_sources[i] + "/" + "**.c";
            glob_t globbed = mace_glob_sources();
            for (int i = 0; i < globbed.gl_pathc; i++) {
                assert(mace_isSource(globbed.gl_pathv[i]));
                mace_object_path(globbed.gl_pathv[i]);
                mace_compile(globbed.gl_pathv[i], object, target->flags, target->kind);
            }

        } else if (mace_isSource(target->_sources[i])) {
            // token is a source file
            mace_object_path(token);
            mace_compile(target->_sources[i], object, target->flags, target->kind);

        } else if (mace_isWildcard(target->_sources[i])) {
            // token has a wildcard in it
            glob_t globbed = mace_glob_sources(target->_sources[i]);
            for (int i = 0; i < globbed.gl_pathc; i++) {
                assert(mace_isSource(globbed.gl_pathv[i]));
                mace_object_path(globbed.gl_pathv[i]);
                mace_compile(globbed.gl_pathv[i], object, target->flags, target->kind);
            }
        } else {
            printf("Error: source is neither a .c file, a folder nor has a wildcard in it\n");
            exit(ENOENT);
        }
        if ((objects_num + strlen(object) + 2) >= objects_len) {
            mace_grow_objs();
        }
        if (objects_num > 0) {
            strncpy(objects + objects_num,     " ",    1);
            strncpy(objects + objects_num + 1, object, strlen(object));
        } else {
            strncpy(objects + objects_num,     object, strlen(object));
        }

        objects_num += strlen(object) + 2;
        token = strtok(NULL, " ");
    } while (token != NULL);


    /* --- Linking --- */
    if (target->kind == MACE_LIBRARY) {
        char *lib = mace_library_path(target->_name);
        mace_link(objects, lib);
        free(lib);
    } else if (target->kind == MACE_EXECUTABLE) {
        // mace_compile(source, object, flags);
    }

}

void mace_build_targets(struct Target *targets, size_t len) {
    assert(targets != NULL);
    for (int i = 0; i < len; i++) {
        printf("target %i\n", i);
        mace_build_target(&targets[i]);
        printf("target %i DONE\n", i);
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
size_t target_num      = 0;
size_t target_len      = 2;

void Target_Free(struct Target *target) {
    if (target->_sources != NULL) {
        for (int i = 0; i < target->_sources_num; ++i) {
            if (target->_sources[i] == NULL)
                continue;

            free(target->_sources[i]);
        }

        free(target->_sources);
        target->_sources = NULL;
    }
}

void mace_free() {
    for (int i = 0; i < target_num; i++) {
        Target_Free(&targets[i]);
    }
    if (targets != NULL) {
        free(targets);
    }
    if (object != NULL) {
        free(object);
    }
    if (objects != NULL) {
        free(objects);
    }
}

int main(int argc, char *argv[]) {
    /* --- Preliminaries --- */
    targets = malloc(target_len  * sizeof(*targets));
    object  = malloc(object_len  * sizeof(*object));
    objects = malloc(objects_len * sizeof(*objects));

    mace(argc, argv);
    size_t len = 0;
    // mace_target_dependency(targets, len);
    // mace_compile("   mace.c", "baka.out", NULL);
    printf("target_num %d\n", target_num);
    mace_build_targets(targets, target_num);
    printf("mace_free \n");
    mace_free(targets);
    printf("FINISH\n");
    return (0);
}
