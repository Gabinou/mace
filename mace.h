
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

/*----------------------------------------------------------------------------*/
/*                                 PUBLIC API                                 */
/*----------------------------------------------------------------------------*/

/************************************ mace ************************************/
// User entry point. User responsibilities:
//   1- Set compiler
//   2- Add targets
extern int mace(int argc, char *argv[]);

/******************************* MACE_TARGET_KIND ******************************/
enum MACE_TARGET_KIND { // for target.kind
    MACE_EXECUTABLE      = 1,
    MACE_LIBRARY         = 2,
};

/**************************** Target struct ***********************************/
// Contains all information necessary to compile target
struct Target {
    /* User-set public members */
    const char *includes;          /* directories,           ' ' separated    */
    const char *sources;           /* files, glob patterns,  ' ' separated    */
    const char *sources_exclude;   /* files, glob patterns,  ' ' separated    */
    const char *base_dir;          /* directory,                              */
    const char *links;             /* libraries or targets   ' ' separated    */
    const char *flags;             /* passed as is to compiler                */
    const char *message_pre_build;
    const char *command_pre_build;
    const char *message_post_build;
    const char *command_post_build;
    int         kind;              /* MACE_TARGET_KIND                        */

    /* Private members, set automatically by mace                             */
    char      *_name;              /* target name set by user                 */
    uint64_t   _hash;              /* target name hash,                       */
    int        _order;             /* target order added by user              */
   
    // Note: argv should have same length allocated
    //       Use temporary variable to store length,m then realloc to argc
    char      **_argv;             /* buffer for argv to exec build commands  */
    int         _argc;             /* number of arguments in argv             */
    char      **_argv_includes;    /* includes, in argv form                  */
    int         _argc_includes;    /* number of arguments in argv_includes    */
    char      **_argv_sources;     /* sources, in argv form                   */
    int         _argc_sources;     /* number of arguments in argv_sources     */
    char      **_argv_links;       /* linked libraries, in argv form          */
    int         _argc_links;       /* number of arguments in argv_links       */
    char      **_argv_flags;       /* user flags, in argv form                */
    int         _argc_flags;       /* number of arguments in argv_flags       */

    uint64_t  *_deps_links;        /* target or libs hashes                   */
    size_t     _deps_links_num;    /* target or libs hashes                   */
    size_t     _deps_links_len;    /* target or libs hashes                   */
    size_t     _d_cnt;             /* dependency count, for build order       */
};

/* --- EXAMPLE TARGET --- */
// Use struct Designated Initializer, guaranteeing unitialized values to 0/NULL.
/*
* struct Target mytarget = {
*     .includes           = "include include/foo",
*     .sources            = "src src/bar",
*     .sources_exclude    = "src/main.c",
*     .links              = "lib1 lib2 mytarget2 ",
*     .kind               = MACE_LIBRARY,
* };
*/


/******************************** DECLARATIONS ********************************/

/* --- mace --- */
void  mace_init();
void  mace_free();
uint64_t mace_hash(char *str);

char *mace_str_buffer(const char *const strlit);
char *mace_copy_str(char *restrict buffer, const char *str);
char **mace_argv_flags(int *len, int *argc, char **argv, const char *includes, const char *flag);
void mace_add_target(struct Target *target, char *name);

/* --- mace_setters --- */
char *mace_set_obj_dir(char    *obj);
char *mace_set_build_dir(char  *build);

/* --- mace_Target --- */
void Target_Free(struct Target        *target);
void Target_Free_argv(struct Target   *target);
bool Target_hasDep(struct Target      *target, uint64_t hash);
void Target_Deps_Hash(struct Target   *target);
void Target_Source_Add(struct Target  *target, char    *token);
void Target_argv_user(struct Target   *target);
int Target_Order();


int globerr(const char *path, int eerrno);
glob_t mace_glob_sources(const char *path);

/* --- mace_exec --- */
pid_t mace_exec(const char *exec, char *const arguments[]);
void  mace_wait_pid(int pid);

