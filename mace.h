
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
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

/* --- MACE FUNCTION --- */
// User entry point. Must be implemented by user.
// Must:
//   1- Set compiler    -> MACE_SET_COMPILER
//   2- Add targets     -> MACE_ADD_TARGET
// Optional:
//   3- Set build_dir   -> mace_set_obj_dir
//   4- Set obj_dir     -> mace_set_build_dir
extern int mace(int argc, char *argv[]);

/*-----------------------------------*/
/*              EXAMPLE               /
*           MACE FUNCTION             /
* int mace(int argc, char *argv[]) {  /
*   MACE_SET_COMPILER(gcc);           /
*   MACE_ADD_COMMAND(before_foo1);    /
*   MACE_ADD_TARGET(foo1);            /
*   MACE_ADD_COMMAND(after_foo1);     /
*   MACE_ADD_TARGET(foo2);            /
* };                                  /
/*-----------------------------------*/

/* --- SETTERS --- */
/* -- Compiler -- */
#define MACE_SET_COMPILER(compiler) _MACE_SET_COMPILER(compiler)
#define _MACE_SET_COMPILER(compiler) cc = #compiler

/* -- Directores -- */
char *mace_set_obj_dir(char   *obj);
char *mace_set_build_dir(char *build);

/* -- Separator -- */
void mace_set_separator(char *sep);

/* --- Commands --- */
struct Command;
// - Commands run before target at build_order.
//  - ONE command per build_order.
// - Multiple commands at same build_order run first-come first-serve.
// - MACE_ADD_COMMAND will add command to run before next target.
#define MACE_ADD_COMMAND(command) mace_add_command(&command, #command, target_num)
void mace_add_command(struct Command *command, char *name, int build_order);

/* --- Targets --- */
struct Target;
#define MACE_ADD_TARGET(target) mace_add_target(&target, #target)
void mace_add_target(struct Target *target, char *name);

/******************************* TARGET STRUCT ********************************/
struct Target {

    /*---------------------------- PUBLIC MEMBERS ----------------------------*/
    const char *includes;          /* directories,           ' ' separated    */
    const char *sources;           /* files, glob patterns,  ' ' separated    */
    const char *sources_exclude;   /* files, glob patterns,  ' ' separated    */
    const char *base_dir;          /* directory,                              */
    const char *links;             /* libraries or targets   ' ' separated    */
    const char *flags;             /* passed as is to compiler                */
    // These commands should be necessary to build target
    const char *command_pre_build; /* command ran before building target      */
    const char *command_post_build;/* command ran after building target       */
    const char *message_pre_build; /* message printed before building target  */
    const char *message_post_build;/* message printed after building target   */
    int         kind;              /* MACE_TARGET_KIND                        */
    /* allatonce: Compile all .o objects at once (calls gcc one time).        */
    /* Compiles slightly faster: gcc is called once per .c file when false.   */
    /* WARNING: Broken when multiple sources have the same filename.          */
    bool allatonce;

    /*-----------------------------------------------------------------*/
    /*                            EXAMPLE                               /
    /*                       TARGET DEFINITION                          /
    /* Designated Initializer -> unitialized values are set to 0/NULL   /
    *                                                                   /
    * struct Target mytarget = {                                        /
    *     .includes           = "include include/foo",                  /
    *     .sources            = "src/* src/bar.c",                      /
    *     .sources_exclude    = "src/main.c",                           /
    *     .links              = "lib1 lib2 mytarget2",                  /
    *     .kind               = MACE_LIBRARY,                           /
    * };                                                                /
    * NOTE: default separator is " ", set with 'mace_set_separator'     /
    *                                                                   /
    /*-----------------------------------------------------------------*/

    /*---------------------------- PRIVATE MEMBERS ---------------------------*/

    /* -- DO NOT TOUCH! Set automatically by mace. DO NOT TOUCH! --  */
    char      *_name;              /* target name set by user                 */
    uint64_t   _hash;              /* target name hash,                       */
    int        _order;             /* target order added by user              */

    char      **_argv;             /* buffer for argv to exec build commands  */
    int         _argc;             /* number of arguments in argv             */
    int         _arg_len;          /* alloced len of argv                     */
    char      **_argv_includes;    /* includes, in argv form                  */
    int         _argc_includes;    /* number of arguments in argv_includes    */
    char      **_argv_links;       /* linked libraries, in argv form          */
    int         _argc_links;       /* number of arguments in argv_links       */
    char      **_argv_flags;       /* user flags, in argv form                */
    int         _argc_flags;       /* number of arguments in argv_flags       */

    char      **_argv_sources;     /* sources, in argv form                   */
    int         _argc_sources;     /* number of arguments in argv_sources     */
    int         _len_sources;      /* alloc len of arguments in argv_sources  */

    uint64_t   *_argv_objects_hash;/* sources, in argv form                   */
    int        *_argv_objects_cnt; /* sources, in argv form                   */
    int         _argc_objects_hash;/* number of arguments in argv_sources     */
    char      **_argv_objects;     /* sources, in argv form                   */
    int         _argc_objects;     /* number of arguments in argv_sources     */
    int         _len_objects;      /* alloc len of arguments in argv_sources  */

    uint64_t  *_deps_links;        /* target or libs hashes                   */
    size_t     _deps_links_num;    /* target or libs hashes                   */
    size_t     _deps_links_len;    /* target or libs hashes                   */
    size_t     _d_cnt;             /* dependency count, for build order       */
    /* -- DO NOT TOUCH! Set automatically by mace. DO NOT TOUCH! --  */
};

/******************************* COMMAND STRUCT *******************************/
struct Command {
    /*---------------------------- PUBLIC MEMBERS ----------------------------*/
    const char *command;                 /* command string,  ' ' separated    */

    /*---------------------------- PRIVATE MEMBERS ---------------------------*/
    /* -- DO NOT TOUCH! Set automatically by mace. DO NOT TOUCH! --  */
    char       *_name;             /* target name set by user                 */
    char      **_argv;             /* buffer for argv to exec build commands  */
    int         _argc;             /* number of arguments in argv             */
    int         _arg_len;          /* alloced len of argv                     */
    /* -- DO NOT TOUCH! Set automatically by mace. DO NOT TOUCH! --  */
};


/*----------------------------------------------------------------------------*/
/*                               MACE INTERNALS                               */
/*----------------------------------------------------------------------------*/

/********************************** CONSTANTS *********************************/

enum MACE {
    MACE_DEFAULT_TARGET_LEN     =   8,
    MACE_DEFAULT_OBJECT_LEN     =  16,
    MACE_DEFAULT_OBJECTS_LEN    = 128,
    MACE_CWD_BUFFERSIZE         = 128,
};

enum MACE_TARGET_KIND { // for target.kind
    MACE_EXECUTABLE      = 1,
    MACE_STATIC_LIBRARY  = 2,
    MACE_SHARED_LIBRARY  = 3,
    MACE_DYNAMIC_LIBRARY = 3,
};

enum MACE_ARGV { // for various argv
    MACE_ARGV_CC         = 0,
    MACE_ARGV_SOURCE     = 1, // single source compilation
    MACE_ARGV_OBJECT     = 2, // single source compilation
    MACE_ARGV_OTHER      = 3, // single source compilation
};

/******************************** DECLARATIONS ********************************/

/* --- mace --- */
void mace_init();
void mace_free();
void mace_post_user();
void mace_exec_print(char *const arguments[], size_t argnum);

/* --- mace_hashing --- */
uint64_t mace_hash(char *str);

/* --- mace_utils --- */
/* -- str -- */
char  *mace_str_buffer(const char *const strlit);
char  *mace_str_copy(char *restrict buffer, const char *str);

/* -- argv -- */
char **mace_argv_flags(int *len, int *argc, char **argv, const char *includes, const char *flag,
                       bool path);
char **mace_argv_grow(char **argv, int *argc, int *arg_len);
void   mace_argv_free(char **argv, int argc);

/* --- mace_setters --- */
char *mace_set_obj_dir(char    *obj);
char *mace_set_build_dir(char  *build);

/* --- mace_Target --- */
void mace_add_target(struct Target   *target,  char *name);
void mace_add_command(struct Command *command, char *name, int build_order);

/* -- Command OOP -- */
void mace_Command_Free(struct Command *command);
void mace_Command_Free_argv(struct Command *command);
void mace_Command_Parse_User(struct Command *command);

/* -- Target OOP -- */
void mace_Target_Free(struct Target              *target);
bool mace_Target_hasDep(struct Target            *target, uint64_t hash);
void mace_Target_compile(struct Target           *target);
void Target_Free_notargv(struct Target           *target);
void mace_Target_Free_argv(struct Target         *target);
void mace_Target_Deps_Hash(struct Target         *target);
void mace_Target_argv_init(struct Target         *target);
void mace_Target_argv_grow(struct Target         *target);
bool mace_Target_Source_Add(struct Target        *target, char    *token);
void mace_Target_Object_Add(struct Target        *target, char    *token);
void mace_Target_Parse_User(struct Target        *target);
void mace_Target_argv_allatonce(struct Target    *target);
void mace_Target_compile_allatonce(struct Target *target);

/* --- mace_glob --- */
int     mace_globerr(const char *path, int eerrno);
glob_t  mace_glob_sources(const char *path);

/* --- mace_exec --- */
pid_t mace_exec(const char *exec, char *const arguments[]);
void  mace_wait_pid(int pid);

/* --- mace_build --- */
/* -- linking -- */
void mace_link_static_library(char *target, char **av_o, int ac_o);
void mace_link_dynamic_library(char *target, char *objects);
void mace_link_executable(char *target, char **av_o, int ac_o, char **av_l,
                          int ac_l, char **av_f, int ac_f);

/* -- compiling object files -> .o -- */
void mace_compile_glob(struct Target *target, char *globsrc, const char *restrict flags);
void mace_build_targets();

/* -- build_order -- */
/* build order of all targets */
void mace_targets_build_order();
/* build order of target links */
void mace_links_build_order(struct Target target, size_t *o_cnt);

/* --- mace_is --- */
int mace_isWildcard(const char *str);
int mace_isSource(const   char *path);
int mace_isObject(const   char *path);
int mace_isDir(const      char *path);

/* --- mace_filesystem --- */
void  mace_mkdir(const char    *path);
void  mace_object_path(char    *source);
char *mace_library_path(char   *target_name);

/* --- mace_globals --- */
bool verbose = false;

/* -- separator -- */
char *mace_separator = " ";

/* -- Checksum -- */
// char *checksum = "sha256sum";
char *checksum = "sha1DC";
// -> git uses sha1DC and SO WILL I

/* -- Compiler -- */
char *cc      = NULL; // DESIGN QUESTION: Should I set a default?
char *ar      = "ar";
char *install = "install";

/* -- current working directory -- */
char cwd[MACE_CWD_BUFFERSIZE];

/* -- build order -- */
size_t *build_order = NULL;
size_t  build_order_num = 0;

/* -- list of commands added by user -- */
struct Command *commands    = NULL;   /* [order] is as added by user      */

/* -- list of targets added by user -- */
struct Target  *targets    = NULL;   /* [order] is as added by user      */
size_t          target_num = 0;
size_t          target_len = 0;