/* --- mace_build --- */
void mace_link(char *objects, char *target);
void mace_compile(const char *restrict source, char *restrict object, int kind);
void mace_compile_glob(struct Target *target, char *globsrc, const char *restrict flags, int kind);

/* --- mace_is --- */
int mace_isWildcard(const   char *str);
int mace_isSource(const     char *path);
int mace_isObject(const     char *path);
int mace_isDir(const        char *path);

/* --- mace_filesystem --- */
void  mace_mkdir(const char    *path);
void  mace_object_path(char    *source);
char *mace_library_path(char   *target_name);

/* --- mace_globals --- */
struct Target  *targets;        /* [order] is as added by user */
size_t          target_num;
size_t          target_len;
char           *obj_dir;
char           *object;
size_t          object_len;
char           *objects;
size_t          objects_len;
size_t          objects_num;
char           *build_dir;

void mace_grow_obj();
void mace_grow_objs();

/******************************* MACE_ADD_TARGET ******************************/
// Adds user-defined target to internal array of targets.

// Convenience macro
#define MACE_ADD_TARGET(target) mace_add_target(&target, #target)

void mace_add_target(struct Target *target, char *name) {
    targets[target_num]        = *target;
    targets[target_num]._name  = name;
    targets[target_num]._hash  = mace_hash(name);
    targets[target_num]._order = target_num;
    Target_Deps_Hash(&targets[target_num]);
    if (++target_num == target_len) {
        target_len *= 2;
        targets     = realloc(targets, target_len * sizeof(*targets));
    }

}

/******************************** Phony struct ********************************/
// Builds dependencies, then runs command.
struct Command {
    char *command;
    char *deps;      /* targets,               ' ' separated */
};

/****************************** MACE_ADD_COMMAND ******************************/
// Command targets are only built when called explicitely e.g. <./build> install
// Default phony: 'clean' removes all targets.
#define MACE_ADD_COMMAND(a)
// How to dermine command order?
//  - If command has dependencies, order computed from dependency graph
//  - If command has NO dependencies, user can set order

/********************************* mace_hash **********************************/
uint64_t mace_hash(char *str) {
    /* djb2 hashing algorithm by Dan Bernstein.
    * Description: This algorithm (k=33) was first reported by dan bernstein many
    * years ago in comp.lang.c. Another version of this algorithm (now favored by bernstein)
    * uses xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic of number 33
    * (why it works better than many other constants, prime or not) has never been adequately explained.
    * [1] https://stackoverflow.com/questions/7666509/hash-function-for-string
    * [2] http://www.cse.yorku.ca/~oz/hash.html */
    uint64_t hash = 5381;
    int32_t str_char;
    while ((str_char = *str++))
        hash = ((hash << 5) + hash) + str_char; /* hash * 33 + c */
    return (hash);
}

/******************************* MACE_CHECKSUM ********************************/
// char *checksum = "sha256sum";
char *checksum = "sha1DC";
// -> git uses sha1DC and SO WILL I

/******************************* MACE_COMPILER ********************************/
char *cc;
char *ar = "ar";
// 1- Save compiler name string
#define MACE_SET_COMPILER(compiler) _MACE_SET_COMPILER(compiler)
#define _MACE_SET_COMPILER(compiler) cc = #compiler

/****************************** MACE_SET_obj_dir ******************************/
// Sets where the object files will be placed during build.
char *mace_set_obj_dir(char *obj) {
    return (obj_dir = mace_copy_str(obj_dir, obj));
}

char *mace_set_build_dir(char *build) {
    return (build_dir = mace_copy_str(build_dir, build));
}

char *mace_copy_str(char *restrict buffer, const char *str) {
    if (buffer != NULL) {
        free(buffer);
    }
    size_t len = strlen(str);
    buffer = calloc(len + 1, sizeof(*buffer));
    strncpy(buffer, str, len);
    return (buffer);
}

/**************************** parg ***********************************/
// Slightly pruned version of parg for arguments parsing.

/************************************ argv ************************************/
//