/* -- buffer to write object -- */
char           *object     = NULL;
size_t          object_len = 0;

/* -- directories -- */
char           *obj_dir    = NULL;   /* intermediary .o files            */
char           *build_dir  = NULL;   /* linked libraries, executables    */

/* -- mace_globals control -- */
void mace_object_grow();

/*********************************** MACROS ***********************************/

#define MACE_STRINGIFY(x) _MACE_STRINGIFY(x)
#define _MACE_STRINGIFY(x) #x

/******************************* MACE_ADD_TARGET ******************************/
#define vprintf(format, ...) do {\
        if (verbose)\
            printf(format, ##__VA_ARGS__);\
    } while(0)

/******************************* MACE_ADD_TARGET ******************************/
void mace_grow_targets() {
    target_len *= 2;
    targets     = realloc(targets,     target_len * sizeof(*targets));
    commands    = realloc(commands,    target_len * sizeof(*commands));
    build_order = realloc(build_order, target_len * sizeof(*build_order));
}

void mace_add_command(struct Command *command, char *name, int build_order) {
    printf("mace_add_command %d\n", build_order);
    if (build_order >= target_len)
        mace_grow_targets();
    commands[build_order]        = *command;
    commands[build_order]._name  =  name;

    printf("commands[build_order] %s\n", commands[build_order]._name);
    mace_Command_Parse_User(&commands[build_order]);
    mace_exec_print(commands[build_order]._argv, commands[build_order]._argc);
    printf("commands[build_order] %s\n", commands[build_order]._name);
}

void mace_add_target(struct Target *target, char *name) {
    targets[target_num]        = *target;
    targets[target_num]._name  = name;
    targets[target_num]._hash  = mace_hash(name);
    targets[target_num]._order = target_num;
    mace_Target_Deps_Hash(&targets[target_num]);
    mace_Target_Parse_User(&targets[target_num]);
    mace_Target_argv_init(&targets[target_num]);
    if (++target_num >= target_len) {
        mace_grow_targets();
    }
}

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


/****************************** MACE_SET_obj_dir ******************************/
// Sets where the object files will be placed during build.
char *mace_set_obj_dir(char *obj) {
    return (obj_dir = mace_str_copy(obj_dir, obj));
}

char *mace_set_build_dir(char *build) {
    return (build_dir = mace_str_copy(build_dir, build));
}

char *mace_str_copy(char *restrict buffer, const char *str) {
    if (buffer != NULL)
        free(buffer);
    size_t len  = strlen(str);
    buffer      = calloc(len + 1, sizeof(*buffer));
    strncpy(buffer, str, len);
    return (buffer);
}

/************************************ parg ************************************/
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

void mace_argv_free(char **argv, int argc) {
    for (int i = 0; i < argc; i++) {
        if (argv[i] != NULL) {
            free(argv[i]);
        }
    }
    free(argv);
}

char **mace_argv_flags(int *len, int *argc, char **argv, const char *user_str, const char *flag,
                       bool path) {
    assert(argc != NULL);
    assert(len != NULL);
    assert((*len) > 0);
    size_t flag_len = (flag == NULL) ? 0 : strlen(flag);

    /* -- Copy user_str into modifiable buffer -- */
    char *buffer = mace_str_buffer(user_str);

    char *token = strtok(buffer, mace_separator);
    while (token != NULL) {
        argv = argv_grows(len, argc, argv);
        char *to_use = token;
        char *pathstr = NULL;
        if (path) {
            pathstr = realpath(token, NULL);
            to_use = pathstr;
        }
        size_t to_use_len = strlen(to_use);

        size_t total_len = (to_use_len + flag_len + 1);
        size_t i = 0;
        char *arg = calloc(total_len, sizeof(*arg));

        /* - Copy flag into arg - */
        if (flag_len > 0) {
            strncpy(arg, flag, flag_len);
            i += flag_len;
        }

        /* - Copy token into arg - */
        strncpy(arg + i, to_use, to_use_len);
        i += to_use_len;
        argv[(*argc)++] = arg;

        token = strtok(NULL, mace_separator);
    }

    free(buffer);
    return (argv);
}

void mace_Command_Parse_User(struct Command *command) {
    mace_Command_Free_argv(command);
    int len, bytesize;
    /* -- Make _argv_includes to argv -- */
    if (command->command == NULL) {
        perror("Command was added with NULL command string.");
        exit(EPERM);
    }
    len = 8;
    command->_argc = 0;
    command->_argv = calloc(len, sizeof(*command->_argv));
    command->_argv = mace_argv_flags(&len, &command->_argc, command->_argv,
                                     command->command, NULL, false);
    bytesize       = command->_argc * sizeof(*command->_argv);
    command->_argv = realloc(command->_argv, bytesize);
    mace_exec_print(command->_argv, command->_argc);
}