void argv_free(int argc, char **argv) {
    if (argv == NULL)
        return;

    for (int i = 0; i < argc; i++) {
        free(argv[i]);
        argv[i] = NULL;
    }
    argv = NULL;
}

char **argv_grows(int *len, int *argc, char **argv) {
    if ((*argc) >= (*len)) {
        (*len) *= 2;
        argv = realloc(argv, (*len) * sizeof(*argv));
    }
    return (argv);
}

char **mace_argv_flags(int *len, int *argc, char **argv, const char *user_str, const char *flag) {
    assert(argc != NULL);
    assert(len != NULL);
    assert((*len) > 0);
    size_t flag_len = (flag == NULL) ? 0 : strlen(flag);

    /* -- Copy user_str into modifiable buffer -- */
    char *buffer = mace_str_buffer(user_str);

    char *token = strtok(buffer, " ");
    do {
        argv = argv_grows(len, argc, argv);

        size_t token_len = strlen(token);
        char *arg = calloc(token_len + 3, sizeof(*arg));

        /* - Copy token into arg - */
        if (flag_len > 0) {
            strncpy(arg, flag, flag_len);
        }
        strncpy(arg + flag_len, token, token_len);
        argv[(*argc)++] = arg;

        token = strtok(NULL, " ");
    } while (token != NULL);

    free(buffer);
    return (argv);
}

void Target_argv_user(struct Target *target) {
    // Makes flags for target includes, links libraries, and flags
    //  NOT sources: they can be folders, so need to be globbed
    Target_Free_argv(target);
    int len;

    /* -- Make _argv_includes to argv -- */
    if(target->includes != NULL) {
        len = 8;
        target->_argc_includes = 0;
        target->_argv_includes = malloc(len * sizeof(*target->_argv_includes));
        target->_argv_includes = mace_argv_flags(&len, &target->_argc_includes, target->_argv_includes, target->includes, "-I");
        target->_argv_includes = realloc(target->_argv_includes, target->_argc_includes * sizeof(*target->_argv_includes));
    }

    /* -- Make _argv_links to argv -- */
    if(target->links != NULL) {
        len = 8;
        target->_argc_links = 0;
        target->_argv_links = malloc(len * sizeof(*target->_argv_links));
        target->_argv_links = mace_argv_flags(&len, &target->_argc_links, target->_argv_links, target->links, "-l");
        target->_argv_links = realloc(target->_argv_links, target->_argc_links * sizeof(*target->_argv_links));
    }

    /* -- Make _argv_flags to argv -- */
    if(target->flags != NULL) {
        len = 8;
        target->_argc_flags = 0;
        target->_argv_flags = malloc(len * sizeof(*target->_argv_flags));
        target->_argv_flags = mace_argv_flags(&len, &target->_argc_flags, target->_argv_flags, target->flags, NULL);
        target->_argv_flags = realloc(target->_argv_flags, target->_argc_flags * sizeof(*target->_argv_flags));
    }
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

/********************************* mace_exec **********************************/
// Execute command in forked process.
void mace_exec_print(char *const arguments[], size_t argnum) {
    for (int i = 0; i < argnum; i++) {
        printf("%s ", arguments[i]);
    }
    printf("\n");
}
pid_t mace_exec(const char *exec, char *const arguments[]) {
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

// Wait on process with pid to finish
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
            printf("program didn't terminate- normally\n");
            exit(WEXITSTATUS(status));
        }
    }
}

/********************************* mace_build **********************************/
/* Build all sources from target to object */
void mace_link(char *objects, char *target) {
    char *arguments[] = {ar, "-rcs", target, objects, NULL};
    printf("Linking \t%s \n", target);
    // TODO: split objects into individual arguments

    // mace_exec_print(arguments, sizeof(arguments)/sizeof(*arguments));
    pid_t pid = mace_exec(ar, arguments);
    mace_wait_pid(pid);
}

/* Compile a single source file to object */
// void mace_compile(const char *restrict source, char *restrict object, const char *restrict user_flags, char *restrict include_flags, char *restrict link_flags, int kind) {
//     char libflag[3] = "";
//     if (kind == MACE_LIBRARY)
//         strncpy(libflag, "-c", 2);

//     char *absrc = realpath(source, NULL);
//     char *pos = strrchr(absrc, '/');
//     char *source_file = (pos == NULL) ? absrc : pos + 1;
//     printf("Compiling   \t%s \n", source_file);
//     char *aflags = NULL;
//     if (user_flags != NULL) {
//         size_t flags_len = strlen(user_flags);
//         aflags = calloc(flags_len + 1, sizeof(*aflags));
//         strncpy(aflags, user_flags, flags_len);
//     }
//     // TODO: split user_flags, include_flags, link_flags into individual arguments
//     char *const arguments[] = {cc, absrc, include_flags, link_flags, libflag,"-v", "-o", object, aflags, NULL};
//     printf("aflags %s\n", aflags);
//     printf("object %sAAAA\n", object);
//     // mace_exec_print(arguments, sizeof(arguments)/sizeof(*arguments));
//     pid_t pid = mace_exec(cc, arguments);
//     mace_wait_pid(pid);
//     printf("Compiled   \t%s \n", source_file);
//     free(absrc);
//     free(aflags);
// }

void mace_compile2(char *source, char *object, char **argv, int *_argc, int *argl,
                   char **argv_includes, char **argv_sources, char **argv_links, char **argv_flags, int kind) {

}


void mace_compile3(char *sources, char *objects, struct Target * target) {
    // Sources might be ONE file, might be ALL files, depends.
    /* - Reset argv - */

    /* -- Parsing input strings to argv -- */
    /* - Parse sources into _argv_sources - */
    /* - Parse object into _argv_object - */

    /* -- Building argv -- */
    /* - Add Compiler to argv - */
    /* - Add sources to argv - */
    /* - Add objects to argv - */
    /* - Add _argv_includes to argv - */
    /* - Add _argv_sources to argv - */
    /* - Add _argv_links to argv - */
    /* - Add _argv_flags to argv - */

    /* -- Actual compilation -- */
    // pid_t pid = mace_exec(cc, argv);
    // mace_wait_pid(pid);
}



/* Compile globbed files to objects */
void mace_compile_glob(struct Target *target, char *globsrc, const char *restrict flags, int kind) {
    glob_t globbed = mace_glob_sources(globsrc);
    for (int i = 0; i < globbed.gl_pathc; i++) {
        assert(mace_isSource(globbed.gl_pathv[i]));
        char *pos = strrchr(globbed.gl_pathv[i], '/');
        char *source_file = (pos == NULL) ? globbed.gl_pathv[i] : pos;
        // Target_Source_Add(target, source_file);
        mace_object_path(source_file);
        // mace_compile(globbed.gl_pathv[i], object, flags, target->_include_flags, target->_link_flags, kind);
    }
    globfree(&globbed);
}

/********************************** mace_is ***********************************/
int mace_isWildcard(const char *str) {
    return ((strchr(str, '*') != NULL));
}

int mace_isSource(const char *path) {
    size_t len  = strlen(path);
    int out     = path[len - 1]       == 'c'; /* C source extension: .c */
    out        &= path[len - 2]       == '.'; /* C source extension: .c */
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

/****************************** mace_filesystem ********************************/
void mace_mkdir(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        mkdir(path, 0700);
    }
}

char *mace_library_path(char *target_name) {
    assert(target_name != NULL);
    size_t bld_len = strlen(build_dir);
    size_t tar_len = strlen(target_name);

    char *lib = calloc((bld_len + tar_len + 6), sizeof(*lib));
    size_t full_len = 0;
    strncpy(lib,                build_dir,   bld_len);
    full_len += bld_len;
    if (build_dir[0] != '/') {
        strncpy(lib + full_len, "/",         1);
        full_len++;
    }
    strncpy(lib + full_len,     "lib",       3);
    full_len += 3;
    strncpy(lib + full_len,     target_name, tar_len);
    full_len += tar_len;
    strncpy(lib + full_len,     ".a",        2);
    return (lib);
}