void mace_Target_Parse_User(struct Target *target) {
    // Makes flags for target includes, links libraries, and flags
    //  NOT sources: they can be folders, so need to be globbed
    mace_Target_Free_argv(target);
    int len, bytesize;

    /* -- Make _argv_includes to argv -- */
    if (target->includes != NULL) {
        len = 8;
        target->_argc_includes = 0;
        target->_argv_includes = calloc(len, sizeof(*target->_argv_includes));
        target->_argv_includes = mace_argv_flags(&len, &target->_argc_includes,
                                                 target->_argv_includes,
                                                 target->includes, "-I", true);
        bytesize               = target->_argc_includes * sizeof(*target->_argv_includes);
        target->_argv_includes = realloc(target->_argv_includes, bytesize);
    }

    /* -- Make _argv_links to argv -- */
    if (target->links != NULL) {
        len = 8;
        target->_argc_links = 0;
        target->_argv_links = calloc(len, sizeof(*target->_argv_links));
        target->_argv_links = mace_argv_flags(&len, &target->_argc_links, target->_argv_links,
                                              target->links, "-l", false);
        bytesize            = target->_argc_links * sizeof(*target->_argv_links);
        target->_argv_links = realloc(target->_argv_links, bytesize);
    }

    /* -- Make _argv_flags to argv -- */
    if (target->flags != NULL) {
        len = 8;
        target->_argc_flags = 0;
        target->_argv_flags = calloc(len, sizeof(*target->_argv_flags));
        target->_argv_flags = mace_argv_flags(&len, &target->_argc_flags, target->_argv_flags,
                                              target->flags, NULL, false);
        bytesize            = target->_argc_flags * sizeof(*target->_argv_flags);
        target->_argv_flags = realloc(target->_argv_flags, bytesize);
    }
}

void mace_Target_argv_grow(struct Target *target) {
    target->_argv = mace_argv_grow(target->_argv, &target->_argc, &target->_arg_len);
}


char **mace_argv_grow(char **argv, int *argc, int *arg_len) {
    if (*argc >= *arg_len) {
        (*arg_len) *= 2;
        argv = realloc(argv, *arg_len * sizeof(*argv));
    }
    return (argv);
}

// should be called after all sources have been added.
void mace_Target_argv_allatonce(struct Target *target) {
    if (target->_argv == NULL) {
        target->_arg_len = 8;
        target->_argc = 0;
        target->_argv = calloc(target->_arg_len, sizeof(*target->_argv));
    }
    target->_argv[MACE_ARGV_CC] = cc;
    target->_argc = MACE_ARGV_CC + 1;

    /* -- argv sources -- */
    if ((target->_argc_sources > 0) && (target->_argv_sources != NULL)) {
        for (int i = 0; i < target->_argc_sources; i++) {
            mace_Target_argv_grow(target);
            target->_argv[target->_argc++] = target->_argv_sources[i];
        }
    }

    /* -- argv includes -- */
    if ((target->_argc_includes > 0) && (target->_argv_includes != NULL)) {
        for (int i = 0; i < target->_argc_includes; i++) {
            mace_Target_argv_grow(target);
            target->_argv[target->_argc++] = target->_argv_includes[i];
        }
    }

    /* -- argv -L flag for build_dir -- */
    mace_Target_argv_grow(target);
    assert(build_dir != NULL);
    size_t build_dir_len = strlen(build_dir);
    char *ldirflag = calloc(3 + build_dir_len, sizeof(*ldirflag));
    strncpy(ldirflag, "-L", 2);
    strncpy(ldirflag + 2, build_dir, build_dir_len);
    target->_argv[target->_argc++] = ldirflag;

    /* -- argv -c flag for libraries -- */
    mace_Target_argv_grow(target);
    char *compflag = calloc(3, sizeof(*compflag));
    strncpy(compflag, "-c", 2);
    target->_argv[target->_argc++] = compflag;
}

// should be called after mace_Target_Parse_User
void mace_Target_argv_init(struct Target *target) {
    if (target->_argv == NULL) {
        target->_arg_len = 8;
        target->_argc = 0;
        target->_argv = calloc(target->_arg_len, sizeof(*target->_argv));
    }
    target->_argv[MACE_ARGV_CC] = cc;
    /* --- Adding argvs common to all --- */
    target->_argc = MACE_ARGV_OTHER;
    /* -- argv user flags -- */
    if ((target->_argc_flags > 0) && (target->_argv_flags != NULL)) {
        for (int i = 0; i < target->_argc_flags; i++) {
            mace_Target_argv_grow(target);
            target->_argv[target->_argc++] = target->_argv_flags[i];
        }
    }
    /* -- argv includes -- */
    if ((target->_argc_includes > 0) && (target->_argv_includes != NULL)) {
        for (int i = 0; i < target->_argc_includes; i++) {
            mace_Target_argv_grow(target);
            target->_argv[target->_argc++] = target->_argv_includes[i];
        }
    }
    /* -- argv links -- */
    if ((target->_argc_links > 0) && (target->_argv_links != NULL)) {
        for (int i = 0; i < target->_argc_links; i++) {
            mace_Target_argv_grow(target);
            target->_argv[target->_argc++] = target->_argv_links[i];
        }
    }

    /* -- argv -L flag for build_dir -- */
    mace_Target_argv_grow(target);
    assert(build_dir != NULL);
    size_t build_dir_len = strlen(build_dir);
    char *ldirflag = calloc(3 + build_dir_len, sizeof(*ldirflag));
    strncpy(ldirflag, "-L", 2);
    strncpy(ldirflag + 2, build_dir, build_dir_len);
    target->_argv[target->_argc++] = ldirflag;

    /* -- argv -c flag for libraries -- */
    mace_Target_argv_grow(target);
    char *compflag = calloc(3, sizeof(*compflag));
    strncpy(compflag, "-c", 2);
    target->_argv[target->_argc++] = compflag;

    mace_Target_argv_grow(target);
    target->_argv[target->_argc] = NULL;
}

void mace_set_separator(char *sep) {
    if (sep == NULL) {
        perror("Separator should not be NULL.");
        exit(EPERM);
    }
    if (strlen(sep) != 1) {
        perror("Separator should have length one.");
        exit(EPERM);
    }
    mace_separator = sep;
}

/****************************** mace_glob_sources *****************************/
glob_t mace_glob_sources(const char *path) {
    /* If source is a folder, get all .c files in it */
    glob_t  globbed;
    int     flags   = 0;
    int     ret     = glob(path, flags, mace_globerr, &globbed);
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

int mace_globerr(const char *path, int eerrno) {
    fprintf(stderr, "%s: %s\n", path, strerror(eerrno));
    exit(eerrno);
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
        perror("Error: forking issue. \n");
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
            //pass
        } else if (WIFEXITED(status) &&  WEXITSTATUS(status)) {
            if (WEXITSTATUS(status) == 127) {
                // execvp failed
                perror("execvp failed\n");
                exit(WEXITSTATUS(status));
            } else {
                perror("program terminated normally, but returned a non-zero status\n");
                exit(WEXITSTATUS(status));
            }
        } else {
            perror("program didn't terminate- normally\n");
            exit(WEXITSTATUS(status));
        }
    }
}

/********************************* mace_build **********************************/
/* Build all sources from target to object */
void mace_link_static_library(char *target, char **argv_objects, int argc_objects) {
    vprintf("Linking \t%s \n", target);
    int arg_len = 8;
    int argc = 0;
    char **argv = calloc(arg_len, sizeof(*argv));

    argv[argc++] = ar;
    /* --- Adding -rcs flag --- */
    char *rcsflag       = calloc(5, sizeof(*rcsflag));
    strncpy(rcsflag, "-rcs", 4);
    argv[argc++] = rcsflag;

    /* --- Adding target --- */
    size_t target_len = strlen(target);
    char *targetv       = calloc(target_len + 1, sizeof(*rcsflag));
    strncpy(targetv, target, target_len);
    argv[argc++] = targetv;


    /* --- Adding objects --- */
    if ((argc_objects > 0) && (argv_objects != NULL)) {
        for (int i = 0; i < argc_objects; i++) {
            argv = mace_argv_grow(argv, &argc, &arg_len);
            argv[argc++] = argv_objects[i] + strlen("-o");
        }
    }

    mace_exec_print(argv, argc);
    pid_t pid = mace_exec(ar, argv);
    mace_wait_pid(pid);
}


void mace_link_executable(char *target, char **argv_objects, int argc_objects, char **argv_links,
                          int argc_links, char **argv_flags, int argc_flags) {
    vprintf("Linking \t%s \n", target);

    if (cc == NULL) {
        perror("C compiler not set, exiting.");
        exit(EFAULT);
    }
    int arg_len = 8;
    int argc = 0;
    char **argv = calloc(arg_len, sizeof(*argv));

    argv[argc++] = cc;
    /* --- Adding executable output --- */
    size_t target_len = strlen(target);
    char *oflag       = calloc(target_len + 3, sizeof(*oflag));
    strncpy(oflag, "-o", 2);
    strncpy(oflag + 2, target, target_len);
    argv[argc++] = oflag;

    /* --- Adding objects --- */
    if ((argc_objects > 0) && (argv_objects != NULL)) {
        for (int i = 0; i < argc_objects; i++) {
            argv = mace_argv_grow(argv, &argc, &arg_len);
            argv[argc++] = argv_objects[i] + strlen("-o");
        }
    }

    /* -- argv user flags -- */
    if ((argc_flags > 0) && (argv_flags != NULL)) {
        for (int i = 0; i < argc_flags; i++) {
            argv = mace_argv_grow(argv, &argc, &arg_len);
            argv[argc++] = argv_flags[i];
        }
    }

    /* -- argv links -- */
    if ((argc_links > 0) && (argv_links != NULL)) {
        for (int i = 0; i < argc_links; i++) {
            argv = mace_argv_grow(argv, &argc, &arg_len);
            argv[argc++] = argv_links[i];
        }
    }

    /* -- argv -L flag for build_dir -- */
    argv = mace_argv_grow(argv, &argc, &arg_len);
    size_t build_dir_len = strlen(build_dir);
    char *ldirflag = calloc(3 + build_dir_len, sizeof(*ldirflag));
    strncpy(ldirflag, "-L", 2);
    strncpy(ldirflag + 2, build_dir, build_dir_len);
    argv[argc++] = ldirflag;

    mace_exec_print(argv, argc);
    pid_t pid = mace_exec(cc, argv);
    mace_wait_pid(pid);
    free(argv);
}

void mace_link_dynamic_library(char *target, char *objects) {
    // NOTE: -fPIC is needed on all object files in a shared library
    // command: gcc -shared -fPIC ...
    //
}


void mace_Target_compile_allatonce(struct Target *target) {
    // Compile ALL objects at once
    /* -- Move to obj_dir -- */
    assert(chdir(cwd) == 0);
    assert(chdir(obj_dir) == 0);

    /* -- Prepare argv -- */
    mace_Target_argv_allatonce(target);

    /* -- Actual compilation -- */
    mace_exec_print(target->_argv, target->_argc);
    pid_t pid = mace_exec(cc, target->_argv);
    mace_wait_pid(pid);

    /* -- Go back to cwd -- */
    assert(chdir(cwd) == 0);
}

void mace_Target_compile(struct Target *target) {
    // Compile latest object
    assert(target != NULL);
    assert(target->_argv != NULL);
    assert(target->_argv_sources[target->_argc_sources - 1] != NULL);
    assert(target->_argv_objects[target->_argc_objects - 1] != NULL);

    /* - Single source argv - */
    printf("Compile %s\n", target->_argv_sources[target->_argc_sources - 1]);

    // argv[0] is always cc
    // argv[1] is always source
    target->_argv[MACE_ARGV_SOURCE] = target->_argv_sources[target->_argc_sources - 1];

    // argv[2] is always object (includeing -o flag)
    target->_argv[MACE_ARGV_OBJECT] = target->_argv_objects[target->_argc_objects - 1];
    // rest of argv should be set previously by mace_Target_argv_init

    /* -- Actual compilation -- */
    // mace_exec_print(target->_argv, target->_argc);
    pid_t pid = mace_exec(cc, target->_argv);
    mace_wait_pid(pid);
}