/******************************* mace_globals *********************************/
char   *object      = NULL;
size_t  object_len  = 16;
char   *objects     = NULL;
size_t  objects_len = 128;
size_t  objects_num = 0;

char   *obj_dir     = NULL;
char   *build_dir   = NULL;

void mace_grow_obj() {
    object_len *= 2;
    object      = realloc(object, object_len * sizeof(*object));
}

void mace_grow_objs() {
    objects_len *= 2;
    objects      = realloc(objects, objects_len * sizeof(*objects));
}

void mace_object_path(char *source) {
    /* --- Expanding path --- */
    char *path = realpath(obj_dir, NULL);
    if (path == NULL) {
        printf("Object directory '%s' does not exist.\n", obj_dir);
        exit(ENOENT);
    }

    /* --- Grow object --- */
    size_t source_len = strlen(source);
    size_t path_len;
    while (((path_len = strlen(path)) + source_len + 2) >= object_len)
        mace_grow_obj();
    memset(object, 0, object_len * sizeof(*object));

    /* --- Writing path to object --- */
    strncpy(object, path, path_len);
    if (source[0] != '/')
        object[path_len++] = '/';
    strncpy(object + path_len, source, source_len);
    object[strlen(object) - 1] = 'o';

    free(path);
}

char *mace_str_buffer(const char *strlit) {
    size_t  litlen  = strlen(strlit);
    char   *buffer  = calloc(litlen + 1, sizeof(*buffer));
    strncpy(buffer, strlit, litlen);
    return (buffer);
}