void Target_Object_Hash_Add(struct Target *target, uint64_t hash) {
    target->_argv_objects_hash[target->_argc_objects_hash] = hash;
    target->_argv_objects_cnt[target->_argc_objects_hash++] = 0;
}

int Target_hasObjectHash(struct Target *target, uint64_t hash) {
    if (target->_argv_objects_hash == NULL)
        return (-1);

    for (int i = 0; i < target->_argc_objects_hash; i++) {
        if (hash == target->_argv_objects_hash[i])
            return (i);
    }

    return (-1);
}

void mace_Target_Object_Add(struct Target *target, char *token) {
    if (target->_argv_objects == NULL) {
        target->_len_objects = 8;
        target->_argv_objects = malloc(target->_len_objects * sizeof(*target->_argv_objects));
    }
    if (target->_argv_objects_cnt == NULL) {
        target->_argv_objects_cnt = malloc(target->_len_objects * sizeof(*target->_argv_objects_cnt));
    }
    if (target->_argv_objects_hash == NULL) {
        target->_argv_objects_hash = calloc(target->_len_objects, sizeof(*target->_argv_objects_hash));
    }

    if (token == NULL)
        return;
    target->_argv_objects = argv_grows(&target->_len_objects, &target->_argc_objects,
                                       target->_argv_objects);

    if (target->_len_objects >= target->_argc_objects_hash) {
        target->_argv_objects_hash = realloc(target->_argv_objects_hash,
                                             target->_len_objects * sizeof(*target->_argv_objects_hash));
        target->_argv_objects_cnt = realloc(target->_argv_objects_cnt,
                                            target->_len_objects * sizeof(*target->_argv_objects_cnt));
    }

    uint64_t hash = mace_hash(token);

    int hash_id = Target_hasObjectHash(target, hash);

    if (hash_id < 0) {
        Target_Object_Hash_Add(target, hash);
    } else {
        target->_argv_objects_cnt[hash_id]++;
        if (target->_argv_objects_cnt[hash_id] >= 10) {
            perror("Too many same name sources/objects");
            exit(-1);
        }
    }

    size_t token_len = strlen(token);
    char *flag = "-o";
    size_t flag_len = strlen(flag);
    size_t total_len = token_len + flag_len + 1;
    if (hash_id > 0)
        total_len++;
    char *arg = calloc(total_len, sizeof(*arg));
    strncpy(arg, flag, flag_len);
    strncpy(arg + flag_len, token, token_len);

    if (hash_id > 0) {
        char *pos = strrchr(arg, '.');
        *(pos) = target->_argv_objects_cnt[hash_id] + '0';
        *(pos + 1) = '.';
        *(pos + 2) = 'o';
    }
    target->_argv_objects[target->_argc_objects++] = arg;
}

bool mace_Target_Source_Add(struct Target *target, char *token) {
    bool excluded = false;

    if (target->_argv_sources == NULL) {
        target->_len_sources = 8;
        target->_argv_sources = malloc(target->_len_sources * sizeof(*target->_argv_sources));
    }

    if (token == NULL)
        return (true);

    target->_argv_sources = argv_grows(&target->_len_sources, &target->_argc_sources,
                                       target->_argv_sources);

    size_t token_len = strlen(token);
    char *arg = calloc(token_len + 1, sizeof(*arg));
    strncpy(arg, token, token_len);
    assert(arg != NULL);
    char* rpath = calloc(PATH_MAX, sizeof(*target->_argv_sources));
    rpath = realpath(arg, rpath);
    rpath = realloc(rpath, (strlen(rpath) + 1) * sizeof(*targets));
    if (rpath == NULL) {
        fprintf(stderr, "realpath error : %s\n", strerror(errno));
        exit(errno);
    }
    target->_argv_sources[target->_argc_sources++] = rpath;
    free(arg);
    return (excluded);
}


/* Compile globbed files to objects */
void mace_compile_glob(struct Target *target, char *globsrc, const char *restrict flags) {
    glob_t globbed = mace_glob_sources(globsrc);
    for (int i = 0; i < globbed.gl_pathc; i++) {
        assert(mace_isSource(globbed.gl_pathv[i]));
        char *pos = strrchr(globbed.gl_pathv[i], '/');
        char *source_file = (pos == NULL) ? globbed.gl_pathv[i] : pos + 1;
        bool excluded = mace_Target_Source_Add(target, globbed.gl_pathv[i]);
        if (excluded)
            continue;
        mace_object_path(source_file);
        mace_Target_Object_Add(target, object);
        if (!target->allatonce)
            mace_Target_compile(target);
    }
    globfree(&globbed);
}

/********************************** mace_is ***********************************/
int mace_isTarget(uint64_t hash) {
    for (int i = 0; i < target_num; i++) {
        if (targets[i]._hash == hash)
            return (true);
    }
    return (false);
}

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

char *mace_executable_path(char *target_name) {
    assert(target_name != NULL);
    size_t bld_len = strlen(build_dir);
    size_t tar_len = strlen(target_name);

    char *exec = calloc((bld_len + tar_len + 1), sizeof(*exec));
    size_t full_len = 0;
    strncpy(exec,            build_dir,   bld_len);
    full_len += bld_len;
    if (build_dir[0] != '/') {
        strncpy(exec + full_len, "/",         1);
        full_len++;
    }
    strncpy(exec + full_len, target_name, tar_len);
    return (exec);
}

// TODO: static and dynamic path
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
void mace_object_grow() {
    object_len *= 2;
    object      = realloc(object,   object_len  * sizeof(*object));
}

void mace_object_path(char *source) {
    /* --- Expanding path --- */
    size_t cwd_len      = strlen(cwd);
    size_t obj_dir_len  = strlen(obj_dir);
    char *path = calloc(cwd_len + obj_dir_len + 2, sizeof(path));
    strncpy(path,                cwd,        cwd_len);
    strncpy(path + cwd_len,      "/",        1);
    strncpy(path + cwd_len + 1,  obj_dir,    obj_dir_len);

    if (path == NULL) {
        fprintf(stderr, "Object directory '%s' does not exist.\n", obj_dir);
        exit(ENOENT);
    }

    /* --- Grow object --- */
    size_t source_len = strlen(source);
    size_t path_len;
    while (((path_len = strlen(path)) + source_len + 2) >= object_len)
        mace_object_grow();
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
void mace_run_command(struct Command *command) {
    if (command->_argv == NULL)
        return;

    assert(chdir(cwd) == 0);
    mace_exec_print(command->_argv, command->_argc);
    pid_t pid = mace_exec(install,  command->_argv);
    mace_wait_pid(pid);
}

/******************************** mace_build **********************************/
void mace_build_target(struct Target *target) {
    /* --- Move to target base_dir, compile there --- */
    printf("Build target %s\n", target->_name);
    if (target->base_dir != NULL)
        assert(chdir(target->base_dir) == 0);

    /* --- Parse sources, put into array --- */
    assert(target->kind != 0);
    /* --- Compile sources --- */
    /* --- Preliminaries --- */
    Target_Free_notargv(target);

    /* -- Copy sources into modifiable buffer -- */
    char *buffer = mace_str_buffer(target->sources);

    /* --- Split sources into tokens --- */
    char *token = strtok(buffer, mace_separator);
    do {
        // printf("token %s\n", token);

        if (mace_isDir(token)) {
            /* Glob all sources recursively */
            // printf("isDir %s\n", token);

            size_t srclen  = strlen(token);
            char  *globstr = calloc(srclen + 6, sizeof(*globstr));
            strncpy(globstr,              token,  strlen(token));
            strncpy(globstr + srclen,     "/",    1);
            strncpy(globstr + srclen + 1, "**.c", 4);

            mace_compile_glob(target, globstr, target->flags);
            free(globstr);

        } else if (mace_isWildcard(token)) {
            /* token has a wildcard in it */
            // printf("isWildcard %s\n", token);
            mace_compile_glob(target, token, target->flags);

        } else if (mace_isSource(token)) {
            /* token is a source file */
            // printf("isSource %s\n", token);

            bool excluded = mace_Target_Source_Add(target, token);
            if (!excluded) {
                mace_object_path(token);
                mace_Target_Object_Add(target, object);
                if (!target->allatonce)
                    mace_Target_compile(target);
            }

        } else {
            printf("Error: source is neither a .c file, a folder, nor has a wildcard in it\n");
            exit(ENOENT);
        }

        token = strtok(NULL, mace_separator);
    } while (token != NULL);

    /* --- If allatonce enable, compile now. --- */
    if (target->allatonce)
        mace_Target_compile_allatonce(target);

    /* --- Move back to cwd to link --- */
    assert(chdir(cwd) == 0);

    /* --- Linking --- */
    if (target->kind == MACE_STATIC_LIBRARY) {
        char *lib = mace_library_path(target->_name);
        mace_link_static_library(lib, target->_argv_objects, target->_argc_objects);
        free(lib);
    } else if (target->kind == MACE_EXECUTABLE) {
        char *exec = mace_executable_path(target->_name);
        mace_link_executable(exec, target->_argv_objects, target->_argc_objects, target->_argv_links,
                             target->_argc_links, target->_argv_flags, target->_argc_flags);

    }
    free(buffer);
}

bool mace_isTargetinBuildOrder(size_t order) {
    bool out = false;
    assert(build_order != NULL);
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
    assert(build_order != NULL);
    assert(build_order_num < target_num);
    build_order[build_order_num++] = order;
}

/* - Depth first search through depencies - */
// Builds all target dependencies before building target
void mace_links_build_order(struct Target target, size_t *o_cnt) {
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
    for (target._d_cnt = 0; target._d_cnt < target._deps_links_num; target._d_cnt++) {
        if (!mace_isTarget(target._deps_links[target._d_cnt]))
            continue;

        size_t next_target_order = mace_hash_order(target._deps_links[target._d_cnt]);
        /* Recursively search target's next dependency -> depth first search */
        mace_links_build_order(targets[next_target_order], o_cnt);
    }

    /* Target already in build order, skip */
    if (mace_isTargetinBuildOrder(order))
        return;

    /* All dependencies of target were built, add it to build order */
    if (target._d_cnt != target._deps_links_num) {
        printf("Error: Not all target dependencies before target in build order.");
        exit(1);
    }

    mace_build_order_add(order);
    return;
}

bool mace_Target_hasDep(struct Target *target, uint64_t hash) {
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
            if (mace_Target_hasDep(&targs[j], hash_i))
                // 2- target i's dependency j has i as dependency as well
                return (true);
        }
    }
    return (false);
}

void mace_targets_build_order() {
    size_t o_cnt = 0; /* order count */

    /* If only 1 include, build order is trivial */
    if (target_num == 1) {
        mace_build_order_add(0);
        return;
    }

    /* Visit all targets */
    while (o_cnt < target_num) {
        mace_links_build_order(targets[o_cnt], &o_cnt);
        o_cnt++;
    }
}