/******************************** mace_build **********************************/
void mace_build_target(struct Target *target) {
    /* --- Parse sources, put into array --- */

    assert(target->kind != 0);
    /* --- Compile sources --- */
    /* --- Preliminaries --- */
    Target_Free(target);

    memset(objects, 0, objects_len * sizeof(*objects));
    objects_num = 0;
    /* -- Copy sources into modifiable buffer -- */
    char *buffer = mace_str_buffer(target->sources);

    /* --- Split sources into tokens --- */
    char *token = strtok(buffer, " ");
    do {
        // printf("token %s\n", token);

        if (mace_isDir(token)) {
            /* Glob all sources recursively */
            // printf("isDir %s\n", token);

            size_t srclen = strlen(token);
            char *globstr = calloc(srclen + 6, sizeof(*globstr));
            strncpy(globstr,              token,  strlen(token));
            strncpy(globstr + srclen,     "/",    1);
            strncpy(globstr + srclen + 1, "**.c", 4);

            // mace_compile_glob(target, globstr, target->flags, MACE_LIBRARY);
            free(globstr);

        } else if (mace_isWildcard(token)) {
            /* token has a wildcard in it */
            // printf("isWildcard %s\n", token);
            // mace_compile_glob(target, token, target->flags, MACE_LIBRARY);

        } else if (mace_isSource(token)) {
            /* token is a source file */
            // printf("isSource %s\n", token));

            mace_object_path(token);
            // mace_compile(target->sources, object, target->flags, target->_include_flags, target->_link_flags, MACE_LIBRARY);

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
    free(buffer);
}

size_t *build_order = NULL;
size_t  build_order_num = 0;

bool mace_isTargetinBuildOrder(size_t order) {
    bool out = false;
    for (int i = 0; i < build_order_num; i++) {
        if (build_order[i] == order) {
            out = true;
            break;
        }
    }
    return (out);
}

int mace_hash_order(uint64_t hash) {
    int order = -1;
    for (int i = 0; i < target_num; i++) {
        if (hash == targets[i]._hash) {
            order = i;
            break;
        }
    }
    return (order);
}

int mace_target_order(struct Target target) {
    return (mace_hash_order(target._hash));
}

void mace_build_order_add(size_t order) {
    if (build_order == NULL) {
        build_order = malloc(target_num * sizeof(*build_order));
        build_order_num = 0;
    }
    assert(build_order_num < target_num);
    build_order[build_order_num++] = order;
}

/* - Depth first search through depencies - */
// Builds all target dependencies before building target
void mace_deps_build_order(struct Target target, size_t *o_cnt) {
    /* o_cnt should never be geq to target_num */
    if ((*o_cnt) >= target_num)
        return;

    size_t order = mace_target_order(target); // target order
    /* Target already in build order, skip */
    if (mace_isTargetinBuildOrder(order))
        return;

    /* Target has no dependencies, add target to build order */
    if (target._deps_links == NULL) {
        mace_build_order_add(order);
        return;
    }

    /* Visit all target dependencies */
    target._d_cnt = 0;
    while (target._d_cnt < target._deps_links_num) {
        uint64_t next_dep = target._d_cnt++;
        size_t next_target_order = mace_hash_order(target._deps_links[next_dep]);
        /* Recursively search target's next dependency -> depth first search */
        mace_deps_build_order(targets[next_target_order], o_cnt);
    }

    /* Target already in build order, skip */
    if (mace_isTargetinBuildOrder(order))
        return;

    /* All dependencies of target were built, add it to build order */
    if (target._d_cnt == target._deps_links_num) {
        mace_build_order_add(order);
        return;
    }
}

bool Target_hasDep(struct Target *target, uint64_t hash) {
    for (int i = 0; i < target->_deps_links_num; i++) {
        if (target->_deps_links[i] == hash)
            return (true);
    }
    return (false);
}

bool mace_circular_deps(struct Target *targs, size_t len) {
    // Circular dependency conditions
    //   1- Target i has j dependency
    //   2- Target j has i dependency
    for (int i = 0; i < target_num; i++) {
        uint64_t hash_i = targs[i]._hash;
        // 1- going through target i's dependencies
        for (int z = 0; z < targs[i]._deps_links_num; z++) {
            int j = mace_hash_order(targs[i]._deps_links[z]);
            if (i == j) {
                printf("Warning: Target depends on itself. Remove '%s' from its 'links'.\n",
                       targs[i]._name);
                continue;
            }
            if (Target_hasDep(&targs[j], hash_i))
                // 2- target i's dependency j has i as dependency as well
                return (true);
        }
    }
    return (false);
}

void mace_target_build_order(struct Target *targs, size_t len) {

    /* Skip if no targets */
    if ((targs == NULL) || (len == 0)) {
        printf("No targets. Skipping build order computation.\n");
        return;
    }

    size_t o_cnt = 0; /* order count */

    /* Check for circular dependency */
    if (mace_circular_deps(targs, len)) {
        printf("Circular dependency in linked library detected. Exiting\n");
        exit(EDOM);
    }

    /* If only 1 include, build order is trivial */
    if (len == 1) {
        mace_build_order_add(0);
        return;
    }

    /* Visit all targs */
    while (o_cnt < target_num) {
        mace_deps_build_order(targs[o_cnt], &o_cnt);
        o_cnt++;
    }
}

void mace_build_targets(struct Target *targs, size_t len) {
    if ((targs == NULL) || (len == 0) || (build_order == NULL) || (build_order_num == 0)) {
        printf("No targets to compile. Exiting.\n");
        return;
    }
    for (int z = 0; z < build_order_num; z++) {
        int i = build_order[z];
        mace_build_target(&targs[i]);
    }
}

/*----------------------------------------------------------------------------*/
/*                               MACE INTERNALS                               */
/*----------------------------------------------------------------------------*/
struct Target *targets = NULL;
size_t target_num      = 0;
size_t target_len      = 0;

enum MACE {
    MACE_DEFAULT_TARGET_LEN     =   8,
    MACE_DEFAULT_OBJECT_LEN     =  16,
    MACE_DEFAULT_OBJECTS_LEN    = 128,
};

void Target_Free(struct Target *target) {
    if (target->_deps_links != NULL) {
        free(target->_deps_links);
        target->_deps_links = NULL;
    }
    Target_Free_argv(target);
}

void Target_Free_argv(struct Target *target) { 
    if (target->_argv_includes != NULL) {
        for (int i = 0; i < target->_argc_includes; i++) {
            free(target->_argv_includes[i]);
        }
        free(target->_argv_includes);
        target->_argv_includes = NULL;
        target->_argc_includes = 0;
    }
    if (target->_argv_sources != NULL) {
        for (int i = 0; i < target->_argc_sources; i++) {
            free(target->_argv_sources[i]);
        }
        free(target->_argv_sources);
        target->_argv_sources = NULL;
        target->_argc_sources = 0;
    }
    if (target->_argv_links != NULL) {
        for (int i = 0; i < target->_argc_links; i++) {
            free(target->_argv_links[i]);
        }
        free(target->_argv_links);
        target->_argv_links = NULL;
        target->_argc_links = 0;
    }
    if (target->_argv_flags != NULL) {
        for (int i = 0; i < target->_argc_flags; i++) {
            free(target->_argv_flags[i]);
        }
        free(target->_argv_flags);
        target->_argv_flags = NULL;
        target->_argc_flags = 0;
    }
}

void mace_init() {
    mace_free();

    /* --- Memory allocation --- */
    target_len  = MACE_DEFAULT_TARGET_LEN;
    object_len  = MACE_DEFAULT_OBJECT_LEN;
    objects_len = MACE_DEFAULT_OBJECTS_LEN;

    targets = malloc(target_len  * sizeof(*targets));
    object  = malloc(object_len  * sizeof(*object));
    objects = malloc(objects_len * sizeof(*objects));

    /* --- Default output folders --- */
    mace_set_build_dir("build/");
    mace_set_obj_dir("obj/");
}

void mace_free() {
    for (int i = 0; i < target_num; i++) {
        Target_Free(&targets[i]);
    }
    if (targets != NULL) {
        free(targets);
        targets = NULL;
    }
    if (object != NULL) {
        free(object);
        object = NULL;
    }
    if (objects != NULL) {
        free(objects);
        objects = NULL;
    }
    if (obj_dir != NULL) {
        free(obj_dir);
        obj_dir = NULL;
    }
    if (build_dir != NULL) {
        free(build_dir);
        build_dir = NULL;
    }
    if (build_order != NULL) {
        free(build_order);
        build_order = NULL;
    }
    build_order_num = 0;
    target_num = 0;
    object_len = 0;
    objects_num = 0;
}

void Target_Deps_Hash(struct Target *target) {
    /* --- Preliminaries --- */
    if (target->links == NULL)
        return;

    /* --- Alloc space for deps --- */
    target->_deps_links_num = 0;
    target->_deps_links_len = 16;
    if (target->_deps_links != NULL) {
        free(target->_deps_links);
    }
    target->_deps_links = malloc(target->_deps_links_len * sizeof(*target->_deps_links));
    /* --- Copy links into modifiable buffer --- */
    char *buffer = mace_str_buffer(target->links);
    /* --- Split links into tokens, --- */
    char *token = strtok(buffer, " ");

    /* --- Hash tokens into _deps_links --- */
    do {
        target->_deps_links[target->_deps_links_num++] = mace_hash(token);
        token = strtok(NULL, " ");
    } while (token != NULL);
    free(buffer);
}


/************************************ main ************************************/
// 1- Runs mace function, get all info from user:
//   a- Get compiler
//   b- Get targets
// 2- Builds dependency graph from targets
// 3- Determine which targets need to be recompiled
// 4- Build the targets
// if `mace clean` is called (clean target), rm all targets

int main(int argc, char *argv[]) {
    /* --- Preliminaries --- */
    printf("START\n");
    mace_init();

    mace(argc, argv);
    mace_mkdir(obj_dir);
    mace_mkdir(build_dir);

    mace_target_build_order(targets, target_num);
    mace_build_targets(targets, target_num);
    mace_free();
    printf("FINISH\n");
    return (0);
}