void mace_build_targets() {
    int z = 0;
    for (z = 0; z < build_order_num; z++) {
        mace_run_command(&commands[z]);
        mace_build_target(&targets[build_order[z]]);
    }
    mace_run_command(&commands[z]);
}

/*----------------------------------------------------------------------------*/
/*                               MACE INTERNALS                               */
/*----------------------------------------------------------------------------*/

void mace_Command_Free(struct Command *command) {
    mace_Command_Free_argv(command);
}

void mace_Command_Free_argv(struct Command *command) {
    mace_argv_free(command->_argv, command->_argc);
    command->_argv = NULL;
    command->_argc = 0;
}

void mace_Target_Free(struct Target *target) {
    Target_Free_notargv(target);
    mace_Target_Free_argv(target);
}

void Target_Free_notargv(struct Target *target) {
    if (target->_deps_links != NULL) {
        free(target->_deps_links);
        target->_deps_links = NULL;
    }
}

void mace_Target_Free_argv(struct Target *target) {
    mace_argv_free(target->_argv_includes, target->_argc_includes);
    target->_argv_includes = NULL;
    target->_argc_includes = 0;
    mace_argv_free(target->_argv_sources, target->_argc_sources);
    target->_argv_sources = NULL;
    target->_argc_sources = 0;
    mace_argv_free(target->_argv_links, target->_argc_links);
    target->_argv_links = NULL;
    target->_argc_links = 0;
    mace_argv_free(target->_argv_flags, target->_argc_flags);
    target->_argv_flags = NULL;
    target->_argc_flags = 0;
    mace_argv_free(target->_argv_sources, target->_argc_sources);
    target->_argv_sources = NULL;
    target->_argc_sources = 0;
    mace_argv_free(target->_argv_objects, target->_argc_objects);
    target->_argv_objects = NULL;
    target->_argc_objects = 0;
    if ((target->_argv != NULL) && (target->_argc > 0))  {
        if (target->_argv[target->_argc - 1] != NULL) {
            free(target->_argv[target->_argc - 1]);
            target->_argv[target->_argc - 1] = NULL;
        }

        if (target->_argv[target->_argc - 2] != NULL) {
            free(target->_argv[target->_argc - 2]);
            target->_argv[target->_argc - 2] = NULL;
        }
        free(target->_argv);
        target->_argv = NULL;
    }
}

void mace_post_build_order() {
    // Checks that build order:
    if ((build_order == NULL) || (build_order_num == 0)) {
        printf("No targets in build order. Exiting.\n");
        exit(EDOM);
    }
}

void mace_post_user() {
    // Checks that user:
    //   1- Set compiler,
    //   2- Added at least one target,
    //   3- Did not add a circular dependency.
    // If not exit with error.

    /* Check that compiler is set */
    if (cc == NULL) {
        perror("Compiler not set. Exiting.\n");
        exit(ENXIO);
    }

    /* Check that a target exists */
    if ((targets == NULL) || (target_num <= 0)) {
        perror("No targets to compile. Exiting.\n");
        exit(ENXIO);
    }

    /* Check for circular dependency */
    if (mace_circular_deps(targets, target_num)) {
        perror("Circular dependency in linked library detected. Exiting\n");
        exit(ENXIO);
    }
}


void mace_init() {
    mace_free();
    if (getcwd(cwd, MACE_CWD_BUFFERSIZE) == NULL) {
        perror("getcwd() error");
        exit(errno);
    }

    /* --- Memory allocation --- */
    if (build_order == NULL) {
        build_order = malloc(target_len * sizeof(*build_order));
        build_order_num = 0;
    }

    target_len  = MACE_DEFAULT_TARGET_LEN;
    object_len  = MACE_DEFAULT_OBJECT_LEN;

    object   = calloc(object_len, sizeof(*object));
    targets  = calloc(target_len, sizeof(*targets));
    commands = calloc(target_len, sizeof(*commands));

    /* --- Default output folders --- */
    mace_set_build_dir("build/");
    mace_set_obj_dir("obj/");
}

void mace_free() {
    for (int i = 0; i < target_num; i++) {
        mace_Target_Free(&targets[i]);
    }
    if (targets != NULL) {
        free(targets);
        targets = NULL;
    }
    if (object != NULL) {
        free(object);
        object = NULL;
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
    target_num      = 0;
    object_len      = 0;
    build_order_num = 0;
}

void mace_Target_Deps_Hash(struct Target *target) {
    /* --- Preliminaries --- */
    if (target->links == NULL)
        return;

    /* --- Alloc space for deps --- */
    target->_deps_links_num =  0;
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
// 2- Builds dependency graph from targets' links
// 3- TODO: Determine which targets need to be recompiled
// 4- Build the targets
// TODO: if `mace clean` is called (clean target), rm all targets
#ifndef MACE_OVERRIDE_MAIN
int main(int argc, char *argv[]) {
    printf("START\n");

    /* --- Get cwd, alloc memory, set defaults. --- */
    mace_init();

    /* --- User function --- */
    // Sets compiler, add targets and commands.
    mace(argc, argv);

    /* --- Post-user checks: compiler set, at least one target exists. --- */
    mace_post_user();

    /* --- Make output directories. --- */
    mace_mkdir(obj_dir);
    mace_mkdir(build_dir);

    /* --- Compute build order using targets links list. --- */
    mace_targets_build_order();

    /* --- Perform compilation with buil_order --- */
    mace_build_targets();

    /* --- Finish --- */
    mace_free();
    printf("FINISH\n");
    return (0);
}
#endif /* MACE_OVERRIDE_MAIN */