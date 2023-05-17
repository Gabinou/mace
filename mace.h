/*
* mace.h
*
* Copyright (C) Gabriel Taillon, 2023
*
* Single header build system. Uses C to exclusively build C.
* Simple and specific. No config files with weird syntax.
*
* Usage:
*  - Write a macefile.c
*       - Implement the function mace.
*       - Set compiler, add targets, add commands, etc.
*       - Compile and run to build. Manually, or with the convenience executable.
*
* See README for more details.
*/

#define _XOPEN_SOURCE 500
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
#include <ftw.h>

#ifndef MACE_CONVENIENCE_EXECUTABLE
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
*   MACE_SET_OBJ_DIR(obj);            /
*   MACE_SET_BUILD_DIR(build);        /
*   MACE_ADD_TARGET(foo1);            /
* };                                  /
/*-----------------------------------*/

/* --- SETTERS --- */
/* -- Compiler -- */
#define MACE_SET_COMPILER(compiler) _MACE_SET_COMPILER(compiler)
#define _MACE_SET_COMPILER(compiler) cc = #compiler

/* -- Directories -- */
/* - obj_dir - */
char *mace_set_obj_dir(char   *obj);
#define MACE_SET_OBJ_DIR(dir) _MACE_SET_OBJ_DIR(dir)
#define _MACE_SET_OBJ_DIR(dir) mace_set_obj_dir(#dir)

/* - build_dir - */
char *mace_set_build_dir(char *build);
#define MACE_SET_BUILD_DIR(dir) _MACE_SET_BUILD_DIR(dir)
#define _MACE_SET_BUILD_DIR(dir) mace_set_build_dir(#dir)

/* -- Separator -- */
void mace_set_separator(char *sep);
#define MACE_SET_SEPARATOR(sep) _MACE_SET_SEPARATOR(sep)
#define _MACE_SET_SEPARATOR(sep) mace_set_separator(#sep)

/* --- Targets --- */
struct Target;
#define MACE_ADD_TARGET(target)     mace_add_target(&target, #target)
#define MACE_DEFAULT_TARGET(target) mace_set_default_target(#target)

void mace_add_target(struct Target *restrict target, char *restrict name);

void mace_set_default_target(char *name);
// To compile default target:
//  1- Compute build order starting from this target.
//  2- Build all targets in `build_order` until default target is reached


/******************************* TARGET STRUCT ********************************/
struct Target {
    /*---------------------------- PUBLIC MEMBERS ----------------------------*/
    const char *includes;          /* dirs                                    */
    const char *sources;           /* files, dirs, glob                       */
    const char *excludes;          /* files                                   */
    const char *base_dir;          /* dir                                     */
    /* Links are targets or libraries, are be built before. */
    const char *links;             /* libraries or targets                    */
    /* Dependencies are targets, are built before.*/
    const char *dependencies;      /* targets                                 */
    const char *flags;             /* passed as is to compiler                */

    const char *command_pre_build; /* command ran before building target      */
    const char *command_post_build;/* command ran after  building target      */
    const char *message_pre_build; /* message printed before building target  */
    const char *message_post_build;/* message printed after  building target  */

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
    *     .dependencies       = "mytarget1",                            /
    *     .links              = "lib1 lib2 mytarget2",                  /
    *     .kind               = MACE_LIBRARY,                           /
    * };                                                                /
    * NOTE: default separator is " ", set with 'mace_set_separator'     /
    *                                                                   /
    /*-----------------------------------------------------------------*/

    /*---------------------------- PRIVATE MEMBERS ---------------------------*/
    char *restrict _name;          /* target name set by user                 */
    uint64_t       _hash;          /* target name hash,                       */
    int            _order;         /* target order added by user              */

    /* --- Compilation --- */
    char **restrict _argv;         /* buffer for argv to exec build commands  */
    int             _argc;         /* number of arguments in argv             */
    int             _arg_len;      /* alloced len of argv                     */
    char **restrict _argv_includes;/* includes, in argv form                  */
    int             _argc_includes;/* number of arguments in argv_includes    */
    char **restrict _argv_links;   /* linked libraries, in argv form          */
    int             _argc_links;   /* number of arguments in argv_links       */
    char **restrict _argv_flags;   /* user flags, in argv form                */
    int             _argc_flags;   /* number of arguments in argv_flags       */
    char **restrict _argv_sources; /* sources, in argv form                   */
    int             _argc_sources; /* number of arguments in argv_sources     */
    int             _len_sources;  /* alloc len of arguments in argv_sources  */

    uint64_t *restrict _argv_objects_hash;/* sources, in argv form            */
    int      *restrict _argv_objects_cnt; /* sources, in argv form            */
    int             _argc_objects_hash;/* number of arguments in argv_sources */
    char **restrict _argv_objects;   /* sources, in argv form                 */

    /* -- Exclusions --  */
    uint64_t  *restrict _excludes;
    int _excludes_num;
    int _excludes_len;

    /* --- Dependencies ---  */
    uint64_t *restrict _deps_links;/* target or libs hashes                   */
    size_t     _deps_links_num;    /* target or libs hashes                   */
    size_t     _deps_links_len;    /* target or libs hashes                   */
    size_t     _d_cnt;             /* dependency count, for build order       */

    /* --- Recompile switched ---  */
    bool *_recompiles;
};
#endif /* MACE_CONVENIENCE_EXECUTABLE */

/********************************** STRUCTS *********************************/
struct Mace_Arguments {
    char        *user_target;
    char        *macefile;
    char        *dir;
    char        *cc;
    uint64_t     user_target_hash;
    uint64_t     skip;
    int          jobs;
    bool         debug         : 1;
    bool         silent        : 1;
    bool         skip_checksum : 1;
    bool         dry_run       : 1;
};

/********************************** CONSTANTS *********************************/
#define MACE_VER_PATCH 0
#define MACE_VER_MINOR 0
#define MACE_VER_MAJOR 0
#define MACE_VER_STRING "0.0.0"
#define MACE_USAGE_MIDCOLW 12
#ifndef MACE_CONVENIENCE_EXECUTABLE

enum MACE {
    MACE_DEFAULT_TARGET_LEN     =   8,
    MACE_MAX_COMMANDS           =   8,
    MACE_DEFAULT_OBJECT_LEN     =  16,
    MACE_DEFAULT_OBJECTS_LEN    = 128,
    MACE_CWD_BUFFERSIZE         = 128,
    SHA1_LEN                    =  20, /* [bytes] */
};

#define MACE_CLEAN "clean"
#define MACE_ALL "all"

enum MACE_RESERVED_TARGETS {
    MACE_ALL_ORDER              =  -1,
    MACE_CLEAN_ORDER            =  -2,
    MACE_NULL_ORDER             =  -3,
    MACE_RESERVED_TARGETS_NUM   =   2,
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
void mace_post_user(struct Mace_Arguments args);
void mace_exec_print(char *const arguments[], size_t argnum);

/* --- mace_checksum --- */
void mace_sha1cd(char *file, uint8_t hash2[SHA1_LEN]);
char *mace_checksum_filename(char *file);
bool mace_sha1cd_cmp(uint8_t hash1[SHA1_LEN], uint8_t hash2[SHA1_LEN]);

/* --- mace_hashing --- */
uint64_t mace_hash(const char *str);

/* --- mace_utils --- */
/* -- str -- */
char  *mace_str_copy(char *restrict buffer, const char *restrict str);

#endif /* MACE_CONVENIENCE_EXECUTABLE */
char  *mace_str_buffer(const char *const strlit);
/* -- argv -- */
char **mace_argv_flags(int *restrict len, int *restrict argc, char **restrict argv,
                       const char *restrict includes, const char *restrict flag, bool path);
#ifndef MACE_CONVENIENCE_EXECUTABLE

    char **mace_argv_grow(char **restrict argv, int *restrict argc, int *restrict arg_len);
    void   mace_argv_free(char **restrict argv, int argc);

    /* --- mace_setters --- */
    char *mace_set_obj_dir(char    *obj);
    char *mace_set_build_dir(char  *build);

    /* --- mace_Target --- */
    void mace_add_target(struct Target   *restrict target,  char *restrict name);

    /* -- Target OOP -- */
    void mace_Target_Free(struct Target                *target);
    bool mace_Target_hasDep(struct Target              *target, uint64_t hash);
    void mace_Target_compile(struct Target             *target);
    void mace_Target_Deps_Add(struct Target            *target, uint64_t hash);
    bool mace_Target_Checksum(struct Target            *target, char *s, char *o);
    void mace_Target_Free_argv(struct Target           *target);
    void mace_Target_Deps_Hash(struct Target           *target);
    void mace_Target_Deps_Grow(struct Target           *target);
    void mace_Target_argv_init(struct Target           *target);
    void mace_Target_argv_grow(struct Target           *target);
    bool mace_Target_Source_Add(struct Target *restrict target, char *restrict token);
    bool mace_Target_Object_Add(struct Target *restrict target, char *restrict token);
    void mace_Target_precompile(struct Target          *target);
    void mace_Target_Parse_User(struct Target          *target);
    void mace_Target_Parse_Source(struct Target        *target, char *path, char *src);
    void mace_Target_Free_notargv(struct Target        *target);
    void mace_Target_Free_excludes(struct Target       *target);
    bool mace_Target_Recompiles_Add(struct Target      *target, bool add);
    void mace_Target_argv_allatonce(struct Target      *target);
    void mace_Target_compile_allatonce(struct Target   *target);

    /* --- mace_glob --- */
    int     mace_globerr(const char *path, int eerrno);
    glob_t  mace_glob_sources(const char *path);

#endif /* MACE_CONVENIENCE_EXECUTABLE */
/* --- mace_exec --- */
pid_t mace_exec(const char *restrict exec, char *const arguments[]);
void  mace_wait_pid(int pid);
#ifndef MACE_CONVENIENCE_EXECUTABLE

/* --- mace_build --- */
/* -- linking -- */
void mace_link_static_library(char  *restrict target, char **restrict av_o, int ac_o);
void mace_link_dynamic_library(char *restrict target, char *restrict objects);
void mace_link_executable(char *restrict target, char **restrict av_o, int ac_o,
                          char **restrict av_l, int ac_l, char **restrict av_f, int ac_f);
/* --- mace_clean --- */
void mace_clean();
int mace_rmrf(char *path);
int mace_unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf);

/* -- compiling object files -> .o -- */
void mace_compile_glob(struct Target *restrict target, char *restrict globsrc,
                       const char *restrict flags);
void mace_build_targets();
void mace_run_commands(const char *commands);
void mace_print_message(const char *message);
void mace_clean();

/* -- build_order -- */
void mace_default_target_order();
void mace_user_target_order(uint64_t hash);
bool mace_in_build_order(size_t order, int *build_order, int num);

/* build order of all targets */
void mace_targets_build_order();
/* build order of target links */
void mace_deps_links_build_order(struct Target target, size_t *o_cnt);

/* --- mace_is --- */
int mace_isWildcard(const char *str);
int mace_isSource(const   char *path);
int mace_isObject(const   char *path);
int mace_isDir(const      char *path);

/* --- mace_filesystem --- */
void  mace_mkdir(const char     *path);
void  mace_object_path(char     *source);
char *mace_library_path(char    *target_name);

/* --- mace_pqueue --- */
void mace_pqueue_put(pid_t pid);
pid_t mace_pqueue_pop();

/********************************** GLOBALS ***********************************/
bool verbose = false;

/* --- Processes --- */
// Compile objects in parallel.
// Compile targets in series.
pid_t *pqueue   = NULL;
int pnum        = 0;
int plen        = -1;

#endif /* MACE_CONVENIENCE_EXECUTABLE */
/* -- separator -- */
char *mace_separator = " ";
char *mace_command_separator = "&&";
#ifndef MACE_CONVENIENCE_EXECUTABLE

    /* -- Compiler -- */
    char *cc      = NULL; // DESIGN QUESTION: Should I set a default?
    char *ar      = "ar";

    /* -- current working directory -- */
    char cwd[MACE_CWD_BUFFERSIZE];

    /* -- Reserved targets hashes -- */
    uint64_t mace_reserved_targets[MACE_RESERVED_TARGETS_NUM];
    uint64_t mace_default_target_hash = 0;
    int mace_default_target = MACE_ALL_ORDER;   /* order */
    int mace_user_target    = MACE_NULL_ORDER;  /* order */

    /* -- build order for user target -- */
    int *restrict build_order     = NULL;
    int  build_order_num          = 0;

    /* -- list of targets added by user -- */
    struct Target  *restrict targets     = NULL;   /* [order] is as added by user */
    size_t          target_num = 0;
    size_t          target_len = 0;

    /* -- buffer to write object -- */
    char           *restrict object      = NULL;
    size_t          object_len = 0;

    /* -- directories -- */
    char           *restrict obj_dir     = NULL;   /* intermediary .o files       */
    char           *restrict build_dir   = NULL;   /* linked libraries, execs     */

    /* -- mace_globals control -- */
    void mace_object_grow();
#endif /* MACE_CONVENIENCE_EXECUTABLE */

/***************************** SHA1DC DECLARATION *****************************/
#ifndef MACE_CONVENIENCE_EXECUTABLE

/***
* Copyright 2017 Marc Stevens <marc@marc-stevens.nl>, Dan Shumow <danshu@microsoft.com>
* Distributed under the MIT Software License.
* See accompanying file LICENSE.txt or copy at
* https://opensource.org/licenses/MIT
***/

#ifndef SHA1DC_SHA1_H
#define SHA1DC_SHA1_H

#ifndef SHA1DC_NO_STANDARD_INCLUDES
    #include <stdint.h>
#endif /* SHA1DC_NO_STANDARD_INCLUDES */

/* sha-1 compression function that takes an already expanded message, and additionally store intermediate states */
/* only stores states ii (the state between step ii-1 and step ii) when DOSTORESTATEii is defined in ubc_check.h */
void sha1_compression_states(uint32_t[5], const uint32_t[16], uint32_t[80], uint32_t[80][5]);

/*
// Function type for sha1_recompression_step_T (uint32_t ihvin[5], uint32_t ihvout[5], const uint32_t me2[80], const uint32_t state[5]).
// Where 0 <= T < 80
//       me2 is an expanded message (the expansion of an original message block XOR'ed with a disturbance vector's message block difference.)
//       state is the internal state (a,b,c,d,e) before step T of the SHA-1 compression function while processing the original message block.
// The function will return:
//       ihvin: The reconstructed input chaining value.
//       ihvout: The reconstructed output chaining value.
*/
typedef void(*sha1_recompression_type)(uint32_t *, uint32_t *, const uint32_t *, const uint32_t *);

/* A callback function type that can be set to be called when a collision block has been found: */
/* void collision_block_callback(uint64_t byteoffset, const uint32_t ihvin1[5], const uint32_t ihvin2[5], const uint32_t m1[80], const uint32_t m2[80]) */
typedef void(*collision_block_callback)(uint64_t, const uint32_t *, const uint32_t *,
                                        const uint32_t *, const uint32_t *);

/* The SHA-1 context. */
typedef struct {
    uint64_t total;
    uint32_t ihv[5];
    unsigned char buffer[64];
    int found_collision;
    int safe_hash;
    int detect_coll;
    int ubc_check;
    int reduced_round_coll;
    collision_block_callback callback;

    uint32_t ihv1[5];
    uint32_t ihv2[5];
    uint32_t m1[80];
    uint32_t m2[80];
    uint32_t states[80][5];
} SHA1_CTX;

/* Initialize SHA-1 context. */
void SHA1DCInit(SHA1_CTX *);

/*
    Function to enable safe SHA-1 hashing:
    Collision attacks are thwarted by hashing a detected near-collision block 3 times.
    Think of it as extending SHA-1 from 80-steps to 240-steps for such blocks:
        The best collision attacks against SHA-1 have complexity about 2^60,
        thus for 240-steps an immediate lower-bound for the best cryptanalytic attacks would be 2^180.
        An attacker would be better off using a generic birthday search of complexity 2^80.

   Enabling safe SHA-1 hashing will result in the correct SHA-1 hash for messages where no collision attack was detected,
   but it will result in a different SHA-1 hash for messages where a collision attack was detected.
   This will automatically invalidate SHA-1 based digital signature forgeries.
   Enabled by default.
*/
void SHA1DCSetSafeHash(SHA1_CTX *, int);

/*
    Function to disable or enable the use of Unavoidable Bitconditions (provides a significant speed up).
    Enabled by default
 */
void SHA1DCSetUseUBC(SHA1_CTX *, int);

/*
    Function to disable or enable the use of Collision Detection.
    Enabled by default.
 */
void SHA1DCSetUseDetectColl(SHA1_CTX *, int);

/* function to disable or enable the detection of reduced-round SHA-1 collisions */
/* disabled by default */
void SHA1DCSetDetectReducedRoundCollision(SHA1_CTX *, int);

/* function to set a callback function, pass NULL to disable */
/* by default no callback set */
void SHA1DCSetCallback(SHA1_CTX *, collision_block_callback);

/* update SHA-1 context with buffer contents */
void SHA1DCUpdate(SHA1_CTX *, const char *, size_t);

/* obtain SHA-1 hash from SHA-1 context */
/* returns: 0 = no collision detected, otherwise = collision found => warn user for active attack */
int  SHA1DCFinal(unsigned char[SHA1_LEN], SHA1_CTX *);

#ifdef SHA1DC_CUSTOM_TRAILING_INCLUDE_SHA1_H
    #include SHA1DC_CUSTOM_TRAILING_INCLUDE_SHA1_H
#endif /* SHA1DC_CUSTOM_TRAILING_INCLUDE_SHA1_H */

#endif /* SHA1DC_SHA1_H */
/***
* Copyright 2017 Marc Stevens <marc@marc-stevens.nl>, Dan Shumow <danshu@microsoft.com>
* Distributed under the MIT Software License.
* See accompanying file LICENSE.txt or copy at
* https://opensource.org/licenses/MIT
***/

/*
// this file was generated by the 'parse_bitrel' program in the tools section
// using the data files from directory 'tools/data/3565'
//
// sha1_dvs contains a list of SHA-1 Disturbance Vectors (DV) to check
// dvType, dvK and dvB define the DV: I(K,B) or II(K,B) (see the paper)
// dm[80] is the expanded message block XOR-difference defined by the DV
// testt is the step to do the recompression from for collision detection
// maski and maskb define the bit to check for each DV in the dvmask returned by ubc_check
//
// ubc_check takes as input an expanded message block and verifies the unavoidable bitconditions for all listed DVs
// it returns a dvmask where each bit belonging to a DV is set if all unavoidable bitconditions for that DV have been met
// thus one needs to do the recompression check for each DV that has its bit set
*/

#ifndef SHA1DC_UBC_CHECK_H
#define SHA1DC_UBC_CHECK_H

#ifndef SHA1DC_NO_STANDARD_INCLUDES
    #include <stdint.h>
#endif /* SHA1DC_NO_STANDARD_INCLUDES */

#define DVMASKSIZE 1
typedef struct {
    int dvType;
    int dvK;
    int dvB;
    int testt;
    int maski;
    int maskb;
    uint32_t dm[80];
} dv_info_t;
extern dv_info_t sha1_dvs[];
void ubc_check(const uint32_t W[80], uint32_t dvmask[DVMASKSIZE]);

#define DOSTORESTATE58
#define DOSTORESTATE65

#define CHECK_DVMASK(_DVMASK) (0 != _DVMASK[0])

#ifdef SHA1DC_CUSTOM_TRAILING_INCLUDE_UBC_CHECK_H
    #include SHA1DC_CUSTOM_TRAILING_INCLUDE_UBC_CHECK_H
#endif /* SHA1DC_CUSTOM_TRAILING_INCLUDE_UBC_CHECK_H */

#endif /* SHA1DC_UBC_CHECK_H */
#endif /* MACE_CONVENIENCE_EXECUTABLE */
/*************************** SHA1DC DECLARATION END ***************************/

/****************************** PARG DECLARATION ******************************/
/*
 * parg - parse argv
 *
 * Written in 2015-2016 by Joergen Ibsen
 * Modified in 2023 by Gabriel Taillon for SotA
 *
 * To the extent possible under law, the author(s) have dedicated all
 * copyright and related and neighboring rights to this software to the
 * public domain worldwide. This software is distributed without any
 * warranty. <http://creativecommons.org/publicdomain/zero/1.0/>
 */

#ifndef PARG_INCLUDED
#define PARG_INCLUDED

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PARG_VER_MAJOR 1
#define PARG_VER_MINOR 0
#define PARG_VER_PATCH 2
#define PARG_VER_STRING "1.0.2"

enum PARG_HAS_ARG {
    PARG_NOARG,
    PARG_REQARG, /* required */
    PARG_OPTARG  /* optional */
};

struct parg_state {
    const char *optarg;   /* option argument*/
    int optind;           /* next index */
    int optopt;           /* error option */
    const char *nextchar;
};
extern struct parg_state parg_state_default;

/* Long options for `parg_getopt_long()` */
struct parg_opt {
    const char *name;
    int has_arg;
    int *flag;
    int val;
    const char *arg;
    const char *doc;
};

/* - parg help - */
extern void mace_parg_usage(const char *n, const struct parg_opt *lo);

/* - option matching - */
extern int match_long(struct parg_state *ps, int c, char *const v[], const char *o,
                      const struct parg_opt *lo, int *li);
extern int match_short(struct parg_state *ps, int c, char *const v[], const char *os);

/* - utilities - */
extern void reverse(char *v[], int i, int j);
extern int is_argv_end(const struct parg_state *ps, int c, char *const v[]);

/* - argv reordering - */
extern int parg_reorder_simple(int c, char *v[], const char *os, const struct parg_opt *lo);
extern int parg_reorder(int c, char *v[], const char *os, const struct parg_opt *lo);

/* - parg public API: getopt and getopt_long - */
extern int parg_getopt(struct parg_state *ps, int c, char *const v[], const char *os);
extern int parg_getopt_long(struct parg_state *ps, int c, char *const v[],
                            const char *os, const struct parg_opt *lo, int *li);

#endif /* PARG_INCLUDED */
/**************************** PARG DECLARATION END ****************************/

/*----------------------------------------------------------------------------*/
/*                                MACE SOURCE                                 */
/*----------------------------------------------------------------------------*/
#ifndef MACE_CONVENIENCE_EXECUTABLE
#define vprintf(format, ...) do {\
        if (verbose)\
            printf(format, ##__VA_ARGS__);\
    } while(0)

/******************************* MACE_ADD_TARGET ******************************/
void mace_grow_targets() {
    target_len *= 2;
    targets     = realloc(targets,     target_len * sizeof(*targets));
    build_order = realloc(build_order, target_len * sizeof(*build_order));
}

void mace_add_target(struct Target *target, char *name) {
    targets[target_num]        = *target;
    size_t len = strlen(name);
    targets[target_num]._name  = name;
    uint64_t hash = mace_hash(name);
    for (int i = 0; i < MACE_RESERVED_TARGETS_NUM; i++) {
        if (hash == mace_reserved_targets[i]) {
            fprintf(stderr,  "Error: '%s' is a reserved target name.\n", name);
            exit(EPERM);
        }
    }
    targets[target_num]._hash  = hash;
    targets[target_num]._order = target_num;
    mace_Target_Deps_Hash(&targets[target_num]);
    mace_Target_Parse_User(&targets[target_num]);
    mace_Target_argv_init(&targets[target_num]);
    if (++target_num >= target_len) {
        mace_grow_targets();
    }
}

// To compile default target:
//  1- Compute build order starting from this target.
//  2- Build all targets in `build_order` until default target is reached
void mace_set_default_target(char *name) {
    /* User function */
    mace_default_target_hash = mace_hash(name);
}

void mace_default_target_order() {
    /* Called post-user to compute default target order from hash */
    if (mace_default_target_hash == 0)
        return;

    if (mace_user_target == MACE_CLEAN_ORDER)
        return;

    for (int i = 0; i < target_num; i++) {
        if (mace_default_target_hash == targets[i]._hash) {
            mace_default_target = i;
            return;
        }
    }

    fprintf(stderr, "Default target not found. Exiting");
    exit(EPERM);
}

void mace_user_target_order(uint64_t hash) {
    if (hash == 0)
        return;

    if (hash == mace_hash(MACE_CLEAN)) {
        mace_user_target = MACE_CLEAN_ORDER;
        return;
    }

    for (int i = 0; i < target_num; i++) {
        if (hash == targets[i]._hash) {
            mace_user_target = i;
            return;
        }
    }

    fprintf(stderr, "User target not found. Exiting");
    exit(EPERM);

}
#endif /* MACE_CONVENIENCE_EXECUTABLE */
/********************************* mace_hash **********************************/
uint64_t mace_hash(const char *str) {
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
#ifndef MACE_CONVENIENCE_EXECUTABLE


/****************************** MACE_SET_obj_dir ******************************/
// Sets where the object files will be placed during build.
char *mace_set_obj_dir(char *obj) {
    return (obj_dir = mace_str_copy(obj_dir, obj));
}

char *mace_set_build_dir(char *build) {
    return (build_dir = mace_str_copy(build_dir, build));
}

char *mace_str_copy(char *restrict buffer, const char *restrict str) {
    if (buffer != NULL)
        free(buffer);
    size_t len  = strlen(str);
    buffer      = calloc(len + 1, sizeof(*buffer));
    strncpy(buffer, str, len);
    return (buffer);
}

/************************************ argv ************************************/

#endif /* MACE_CONVENIENCE_EXECUTABLE */
void argv_free(int argc, char **argv) {
    if (argv == NULL)
        return;

    for (int i = 0; i < argc; i++) {
        free(argv[i]);
        argv[i] = NULL;
    }
    argv = NULL;
}

char **argv_grows(int *restrict len, int *restrict argc, char **restrict argv) {
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
            argv[i] = NULL;
        }
    }
    free(argv);
}

char **mace_argv_flags(int *restrict len, int *restrict argc, char **restrict argv,
                       const char *restrict user_str, const char *restrict flag, bool path) {
    assert(argc != NULL);
    assert(len != NULL);
    assert((*len) > 0);
    size_t flag_len = (flag == NULL) ? 0 : strlen(flag);

    /* -- Copy user_str into modifiable buffer -- */
    char *buffer = mace_str_buffer(user_str);
    char *sav = NULL;
    char *token = strtok_r(buffer, mace_separator, &sav);
    while (token != NULL) {
        argv = argv_grows(len, argc, argv);
        char *to_use = token;
        char *rpath = NULL;
        if (path) {
            /* - Expand path - */
            rpath = calloc(PATH_MAX, sizeof(*rpath));
            if (realpath(token, rpath) == NULL) {
                // TODO: remove those prints during tests.
                // printf("Warning! realpath error : %s '%s'\n", strerror(errno), token);
                to_use = token;
                free(rpath);
            } else {
                rpath = realloc(rpath, (strlen(rpath) + 1) * sizeof(*rpath));
                to_use = rpath;
            }
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

        token = strtok_r(NULL, mace_separator, &sav);
        free(rpath);
    }

    free(buffer);
    return (argv);
}
#ifndef MACE_CONVENIENCE_EXECUTABLE

void mace_Target_excludes(struct Target *target) {
    if (target->excludes == NULL)
        return;
    mace_Target_Free_excludes(target);

    target->_excludes_num = 0;
    target->_excludes_len = 8;
    target->_excludes     = calloc(target->_excludes_len, sizeof(*target->_excludes));

    /* -- Copy user_str into modifiable buffer -- */
    char *buffer = mace_str_buffer(target->excludes);
    /* --- Split sources into tokens --- */
    char *token = strtok(buffer, mace_separator);
    do {
        size_t token_len = strlen(token);
        char *arg = calloc(token_len + 1, sizeof(*arg));
        strncpy(arg, token, token_len);
        assert(arg != NULL);

        char *rpath = calloc(PATH_MAX, sizeof(*rpath));
        if (realpath(token, rpath) == NULL)
            printf("Warning! excluded source '%s' does not exist\n", arg);
        rpath = realloc(rpath, (strlen(rpath) + 1) * sizeof(*rpath));

        if (mace_isDir(rpath)) {
            fprintf(stderr, "Error dir '%s' in excludes: files only!\n", rpath);
        } else {
            target->_excludes[target->_excludes_num++] = mace_hash(rpath);
            free(rpath);
        }
        free(arg);
        token = strtok(NULL, mace_command_separator);
    } while (token != NULL);

    free(buffer);
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

    /* -- Exclusions -- */
    mace_Target_excludes(target);
}

void mace_Target_argv_grow(struct Target *target) {
    target->_argv = mace_argv_grow(target->_argv, &target->_argc, &target->_arg_len);
}

void mace_Target_sources_grow(struct Target *target) {
    size_t bytesize;

    /* -- Alloc sources -- */
    if (target->_argv_sources == NULL) {
        target->_len_sources  = 8;
        bytesize = target->_len_sources * sizeof(*target->_argv_sources);
        target->_argv_sources = malloc(bytesize);
    }

    /* -- Alloc recompiles -- */
    if (target->_recompiles == NULL) {
        bytesize = target->_len_sources * sizeof(*target->_recompiles);
        target->_recompiles = malloc(bytesize);
    }

    /* -- Alloc objects -- */
    if (target->_argv_objects == NULL) {
        bytesize = sizeof(*target->_argv_objects);
        target->_argv_objects       = calloc(target->_len_sources, bytesize);
    }
    if (target->_argv_objects_cnt == NULL) {
        bytesize = sizeof(*target->_argv_objects_cnt);
        target->_argv_objects_cnt   = calloc(target->_len_sources, bytesize);
    }
    if (target->_argv_objects_hash == NULL) {
        bytesize = sizeof(*target->_argv_objects_hash);
        target->_argv_objects_hash  = calloc(target->_len_sources, bytesize);
    }

    /* -- Realloc recompiles -- */
    if (target->_argc_sources >= target->_len_sources) {
        bytesize = target->_len_sources * 2 * sizeof(*target->_recompiles);
        target->_recompiles = realloc(target->_recompiles, bytesize);
    }
    /* -- Realloc objects -- */
    if (target->_argc_sources >= target->_len_sources) {
        bytesize = target->_len_sources * 2 * sizeof(*target->_argv_objects);
        target->_argv_objects = realloc(target->_argv_objects, bytesize);
    }

    /* -- Realloc sources -- */
    target->_argv_sources = argv_grows(&target->_len_sources, &target->_argc_sources,
                                       target->_argv_sources);

    /* -- Realloc objects -- */
    if (target->_len_sources >= target->_argc_objects_hash) {
        bytesize = target->_len_sources * sizeof(*target->_argv_objects_hash);
        target->_argv_objects_hash = realloc(target->_argv_objects_hash, bytesize);
        bytesize = target->_len_sources * sizeof(*target->_argv_objects_cnt);
        target->_argv_objects_cnt = realloc(target->_argv_objects_cnt, bytesize);
    }
}

char **mace_argv_grow(char **restrict argv, int *restrict argc, int *restrict arg_len) {
    if (*argc >= *arg_len) {
        (*arg_len) *= 2;
        size_t bytesize = *arg_len * sizeof(*argv);
        argv = realloc(argv, bytesize);
        memset(argv + (*arg_len) / 2, 0, bytesize / 2);
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
        fprintf(stderr, "Separator should not be NULL.\n");
        exit(EPERM);
    }
    if (strlen(sep) != 1) {
        fprintf(stderr, "Separator should have length one.\n");
        exit(EPERM);
    }
    mace_separator = sep;
}

/******************************** mace_pqueue *********************************/

pid_t mace_pqueue_pop() {
    assert(pnum > 0);
    return (pqueue[--pnum]);
}

void mace_pqueue_put(pid_t pid) {
    assert(pnum < plen);
    size_t bytes = plen * sizeof(*pqueue);
    memmove(pqueue + 1, pqueue, bytes);
    pqueue[0] = pid;
    pnum++;
}

// plen = 8
// pnum = 8
// POP -> --pnum evaluates to 7

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

#endif /* MACE_CONVENIENCE_EXECUTABLE */
/********************************* mace_exec **********************************/
// Execute command in forked process.
void mace_exec_print(char *const arguments[], size_t argnum) {
    for (int i = 0; i < argnum; i++) {
        printf("%s ", arguments[i]);
    }
    printf("\n");
}

pid_t mace_exec(const char *restrict exec, char *const arguments[]) {
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Error: forking issue.\n");
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
            /* pass */
        } else if (WIFEXITED(status) &&  WEXITSTATUS(status)) {
            if (WEXITSTATUS(status) == 127) {
                /* execvp failed */
                fprintf(stderr, "execvp failed.\n");
                exit(WEXITSTATUS(status));
            } else {
                fprintf(stderr, "Fork returned a non-zero status.\n");
                exit(WEXITSTATUS(status));
            }
        } else {
            fprintf(stderr, "Fork didn't terminate normally.\n");
            exit(WEXITSTATUS(status));
        }
    }
}
#ifndef MACE_CONVENIENCE_EXECUTABLE
/********************************* mace_build **********************************/
/* Build all sources from target to object */
void mace_link_static_library(char *restrict target, char **restrict argv_objects,
                              int argc_objects) {
    vprintf("Linking \t%s \n", target);
    int arg_len = 8;
    int argc = 0;
    char **argv         = calloc(arg_len, sizeof(*argv));

    argv[argc++] = ar;
    /* --- Adding -rcs flag --- */
    char *rcsflag       = calloc(5, sizeof(*rcsflag));
    strncpy(rcsflag, "-rcs", 4);
    int crcsflag = argc;
    argv[argc++] = rcsflag;

    /* --- Adding target --- */
    size_t target_len = strlen(target);
    char *targetv     = calloc(target_len + 1, sizeof(*rcsflag));
    strncpy(targetv, target, target_len);
    int targetc  = argc;
    argv[argc++] = targetv;

    /* --- Adding objects --- */
    if ((argc_objects > 0) && (argv_objects != NULL)) {
        for (int i = 0; i < argc_objects; i++) {
            argv = mace_argv_grow(argv, &argc, &arg_len);
            argv[argc++] = argv_objects[i] + strlen("-o");
        }
    }

    // mace_exec_print(argv, argc);
    pid_t pid = mace_exec(ar, argv);
    mace_wait_pid(pid);

    free(argv[crcsflag]);
    free(argv[targetc]);
    free(argv);
}


void mace_link_executable(char *restrict target, char **restrict argv_objects, int argc_objects,
                          char **restrict argv_links, int argc_links,
                          char **restrict argv_flags, int argc_flags) {
    vprintf("Linking \t%s \n", target);

    int arg_len = 16;
    int argc    = 0;
    char **argv = calloc(arg_len, sizeof(*argv));

    argv[argc++] = cc;
    /* --- Adding executable output --- */
    size_t target_len = strlen(target);
    char *oflag       = calloc(target_len + 3, sizeof(*oflag));
    strncpy(oflag, "-o", 2);
    strncpy(oflag + 2, target, target_len);
    int oflag_i = argc++;
    argv[oflag_i] = oflag;

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
    int ldirflag_i = argc++;
    argv[ldirflag_i] = ldirflag;

    // mace_exec_print(argv, argc);
    pid_t pid = mace_exec(cc, argv);
    mace_wait_pid(pid);

    free(argv[oflag_i]);
    free(argv[ldirflag_i]);
    free(argv);
}

void mace_link_dynamic_library(char *restrict target, char *restrict objects) {
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
    // mace_exec_print(target->_argv, target->_argc);
    pid_t pid = mace_exec(cc, target->_argv);
    mace_wait_pid(pid);

    /* -- Go back to cwd -- */
    assert(chdir(cwd) == 0);
}

void mace_Target_precompile(struct Target *target) {
    // printf("mace_Target_precompile\n");
    /* Compute latest object dependency */
    // TODO: only if source changed
    assert(target != NULL);
    assert(target->_argv != NULL);
    int argc = 0;
    target->_argv[target->_argc++] = "-MM";
    /* - Single source argv - */
    while (true) {
        /* - Skip if no recompiles - */
        if ((argc < target->_argc_sources) && (!target->_recompiles[argc])) {
            // printf("Pre-Compile SKIP %s\n", target->_argv_sources[argc]);
            argc++;
            continue;
        }
        /* - Add process to queue - */
        if (argc < target->_argc_sources) {
            printf("Pre-Compile %s\n", target->_argv_sources[argc]);
            target->_argv[MACE_ARGV_SOURCE] = target->_argv_sources[argc];
            target->_argv[MACE_ARGV_OBJECT] = target->_argv_objects[argc];
            size_t len = strlen(target->_argv[MACE_ARGV_OBJECT]);
            target->_argv[MACE_ARGV_OBJECT][len - 1] = 'd';

            // TODO: test with clang and tcc
            argc++;

            /* -- Actual pre-compilation -- */
            // mace_exec_print(target->_argv, target->_argc);
            pid_t pid = mace_exec(cc, target->_argv);
            mace_pqueue_put(pid);

            target->_argv[MACE_ARGV_OBJECT][len - 1] = 'o';
        }

        /* Prioritize adding process to queue */
        if ((argc < target->_argc_sources) && (pnum < plen))
            continue;

        /* Wait for process */
        if (pnum > 0) {
            pid_t wait = mace_pqueue_pop();
            mace_wait_pid(wait);
        }

        /* Check if more to compile */
        if ((pnum == 0) && (argc == target->_argc_sources))
            break;
    }
    target->_argv[--target->_argc] = NULL;
}

void mace_Target_compile(struct Target *target) {
    // TODO compile all objects
    // Compile latest object
    assert(target != NULL);
    assert(target->_argv != NULL);
    int argc = 0;
    bool queue_fulled = 0;
    /* - Single source argv - */
    while (true) {
        /* - Skip if no recompiles - */
        if ((argc < target->_argc_sources) && (!target->_recompiles[argc])) {
            // printf("Compile SKIP %s\n", target->_argv_sources[argc]);
            argc++;
            continue;
        }

        /* - Add process to queue - */
        if (argc < target->_argc_sources) {
            printf("Compile %s\n", target->_argv_sources[argc]);
            target->_argv[MACE_ARGV_SOURCE] = target->_argv_sources[argc];
            target->_argv[MACE_ARGV_OBJECT] = target->_argv_objects[argc];
            size_t len = strlen(target->_argv[MACE_ARGV_OBJECT]);
            argc++;

            /* -- Actual compilation -- */
            // mace_exec_print(target->_argv, target->_argc);
            pid_t pid = mace_exec(cc, target->_argv);
            mace_pqueue_put(pid);
        }

        /* Prioritize adding process to queue */
        if ((argc < target->_argc_sources) && (pnum < plen))
            continue;

        /* Wait for process */
        if (pnum > 0) {
            pid_t wait = mace_pqueue_pop();
            mace_wait_pid(wait);
        }

        /* Check if more to compile */
        if ((pnum == 0) && (argc == target->_argc_sources))
            break;
    }
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

bool mace_Target_Recompiles_Add(struct Target *target, bool add) {
    // printf("recompiles: %d\n", add);
    target->_recompiles[target->_argc_sources - 1] = add;
}


bool mace_Target_Object_Add(struct Target *restrict target, char *restrict token) {
    // printf("mace_Target_Object_Add\n");
    if (token == NULL)
        return (false);

    uint64_t hash = mace_hash(token);
    int hash_id = Target_hasObjectHash(target, hash);

    if (hash_id < 0) {
        Target_Object_Hash_Add(target, hash);
    } else {
        target->_argv_objects_cnt[hash_id]++;
        if (target->_argv_objects_cnt[hash_id] >= 10) {
            fprintf(stderr, "Too many same name sources/objects\n");
            exit(EPERM);
        }
    }

    /* -- Append object to arg -- */
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

    /* -- Actualling adding object here -- */
    // printf("Adding object: %s\n", arg);
    target->_argv_objects[target->_argc_sources - 1] = arg;

    // Does object file exist
    bool exists = access(arg + 2, F_OK) == 0;
    return (exists);
}

bool mace_Target_Checksum(struct Target *target, char *source_path, char *obj_path) {
    /* -- Checksum -- */
    /* - Compute current checksum - */
    uint8_t hash_current[SHA1_LEN];
    mace_sha1cd(source_path, hash_current);

    /* - Read existing checksum file - */
    assert(chdir(cwd) == 0);
    bool changed = true; // set to false only if checksum file exists, changed
    uint8_t hash_previous[SHA1_LEN] = {0};
    char *checksum_path = mace_checksum_filename(obj_path);
    FILE *fd = fopen(checksum_path, "r");
    if (fd != NULL) {
        fseek(fd, 0, SEEK_SET);
        size_t size = fread(hash_previous, 1, SHA1_LEN, fd);
        if (size != SHA1_LEN) {
            fprintf(stderr, "Could not read checksum from '%s'. Deleting. \n", checksum_path);
            fclose(fd);
            remove(checksum_path);
            exit(EIO);
        }
        changed = !mace_sha1cd_cmp(hash_previous, hash_current);
        fclose(fd);
    }

    /* - Write checksum file, if changed or didn't exist - */
    if (changed) {
        fd = fopen(checksum_path, "w");
        if (fd == NULL) {
            fprintf(stderr, "Error:%d %s\n", errno, strerror(errno));
            exit(EPERM);
        }
        fwrite(hash_current, 1, SHA1_LEN, fd); // SHA1_LEN
        fclose(fd);
    }
    free(checksum_path);

    if (target->base_dir != NULL)
        assert(chdir(target->base_dir) == 0);

    return (changed);
}

bool mace_Target_Source_Add(struct Target *restrict target, char *restrict token) {
    if (token == NULL)
        return (true);

    mace_Target_sources_grow(target);

    size_t token_len = strlen(token);
    char *arg = calloc(token_len + 1, sizeof(*arg));
    strncpy(arg, token, token_len);
    assert(arg != NULL);

    /* - Expand path - */
    char *rpath = calloc(PATH_MAX, sizeof(*rpath));
    if (realpath(arg, rpath) == NULL) {
        printf("Warning! realpath error : %s '%s'\n", strerror(errno), arg);
        free(rpath);
        rpath = arg;
    } else {
        rpath = realloc(rpath, (strlen(rpath) + 1) * sizeof(*rpath));
        free(arg);
    }

    /* - Check if file is excluded - */
    uint64_t rpath_hash = mace_hash(rpath);
    for (int i = 0; i < target->_excludes_num; i++) {
        if (target->_excludes[i] == rpath_hash)
            return (true);
    }

    /* -- Actually adding source here -- */
    // printf("Adding source: %s\n", rpath);
    target->_argv_sources[target->_argc_sources++] = rpath;

    return (false);
}

void mace_Target_Parse_Source(struct Target *restrict target, char *path, char *src) {
    bool excluded = mace_Target_Source_Add(target, path);
    if (!excluded) {
        mace_object_path(src);
        bool exists  = mace_Target_Object_Add(target, object);
        size_t i = target->_argc_sources - 1;
        bool changed = mace_Target_Checksum(target, target->_argv_sources[i], target->_argv_objects[i]);
        mace_Target_Recompiles_Add(target, !excluded && (changed || !exists));
    }
}


/* Compile globbed files to objects */
void mace_compile_glob(struct Target *restrict target, char *restrict globsrc,
                       const char *restrict flags) {
    glob_t globbed = mace_glob_sources(globsrc);
    for (int i = 0; i < globbed.gl_pathc; i++) {
        assert(mace_isSource(globbed.gl_pathv[i]));
        char *pos = strrchr(globbed.gl_pathv[i], '/');
        char *source_file = (pos == NULL) ? globbed.gl_pathv[i] : pos + 1;
        /* - Compute source and object filenames - */
        mace_Target_Parse_Source(target, globbed.gl_pathv[i], source_file);
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
        mkdir(path, 0777);
        chmod(path, 0777);
    }
}

char *mace_executable_path(char *target_name) {
    assert(target_name != NULL);
    size_t bld_len = strlen(build_dir);
    size_t tar_len = strlen(target_name);

    char *exec = calloc((bld_len + tar_len + 2), sizeof(*exec));
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

    char *lib = calloc((bld_len + tar_len + 7), sizeof(*lib));
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
    char *path = calloc(cwd_len + obj_dir_len + 2, sizeof(*path));
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
#endif /* MACE_CONVENIENCE_EXECUTABLE */
char *mace_str_buffer(const char *strlit) {
    size_t  litlen  = strlen(strlit);
    char   *buffer  = calloc(litlen + 1, sizeof(*buffer));
    strncpy(buffer, strlit, litlen);
    return (buffer);
}
#ifndef MACE_CONVENIENCE_EXECUTABLE

void mace_print_message(const char *message) {
    if (message == NULL)
        return;

    printf("%s\n", message);
}
/********************************* mace_clean *********************************/
int mace_unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    int rv = remove(fpath);

    if (rv)
        fprintf(stderr, "Could not remove '%s'.\n", fpath);

    return rv;
}

int mace_rmrf(char *path) {
    return nftw(path, mace_unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

void mace_clean() {
    printf("Cleaning\n");
    printf("Removing '%s'\n", obj_dir);
    mace_rmrf(obj_dir);
    printf("Removing '%s'\n", build_dir);
    mace_rmrf(build_dir);
}


/******************************** mace_build **********************************/
void mace_run_commands(const char *commands) {
    if (commands == NULL)
        return;

    assert(chdir(cwd) == 0);
    int argc = 0, len = 8, bytesize;
    char **argv = calloc(len, sizeof(*argv));

    /* -- Copy sources into modifiable buffer -- */
    char *buffer = mace_str_buffer(commands);

    /* --- Split sources into tokens --- */
    char *token = strtok(buffer, mace_command_separator);

    do {
        for (int i = 0; i < argc; i++) {
            if (argv[i] != NULL) {
                free(argv[i]);
                argv[i] = NULL;
            }
        }

        argc = 0;
        argv = mace_argv_flags(&len, &argc, argv, token, NULL, false);

        // mace_exec_print(argv, argc);
        pid_t pid = mace_exec(argv[0], argv);
        mace_wait_pid(pid);

        token = strtok(NULL, mace_command_separator);
    } while (token != NULL);

    free(argv);
    free(buffer);
}

void mace_build_target(struct Target *target) {
    /* --- Move to target base_dir, compile there --- */
    printf("Build target %s\n", target->_name);
    if (target->base_dir != NULL)
        assert(chdir(target->base_dir) == 0);

    /* --- Parse sources, put into array --- */
    assert(target->kind != 0);
    /* --- Compile sources --- */
    /* --- Preliminaries --- */
    mace_Target_Free_notargv(target);

    /* -- Copy sources into modifiable buffer -- */
    char *buffer = mace_str_buffer(target->sources);

    /* --- Parse sources --- */
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
            mace_Target_Parse_Source(target, token, token);

        } else {
            printf("Error: source is neither a .c file, a folder, nor has a wildcard in it\n");
            exit(ENOENT);
        }

        token = strtok(NULL, mace_separator);
    } while (token != NULL);

    /* --- Compile now. --- */
    mace_Target_precompile(target);
    mace_Target_compile(target); // faster than make with no pre-compile

    // /* -- allatonce -- */
    // if (target->allatonce)
    //     mace_Target_compile_allatonce(target);

    /* --- Move back to cwd to link --- */
    assert(chdir(cwd) == 0);

    /* --- Linking --- */
    if (target->kind == MACE_STATIC_LIBRARY) {
        char *lib = mace_library_path(target->_name);
        mace_link_static_library(lib, target->_argv_objects, target->_argc_sources);
        free(lib);
    } else if (target->kind == MACE_EXECUTABLE) {
        char *exec = mace_executable_path(target->_name);
        mace_link_executable(exec, target->_argv_objects, target->_argc_sources, target->_argv_links,
                             target->_argc_links, target->_argv_flags, target->_argc_flags);
        free(exec);
    }
    assert(chdir(cwd) == 0);
    free(buffer);
}

bool mace_in_build_order(size_t order, int *b_order, int num) {
    bool out = false;
    assert(b_order != NULL);
    for (int i = 0; i < num; i++) {
        if (b_order[i] == order) {
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
    if (mace_in_build_order(order, build_order, build_order_num)) {
        fprintf(stderr, "Target ID is already in build_order. Exiting.");
        exit(EPERM);
    }
    build_order[build_order_num++] = order;
}

/* - Depth first search through depencies - */
// Builds all target dependencies before building target
void mace_deps_links_build_order(struct Target target, size_t *restrict o_cnt) {
    /* o_cnt should never be geq to target_num */
    if ((*o_cnt) >= target_num)
        return;

    size_t order = mace_target_order(target); // target order
    /* Target already in build order, skip */
    if (mace_in_build_order(order, build_order, build_order_num))
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
        mace_deps_links_build_order(targets[next_target_order], o_cnt);
    }

    /* Target already in build order, skip */
    if (mace_in_build_order(order, build_order, build_order_num))
        return;

    /* All dependencies of target were built, add it to build order */
    if (target._d_cnt != target._deps_links_num) {
        printf("Error: Not all target dependencies before target in build order.");
        exit(EPERM);
    }

    mace_build_order_add(order);
    return;
}

bool mace_Target_hasDep(struct Target *target, uint64_t hash) {
    if (target->_deps_links == NULL)
        return (false);

    for (int i = 0; i < target->_deps_links_num; i++) {
        if (target->_deps_links[i] == hash)
            return (true);
    }
    return (false);
}

bool mace_circular_deps(struct Target *targs, size_t len) {
    /* -- Circular dependency conditions -- */
    /*   1- Target i has j dependency       */
    /*   2- Target j has i dependency       */
    for (int i = 0; i < target_num; i++) {
        uint64_t hash_i = targs[i]._hash;
        /* 1- going through target i's dependencies */
        for (int z = 0; z < targs[i]._deps_links_num; z++) {
            int j = mace_hash_order(targs[i]._deps_links[z]);

            /* Dependency is not in list of targets */
            if (j < 0)
                continue;

            /* Dependency is self */
            if (i == j) {
                printf("Warning: Target '%s' depends on itself.\n", targs[i]._name);
                continue;
            }

            /* Dependency is other target */
            if (mace_Target_hasDep(&targs[j], hash_i))
                /* 2- target i's dependency j has i as dependency as well */
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
    /* If user_target is clean, no build order */
    if (mace_user_target == MACE_CLEAN_ORDER)
        return;

    /* If user_target not set and default taregt is clean, no build order */
    if ((mace_user_target == MACE_NULL_ORDER) && (mace_default_target == MACE_CLEAN_ORDER))
        return;

    // If user_target is not all, or default_target is not all
    //  - Build only specified target
    if ((mace_user_target > MACE_ALL_ORDER) || (mace_default_target > MACE_ALL_ORDER)) {
        /* Build dependencies of default target, and itself only */
        o_cnt = mace_user_target > MACE_ALL_ORDER ? mace_user_target : mace_default_target;
        mace_deps_links_build_order(targets[o_cnt], &o_cnt);

        // int user_order = mace_target_order(targets[o_cnt]);
        // if (mace_in_build_order(user_order, build_order, build_order_num)) {

        // }
        return;
    }
    // If user_target is all, or default_target is all and no user_target
    bool cond;
    cond = (mace_user_target == MACE_NULL_ORDER) && (mace_default_target == MACE_ALL_ORDER);
    cond |= (mace_user_target == MACE_ALL_ORDER);
    assert(cond);

    o_cnt = 0;
    /* Visit all targets */
    while (o_cnt < target_num) {
        mace_deps_links_build_order(targets[o_cnt], &o_cnt);
        o_cnt++;
    }
}

void mace_build_targets() {
    int z = 0;
    for (z = 0; z < build_order_num; z++) {
        mace_run_commands(targets[build_order[z]].command_pre_build);
        mace_print_message(targets[build_order[z]].message_pre_build);
        mace_build_target(&targets[build_order[z]]);
        mace_print_message(targets[build_order[z]].message_post_build);
        mace_run_commands(targets[build_order[z]].command_post_build);
    }
}

void mace_Target_Free(struct Target *target) {
    mace_Target_Free_argv(target);
    mace_Target_Free_notargv(target);
    mace_Target_Free_excludes(target);
}

void mace_Target_Free_excludes(struct Target *target) {
    if (target->_excludes != NULL) {
        free(target->_excludes);
        target->_excludes = NULL;
    }
}

void mace_Target_Free_notargv(struct Target *target) {
    if (target->_deps_links != NULL) {
        free(target->_deps_links);
        target->_deps_links = NULL;
    }
    if (target->_recompiles != NULL) {
        free(target->_recompiles);
        target->_recompiles = NULL;
    }

}

void mace_Target_Free_argv(struct Target *target) {
    mace_argv_free(target->_argv_includes, target->_argc_includes);
    target->_argv_includes  = NULL;
    target->_argc_includes  = 0;
    mace_argv_free(target->_argv_links, target->_argc_links);
    target->_argv_links     = NULL;
    target->_argc_links     = 0;
    mace_argv_free(target->_argv_flags, target->_argc_flags);
    target->_argv_flags     = NULL;
    target->_argc_flags     = 0;
    mace_argv_free(target->_argv_sources, target->_argc_sources);
    target->_argv_sources   = NULL;
    mace_argv_free(target->_argv_objects, target->_argc_sources);
    target->_argv_objects   = NULL;
    target->_argc_sources   = 0;
    free(target->_argv_objects_cnt);
    target->_argv_objects_cnt   = NULL;
    free(target->_argv_objects_hash);
    target->_argv_objects_hash  = NULL;
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

void mace_post_user(struct Mace_Arguments args) {
    //   1- Moves to user set dir if not NULL,
    //   2- Checks set compiler,
    //   3- Checks that at least one target exists,
    //   4- Checks that there are no circular dependency.
    //   5- Compute user_target order.
    //   6- Computes default target order from default target_hash.
    //   7- Alloc queue for processes.
    // If not exit with error.

    if (args.dir != NULL) {
        assert(chdir(args.dir) == 0);
    }

    /* Check that compiler is set */
    if (cc == NULL) {
        fprintf(stderr, "Compiler not set. Exiting.\n");
        exit(ENXIO);
    }

    /* Check that a target exists */
    if ((targets == NULL) || (target_num <= 0)) {
        fprintf(stderr, "No targets to compile. Exiting.\n");
        exit(ENXIO);
    }

    /* Check for circular dependency */
    if (mace_circular_deps(targets, target_num)) {
        fprintf(stderr, "Circular dependency in linked library detected. Exiting\n");
        exit(ENXIO);
    }

    /* Check which target user wants to compile */
    mace_user_target_order(args.user_target_hash);
    mace_default_target_order();

    /* Process queue alloc */
    assert(args.jobs >= 1);
    plen = args.jobs;
    pqueue = calloc(plen, sizeof(*pqueue));

}


void mace_init() {
    mace_free();
    if (getcwd(cwd, MACE_CWD_BUFFERSIZE) == NULL) {
        fprintf(stderr, "getcwd() error\n");
        exit(errno);
    }

    /* --- Reserved target names --- */
    int i = MACE_CLEAN_ORDER + MACE_RESERVED_TARGETS_NUM;
    mace_reserved_targets[i]    = mace_hash(MACE_CLEAN);
    i = MACE_ALL_ORDER + MACE_RESERVED_TARGETS_NUM;
    mace_reserved_targets[i]    = mace_hash(MACE_ALL);

    /* --- Memory allocation --- */
    target_len      = MACE_DEFAULT_TARGET_LEN;
    object_len      = MACE_DEFAULT_OBJECT_LEN;
    build_order_num = 0;

    object      = calloc(object_len, sizeof(*object));
    targets     = calloc(target_len, sizeof(*targets));
    build_order = calloc(target_len, sizeof(*build_order));

    /* --- Default output folders --- */
    mace_set_build_dir("build/");
    mace_set_obj_dir("obj/");
}

void mace_free() {
    for (int i = 0; i < target_num; i++) {
        mace_Target_Free(&targets[i]);
    }
    if (pqueue != NULL) {
        free(pqueue);
        pqueue = NULL;
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

    target_num              = 0;
    object_len              = 0;
    build_order_num         = 0;
}

void mace_Target_Deps_Grow(struct Target *target) {
    if (target->_deps_links_len <= target->_deps_links_num) {
        target->_deps_links_len *= 2;
        target->_deps_links = realloc(target->_deps_links,
                                      target->_deps_links_len * sizeof(*target->_deps_links));
    }
}

void mace_Target_Deps_Add(struct Target *target, uint64_t hash) {
    if (!mace_Target_hasDep(target, hash)) {
        mace_Target_Deps_Grow(target);
        target->_deps_links[target->_deps_links_num++] = hash;
    }
}

void mace_Target_Deps_Hash(struct Target *target) {
    /* --- Preliminaries --- */
    if ((target->links == NULL) && (target->dependencies == NULL))
        return;

    /* --- Alloc space for deps --- */
    target->_deps_links_num =  0;
    target->_deps_links_len = 16;
    if (target->_deps_links != NULL) {
        free(target->_deps_links);
    }
    target->_deps_links = malloc(target->_deps_links_len * sizeof(*target->_deps_links));

    /* --- Add links to _deps_links --- */
    do {
        if (target->links == NULL)
            break;

        /* --- Copy links into modifiable buffer --- */
        char *buffer = mace_str_buffer(target->links);

        /* --- Split links into tokens, --- */
        char *token = strtok(buffer, " ");

        /* --- Hash tokens into _deps_links --- */
        do {
            mace_Target_Deps_Add(target, mace_hash(token));
            token = strtok(NULL, " ");
        } while (token != NULL);
        free(buffer);
    } while (false);

    /* --- Add dependencies to _deps_links --- */
    do {
        if (target->dependencies == NULL)
            break;

        /* --- Copy links into modifiable buffer --- */
        char *buffer = mace_str_buffer(target->dependencies);

        /* --- Split links into tokens, --- */
        char *token = strtok(buffer, " ");

        /* --- Hash tokens into _deps_links --- */
        do {
            mace_Target_Deps_Add(target, mace_hash(token));
            token = strtok(NULL, " ");
        } while (token != NULL);
        free(buffer);
    } while (false);
}

/********************************** checksums *********************************/
char *mace_checksum_filename(char *file) {
    // Files should be .c or .h
    size_t path_len  = strlen(file);
    char *dot   = strchr(file, '.'); // last dot in path
    char *slash = strrchr(file, '/'); // last slash in path
    if (dot == NULL) {
        fprintf(stderr, "Could not find extension in filename");
        exit(EPERM);
    }

    int dot_i   = (int)(dot   - file);
    int slash_i = (slash == NULL) ? 0 : (int)(slash - file + 1);
    size_t obj_dir_len  = strlen(obj_dir);
    size_t file_len  = dot_i - slash_i;

    size_t checksum_len  = (file_len + 6) + obj_dir_len + 1;

    char *sha1   = calloc(checksum_len, sizeof(*sha1));
    strncpy(sha1, obj_dir, obj_dir_len);
    size_t total = obj_dir_len;
    strncpy(sha1 + total, "/", 1);
    total += 1;
    strncpy(sha1 + total, file + slash_i, file_len);
    total += file_len;
    strncpy(sha1 + total, ".sha1", 5);
    return (sha1);
}

inline bool mace_sha1cd_cmp(uint8_t hash1[SHA1_LEN], uint8_t hash2[SHA1_LEN]) {
    return (memcmp(hash1, hash2, SHA1_LEN) == 0);
}

void mace_sha1cd(char *file, uint8_t hash[SHA1_LEN]) {
    assert(file != NULL);
    size_t size;
    int i, j, foundcollision;

    /* - open file - */
    FILE *fd = fopen(file, "rb");
    if (fd == NULL) {
        fprintf(stderr, "cannot open file: %s: %s\n", file, strerror(errno));
        exit(EPERM);
    }

    /* - compute checksum - */
    SHA1_CTX ctx2;
    SHA1DCInit(&ctx2);
    char buffer[USHRT_MAX + 1];
    while (true) {
        size = fread(buffer, 1, (USHRT_MAX + 1), fd);
        SHA1DCUpdate(&ctx2, buffer, (unsigned)(size));
        if (size != (USHRT_MAX + 1))
            break;
    }
    if (ferror(fd)) {
        fprintf(stderr, " file read error: %s: %s\n", file, strerror(errno));
        exit(EPERM);
    }
    if (!feof(fd)) {
        fprintf(stderr, "not end of file?: %s: %s\n", file, strerror(errno));
        exit(EPERM);
    }

    /* - check for collision - */
    foundcollision = SHA1DCFinal(hash, &ctx2);

    if (foundcollision) {
        fprintf(stderr, "sha1dc: collision detected");
        exit(EPERM);
    }

    fclose(fd);
}
#endif /* MACE_CONVENIENCE_EXECUTABLE */
/*----------------------------------------------------------------------------*/
/*                               MACE SOURCE END                              */
/*----------------------------------------------------------------------------*/

/******************************** SHA1DC SOURCE *******************************/
#ifndef MACE_CONVENIENCE_EXECUTABLE

/***
* Copyright 2017 Marc Stevens <marc@marc-stevens.nl>, Dan Shumow (danshu@microsoft.com)
* Distributed under the MIT Software License.
* See accompanying file LICENSE.txt or copy at
* https://opensource.org/licenses/MIT
***/

#ifndef SHA1DC_NO_STANDARD_INCLUDES
    #include <string.h>
    #include <memory.h>
    #include <stdio.h>
    #include <stdlib.h>
    #ifdef __unix__
        #include <sys/types.h> /* make sure macros like _BIG_ENDIAN visible */
    #endif /* __unix__ */
#endif /* SHA1DC_NO_STANDARD_INCLUDES */

#ifdef SHA1DC_CUSTOM_INCLUDE_SHA1_C
    #include SHA1DC_CUSTOM_INCLUDE_SHA1_C
#endif /* SHA1DC_CUSTOM_INCLUDE_SHA1_C */

#ifndef SHA1DC_INIT_SAFE_HASH_DEFAULT
    #define SHA1DC_INIT_SAFE_HASH_DEFAULT 1
#endif /* SHA1DC_INIT_SAFE_HASH_DEFAULT */

#if (defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || \
defined(i386) || defined(__i386) || defined(__i386__) || defined(__i486__)  || \
defined(__i586__) || defined(__i686__) || defined(_M_IX86) || defined(__X86__) || \
defined(_X86_) || defined(__THW_INTEL__) || defined(__I86__) || defined(__INTEL__) || \
defined(__386) || defined(_M_X64) || defined(_M_AMD64))
#define SHA1DC_ON_INTEL_LIKE_PROCESSOR
#endif

/*
   Because Little-Endian architectures are most common,
   we only set SHA1DC_BIGENDIAN if one of these conditions is met.
   Note that all MSFT platforms are little endian,
   so none of these will be defined under the MSC compiler.
   If you are compiling on a big endian platform and your compiler does not define one of these,
   you will have to add whatever macros your tool chain defines to indicate Big-Endianness.
 */

#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)
/*
 * Should detect Big Endian under GCC since at least 4.6.0 (gcc svn
 * rev #165881). See
 * https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
 *
 * This also works under clang since 3.2, it copied the GCC-ism. See
 * clang.git's 3b198a97d2 ("Preprocessor: add __BYTE_ORDER__
 * predefined macro", 2012-07-27)
 */
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define SHA1DC_BIGENDIAN
#endif

/* Not under GCC-alike */
#elif defined(__BYTE_ORDER) && defined(__BIG_ENDIAN)
/*
 * Should detect Big Endian under glibc.git since 14245eb70e ("entered
 * into RCS", 1992-11-25). Defined in <endian.h> which will have been
 * brought in by standard headers. See glibc.git and
 * https://sourceforge.net/p/predef/wiki/Endianness/
 */
#if __BYTE_ORDER == __BIG_ENDIAN
    #define SHA1DC_BIGENDIAN
#endif

/* Not under GCC-alike or glibc */
#elif defined(_BYTE_ORDER) && defined(_BIG_ENDIAN) && defined(_LITTLE_ENDIAN)
/*
 * *BSD and newlib (embedded linux, cygwin, etc).
 * the defined(_BIG_ENDIAN) && defined(_LITTLE_ENDIAN) part prevents
 * this condition from matching with Solaris/sparc.
 * (Solaris defines only one endian macro)
 */
#if _BYTE_ORDER == _BIG_ENDIAN
    #define SHA1DC_BIGENDIAN
#endif

/* Not under GCC-alike or glibc or *BSD or newlib */
#elif (defined(__ARMEB__) || defined(__THUMBEB__) || defined(__AARCH64EB__) || \
defined(__MIPSEB__) || defined(__MIPSEB) || defined(_MIPSEB) || \
defined(__sparc))
/*
 * Should define Big Endian for a whitelist of known processors. See
 * https://sourceforge.net/p/predef/wiki/Endianness/ and
 * http://www.oracle.com/technetwork/server-storage/solaris/portingtosolaris-138514.html
 */
#define SHA1DC_BIGENDIAN

/* Not under GCC-alike or glibc or *BSD or newlib or <processor whitelist> */
#elif (defined(_AIX) || defined(__hpux))

/*
 * Defines Big Endian on a whitelist of OSs that are known to be Big
 * Endian-only. See
 * https://public-inbox.org/git/93056823-2740-d072-1ebd-46b440b33d7e@felt.demon.nl/
 */
#define SHA1DC_BIGENDIAN

/* Not under GCC-alike or glibc or *BSD or newlib or <processor whitelist> or <os whitelist> */
#elif defined(SHA1DC_ON_INTEL_LIKE_PROCESSOR)
/*
 * As a last resort before we do anything else we're not 100% sure
 * about below, we blacklist specific processors here. We could add
 * more, see e.g. https://wiki.debian.org/ArchitectureSpecificsMemo
 */
#else /* Not under GCC-alike or glibc or *BSD or newlib or <processor whitelist> or <os whitelist> or <processor blacklist> */

/* We do nothing more here for now */
/*#error "Uncomment this to see if you fall through all the detection"*/

#endif /* Big Endian detection */

#if (defined(SHA1DC_FORCE_LITTLEENDIAN) && defined(SHA1DC_BIGENDIAN))
    #undef SHA1DC_BIGENDIAN
#endif
#if (defined(SHA1DC_FORCE_BIGENDIAN) && !defined(SHA1DC_BIGENDIAN))
    #define SHA1DC_BIGENDIAN
#endif
/*ENDIANNESS SELECTION*/

#ifndef SHA1DC_FORCE_ALIGNED_ACCESS
    #if defined(SHA1DC_FORCE_UNALIGNED_ACCESS) || defined(SHA1DC_ON_INTEL_LIKE_PROCESSOR)
        #define SHA1DC_ALLOW_UNALIGNED_ACCESS
    #endif /*UNALIGNED ACCESS DETECTION*/
#endif /*FORCE ALIGNED ACCESS*/

#define rotate_right(x,n) (((x)>>(n))|((x)<<(32-(n))))
#define rotate_left(x,n)  (((x)<<(n))|((x)>>(32-(n))))

#define sha1_bswap32(x) \
    {x = ((x << 8) & 0xFF00FF00) | ((x >> 8) & 0xFF00FF); x = (x << 16) | (x >> 16);}

#define sha1_mix(W, t)  (rotate_left(W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16], 1))

#ifdef SHA1DC_BIGENDIAN
    #define sha1_load(m, t, temp)  { temp = m[t]; }
#else
    #define sha1_load(m, t, temp)  { temp = m[t]; sha1_bswap32(temp); }
#endif

#define sha1_store(W, t, x) *(volatile uint32_t *)&W[t] = x

#define sha1_f1(b,c,d) ((d)^((b)&((c)^(d))))
#define sha1_f2(b,c,d) ((b)^(c)^(d))
#define sha1_f3(b,c,d) (((b)&(c))+((d)&((b)^(c))))
#define sha1_f4(b,c,d) ((b)^(c)^(d))

#define HASHCLASH_SHA1COMPRESS_ROUND1_STEP(a, b, c, d, e, m, t) \
    { e += rotate_left(a, 5) + sha1_f1(b,c,d) + 0x5A827999 + m[t]; b = rotate_left(b, 30); }
#define HASHCLASH_SHA1COMPRESS_ROUND2_STEP(a, b, c, d, e, m, t) \
    { e += rotate_left(a, 5) + sha1_f2(b,c,d) + 0x6ED9EBA1 + m[t]; b = rotate_left(b, 30); }
#define HASHCLASH_SHA1COMPRESS_ROUND3_STEP(a, b, c, d, e, m, t) \
    { e += rotate_left(a, 5) + sha1_f3(b,c,d) + 0x8F1BBCDC + m[t]; b = rotate_left(b, 30); }
#define HASHCLASH_SHA1COMPRESS_ROUND4_STEP(a, b, c, d, e, m, t) \
    { e += rotate_left(a, 5) + sha1_f4(b,c,d) + 0xCA62C1D6 + m[t]; b = rotate_left(b, 30); }

#define HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(a, b, c, d, e, m, t) \
    { b = rotate_right(b, 30); e -= rotate_left(a, 5) + sha1_f1(b,c,d) + 0x5A827999 + m[t]; }
#define HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(a, b, c, d, e, m, t) \
    { b = rotate_right(b, 30); e -= rotate_left(a, 5) + sha1_f2(b,c,d) + 0x6ED9EBA1 + m[t]; }
#define HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(a, b, c, d, e, m, t) \
    { b = rotate_right(b, 30); e -= rotate_left(a, 5) + sha1_f3(b,c,d) + 0x8F1BBCDC + m[t]; }
#define HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(a, b, c, d, e, m, t) \
    { b = rotate_right(b, 30); e -= rotate_left(a, 5) + sha1_f4(b,c,d) + 0xCA62C1D6 + m[t]; }

#define SHA1COMPRESS_FULL_ROUND1_STEP_LOAD(a, b, c, d, e, m, W, t, temp) \
    {sha1_load(m, t, temp); sha1_store(W, t, temp); e += temp + rotate_left(a, 5) + sha1_f1(b,c,d) + 0x5A827999; b = rotate_left(b, 30);}

#define SHA1COMPRESS_FULL_ROUND1_STEP_EXPAND(a, b, c, d, e, W, t, temp) \
    {temp = sha1_mix(W, t); sha1_store(W, t, temp); e += temp + rotate_left(a, 5) + sha1_f1(b,c,d) + 0x5A827999; b = rotate_left(b, 30); }

#define SHA1COMPRESS_FULL_ROUND2_STEP(a, b, c, d, e, W, t, temp) \
    {temp = sha1_mix(W, t); sha1_store(W, t, temp); e += temp + rotate_left(a, 5) + sha1_f2(b,c,d) + 0x6ED9EBA1; b = rotate_left(b, 30); }

#define SHA1COMPRESS_FULL_ROUND3_STEP(a, b, c, d, e, W, t, temp) \
    {temp = sha1_mix(W, t); sha1_store(W, t, temp); e += temp + rotate_left(a, 5) + sha1_f3(b,c,d) + 0x8F1BBCDC; b = rotate_left(b, 30); }

#define SHA1COMPRESS_FULL_ROUND4_STEP(a, b, c, d, e, W, t, temp) \
    {temp = sha1_mix(W, t); sha1_store(W, t, temp); e += temp + rotate_left(a, 5) + sha1_f4(b,c,d) + 0xCA62C1D6; b = rotate_left(b, 30); }


#define SHA1_STORE_STATE(i) states[i][0] = a; states[i][1] = b; states[i][2] = c; states[i][3] = d; states[i][4] = e;

#ifdef BUILDNOCOLLDETECTSHA1COMPRESSION
void sha1_compression(uint32_t ihv[5], const uint32_t m[16]) {
    uint32_t W[80];
    uint32_t a, b, c, d, e;
    unsigned i;

    memcpy(W, m, 16 * 4);
    for (i = 16; i < 80; ++i)
        W[i] = sha1_mix(W, i);

    a = ihv[0];
    b = ihv[1];
    c = ihv[2];
    d = ihv[3];
    e = ihv[4];

    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(a, b, c, d, e, W, 0);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(e, a, b, c, d, W, 1);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(d, e, a, b, c, W, 2);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(c, d, e, a, b, W, 3);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(b, c, d, e, a, W, 4);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(a, b, c, d, e, W, 5);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(e, a, b, c, d, W, 6);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(d, e, a, b, c, W, 7);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(c, d, e, a, b, W, 8);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(b, c, d, e, a, W, 9);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(a, b, c, d, e, W, 10);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(e, a, b, c, d, W, 11);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(d, e, a, b, c, W, 12);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(c, d, e, a, b, W, 13);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(b, c, d, e, a, W, 14);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(a, b, c, d, e, W, 15);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(e, a, b, c, d, W, 16);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(d, e, a, b, c, W, 17);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(c, d, e, a, b, W, 18);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(b, c, d, e, a, W, 19);

    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(a, b, c, d, e, W, 20);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(e, a, b, c, d, W, 21);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(d, e, a, b, c, W, 22);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(c, d, e, a, b, W, 23);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(b, c, d, e, a, W, 24);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(a, b, c, d, e, W, 25);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(e, a, b, c, d, W, 26);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(d, e, a, b, c, W, 27);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(c, d, e, a, b, W, 28);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(b, c, d, e, a, W, 29);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(a, b, c, d, e, W, 30);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(e, a, b, c, d, W, 31);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(d, e, a, b, c, W, 32);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(c, d, e, a, b, W, 33);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(b, c, d, e, a, W, 34);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(a, b, c, d, e, W, 35);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(e, a, b, c, d, W, 36);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(d, e, a, b, c, W, 37);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(c, d, e, a, b, W, 38);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(b, c, d, e, a, W, 39);

    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(a, b, c, d, e, W, 40);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(e, a, b, c, d, W, 41);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(d, e, a, b, c, W, 42);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(c, d, e, a, b, W, 43);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(b, c, d, e, a, W, 44);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(a, b, c, d, e, W, 45);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(e, a, b, c, d, W, 46);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(d, e, a, b, c, W, 47);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(c, d, e, a, b, W, 48);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(b, c, d, e, a, W, 49);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(a, b, c, d, e, W, 50);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(e, a, b, c, d, W, 51);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(d, e, a, b, c, W, 52);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(c, d, e, a, b, W, 53);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(b, c, d, e, a, W, 54);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(a, b, c, d, e, W, 55);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(e, a, b, c, d, W, 56);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(d, e, a, b, c, W, 57);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(c, d, e, a, b, W, 58);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(b, c, d, e, a, W, 59);

    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(a, b, c, d, e, W, 60);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(e, a, b, c, d, W, 61);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(d, e, a, b, c, W, 62);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(c, d, e, a, b, W, 63);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(b, c, d, e, a, W, 64);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(a, b, c, d, e, W, 65);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(e, a, b, c, d, W, 66);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(d, e, a, b, c, W, 67);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(c, d, e, a, b, W, 68);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(b, c, d, e, a, W, 69);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(a, b, c, d, e, W, 70);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(e, a, b, c, d, W, 71);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(d, e, a, b, c, W, 72);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(c, d, e, a, b, W, 73);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(b, c, d, e, a, W, 74);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(a, b, c, d, e, W, 75);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(e, a, b, c, d, W, 76);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(d, e, a, b, c, W, 77);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(c, d, e, a, b, W, 78);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(b, c, d, e, a, W, 79);

    ihv[0] += a;
    ihv[1] += b;
    ihv[2] += c;
    ihv[3] += d;
    ihv[4] += e;
}
#endif /*BUILDNOCOLLDETECTSHA1COMPRESSION*/


static void sha1_compression_W(uint32_t ihv[5], const uint32_t W[80]) {
    uint32_t a = ihv[0], b = ihv[1], c = ihv[2], d = ihv[3], e = ihv[4];

    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(a, b, c, d, e, W, 0);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(e, a, b, c, d, W, 1);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(d, e, a, b, c, W, 2);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(c, d, e, a, b, W, 3);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(b, c, d, e, a, W, 4);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(a, b, c, d, e, W, 5);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(e, a, b, c, d, W, 6);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(d, e, a, b, c, W, 7);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(c, d, e, a, b, W, 8);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(b, c, d, e, a, W, 9);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(a, b, c, d, e, W, 10);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(e, a, b, c, d, W, 11);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(d, e, a, b, c, W, 12);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(c, d, e, a, b, W, 13);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(b, c, d, e, a, W, 14);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(a, b, c, d, e, W, 15);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(e, a, b, c, d, W, 16);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(d, e, a, b, c, W, 17);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(c, d, e, a, b, W, 18);
    HASHCLASH_SHA1COMPRESS_ROUND1_STEP(b, c, d, e, a, W, 19);

    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(a, b, c, d, e, W, 20);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(e, a, b, c, d, W, 21);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(d, e, a, b, c, W, 22);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(c, d, e, a, b, W, 23);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(b, c, d, e, a, W, 24);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(a, b, c, d, e, W, 25);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(e, a, b, c, d, W, 26);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(d, e, a, b, c, W, 27);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(c, d, e, a, b, W, 28);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(b, c, d, e, a, W, 29);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(a, b, c, d, e, W, 30);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(e, a, b, c, d, W, 31);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(d, e, a, b, c, W, 32);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(c, d, e, a, b, W, 33);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(b, c, d, e, a, W, 34);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(a, b, c, d, e, W, 35);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(e, a, b, c, d, W, 36);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(d, e, a, b, c, W, 37);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(c, d, e, a, b, W, 38);
    HASHCLASH_SHA1COMPRESS_ROUND2_STEP(b, c, d, e, a, W, 39);

    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(a, b, c, d, e, W, 40);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(e, a, b, c, d, W, 41);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(d, e, a, b, c, W, 42);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(c, d, e, a, b, W, 43);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(b, c, d, e, a, W, 44);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(a, b, c, d, e, W, 45);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(e, a, b, c, d, W, 46);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(d, e, a, b, c, W, 47);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(c, d, e, a, b, W, 48);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(b, c, d, e, a, W, 49);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(a, b, c, d, e, W, 50);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(e, a, b, c, d, W, 51);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(d, e, a, b, c, W, 52);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(c, d, e, a, b, W, 53);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(b, c, d, e, a, W, 54);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(a, b, c, d, e, W, 55);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(e, a, b, c, d, W, 56);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(d, e, a, b, c, W, 57);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(c, d, e, a, b, W, 58);
    HASHCLASH_SHA1COMPRESS_ROUND3_STEP(b, c, d, e, a, W, 59);

    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(a, b, c, d, e, W, 60);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(e, a, b, c, d, W, 61);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(d, e, a, b, c, W, 62);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(c, d, e, a, b, W, 63);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(b, c, d, e, a, W, 64);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(a, b, c, d, e, W, 65);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(e, a, b, c, d, W, 66);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(d, e, a, b, c, W, 67);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(c, d, e, a, b, W, 68);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(b, c, d, e, a, W, 69);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(a, b, c, d, e, W, 70);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(e, a, b, c, d, W, 71);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(d, e, a, b, c, W, 72);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(c, d, e, a, b, W, 73);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(b, c, d, e, a, W, 74);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(a, b, c, d, e, W, 75);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(e, a, b, c, d, W, 76);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(d, e, a, b, c, W, 77);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(c, d, e, a, b, W, 78);
    HASHCLASH_SHA1COMPRESS_ROUND4_STEP(b, c, d, e, a, W, 79);

    ihv[0] += a;
    ihv[1] += b;
    ihv[2] += c;
    ihv[3] += d;
    ihv[4] += e;
}



void sha1_compression_states(uint32_t ihv[5], const uint32_t m[16], uint32_t W[80],
     uint32_t states[80][5]) {
    uint32_t a = ihv[0], b = ihv[1], c = ihv[2], d = ihv[3], e = ihv[4];
    uint32_t temp;

    #ifdef DOSTORESTATE00
    SHA1_STORE_STATE(0)
    #endif
    SHA1COMPRESS_FULL_ROUND1_STEP_LOAD(a, b, c, d, e, m, W, 0, temp);

    #ifdef DOSTORESTATE01
    SHA1_STORE_STATE(1)
    #endif
    SHA1COMPRESS_FULL_ROUND1_STEP_LOAD(e, a, b, c, d, m, W, 1, temp);

    #ifdef DOSTORESTATE02
    SHA1_STORE_STATE(2)
    #endif
    SHA1COMPRESS_FULL_ROUND1_STEP_LOAD(d, e, a, b, c, m, W, 2, temp);

    #ifdef DOSTORESTATE03
    SHA1_STORE_STATE(3)
    #endif
    SHA1COMPRESS_FULL_ROUND1_STEP_LOAD(c, d, e, a, b, m, W, 3, temp);

    #ifdef DOSTORESTATE04
    SHA1_STORE_STATE(4)
    #endif
    SHA1COMPRESS_FULL_ROUND1_STEP_LOAD(b, c, d, e, a, m, W, 4, temp);

    #ifdef DOSTORESTATE05
    SHA1_STORE_STATE(5)
    #endif
    SHA1COMPRESS_FULL_ROUND1_STEP_LOAD(a, b, c, d, e, m, W, 5, temp);

    #ifdef DOSTORESTATE06
    SHA1_STORE_STATE(6)
    #endif
    SHA1COMPRESS_FULL_ROUND1_STEP_LOAD(e, a, b, c, d, m, W, 6, temp);

    #ifdef DOSTORESTATE07
    SHA1_STORE_STATE(7)
    #endif
    SHA1COMPRESS_FULL_ROUND1_STEP_LOAD(d, e, a, b, c, m, W, 7, temp);

    #ifdef DOSTORESTATE08
    SHA1_STORE_STATE(8)
    #endif
    SHA1COMPRESS_FULL_ROUND1_STEP_LOAD(c, d, e, a, b, m, W, 8, temp);

    #ifdef DOSTORESTATE09
    SHA1_STORE_STATE(9)
    #endif
    SHA1COMPRESS_FULL_ROUND1_STEP_LOAD(b, c, d, e, a, m, W, 9, temp);

    #ifdef DOSTORESTATE10
    SHA1_STORE_STATE(10)
    #endif
    SHA1COMPRESS_FULL_ROUND1_STEP_LOAD(a, b, c, d, e, m, W, 10, temp);

    #ifdef DOSTORESTATE11
    SHA1_STORE_STATE(11)
    #endif
    SHA1COMPRESS_FULL_ROUND1_STEP_LOAD(e, a, b, c, d, m, W, 11, temp);

    #ifdef DOSTORESTATE12
    SHA1_STORE_STATE(12)
    #endif
    SHA1COMPRESS_FULL_ROUND1_STEP_LOAD(d, e, a, b, c, m, W, 12, temp);

    #ifdef DOSTORESTATE13
    SHA1_STORE_STATE(13)
    #endif
    SHA1COMPRESS_FULL_ROUND1_STEP_LOAD(c, d, e, a, b, m, W, 13, temp);

    #ifdef DOSTORESTATE14
    SHA1_STORE_STATE(14)
    #endif
    SHA1COMPRESS_FULL_ROUND1_STEP_LOAD(b, c, d, e, a, m, W, 14, temp);

    #ifdef DOSTORESTATE15
    SHA1_STORE_STATE(15)
    #endif
    SHA1COMPRESS_FULL_ROUND1_STEP_LOAD(a, b, c, d, e, m, W, 15, temp);

    #ifdef DOSTORESTATE16
    SHA1_STORE_STATE(16)
    #endif
    SHA1COMPRESS_FULL_ROUND1_STEP_EXPAND(e, a, b, c, d, W, 16, temp);

    #ifdef DOSTORESTATE17
    SHA1_STORE_STATE(17)
    #endif
    SHA1COMPRESS_FULL_ROUND1_STEP_EXPAND(d, e, a, b, c, W, 17, temp);

    #ifdef DOSTORESTATE18
    SHA1_STORE_STATE(18)
    #endif
    SHA1COMPRESS_FULL_ROUND1_STEP_EXPAND(c, d, e, a, b, W, 18, temp);

    #ifdef DOSTORESTATE19
    SHA1_STORE_STATE(19)
    #endif
    SHA1COMPRESS_FULL_ROUND1_STEP_EXPAND(b, c, d, e, a, W, 19, temp);



    #ifdef DOSTORESTATE20
    SHA1_STORE_STATE(20)
    #endif
    SHA1COMPRESS_FULL_ROUND2_STEP(a, b, c, d, e, W, 20, temp);

    #ifdef DOSTORESTATE21
    SHA1_STORE_STATE(21)
    #endif
    SHA1COMPRESS_FULL_ROUND2_STEP(e, a, b, c, d, W, 21, temp);

    #ifdef DOSTORESTATE22
    SHA1_STORE_STATE(22)
    #endif
    SHA1COMPRESS_FULL_ROUND2_STEP(d, e, a, b, c, W, 22, temp);

    #ifdef DOSTORESTATE23
    SHA1_STORE_STATE(23)
    #endif
    SHA1COMPRESS_FULL_ROUND2_STEP(c, d, e, a, b, W, 23, temp);

    #ifdef DOSTORESTATE24
    SHA1_STORE_STATE(24)
    #endif
    SHA1COMPRESS_FULL_ROUND2_STEP(b, c, d, e, a, W, 24, temp);

    #ifdef DOSTORESTATE25
    SHA1_STORE_STATE(25)
    #endif
    SHA1COMPRESS_FULL_ROUND2_STEP(a, b, c, d, e, W, 25, temp);

    #ifdef DOSTORESTATE26
    SHA1_STORE_STATE(26)
    #endif
    SHA1COMPRESS_FULL_ROUND2_STEP(e, a, b, c, d, W, 26, temp);

    #ifdef DOSTORESTATE27
    SHA1_STORE_STATE(27)
    #endif
    SHA1COMPRESS_FULL_ROUND2_STEP(d, e, a, b, c, W, 27, temp);

    #ifdef DOSTORESTATE28
    SHA1_STORE_STATE(28)
    #endif
    SHA1COMPRESS_FULL_ROUND2_STEP(c, d, e, a, b, W, 28, temp);

    #ifdef DOSTORESTATE29
    SHA1_STORE_STATE(29)
    #endif
    SHA1COMPRESS_FULL_ROUND2_STEP(b, c, d, e, a, W, 29, temp);

    #ifdef DOSTORESTATE30
    SHA1_STORE_STATE(30)
    #endif
    SHA1COMPRESS_FULL_ROUND2_STEP(a, b, c, d, e, W, 30, temp);

    #ifdef DOSTORESTATE31
    SHA1_STORE_STATE(31)
    #endif
    SHA1COMPRESS_FULL_ROUND2_STEP(e, a, b, c, d, W, 31, temp);

    #ifdef DOSTORESTATE32
    SHA1_STORE_STATE(32)
    #endif
    SHA1COMPRESS_FULL_ROUND2_STEP(d, e, a, b, c, W, 32, temp);

    #ifdef DOSTORESTATE33
    SHA1_STORE_STATE(33)
    #endif
    SHA1COMPRESS_FULL_ROUND2_STEP(c, d, e, a, b, W, 33, temp);

    #ifdef DOSTORESTATE34
    SHA1_STORE_STATE(34)
    #endif
    SHA1COMPRESS_FULL_ROUND2_STEP(b, c, d, e, a, W, 34, temp);

    #ifdef DOSTORESTATE35
    SHA1_STORE_STATE(35)
    #endif
    SHA1COMPRESS_FULL_ROUND2_STEP(a, b, c, d, e, W, 35, temp);

    #ifdef DOSTORESTATE36
    SHA1_STORE_STATE(36)
    #endif
    SHA1COMPRESS_FULL_ROUND2_STEP(e, a, b, c, d, W, 36, temp);

    #ifdef DOSTORESTATE37
    SHA1_STORE_STATE(37)
    #endif
    SHA1COMPRESS_FULL_ROUND2_STEP(d, e, a, b, c, W, 37, temp);

    #ifdef DOSTORESTATE38
    SHA1_STORE_STATE(38)
    #endif
    SHA1COMPRESS_FULL_ROUND2_STEP(c, d, e, a, b, W, 38, temp);

    #ifdef DOSTORESTATE39
    SHA1_STORE_STATE(39)
    #endif
    SHA1COMPRESS_FULL_ROUND2_STEP(b, c, d, e, a, W, 39, temp);



    #ifdef DOSTORESTATE40
    SHA1_STORE_STATE(40)
    #endif
    SHA1COMPRESS_FULL_ROUND3_STEP(a, b, c, d, e, W, 40, temp);

    #ifdef DOSTORESTATE41
    SHA1_STORE_STATE(41)
    #endif
    SHA1COMPRESS_FULL_ROUND3_STEP(e, a, b, c, d, W, 41, temp);

    #ifdef DOSTORESTATE42
    SHA1_STORE_STATE(42)
    #endif
    SHA1COMPRESS_FULL_ROUND3_STEP(d, e, a, b, c, W, 42, temp);

    #ifdef DOSTORESTATE43
    SHA1_STORE_STATE(43)
    #endif
    SHA1COMPRESS_FULL_ROUND3_STEP(c, d, e, a, b, W, 43, temp);

    #ifdef DOSTORESTATE44
    SHA1_STORE_STATE(44)
    #endif
    SHA1COMPRESS_FULL_ROUND3_STEP(b, c, d, e, a, W, 44, temp);

    #ifdef DOSTORESTATE45
    SHA1_STORE_STATE(45)
    #endif
    SHA1COMPRESS_FULL_ROUND3_STEP(a, b, c, d, e, W, 45, temp);

    #ifdef DOSTORESTATE46
    SHA1_STORE_STATE(46)
    #endif
    SHA1COMPRESS_FULL_ROUND3_STEP(e, a, b, c, d, W, 46, temp);

    #ifdef DOSTORESTATE47
    SHA1_STORE_STATE(47)
    #endif
    SHA1COMPRESS_FULL_ROUND3_STEP(d, e, a, b, c, W, 47, temp);

    #ifdef DOSTORESTATE48
    SHA1_STORE_STATE(48)
    #endif
    SHA1COMPRESS_FULL_ROUND3_STEP(c, d, e, a, b, W, 48, temp);

    #ifdef DOSTORESTATE49
    SHA1_STORE_STATE(49)
    #endif
    SHA1COMPRESS_FULL_ROUND3_STEP(b, c, d, e, a, W, 49, temp);

    #ifdef DOSTORESTATE50
    SHA1_STORE_STATE(50)
    #endif
    SHA1COMPRESS_FULL_ROUND3_STEP(a, b, c, d, e, W, 50, temp);

    #ifdef DOSTORESTATE51
    SHA1_STORE_STATE(51)
    #endif
    SHA1COMPRESS_FULL_ROUND3_STEP(e, a, b, c, d, W, 51, temp);

    #ifdef DOSTORESTATE52
    SHA1_STORE_STATE(52)
    #endif
    SHA1COMPRESS_FULL_ROUND3_STEP(d, e, a, b, c, W, 52, temp);

    #ifdef DOSTORESTATE53
    SHA1_STORE_STATE(53)
    #endif
    SHA1COMPRESS_FULL_ROUND3_STEP(c, d, e, a, b, W, 53, temp);

    #ifdef DOSTORESTATE54
    SHA1_STORE_STATE(54)
    #endif
    SHA1COMPRESS_FULL_ROUND3_STEP(b, c, d, e, a, W, 54, temp);

    #ifdef DOSTORESTATE55
    SHA1_STORE_STATE(55)
    #endif
    SHA1COMPRESS_FULL_ROUND3_STEP(a, b, c, d, e, W, 55, temp);

    #ifdef DOSTORESTATE56
    SHA1_STORE_STATE(56)
    #endif
    SHA1COMPRESS_FULL_ROUND3_STEP(e, a, b, c, d, W, 56, temp);

    #ifdef DOSTORESTATE57
    SHA1_STORE_STATE(57)
    #endif
    SHA1COMPRESS_FULL_ROUND3_STEP(d, e, a, b, c, W, 57, temp);

    #ifdef DOSTORESTATE58
    SHA1_STORE_STATE(58)
    #endif
    SHA1COMPRESS_FULL_ROUND3_STEP(c, d, e, a, b, W, 58, temp);

    #ifdef DOSTORESTATE59
    SHA1_STORE_STATE(59)
    #endif
    SHA1COMPRESS_FULL_ROUND3_STEP(b, c, d, e, a, W, 59, temp);




    #ifdef DOSTORESTATE60
    SHA1_STORE_STATE(60)
    #endif
    SHA1COMPRESS_FULL_ROUND4_STEP(a, b, c, d, e, W, 60, temp);

    #ifdef DOSTORESTATE61
    SHA1_STORE_STATE(61)
    #endif
    SHA1COMPRESS_FULL_ROUND4_STEP(e, a, b, c, d, W, 61, temp);

    #ifdef DOSTORESTATE62
    SHA1_STORE_STATE(62)
    #endif
    SHA1COMPRESS_FULL_ROUND4_STEP(d, e, a, b, c, W, 62, temp);

    #ifdef DOSTORESTATE63
    SHA1_STORE_STATE(63)
    #endif
    SHA1COMPRESS_FULL_ROUND4_STEP(c, d, e, a, b, W, 63, temp);

    #ifdef DOSTORESTATE64
    SHA1_STORE_STATE(64)
    #endif
    SHA1COMPRESS_FULL_ROUND4_STEP(b, c, d, e, a, W, 64, temp);

    #ifdef DOSTORESTATE65
    SHA1_STORE_STATE(65)
    #endif
    SHA1COMPRESS_FULL_ROUND4_STEP(a, b, c, d, e, W, 65, temp);

    #ifdef DOSTORESTATE66
    SHA1_STORE_STATE(66)
    #endif
    SHA1COMPRESS_FULL_ROUND4_STEP(e, a, b, c, d, W, 66, temp);

    #ifdef DOSTORESTATE67
    SHA1_STORE_STATE(67)
    #endif
    SHA1COMPRESS_FULL_ROUND4_STEP(d, e, a, b, c, W, 67, temp);

    #ifdef DOSTORESTATE68
    SHA1_STORE_STATE(68)
    #endif
    SHA1COMPRESS_FULL_ROUND4_STEP(c, d, e, a, b, W, 68, temp);

    #ifdef DOSTORESTATE69
    SHA1_STORE_STATE(69)
    #endif
    SHA1COMPRESS_FULL_ROUND4_STEP(b, c, d, e, a, W, 69, temp);

    #ifdef DOSTORESTATE70
    SHA1_STORE_STATE(70)
    #endif
    SHA1COMPRESS_FULL_ROUND4_STEP(a, b, c, d, e, W, 70, temp);

    #ifdef DOSTORESTATE71
    SHA1_STORE_STATE(71)
    #endif
    SHA1COMPRESS_FULL_ROUND4_STEP(e, a, b, c, d, W, 71, temp);

    #ifdef DOSTORESTATE72
    SHA1_STORE_STATE(72)
    #endif
    SHA1COMPRESS_FULL_ROUND4_STEP(d, e, a, b, c, W, 72, temp);

    #ifdef DOSTORESTATE73
    SHA1_STORE_STATE(73)
    #endif
    SHA1COMPRESS_FULL_ROUND4_STEP(c, d, e, a, b, W, 73, temp);

    #ifdef DOSTORESTATE74
    SHA1_STORE_STATE(74)
    #endif
    SHA1COMPRESS_FULL_ROUND4_STEP(b, c, d, e, a, W, 74, temp);

    #ifdef DOSTORESTATE75
    SHA1_STORE_STATE(75)
    #endif
    SHA1COMPRESS_FULL_ROUND4_STEP(a, b, c, d, e, W, 75, temp);

    #ifdef DOSTORESTATE76
    SHA1_STORE_STATE(76)
    #endif
    SHA1COMPRESS_FULL_ROUND4_STEP(e, a, b, c, d, W, 76, temp);

    #ifdef DOSTORESTATE77
    SHA1_STORE_STATE(77)
    #endif
    SHA1COMPRESS_FULL_ROUND4_STEP(d, e, a, b, c, W, 77, temp);

    #ifdef DOSTORESTATE78
    SHA1_STORE_STATE(78)
    #endif
    SHA1COMPRESS_FULL_ROUND4_STEP(c, d, e, a, b, W, 78, temp);

    #ifdef DOSTORESTATE79
    SHA1_STORE_STATE(79)
    #endif
    SHA1COMPRESS_FULL_ROUND4_STEP(b, c, d, e, a, W, 79, temp);



    ihv[0] += a;
    ihv[1] += b;
    ihv[2] += c;
    ihv[3] += d;
    ihv[4] += e;
}




#define SHA1_RECOMPRESS(t) \
    static void sha1recompress_fast_ ## t (uint32_t ihvin[5], uint32_t ihvout[5], const uint32_t me2[80], const uint32_t state[5]) \
    { \
        uint32_t a = state[0], b = state[1], c = state[2], d = state[3], e = state[4]; \
        if (t > 79) HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(b, c, d, e, a, me2, 79); \
        if (t > 78) HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(c, d, e, a, b, me2, 78); \
        if (t > 77) HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(d, e, a, b, c, me2, 77); \
        if (t > 76) HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(e, a, b, c, d, me2, 76); \
        if (t > 75) HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(a, b, c, d, e, me2, 75); \
        if (t > 74) HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(b, c, d, e, a, me2, 74); \
        if (t > 73) HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(c, d, e, a, b, me2, 73); \
        if (t > 72) HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(d, e, a, b, c, me2, 72); \
        if (t > 71) HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(e, a, b, c, d, me2, 71); \
        if (t > 70) HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(a, b, c, d, e, me2, 70); \
        if (t > 69) HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(b, c, d, e, a, me2, 69); \
        if (t > 68) HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(c, d, e, a, b, me2, 68); \
        if (t > 67) HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(d, e, a, b, c, me2, 67); \
        if (t > 66) HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(e, a, b, c, d, me2, 66); \
        if (t > 65) HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(a, b, c, d, e, me2, 65); \
        if (t > 64) HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(b, c, d, e, a, me2, 64); \
        if (t > 63) HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(c, d, e, a, b, me2, 63); \
        if (t > 62) HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(d, e, a, b, c, me2, 62); \
        if (t > 61) HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(e, a, b, c, d, me2, 61); \
        if (t > 60) HASHCLASH_SHA1COMPRESS_ROUND4_STEP_BW(a, b, c, d, e, me2, 60); \
        if (t > 59) HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(b, c, d, e, a, me2, 59); \
        if (t > 58) HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(c, d, e, a, b, me2, 58); \
        if (t > 57) HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(d, e, a, b, c, me2, 57); \
        if (t > 56) HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(e, a, b, c, d, me2, 56); \
        if (t > 55) HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(a, b, c, d, e, me2, 55); \
        if (t > 54) HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(b, c, d, e, a, me2, 54); \
        if (t > 53) HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(c, d, e, a, b, me2, 53); \
        if (t > 52) HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(d, e, a, b, c, me2, 52); \
        if (t > 51) HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(e, a, b, c, d, me2, 51); \
        if (t > 50) HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(a, b, c, d, e, me2, 50); \
        if (t > 49) HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(b, c, d, e, a, me2, 49); \
        if (t > 48) HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(c, d, e, a, b, me2, 48); \
        if (t > 47) HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(d, e, a, b, c, me2, 47); \
        if (t > 46) HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(e, a, b, c, d, me2, 46); \
        if (t > 45) HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(a, b, c, d, e, me2, 45); \
        if (t > 44) HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(b, c, d, e, a, me2, 44); \
        if (t > 43) HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(c, d, e, a, b, me2, 43); \
        if (t > 42) HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(d, e, a, b, c, me2, 42); \
        if (t > 41) HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(e, a, b, c, d, me2, 41); \
        if (t > 40) HASHCLASH_SHA1COMPRESS_ROUND3_STEP_BW(a, b, c, d, e, me2, 40); \
        if (t > 39) HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(b, c, d, e, a, me2, 39); \
        if (t > 38) HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(c, d, e, a, b, me2, 38); \
        if (t > 37) HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(d, e, a, b, c, me2, 37); \
        if (t > 36) HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(e, a, b, c, d, me2, 36); \
        if (t > 35) HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(a, b, c, d, e, me2, 35); \
        if (t > 34) HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(b, c, d, e, a, me2, 34); \
        if (t > 33) HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(c, d, e, a, b, me2, 33); \
        if (t > 32) HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(d, e, a, b, c, me2, 32); \
        if (t > 31) HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(e, a, b, c, d, me2, 31); \
        if (t > 30) HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(a, b, c, d, e, me2, 30); \
        if (t > 29) HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(b, c, d, e, a, me2, 29); \
        if (t > 28) HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(c, d, e, a, b, me2, 28); \
        if (t > 27) HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(d, e, a, b, c, me2, 27); \
        if (t > 26) HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(e, a, b, c, d, me2, 26); \
        if (t > 25) HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(a, b, c, d, e, me2, 25); \
        if (t > 24) HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(b, c, d, e, a, me2, 24); \
        if (t > 23) HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(c, d, e, a, b, me2, 23); \
        if (t > 22) HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(d, e, a, b, c, me2, 22); \
        if (t > 21) HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(e, a, b, c, d, me2, 21); \
        if (t > 20) HASHCLASH_SHA1COMPRESS_ROUND2_STEP_BW(a, b, c, d, e, me2, 20); \
        if (t > 19) HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(b, c, d, e, a, me2, 19); \
        if (t > 18) HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(c, d, e, a, b, me2, 18); \
        if (t > 17) HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(d, e, a, b, c, me2, 17); \
        if (t > 16) HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(e, a, b, c, d, me2, 16); \
        if (t > 15) HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(a, b, c, d, e, me2, 15); \
        if (t > 14) HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(b, c, d, e, a, me2, 14); \
        if (t > 13) HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(c, d, e, a, b, me2, 13); \
        if (t > 12) HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(d, e, a, b, c, me2, 12); \
        if (t > 11) HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(e, a, b, c, d, me2, 11); \
        if (t > 10) HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(a, b, c, d, e, me2, 10); \
        if (t > 9) HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(b, c, d, e, a, me2, 9); \
        if (t > 8) HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(c, d, e, a, b, me2, 8); \
        if (t > 7) HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(d, e, a, b, c, me2, 7); \
        if (t > 6) HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(e, a, b, c, d, me2, 6); \
        if (t > 5) HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(a, b, c, d, e, me2, 5); \
        if (t > 4) HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(b, c, d, e, a, me2, 4); \
        if (t > 3) HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(c, d, e, a, b, me2, 3); \
        if (t > 2) HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(d, e, a, b, c, me2, 2); \
        if (t > 1) HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(e, a, b, c, d, me2, 1); \
        if (t > 0) HASHCLASH_SHA1COMPRESS_ROUND1_STEP_BW(a, b, c, d, e, me2, 0); \
        ihvin[0] = a; ihvin[1] = b; ihvin[2] = c; ihvin[3] = d; ihvin[4] = e; \
        a = state[0]; b = state[1]; c = state[2]; d = state[3]; e = state[4]; \
        if (t <= 0) HASHCLASH_SHA1COMPRESS_ROUND1_STEP(a, b, c, d, e, me2, 0); \
        if (t <= 1) HASHCLASH_SHA1COMPRESS_ROUND1_STEP(e, a, b, c, d, me2, 1); \
        if (t <= 2) HASHCLASH_SHA1COMPRESS_ROUND1_STEP(d, e, a, b, c, me2, 2); \
        if (t <= 3) HASHCLASH_SHA1COMPRESS_ROUND1_STEP(c, d, e, a, b, me2, 3); \
        if (t <= 4) HASHCLASH_SHA1COMPRESS_ROUND1_STEP(b, c, d, e, a, me2, 4); \
        if (t <= 5) HASHCLASH_SHA1COMPRESS_ROUND1_STEP(a, b, c, d, e, me2, 5); \
        if (t <= 6) HASHCLASH_SHA1COMPRESS_ROUND1_STEP(e, a, b, c, d, me2, 6); \
        if (t <= 7) HASHCLASH_SHA1COMPRESS_ROUND1_STEP(d, e, a, b, c, me2, 7); \
        if (t <= 8) HASHCLASH_SHA1COMPRESS_ROUND1_STEP(c, d, e, a, b, me2, 8); \
        if (t <= 9) HASHCLASH_SHA1COMPRESS_ROUND1_STEP(b, c, d, e, a, me2, 9); \
        if (t <= 10) HASHCLASH_SHA1COMPRESS_ROUND1_STEP(a, b, c, d, e, me2, 10); \
        if (t <= 11) HASHCLASH_SHA1COMPRESS_ROUND1_STEP(e, a, b, c, d, me2, 11); \
        if (t <= 12) HASHCLASH_SHA1COMPRESS_ROUND1_STEP(d, e, a, b, c, me2, 12); \
        if (t <= 13) HASHCLASH_SHA1COMPRESS_ROUND1_STEP(c, d, e, a, b, me2, 13); \
        if (t <= 14) HASHCLASH_SHA1COMPRESS_ROUND1_STEP(b, c, d, e, a, me2, 14); \
        if (t <= 15) HASHCLASH_SHA1COMPRESS_ROUND1_STEP(a, b, c, d, e, me2, 15); \
        if (t <= 16) HASHCLASH_SHA1COMPRESS_ROUND1_STEP(e, a, b, c, d, me2, 16); \
        if (t <= 17) HASHCLASH_SHA1COMPRESS_ROUND1_STEP(d, e, a, b, c, me2, 17); \
        if (t <= 18) HASHCLASH_SHA1COMPRESS_ROUND1_STEP(c, d, e, a, b, me2, 18); \
        if (t <= 19) HASHCLASH_SHA1COMPRESS_ROUND1_STEP(b, c, d, e, a, me2, 19); \
        if (t <= 20) HASHCLASH_SHA1COMPRESS_ROUND2_STEP(a, b, c, d, e, me2, 20); \
        if (t <= 21) HASHCLASH_SHA1COMPRESS_ROUND2_STEP(e, a, b, c, d, me2, 21); \
        if (t <= 22) HASHCLASH_SHA1COMPRESS_ROUND2_STEP(d, e, a, b, c, me2, 22); \
        if (t <= 23) HASHCLASH_SHA1COMPRESS_ROUND2_STEP(c, d, e, a, b, me2, 23); \
        if (t <= 24) HASHCLASH_SHA1COMPRESS_ROUND2_STEP(b, c, d, e, a, me2, 24); \
        if (t <= 25) HASHCLASH_SHA1COMPRESS_ROUND2_STEP(a, b, c, d, e, me2, 25); \
        if (t <= 26) HASHCLASH_SHA1COMPRESS_ROUND2_STEP(e, a, b, c, d, me2, 26); \
        if (t <= 27) HASHCLASH_SHA1COMPRESS_ROUND2_STEP(d, e, a, b, c, me2, 27); \
        if (t <= 28) HASHCLASH_SHA1COMPRESS_ROUND2_STEP(c, d, e, a, b, me2, 28); \
        if (t <= 29) HASHCLASH_SHA1COMPRESS_ROUND2_STEP(b, c, d, e, a, me2, 29); \
        if (t <= 30) HASHCLASH_SHA1COMPRESS_ROUND2_STEP(a, b, c, d, e, me2, 30); \
        if (t <= 31) HASHCLASH_SHA1COMPRESS_ROUND2_STEP(e, a, b, c, d, me2, 31); \
        if (t <= 32) HASHCLASH_SHA1COMPRESS_ROUND2_STEP(d, e, a, b, c, me2, 32); \
        if (t <= 33) HASHCLASH_SHA1COMPRESS_ROUND2_STEP(c, d, e, a, b, me2, 33); \
        if (t <= 34) HASHCLASH_SHA1COMPRESS_ROUND2_STEP(b, c, d, e, a, me2, 34); \
        if (t <= 35) HASHCLASH_SHA1COMPRESS_ROUND2_STEP(a, b, c, d, e, me2, 35); \
        if (t <= 36) HASHCLASH_SHA1COMPRESS_ROUND2_STEP(e, a, b, c, d, me2, 36); \
        if (t <= 37) HASHCLASH_SHA1COMPRESS_ROUND2_STEP(d, e, a, b, c, me2, 37); \
        if (t <= 38) HASHCLASH_SHA1COMPRESS_ROUND2_STEP(c, d, e, a, b, me2, 38); \
        if (t <= 39) HASHCLASH_SHA1COMPRESS_ROUND2_STEP(b, c, d, e, a, me2, 39); \
        if (t <= 40) HASHCLASH_SHA1COMPRESS_ROUND3_STEP(a, b, c, d, e, me2, 40); \
        if (t <= 41) HASHCLASH_SHA1COMPRESS_ROUND3_STEP(e, a, b, c, d, me2, 41); \
        if (t <= 42) HASHCLASH_SHA1COMPRESS_ROUND3_STEP(d, e, a, b, c, me2, 42); \
        if (t <= 43) HASHCLASH_SHA1COMPRESS_ROUND3_STEP(c, d, e, a, b, me2, 43); \
        if (t <= 44) HASHCLASH_SHA1COMPRESS_ROUND3_STEP(b, c, d, e, a, me2, 44); \
        if (t <= 45) HASHCLASH_SHA1COMPRESS_ROUND3_STEP(a, b, c, d, e, me2, 45); \
        if (t <= 46) HASHCLASH_SHA1COMPRESS_ROUND3_STEP(e, a, b, c, d, me2, 46); \
        if (t <= 47) HASHCLASH_SHA1COMPRESS_ROUND3_STEP(d, e, a, b, c, me2, 47); \
        if (t <= 48) HASHCLASH_SHA1COMPRESS_ROUND3_STEP(c, d, e, a, b, me2, 48); \
        if (t <= 49) HASHCLASH_SHA1COMPRESS_ROUND3_STEP(b, c, d, e, a, me2, 49); \
        if (t <= 50) HASHCLASH_SHA1COMPRESS_ROUND3_STEP(a, b, c, d, e, me2, 50); \
        if (t <= 51) HASHCLASH_SHA1COMPRESS_ROUND3_STEP(e, a, b, c, d, me2, 51); \
        if (t <= 52) HASHCLASH_SHA1COMPRESS_ROUND3_STEP(d, e, a, b, c, me2, 52); \
        if (t <= 53) HASHCLASH_SHA1COMPRESS_ROUND3_STEP(c, d, e, a, b, me2, 53); \
        if (t <= 54) HASHCLASH_SHA1COMPRESS_ROUND3_STEP(b, c, d, e, a, me2, 54); \
        if (t <= 55) HASHCLASH_SHA1COMPRESS_ROUND3_STEP(a, b, c, d, e, me2, 55); \
        if (t <= 56) HASHCLASH_SHA1COMPRESS_ROUND3_STEP(e, a, b, c, d, me2, 56); \
        if (t <= 57) HASHCLASH_SHA1COMPRESS_ROUND3_STEP(d, e, a, b, c, me2, 57); \
        if (t <= 58) HASHCLASH_SHA1COMPRESS_ROUND3_STEP(c, d, e, a, b, me2, 58); \
        if (t <= 59) HASHCLASH_SHA1COMPRESS_ROUND3_STEP(b, c, d, e, a, me2, 59); \
        if (t <= 60) HASHCLASH_SHA1COMPRESS_ROUND4_STEP(a, b, c, d, e, me2, 60); \
        if (t <= 61) HASHCLASH_SHA1COMPRESS_ROUND4_STEP(e, a, b, c, d, me2, 61); \
        if (t <= 62) HASHCLASH_SHA1COMPRESS_ROUND4_STEP(d, e, a, b, c, me2, 62); \
        if (t <= 63) HASHCLASH_SHA1COMPRESS_ROUND4_STEP(c, d, e, a, b, me2, 63); \
        if (t <= 64) HASHCLASH_SHA1COMPRESS_ROUND4_STEP(b, c, d, e, a, me2, 64); \
        if (t <= 65) HASHCLASH_SHA1COMPRESS_ROUND4_STEP(a, b, c, d, e, me2, 65); \
        if (t <= 66) HASHCLASH_SHA1COMPRESS_ROUND4_STEP(e, a, b, c, d, me2, 66); \
        if (t <= 67) HASHCLASH_SHA1COMPRESS_ROUND4_STEP(d, e, a, b, c, me2, 67); \
        if (t <= 68) HASHCLASH_SHA1COMPRESS_ROUND4_STEP(c, d, e, a, b, me2, 68); \
        if (t <= 69) HASHCLASH_SHA1COMPRESS_ROUND4_STEP(b, c, d, e, a, me2, 69); \
        if (t <= 70) HASHCLASH_SHA1COMPRESS_ROUND4_STEP(a, b, c, d, e, me2, 70); \
        if (t <= 71) HASHCLASH_SHA1COMPRESS_ROUND4_STEP(e, a, b, c, d, me2, 71); \
        if (t <= 72) HASHCLASH_SHA1COMPRESS_ROUND4_STEP(d, e, a, b, c, me2, 72); \
        if (t <= 73) HASHCLASH_SHA1COMPRESS_ROUND4_STEP(c, d, e, a, b, me2, 73); \
        if (t <= 74) HASHCLASH_SHA1COMPRESS_ROUND4_STEP(b, c, d, e, a, me2, 74); \
        if (t <= 75) HASHCLASH_SHA1COMPRESS_ROUND4_STEP(a, b, c, d, e, me2, 75); \
        if (t <= 76) HASHCLASH_SHA1COMPRESS_ROUND4_STEP(e, a, b, c, d, me2, 76); \
        if (t <= 77) HASHCLASH_SHA1COMPRESS_ROUND4_STEP(d, e, a, b, c, me2, 77); \
        if (t <= 78) HASHCLASH_SHA1COMPRESS_ROUND4_STEP(c, d, e, a, b, me2, 78); \
        if (t <= 79) HASHCLASH_SHA1COMPRESS_ROUND4_STEP(b, c, d, e, a, me2, 79); \
        ihvout[0] = ihvin[0] + a; ihvout[1] = ihvin[1] + b; ihvout[2] = ihvin[2] + c; ihvout[3] = ihvin[3] + d; ihvout[4] = ihvin[4] + e; \
    }

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4127)  /* Compiler complains about the checks in the above macro being constant. */
#endif

#ifdef DOSTORESTATE0
    SHA1_RECOMPRESS(0)
#endif

#ifdef DOSTORESTATE1
    SHA1_RECOMPRESS(1)
#endif

#ifdef DOSTORESTATE2
    SHA1_RECOMPRESS(2)
#endif

#ifdef DOSTORESTATE3
    SHA1_RECOMPRESS(3)
#endif

#ifdef DOSTORESTATE4
    SHA1_RECOMPRESS(4)
#endif

#ifdef DOSTORESTATE5
    SHA1_RECOMPRESS(5)
#endif

#ifdef DOSTORESTATE6
    SHA1_RECOMPRESS(6)
#endif

#ifdef DOSTORESTATE7
    SHA1_RECOMPRESS(7)
#endif

#ifdef DOSTORESTATE8
    SHA1_RECOMPRESS(8)
#endif

#ifdef DOSTORESTATE9
    SHA1_RECOMPRESS(9)
#endif

#ifdef DOSTORESTATE10
    SHA1_RECOMPRESS(10)
#endif

#ifdef DOSTORESTATE11
    SHA1_RECOMPRESS(11)
#endif

#ifdef DOSTORESTATE12
    SHA1_RECOMPRESS(12)
#endif

#ifdef DOSTORESTATE13
    SHA1_RECOMPRESS(13)
#endif

#ifdef DOSTORESTATE14
    SHA1_RECOMPRESS(14)
#endif

#ifdef DOSTORESTATE15
    SHA1_RECOMPRESS(15)
#endif

#ifdef DOSTORESTATE16
    SHA1_RECOMPRESS(16)
#endif

#ifdef DOSTORESTATE17
    SHA1_RECOMPRESS(17)
#endif

#ifdef DOSTORESTATE18
    SHA1_RECOMPRESS(18)
#endif

#ifdef DOSTORESTATE19
    SHA1_RECOMPRESS(19)
#endif

#ifdef DOSTORESTATE20
    SHA1_RECOMPRESS(20)
#endif

#ifdef DOSTORESTATE21
    SHA1_RECOMPRESS(21)
#endif

#ifdef DOSTORESTATE22
    SHA1_RECOMPRESS(22)
#endif

#ifdef DOSTORESTATE23
    SHA1_RECOMPRESS(23)
#endif

#ifdef DOSTORESTATE24
    SHA1_RECOMPRESS(24)
#endif

#ifdef DOSTORESTATE25
    SHA1_RECOMPRESS(25)
#endif

#ifdef DOSTORESTATE26
    SHA1_RECOMPRESS(26)
#endif

#ifdef DOSTORESTATE27
    SHA1_RECOMPRESS(27)
#endif

#ifdef DOSTORESTATE28
    SHA1_RECOMPRESS(28)
#endif

#ifdef DOSTORESTATE29
    SHA1_RECOMPRESS(29)
#endif

#ifdef DOSTORESTATE30
    SHA1_RECOMPRESS(30)
#endif

#ifdef DOSTORESTATE31
    SHA1_RECOMPRESS(31)
#endif

#ifdef DOSTORESTATE32
    SHA1_RECOMPRESS(32)
#endif

#ifdef DOSTORESTATE33
    SHA1_RECOMPRESS(33)
#endif

#ifdef DOSTORESTATE34
    SHA1_RECOMPRESS(34)
#endif

#ifdef DOSTORESTATE35
    SHA1_RECOMPRESS(35)
#endif

#ifdef DOSTORESTATE36
    SHA1_RECOMPRESS(36)
#endif

#ifdef DOSTORESTATE37
    SHA1_RECOMPRESS(37)
#endif

#ifdef DOSTORESTATE38
    SHA1_RECOMPRESS(38)
#endif

#ifdef DOSTORESTATE39
    SHA1_RECOMPRESS(39)
#endif

#ifdef DOSTORESTATE40
    SHA1_RECOMPRESS(40)
#endif

#ifdef DOSTORESTATE41
    SHA1_RECOMPRESS(41)
#endif

#ifdef DOSTORESTATE42
    SHA1_RECOMPRESS(42)
#endif

#ifdef DOSTORESTATE43
    SHA1_RECOMPRESS(43)
#endif

#ifdef DOSTORESTATE44
    SHA1_RECOMPRESS(44)
#endif

#ifdef DOSTORESTATE45
    SHA1_RECOMPRESS(45)
#endif

#ifdef DOSTORESTATE46
    SHA1_RECOMPRESS(46)
#endif

#ifdef DOSTORESTATE47
    SHA1_RECOMPRESS(47)
#endif

#ifdef DOSTORESTATE48
    SHA1_RECOMPRESS(48)
#endif

#ifdef DOSTORESTATE49
    SHA1_RECOMPRESS(49)
#endif

#ifdef DOSTORESTATE50
    SHA1_RECOMPRESS(50)
#endif

#ifdef DOSTORESTATE51
    SHA1_RECOMPRESS(51)
#endif

#ifdef DOSTORESTATE52
    SHA1_RECOMPRESS(52)
#endif

#ifdef DOSTORESTATE53
    SHA1_RECOMPRESS(53)
#endif

#ifdef DOSTORESTATE54
    SHA1_RECOMPRESS(54)
#endif

#ifdef DOSTORESTATE55
    SHA1_RECOMPRESS(55)
#endif

#ifdef DOSTORESTATE56
    SHA1_RECOMPRESS(56)
#endif

#ifdef DOSTORESTATE57
    SHA1_RECOMPRESS(57)
#endif

#ifdef DOSTORESTATE58
    SHA1_RECOMPRESS(58)
#endif

#ifdef DOSTORESTATE59
    SHA1_RECOMPRESS(59)
#endif

#ifdef DOSTORESTATE60
    SHA1_RECOMPRESS(60)
#endif

#ifdef DOSTORESTATE61
    SHA1_RECOMPRESS(61)
#endif

#ifdef DOSTORESTATE62
    SHA1_RECOMPRESS(62)
#endif

#ifdef DOSTORESTATE63
    SHA1_RECOMPRESS(63)
#endif

#ifdef DOSTORESTATE64
    SHA1_RECOMPRESS(64)
#endif

#ifdef DOSTORESTATE65
    SHA1_RECOMPRESS(65)
#endif

#ifdef DOSTORESTATE66
    SHA1_RECOMPRESS(66)
#endif

#ifdef DOSTORESTATE67
    SHA1_RECOMPRESS(67)
#endif

#ifdef DOSTORESTATE68
    SHA1_RECOMPRESS(68)
#endif

#ifdef DOSTORESTATE69
    SHA1_RECOMPRESS(69)
#endif

#ifdef DOSTORESTATE70
    SHA1_RECOMPRESS(70)
#endif

#ifdef DOSTORESTATE71
    SHA1_RECOMPRESS(71)
#endif

#ifdef DOSTORESTATE72
    SHA1_RECOMPRESS(72)
#endif

#ifdef DOSTORESTATE73
    SHA1_RECOMPRESS(73)
#endif

#ifdef DOSTORESTATE74
    SHA1_RECOMPRESS(74)
#endif

#ifdef DOSTORESTATE75
    SHA1_RECOMPRESS(75)
#endif

#ifdef DOSTORESTATE76
    SHA1_RECOMPRESS(76)
#endif

#ifdef DOSTORESTATE77
    SHA1_RECOMPRESS(77)
#endif

#ifdef DOSTORESTATE78
    SHA1_RECOMPRESS(78)
#endif

#ifdef DOSTORESTATE79
    SHA1_RECOMPRESS(79)
#endif

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

static void sha1_recompression_step(uint32_t step, uint32_t ihvin[5], uint32_t ihvout[5],
                                    const uint32_t me2[80], const uint32_t state[5]) {
    switch (step) {
            #ifdef DOSTORESTATE0
        case 0:
            sha1recompress_fast_0(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE1
        case 1:
            sha1recompress_fast_1(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE2
        case 2:
            sha1recompress_fast_2(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE3
        case 3:
            sha1recompress_fast_3(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE4
        case 4:
            sha1recompress_fast_4(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE5
        case 5:
            sha1recompress_fast_5(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE6
        case 6:
            sha1recompress_fast_6(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE7
        case 7:
            sha1recompress_fast_7(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE8
        case 8:
            sha1recompress_fast_8(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE9
        case 9:
            sha1recompress_fast_9(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE10
        case 10:
            sha1recompress_fast_10(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE11
        case 11:
            sha1recompress_fast_11(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE12
        case 12:
            sha1recompress_fast_12(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE13
        case 13:
            sha1recompress_fast_13(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE14
        case 14:
            sha1recompress_fast_14(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE15
        case 15:
            sha1recompress_fast_15(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE16
        case 16:
            sha1recompress_fast_16(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE17
        case 17:
            sha1recompress_fast_17(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE18
        case 18:
            sha1recompress_fast_18(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE19
        case 19:
            sha1recompress_fast_19(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE20
        case 20:
            sha1recompress_fast_20(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE21
        case 21:
            sha1recompress_fast_21(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE22
        case 22:
            sha1recompress_fast_22(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE23
        case 23:
            sha1recompress_fast_23(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE24
        case 24:
            sha1recompress_fast_24(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE25
        case 25:
            sha1recompress_fast_25(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE26
        case 26:
            sha1recompress_fast_26(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE27
        case 27:
            sha1recompress_fast_27(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE28
        case 28:
            sha1recompress_fast_28(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE29
        case 29:
            sha1recompress_fast_29(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE30
        case 30:
            sha1recompress_fast_30(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE31
        case 31:
            sha1recompress_fast_31(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE32
        case 32:
            sha1recompress_fast_32(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE33
        case 33:
            sha1recompress_fast_33(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE34
        case 34:
            sha1recompress_fast_34(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE35
        case 35:
            sha1recompress_fast_35(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE36
        case 36:
            sha1recompress_fast_36(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE37
        case 37:
            sha1recompress_fast_37(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE38
        case 38:
            sha1recompress_fast_38(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE39
        case 39:
            sha1recompress_fast_39(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE40
        case 40:
            sha1recompress_fast_40(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE41
        case 41:
            sha1recompress_fast_41(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE42
        case 42:
            sha1recompress_fast_42(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE43
        case 43:
            sha1recompress_fast_43(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE44
        case 44:
            sha1recompress_fast_44(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE45
        case 45:
            sha1recompress_fast_45(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE46
        case 46:
            sha1recompress_fast_46(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE47
        case 47:
            sha1recompress_fast_47(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE48
        case 48:
            sha1recompress_fast_48(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE49
        case 49:
            sha1recompress_fast_49(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE50
        case 50:
            sha1recompress_fast_50(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE51
        case 51:
            sha1recompress_fast_51(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE52
        case 52:
            sha1recompress_fast_52(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE53
        case 53:
            sha1recompress_fast_53(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE54
        case 54:
            sha1recompress_fast_54(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE55
        case 55:
            sha1recompress_fast_55(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE56
        case 56:
            sha1recompress_fast_56(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE57
        case 57:
            sha1recompress_fast_57(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE58
        case 58:
            sha1recompress_fast_58(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE59
        case 59:
            sha1recompress_fast_59(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE60
        case 60:
            sha1recompress_fast_60(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE61
        case 61:
            sha1recompress_fast_61(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE62
        case 62:
            sha1recompress_fast_62(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE63
        case 63:
            sha1recompress_fast_63(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE64
        case 64:
            sha1recompress_fast_64(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE65
        case 65:
            sha1recompress_fast_65(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE66
        case 66:
            sha1recompress_fast_66(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE67
        case 67:
            sha1recompress_fast_67(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE68
        case 68:
            sha1recompress_fast_68(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE69
        case 69:
            sha1recompress_fast_69(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE70
        case 70:
            sha1recompress_fast_70(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE71
        case 71:
            sha1recompress_fast_71(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE72
        case 72:
            sha1recompress_fast_72(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE73
        case 73:
            sha1recompress_fast_73(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE74
        case 74:
            sha1recompress_fast_74(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE75
        case 75:
            sha1recompress_fast_75(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE76
        case 76:
            sha1recompress_fast_76(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE77
        case 77:
            sha1recompress_fast_77(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE78
        case 78:
            sha1recompress_fast_78(ihvin, ihvout, me2, state);
            break;
            #endif
            #ifdef DOSTORESTATE79
        case 79:
            sha1recompress_fast_79(ihvin, ihvout, me2, state);
            break;
            #endif
        default:
            abort();
    }

}



static void sha1_process(SHA1_CTX *ctx, const uint32_t block[16]) {
    unsigned i, j;
    uint32_t ubc_dv_mask[DVMASKSIZE] = { 0xFFFFFFFF };
    uint32_t ihvtmp[5];

    ctx->ihv1[0] = ctx->ihv[0];
    ctx->ihv1[1] = ctx->ihv[1];
    ctx->ihv1[2] = ctx->ihv[2];
    ctx->ihv1[3] = ctx->ihv[3];
    ctx->ihv1[4] = ctx->ihv[4];

    sha1_compression_states(ctx->ihv, block, ctx->m1, ctx->states);

    if (ctx->detect_coll) {
        if (ctx->ubc_check) {
            ubc_check(ctx->m1, ubc_dv_mask);
        }

        if (ubc_dv_mask[0] != 0) {
            for (i = 0; sha1_dvs[i].dvType != 0; ++i) {
                if (ubc_dv_mask[0] & ((uint32_t)(1) << sha1_dvs[i].maskb)) {
                    for (j = 0; j < 80; ++j)
                        ctx->m2[j] = ctx->m1[j] ^ sha1_dvs[i].dm[j];

                    sha1_recompression_step(sha1_dvs[i].testt, ctx->ihv2, ihvtmp, ctx->m2,
                                            ctx->states[sha1_dvs[i].testt]);

                    /* to verify SHA-1 collision detection code with collisions for reduced-step SHA-1 */
                    if ((0 == ((ihvtmp[0] ^ ctx->ihv[0]) | (ihvtmp[1] ^ ctx->ihv[1]) | (ihvtmp[2] ^ ctx->ihv[2]) |
                               (ihvtmp[3] ^ ctx->ihv[3]) | (ihvtmp[4] ^ ctx->ihv[4])))
                        || (ctx->reduced_round_coll
                            && 0 == ((ctx->ihv1[0] ^ ctx->ihv2[0]) | (ctx->ihv1[1] ^ ctx->ihv2[1]) |
                                     (ctx->ihv1[2] ^ ctx->ihv2[2]) | (ctx->ihv1[3] ^ ctx->ihv2[3]) | (ctx->ihv1[4] ^ ctx->ihv2[4])))) {
                        ctx->found_collision = 1;

                        if (ctx->safe_hash) {
                            sha1_compression_W(ctx->ihv, ctx->m1);
                            sha1_compression_W(ctx->ihv, ctx->m1);
                        }

                        break;
                    }
                }
            }
        }
    }
}

void SHA1DCInit(SHA1_CTX *ctx) {
    ctx->total = 0;
    ctx->ihv[0] = 0x67452301;
    ctx->ihv[1] = 0xEFCDAB89;
    ctx->ihv[2] = 0x98BADCFE;
    ctx->ihv[3] = 0x10325476;
    ctx->ihv[4] = 0xC3D2E1F0;
    ctx->found_collision = 0;
    ctx->safe_hash = SHA1DC_INIT_SAFE_HASH_DEFAULT;
    ctx->ubc_check = 1;
    ctx->detect_coll = 1;
    ctx->reduced_round_coll = 0;
    ctx->callback = NULL;
}

void SHA1DCSetSafeHash(SHA1_CTX *ctx, int safehash) {
    if (safehash)
        ctx->safe_hash = 1;
    else
        ctx->safe_hash = 0;
}


void SHA1DCSetUseUBC(SHA1_CTX *ctx, int ubc_check) {
    if (ubc_check)
        ctx->ubc_check = 1;
    else
        ctx->ubc_check = 0;
}

void SHA1DCSetUseDetectColl(SHA1_CTX *ctx, int detect_coll) {
    if (detect_coll)
        ctx->detect_coll = 1;
    else
        ctx->detect_coll = 0;
}

void SHA1DCSetDetectReducedRoundCollision(SHA1_CTX *ctx, int reduced_round_coll) {
    if (reduced_round_coll)
        ctx->reduced_round_coll = 1;
    else
        ctx->reduced_round_coll = 0;
}

void SHA1DCSetCallback(SHA1_CTX *ctx, collision_block_callback callback) {
    ctx->callback = callback;
}

void SHA1DCUpdate(SHA1_CTX *ctx, const char *buf, size_t len) {
    unsigned left, fill;

    if (len == 0)
        return;

    left = ctx->total & 63;
    fill = 64 - left;

    if (left && len >= fill) {
        ctx->total += fill;
        memcpy(ctx->buffer + left, buf, fill);
        sha1_process(ctx, (uint32_t *)(ctx->buffer));
        buf += fill;
        len -= fill;
        left = 0;
    }
    while (len >= 64) {
        ctx->total += 64;

        #if defined(SHA1DC_ALLOW_UNALIGNED_ACCESS)
        sha1_process(ctx, (uint32_t *)(buf));
        #else
        memcpy(ctx->buffer, buf, 64);
        sha1_process(ctx, (uint32_t *)(ctx->buffer));
        #endif /* defined(SHA1DC_ALLOW_UNALIGNED_ACCESS) */
        buf += 64;
        len -= 64;
    }
    if (len > 0) {
        ctx->total += len;
        memcpy(ctx->buffer + left, buf, len);
    }
}

static const unsigned char sha1_padding[64] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

int SHA1DCFinal(unsigned char output[SHA1_LEN], SHA1_CTX *ctx) {
    uint32_t last = ctx->total & 63;
    uint32_t padn = (last < 56) ? (56 - last) : (120 - last);
    uint64_t total;
    SHA1DCUpdate(ctx, (const char *)(sha1_padding), padn);

    total = ctx->total - padn;
    total <<= 3;
    ctx->buffer[56] = (unsigned char)(total >> 56);
    ctx->buffer[57] = (unsigned char)(total >> 48);
    ctx->buffer[58] = (unsigned char)(total >> 40);
    ctx->buffer[59] = (unsigned char)(total >> 32);
    ctx->buffer[60] = (unsigned char)(total >> 24);
    ctx->buffer[61] = (unsigned char)(total >> 16);
    ctx->buffer[62] = (unsigned char)(total >> 8);
    ctx->buffer[63] = (unsigned char)(total);
    sha1_process(ctx, (uint32_t *)(ctx->buffer));
    output[0] = (unsigned char)(ctx->ihv[0] >> 24);
    output[1] = (unsigned char)(ctx->ihv[0] >> 16);
    output[2] = (unsigned char)(ctx->ihv[0] >> 8);
    output[3] = (unsigned char)(ctx->ihv[0]);
    output[4] = (unsigned char)(ctx->ihv[1] >> 24);
    output[5] = (unsigned char)(ctx->ihv[1] >> 16);
    output[6] = (unsigned char)(ctx->ihv[1] >> 8);
    output[7] = (unsigned char)(ctx->ihv[1]);
    output[8] = (unsigned char)(ctx->ihv[2] >> 24);
    output[9] = (unsigned char)(ctx->ihv[2] >> 16);
    output[10] = (unsigned char)(ctx->ihv[2] >> 8);
    output[11] = (unsigned char)(ctx->ihv[2]);
    output[12] = (unsigned char)(ctx->ihv[3] >> 24);
    output[13] = (unsigned char)(ctx->ihv[3] >> 16);
    output[14] = (unsigned char)(ctx->ihv[3] >> 8);
    output[15] = (unsigned char)(ctx->ihv[3]);
    output[16] = (unsigned char)(ctx->ihv[4] >> 24);
    output[17] = (unsigned char)(ctx->ihv[4] >> 16);
    output[18] = (unsigned char)(ctx->ihv[4] >> 8);
    output[19] = (unsigned char)(ctx->ihv[4]);
    return ctx->found_collision;
}

#ifdef SHA1DC_CUSTOM_TRAILING_INCLUDE_SHA1_C
    #include SHA1DC_CUSTOM_TRAILING_INCLUDE_SHA1_C
#endif /* SHA1DC_CUSTOM_TRAILING_INCLUDE_SHA1_C */
/***
* Copyright 2017 Marc Stevens <marc@marc-stevens.nl>, Dan Shumow <danshu@microsoft.com>
* Distributed under the MIT Software License.
* See accompanying file LICENSE.txt or copy at
* https://opensource.org/licenses/MIT
***/

/*
// this file was generated by the 'parse_bitrel' program in the tools section
// using the data files from directory 'tools/data/3565'
//
// sha1_dvs contains a list of SHA-1 Disturbance Vectors (DV) to check
// dvType, dvK and dvB define the DV: I(K,B) or II(K,B) (see the paper)
// dm[80] is the expanded message block XOR-difference defined by the DV
// testt is the step to do the recompression from for collision detection
// maski and maskb define the bit to check for each DV in the dvmask returned by ubc_check
//
// ubc_check takes as input an expanded message block and verifies the unavoidable bitconditions for all listed DVs
// it returns a dvmask where each bit belonging to a DV is set if all unavoidable bitconditions for that DV have been met
// thus one needs to do the recompression check for each DV that has its bit set
//
// ubc_check is programmatically generated and the unavoidable bitconditions have been hardcoded
// a directly verifiable version named ubc_check_verify can be found in ubc_check_verify.c
// ubc_check has been verified against ubc_check_verify using the 'ubc_check_test' program in the tools section
*/

#ifndef SHA1DC_NO_STANDARD_INCLUDES
    #include <stdint.h>
#endif
#ifdef SHA1DC_CUSTOM_INCLUDE_UBC_CHECK_C
    #include SHA1DC_CUSTOM_INCLUDE_UBC_CHECK_C
#endif

static const uint32_t DV_I_43_0_bit     = (uint32_t)(1) << 0;
static const uint32_t DV_I_44_0_bit     = (uint32_t)(1) << 1;
static const uint32_t DV_I_45_0_bit     = (uint32_t)(1) << 2;
static const uint32_t DV_I_46_0_bit     = (uint32_t)(1) << 3;
static const uint32_t DV_I_46_2_bit     = (uint32_t)(1) << 4;
static const uint32_t DV_I_47_0_bit     = (uint32_t)(1) << 5;
static const uint32_t DV_I_47_2_bit     = (uint32_t)(1) << 6;
static const uint32_t DV_I_48_0_bit     = (uint32_t)(1) << 7;
static const uint32_t DV_I_48_2_bit     = (uint32_t)(1) << 8;
static const uint32_t DV_I_49_0_bit     = (uint32_t)(1) << 9;
static const uint32_t DV_I_49_2_bit     = (uint32_t)(1) << 10;
static const uint32_t DV_I_50_0_bit     = (uint32_t)(1) << 11;
static const uint32_t DV_I_50_2_bit     = (uint32_t)(1) << 12;
static const uint32_t DV_I_51_0_bit     = (uint32_t)(1) << 13;
static const uint32_t DV_I_51_2_bit     = (uint32_t)(1) << 14;
static const uint32_t DV_I_52_0_bit     = (uint32_t)(1) << 15;
static const uint32_t DV_II_45_0_bit    = (uint32_t)(1) << 16;
static const uint32_t DV_II_46_0_bit    = (uint32_t)(1) << 17;
static const uint32_t DV_II_46_2_bit    = (uint32_t)(1) << 18;
static const uint32_t DV_II_47_0_bit    = (uint32_t)(1) << 19;
static const uint32_t DV_II_48_0_bit    = (uint32_t)(1) << 20;
static const uint32_t DV_II_49_0_bit    = (uint32_t)(1) << 21;
static const uint32_t DV_II_49_2_bit    = (uint32_t)(1) << 22;
static const uint32_t DV_II_50_0_bit    = (uint32_t)(1) << 23;
static const uint32_t DV_II_50_2_bit    = (uint32_t)(1) << 24;
static const uint32_t DV_II_51_0_bit    = (uint32_t)(1) << 25;
static const uint32_t DV_II_51_2_bit    = (uint32_t)(1) << 26;
static const uint32_t DV_II_52_0_bit    = (uint32_t)(1) << 27;
static const uint32_t DV_II_53_0_bit    = (uint32_t)(1) << 28;
static const uint32_t DV_II_54_0_bit    = (uint32_t)(1) << 29;
static const uint32_t DV_II_55_0_bit    = (uint32_t)(1) << 30;
static const uint32_t DV_II_56_0_bit    = (uint32_t)(1) << 31;

dv_info_t sha1_dvs[] = {
    {1, 43, 0, 58, 0, 0, { 0x08000000, 0x9800000c, 0xd8000010, 0x08000010, 0xb8000010, 0x98000000, 0x60000000, 0x00000008, 0xc0000000, 0x90000014, 0x10000010, 0xb8000014, 0x28000000, 0x20000010, 0x48000000, 0x08000018, 0x60000000, 0x90000010, 0xf0000010, 0x90000008, 0xc0000000, 0x90000010, 0xf0000010, 0xb0000008, 0x40000000, 0x90000000, 0xf0000010, 0x90000018, 0x60000000, 0x90000010, 0x90000010, 0x90000000, 0x80000000, 0x00000010, 0xa0000000, 0x20000000, 0xa0000000, 0x20000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x20000000, 0x00000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000040, 0x40000002, 0x80000004, 0x80000080, 0x80000006, 0x00000049, 0x00000103, 0x80000009, 0x80000012, 0x80000202, 0x00000018, 0x00000164, 0x00000408, 0x800000e6, 0x8000004c, 0x00000803, 0x80000161, 0x80000599 } }
    , {1, 44, 0, 58, 0, 1, { 0xb4000008, 0x08000000, 0x9800000c, 0xd8000010, 0x08000010, 0xb8000010, 0x98000000, 0x60000000, 0x00000008, 0xc0000000, 0x90000014, 0x10000010, 0xb8000014, 0x28000000, 0x20000010, 0x48000000, 0x08000018, 0x60000000, 0x90000010, 0xf0000010, 0x90000008, 0xc0000000, 0x90000010, 0xf0000010, 0xb0000008, 0x40000000, 0x90000000, 0xf0000010, 0x90000018, 0x60000000, 0x90000010, 0x90000010, 0x90000000, 0x80000000, 0x00000010, 0xa0000000, 0x20000000, 0xa0000000, 0x20000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x20000000, 0x00000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000040, 0x40000002, 0x80000004, 0x80000080, 0x80000006, 0x00000049, 0x00000103, 0x80000009, 0x80000012, 0x80000202, 0x00000018, 0x00000164, 0x00000408, 0x800000e6, 0x8000004c, 0x00000803, 0x80000161 } }
    , {1, 45, 0, 58, 0, 2, { 0xf4000014, 0xb4000008, 0x08000000, 0x9800000c, 0xd8000010, 0x08000010, 0xb8000010, 0x98000000, 0x60000000, 0x00000008, 0xc0000000, 0x90000014, 0x10000010, 0xb8000014, 0x28000000, 0x20000010, 0x48000000, 0x08000018, 0x60000000, 0x90000010, 0xf0000010, 0x90000008, 0xc0000000, 0x90000010, 0xf0000010, 0xb0000008, 0x40000000, 0x90000000, 0xf0000010, 0x90000018, 0x60000000, 0x90000010, 0x90000010, 0x90000000, 0x80000000, 0x00000010, 0xa0000000, 0x20000000, 0xa0000000, 0x20000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x20000000, 0x00000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000040, 0x40000002, 0x80000004, 0x80000080, 0x80000006, 0x00000049, 0x00000103, 0x80000009, 0x80000012, 0x80000202, 0x00000018, 0x00000164, 0x00000408, 0x800000e6, 0x8000004c, 0x00000803 } }
    , {1, 46, 0, 58, 0, 3, { 0x2c000010, 0xf4000014, 0xb4000008, 0x08000000, 0x9800000c, 0xd8000010, 0x08000010, 0xb8000010, 0x98000000, 0x60000000, 0x00000008, 0xc0000000, 0x90000014, 0x10000010, 0xb8000014, 0x28000000, 0x20000010, 0x48000000, 0x08000018, 0x60000000, 0x90000010, 0xf0000010, 0x90000008, 0xc0000000, 0x90000010, 0xf0000010, 0xb0000008, 0x40000000, 0x90000000, 0xf0000010, 0x90000018, 0x60000000, 0x90000010, 0x90000010, 0x90000000, 0x80000000, 0x00000010, 0xa0000000, 0x20000000, 0xa0000000, 0x20000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x20000000, 0x00000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000040, 0x40000002, 0x80000004, 0x80000080, 0x80000006, 0x00000049, 0x00000103, 0x80000009, 0x80000012, 0x80000202, 0x00000018, 0x00000164, 0x00000408, 0x800000e6, 0x8000004c } }
    , {1, 46, 2, 58, 0, 4, { 0xb0000040, 0xd0000053, 0xd0000022, 0x20000000, 0x60000032, 0x60000043, 0x20000040, 0xe0000042, 0x60000002, 0x80000001, 0x00000020, 0x00000003, 0x40000052, 0x40000040, 0xe0000052, 0xa0000000, 0x80000040, 0x20000001, 0x20000060, 0x80000001, 0x40000042, 0xc0000043, 0x40000022, 0x00000003, 0x40000042, 0xc0000043, 0xc0000022, 0x00000001, 0x40000002, 0xc0000043, 0x40000062, 0x80000001, 0x40000042, 0x40000042, 0x40000002, 0x00000002, 0x00000040, 0x80000002, 0x80000000, 0x80000002, 0x80000040, 0x00000000, 0x80000040, 0x80000000, 0x00000040, 0x80000000, 0x00000040, 0x80000002, 0x00000000, 0x80000000, 0x80000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000004, 0x00000080, 0x00000004, 0x00000009, 0x00000101, 0x00000009, 0x00000012, 0x00000202, 0x0000001a, 0x00000124, 0x0000040c, 0x00000026, 0x0000004a, 0x0000080a, 0x00000060, 0x00000590, 0x00001020, 0x0000039a, 0x00000132 } }
    , {1, 47, 0, 58, 0, 5, { 0xc8000010, 0x2c000010, 0xf4000014, 0xb4000008, 0x08000000, 0x9800000c, 0xd8000010, 0x08000010, 0xb8000010, 0x98000000, 0x60000000, 0x00000008, 0xc0000000, 0x90000014, 0x10000010, 0xb8000014, 0x28000000, 0x20000010, 0x48000000, 0x08000018, 0x60000000, 0x90000010, 0xf0000010, 0x90000008, 0xc0000000, 0x90000010, 0xf0000010, 0xb0000008, 0x40000000, 0x90000000, 0xf0000010, 0x90000018, 0x60000000, 0x90000010, 0x90000010, 0x90000000, 0x80000000, 0x00000010, 0xa0000000, 0x20000000, 0xa0000000, 0x20000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x20000000, 0x00000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000040, 0x40000002, 0x80000004, 0x80000080, 0x80000006, 0x00000049, 0x00000103, 0x80000009, 0x80000012, 0x80000202, 0x00000018, 0x00000164, 0x00000408, 0x800000e6 } }
    , {1, 47, 2, 58, 0, 6, { 0x20000043, 0xb0000040, 0xd0000053, 0xd0000022, 0x20000000, 0x60000032, 0x60000043, 0x20000040, 0xe0000042, 0x60000002, 0x80000001, 0x00000020, 0x00000003, 0x40000052, 0x40000040, 0xe0000052, 0xa0000000, 0x80000040, 0x20000001, 0x20000060, 0x80000001, 0x40000042, 0xc0000043, 0x40000022, 0x00000003, 0x40000042, 0xc0000043, 0xc0000022, 0x00000001, 0x40000002, 0xc0000043, 0x40000062, 0x80000001, 0x40000042, 0x40000042, 0x40000002, 0x00000002, 0x00000040, 0x80000002, 0x80000000, 0x80000002, 0x80000040, 0x00000000, 0x80000040, 0x80000000, 0x00000040, 0x80000000, 0x00000040, 0x80000002, 0x00000000, 0x80000000, 0x80000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000004, 0x00000080, 0x00000004, 0x00000009, 0x00000101, 0x00000009, 0x00000012, 0x00000202, 0x0000001a, 0x00000124, 0x0000040c, 0x00000026, 0x0000004a, 0x0000080a, 0x00000060, 0x00000590, 0x00001020, 0x0000039a } }
    , {1, 48, 0, 58, 0, 7, { 0xb800000a, 0xc8000010, 0x2c000010, 0xf4000014, 0xb4000008, 0x08000000, 0x9800000c, 0xd8000010, 0x08000010, 0xb8000010, 0x98000000, 0x60000000, 0x00000008, 0xc0000000, 0x90000014, 0x10000010, 0xb8000014, 0x28000000, 0x20000010, 0x48000000, 0x08000018, 0x60000000, 0x90000010, 0xf0000010, 0x90000008, 0xc0000000, 0x90000010, 0xf0000010, 0xb0000008, 0x40000000, 0x90000000, 0xf0000010, 0x90000018, 0x60000000, 0x90000010, 0x90000010, 0x90000000, 0x80000000, 0x00000010, 0xa0000000, 0x20000000, 0xa0000000, 0x20000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x20000000, 0x00000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000040, 0x40000002, 0x80000004, 0x80000080, 0x80000006, 0x00000049, 0x00000103, 0x80000009, 0x80000012, 0x80000202, 0x00000018, 0x00000164, 0x00000408 } }
    , {1, 48, 2, 58, 0, 8, { 0xe000002a, 0x20000043, 0xb0000040, 0xd0000053, 0xd0000022, 0x20000000, 0x60000032, 0x60000043, 0x20000040, 0xe0000042, 0x60000002, 0x80000001, 0x00000020, 0x00000003, 0x40000052, 0x40000040, 0xe0000052, 0xa0000000, 0x80000040, 0x20000001, 0x20000060, 0x80000001, 0x40000042, 0xc0000043, 0x40000022, 0x00000003, 0x40000042, 0xc0000043, 0xc0000022, 0x00000001, 0x40000002, 0xc0000043, 0x40000062, 0x80000001, 0x40000042, 0x40000042, 0x40000002, 0x00000002, 0x00000040, 0x80000002, 0x80000000, 0x80000002, 0x80000040, 0x00000000, 0x80000040, 0x80000000, 0x00000040, 0x80000000, 0x00000040, 0x80000002, 0x00000000, 0x80000000, 0x80000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000004, 0x00000080, 0x00000004, 0x00000009, 0x00000101, 0x00000009, 0x00000012, 0x00000202, 0x0000001a, 0x00000124, 0x0000040c, 0x00000026, 0x0000004a, 0x0000080a, 0x00000060, 0x00000590, 0x00001020 } }
    , {1, 49, 0, 58, 0, 9, { 0x18000000, 0xb800000a, 0xc8000010, 0x2c000010, 0xf4000014, 0xb4000008, 0x08000000, 0x9800000c, 0xd8000010, 0x08000010, 0xb8000010, 0x98000000, 0x60000000, 0x00000008, 0xc0000000, 0x90000014, 0x10000010, 0xb8000014, 0x28000000, 0x20000010, 0x48000000, 0x08000018, 0x60000000, 0x90000010, 0xf0000010, 0x90000008, 0xc0000000, 0x90000010, 0xf0000010, 0xb0000008, 0x40000000, 0x90000000, 0xf0000010, 0x90000018, 0x60000000, 0x90000010, 0x90000010, 0x90000000, 0x80000000, 0x00000010, 0xa0000000, 0x20000000, 0xa0000000, 0x20000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x20000000, 0x00000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000040, 0x40000002, 0x80000004, 0x80000080, 0x80000006, 0x00000049, 0x00000103, 0x80000009, 0x80000012, 0x80000202, 0x00000018, 0x00000164 } }
    , {1, 49, 2, 58, 0, 10, { 0x60000000, 0xe000002a, 0x20000043, 0xb0000040, 0xd0000053, 0xd0000022, 0x20000000, 0x60000032, 0x60000043, 0x20000040, 0xe0000042, 0x60000002, 0x80000001, 0x00000020, 0x00000003, 0x40000052, 0x40000040, 0xe0000052, 0xa0000000, 0x80000040, 0x20000001, 0x20000060, 0x80000001, 0x40000042, 0xc0000043, 0x40000022, 0x00000003, 0x40000042, 0xc0000043, 0xc0000022, 0x00000001, 0x40000002, 0xc0000043, 0x40000062, 0x80000001, 0x40000042, 0x40000042, 0x40000002, 0x00000002, 0x00000040, 0x80000002, 0x80000000, 0x80000002, 0x80000040, 0x00000000, 0x80000040, 0x80000000, 0x00000040, 0x80000000, 0x00000040, 0x80000002, 0x00000000, 0x80000000, 0x80000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000004, 0x00000080, 0x00000004, 0x00000009, 0x00000101, 0x00000009, 0x00000012, 0x00000202, 0x0000001a, 0x00000124, 0x0000040c, 0x00000026, 0x0000004a, 0x0000080a, 0x00000060, 0x00000590 } }
    , {1, 50, 0, 65, 0, 11, { 0x0800000c, 0x18000000, 0xb800000a, 0xc8000010, 0x2c000010, 0xf4000014, 0xb4000008, 0x08000000, 0x9800000c, 0xd8000010, 0x08000010, 0xb8000010, 0x98000000, 0x60000000, 0x00000008, 0xc0000000, 0x90000014, 0x10000010, 0xb8000014, 0x28000000, 0x20000010, 0x48000000, 0x08000018, 0x60000000, 0x90000010, 0xf0000010, 0x90000008, 0xc0000000, 0x90000010, 0xf0000010, 0xb0000008, 0x40000000, 0x90000000, 0xf0000010, 0x90000018, 0x60000000, 0x90000010, 0x90000010, 0x90000000, 0x80000000, 0x00000010, 0xa0000000, 0x20000000, 0xa0000000, 0x20000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x20000000, 0x00000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000040, 0x40000002, 0x80000004, 0x80000080, 0x80000006, 0x00000049, 0x00000103, 0x80000009, 0x80000012, 0x80000202, 0x00000018 } }
    , {1, 50, 2, 65, 0, 12, { 0x20000030, 0x60000000, 0xe000002a, 0x20000043, 0xb0000040, 0xd0000053, 0xd0000022, 0x20000000, 0x60000032, 0x60000043, 0x20000040, 0xe0000042, 0x60000002, 0x80000001, 0x00000020, 0x00000003, 0x40000052, 0x40000040, 0xe0000052, 0xa0000000, 0x80000040, 0x20000001, 0x20000060, 0x80000001, 0x40000042, 0xc0000043, 0x40000022, 0x00000003, 0x40000042, 0xc0000043, 0xc0000022, 0x00000001, 0x40000002, 0xc0000043, 0x40000062, 0x80000001, 0x40000042, 0x40000042, 0x40000002, 0x00000002, 0x00000040, 0x80000002, 0x80000000, 0x80000002, 0x80000040, 0x00000000, 0x80000040, 0x80000000, 0x00000040, 0x80000000, 0x00000040, 0x80000002, 0x00000000, 0x80000000, 0x80000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000004, 0x00000080, 0x00000004, 0x00000009, 0x00000101, 0x00000009, 0x00000012, 0x00000202, 0x0000001a, 0x00000124, 0x0000040c, 0x00000026, 0x0000004a, 0x0000080a, 0x00000060 } }
    , {1, 51, 0, 65, 0, 13, { 0xe8000000, 0x0800000c, 0x18000000, 0xb800000a, 0xc8000010, 0x2c000010, 0xf4000014, 0xb4000008, 0x08000000, 0x9800000c, 0xd8000010, 0x08000010, 0xb8000010, 0x98000000, 0x60000000, 0x00000008, 0xc0000000, 0x90000014, 0x10000010, 0xb8000014, 0x28000000, 0x20000010, 0x48000000, 0x08000018, 0x60000000, 0x90000010, 0xf0000010, 0x90000008, 0xc0000000, 0x90000010, 0xf0000010, 0xb0000008, 0x40000000, 0x90000000, 0xf0000010, 0x90000018, 0x60000000, 0x90000010, 0x90000010, 0x90000000, 0x80000000, 0x00000010, 0xa0000000, 0x20000000, 0xa0000000, 0x20000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x20000000, 0x00000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000040, 0x40000002, 0x80000004, 0x80000080, 0x80000006, 0x00000049, 0x00000103, 0x80000009, 0x80000012, 0x80000202 } }
    , {1, 51, 2, 65, 0, 14, { 0xa0000003, 0x20000030, 0x60000000, 0xe000002a, 0x20000043, 0xb0000040, 0xd0000053, 0xd0000022, 0x20000000, 0x60000032, 0x60000043, 0x20000040, 0xe0000042, 0x60000002, 0x80000001, 0x00000020, 0x00000003, 0x40000052, 0x40000040, 0xe0000052, 0xa0000000, 0x80000040, 0x20000001, 0x20000060, 0x80000001, 0x40000042, 0xc0000043, 0x40000022, 0x00000003, 0x40000042, 0xc0000043, 0xc0000022, 0x00000001, 0x40000002, 0xc0000043, 0x40000062, 0x80000001, 0x40000042, 0x40000042, 0x40000002, 0x00000002, 0x00000040, 0x80000002, 0x80000000, 0x80000002, 0x80000040, 0x00000000, 0x80000040, 0x80000000, 0x00000040, 0x80000000, 0x00000040, 0x80000002, 0x00000000, 0x80000000, 0x80000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000004, 0x00000080, 0x00000004, 0x00000009, 0x00000101, 0x00000009, 0x00000012, 0x00000202, 0x0000001a, 0x00000124, 0x0000040c, 0x00000026, 0x0000004a, 0x0000080a } }
    , {1, 52, 0, 65, 0, 15, { 0x04000010, 0xe8000000, 0x0800000c, 0x18000000, 0xb800000a, 0xc8000010, 0x2c000010, 0xf4000014, 0xb4000008, 0x08000000, 0x9800000c, 0xd8000010, 0x08000010, 0xb8000010, 0x98000000, 0x60000000, 0x00000008, 0xc0000000, 0x90000014, 0x10000010, 0xb8000014, 0x28000000, 0x20000010, 0x48000000, 0x08000018, 0x60000000, 0x90000010, 0xf0000010, 0x90000008, 0xc0000000, 0x90000010, 0xf0000010, 0xb0000008, 0x40000000, 0x90000000, 0xf0000010, 0x90000018, 0x60000000, 0x90000010, 0x90000010, 0x90000000, 0x80000000, 0x00000010, 0xa0000000, 0x20000000, 0xa0000000, 0x20000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x20000000, 0x00000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000040, 0x40000002, 0x80000004, 0x80000080, 0x80000006, 0x00000049, 0x00000103, 0x80000009, 0x80000012 } }
    , {2, 45, 0, 58, 0, 16, { 0xec000014, 0x0c000002, 0xc0000010, 0xb400001c, 0x2c000004, 0xbc000018, 0xb0000010, 0x0000000c, 0xb8000010, 0x08000018, 0x78000010, 0x08000014, 0x70000010, 0xb800001c, 0xe8000000, 0xb0000004, 0x58000010, 0xb000000c, 0x48000000, 0xb0000000, 0xb8000010, 0x98000010, 0xa0000000, 0x00000000, 0x00000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x60000000, 0x00000018, 0xe0000000, 0x90000000, 0x30000010, 0xb0000000, 0x20000000, 0x20000000, 0xa0000000, 0x00000010, 0x80000000, 0x20000000, 0x20000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000041, 0x40000022, 0x80000005, 0xc0000082, 0xc0000046, 0x4000004b, 0x80000107, 0x00000089, 0x00000014, 0x8000024b, 0x0000011b, 0x8000016d, 0x8000041a, 0x000002e4, 0x80000054, 0x00000967 } }
    , {2, 46, 0, 58, 0, 17, { 0x2400001c, 0xec000014, 0x0c000002, 0xc0000010, 0xb400001c, 0x2c000004, 0xbc000018, 0xb0000010, 0x0000000c, 0xb8000010, 0x08000018, 0x78000010, 0x08000014, 0x70000010, 0xb800001c, 0xe8000000, 0xb0000004, 0x58000010, 0xb000000c, 0x48000000, 0xb0000000, 0xb8000010, 0x98000010, 0xa0000000, 0x00000000, 0x00000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x60000000, 0x00000018, 0xe0000000, 0x90000000, 0x30000010, 0xb0000000, 0x20000000, 0x20000000, 0xa0000000, 0x00000010, 0x80000000, 0x20000000, 0x20000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000041, 0x40000022, 0x80000005, 0xc0000082, 0xc0000046, 0x4000004b, 0x80000107, 0x00000089, 0x00000014, 0x8000024b, 0x0000011b, 0x8000016d, 0x8000041a, 0x000002e4, 0x80000054 } }
    , {2, 46, 2, 58, 0, 18, { 0x90000070, 0xb0000053, 0x30000008, 0x00000043, 0xd0000072, 0xb0000010, 0xf0000062, 0xc0000042, 0x00000030, 0xe0000042, 0x20000060, 0xe0000041, 0x20000050, 0xc0000041, 0xe0000072, 0xa0000003, 0xc0000012, 0x60000041, 0xc0000032, 0x20000001, 0xc0000002, 0xe0000042, 0x60000042, 0x80000002, 0x00000000, 0x00000000, 0x80000000, 0x00000002, 0x00000040, 0x00000000, 0x80000040, 0x80000000, 0x00000040, 0x80000001, 0x00000060, 0x80000003, 0x40000002, 0xc0000040, 0xc0000002, 0x80000000, 0x80000000, 0x80000002, 0x00000040, 0x00000002, 0x80000000, 0x80000000, 0x80000000, 0x00000002, 0x00000040, 0x00000000, 0x80000040, 0x80000002, 0x00000000, 0x80000000, 0x80000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000004, 0x00000080, 0x00000004, 0x00000009, 0x00000105, 0x00000089, 0x00000016, 0x0000020b, 0x0000011b, 0x0000012d, 0x0000041e, 0x00000224, 0x00000050, 0x0000092e, 0x0000046c, 0x000005b6, 0x0000106a, 0x00000b90, 0x00000152 } }
    , {2, 47, 0, 58, 0, 19, { 0x20000010, 0x2400001c, 0xec000014, 0x0c000002, 0xc0000010, 0xb400001c, 0x2c000004, 0xbc000018, 0xb0000010, 0x0000000c, 0xb8000010, 0x08000018, 0x78000010, 0x08000014, 0x70000010, 0xb800001c, 0xe8000000, 0xb0000004, 0x58000010, 0xb000000c, 0x48000000, 0xb0000000, 0xb8000010, 0x98000010, 0xa0000000, 0x00000000, 0x00000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x60000000, 0x00000018, 0xe0000000, 0x90000000, 0x30000010, 0xb0000000, 0x20000000, 0x20000000, 0xa0000000, 0x00000010, 0x80000000, 0x20000000, 0x20000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000041, 0x40000022, 0x80000005, 0xc0000082, 0xc0000046, 0x4000004b, 0x80000107, 0x00000089, 0x00000014, 0x8000024b, 0x0000011b, 0x8000016d, 0x8000041a, 0x000002e4 } }
    , {2, 48, 0, 58, 0, 20, { 0xbc00001a, 0x20000010, 0x2400001c, 0xec000014, 0x0c000002, 0xc0000010, 0xb400001c, 0x2c000004, 0xbc000018, 0xb0000010, 0x0000000c, 0xb8000010, 0x08000018, 0x78000010, 0x08000014, 0x70000010, 0xb800001c, 0xe8000000, 0xb0000004, 0x58000010, 0xb000000c, 0x48000000, 0xb0000000, 0xb8000010, 0x98000010, 0xa0000000, 0x00000000, 0x00000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x60000000, 0x00000018, 0xe0000000, 0x90000000, 0x30000010, 0xb0000000, 0x20000000, 0x20000000, 0xa0000000, 0x00000010, 0x80000000, 0x20000000, 0x20000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000041, 0x40000022, 0x80000005, 0xc0000082, 0xc0000046, 0x4000004b, 0x80000107, 0x00000089, 0x00000014, 0x8000024b, 0x0000011b, 0x8000016d, 0x8000041a } }
    , {2, 49, 0, 58, 0, 21, { 0x3c000004, 0xbc00001a, 0x20000010, 0x2400001c, 0xec000014, 0x0c000002, 0xc0000010, 0xb400001c, 0x2c000004, 0xbc000018, 0xb0000010, 0x0000000c, 0xb8000010, 0x08000018, 0x78000010, 0x08000014, 0x70000010, 0xb800001c, 0xe8000000, 0xb0000004, 0x58000010, 0xb000000c, 0x48000000, 0xb0000000, 0xb8000010, 0x98000010, 0xa0000000, 0x00000000, 0x00000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x60000000, 0x00000018, 0xe0000000, 0x90000000, 0x30000010, 0xb0000000, 0x20000000, 0x20000000, 0xa0000000, 0x00000010, 0x80000000, 0x20000000, 0x20000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000041, 0x40000022, 0x80000005, 0xc0000082, 0xc0000046, 0x4000004b, 0x80000107, 0x00000089, 0x00000014, 0x8000024b, 0x0000011b, 0x8000016d } }
    , {2, 49, 2, 58, 0, 22, { 0xf0000010, 0xf000006a, 0x80000040, 0x90000070, 0xb0000053, 0x30000008, 0x00000043, 0xd0000072, 0xb0000010, 0xf0000062, 0xc0000042, 0x00000030, 0xe0000042, 0x20000060, 0xe0000041, 0x20000050, 0xc0000041, 0xe0000072, 0xa0000003, 0xc0000012, 0x60000041, 0xc0000032, 0x20000001, 0xc0000002, 0xe0000042, 0x60000042, 0x80000002, 0x00000000, 0x00000000, 0x80000000, 0x00000002, 0x00000040, 0x00000000, 0x80000040, 0x80000000, 0x00000040, 0x80000001, 0x00000060, 0x80000003, 0x40000002, 0xc0000040, 0xc0000002, 0x80000000, 0x80000000, 0x80000002, 0x00000040, 0x00000002, 0x80000000, 0x80000000, 0x80000000, 0x00000002, 0x00000040, 0x00000000, 0x80000040, 0x80000002, 0x00000000, 0x80000000, 0x80000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000004, 0x00000080, 0x00000004, 0x00000009, 0x00000105, 0x00000089, 0x00000016, 0x0000020b, 0x0000011b, 0x0000012d, 0x0000041e, 0x00000224, 0x00000050, 0x0000092e, 0x0000046c, 0x000005b6 } }
    , {2, 50, 0, 65, 0, 23, { 0xb400001c, 0x3c000004, 0xbc00001a, 0x20000010, 0x2400001c, 0xec000014, 0x0c000002, 0xc0000010, 0xb400001c, 0x2c000004, 0xbc000018, 0xb0000010, 0x0000000c, 0xb8000010, 0x08000018, 0x78000010, 0x08000014, 0x70000010, 0xb800001c, 0xe8000000, 0xb0000004, 0x58000010, 0xb000000c, 0x48000000, 0xb0000000, 0xb8000010, 0x98000010, 0xa0000000, 0x00000000, 0x00000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x60000000, 0x00000018, 0xe0000000, 0x90000000, 0x30000010, 0xb0000000, 0x20000000, 0x20000000, 0xa0000000, 0x00000010, 0x80000000, 0x20000000, 0x20000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000041, 0x40000022, 0x80000005, 0xc0000082, 0xc0000046, 0x4000004b, 0x80000107, 0x00000089, 0x00000014, 0x8000024b, 0x0000011b } }
    , {2, 50, 2, 65, 0, 24, { 0xd0000072, 0xf0000010, 0xf000006a, 0x80000040, 0x90000070, 0xb0000053, 0x30000008, 0x00000043, 0xd0000072, 0xb0000010, 0xf0000062, 0xc0000042, 0x00000030, 0xe0000042, 0x20000060, 0xe0000041, 0x20000050, 0xc0000041, 0xe0000072, 0xa0000003, 0xc0000012, 0x60000041, 0xc0000032, 0x20000001, 0xc0000002, 0xe0000042, 0x60000042, 0x80000002, 0x00000000, 0x00000000, 0x80000000, 0x00000002, 0x00000040, 0x00000000, 0x80000040, 0x80000000, 0x00000040, 0x80000001, 0x00000060, 0x80000003, 0x40000002, 0xc0000040, 0xc0000002, 0x80000000, 0x80000000, 0x80000002, 0x00000040, 0x00000002, 0x80000000, 0x80000000, 0x80000000, 0x00000002, 0x00000040, 0x00000000, 0x80000040, 0x80000002, 0x00000000, 0x80000000, 0x80000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000004, 0x00000080, 0x00000004, 0x00000009, 0x00000105, 0x00000089, 0x00000016, 0x0000020b, 0x0000011b, 0x0000012d, 0x0000041e, 0x00000224, 0x00000050, 0x0000092e, 0x0000046c } }
    , {2, 51, 0, 65, 0, 25, { 0xc0000010, 0xb400001c, 0x3c000004, 0xbc00001a, 0x20000010, 0x2400001c, 0xec000014, 0x0c000002, 0xc0000010, 0xb400001c, 0x2c000004, 0xbc000018, 0xb0000010, 0x0000000c, 0xb8000010, 0x08000018, 0x78000010, 0x08000014, 0x70000010, 0xb800001c, 0xe8000000, 0xb0000004, 0x58000010, 0xb000000c, 0x48000000, 0xb0000000, 0xb8000010, 0x98000010, 0xa0000000, 0x00000000, 0x00000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x60000000, 0x00000018, 0xe0000000, 0x90000000, 0x30000010, 0xb0000000, 0x20000000, 0x20000000, 0xa0000000, 0x00000010, 0x80000000, 0x20000000, 0x20000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000041, 0x40000022, 0x80000005, 0xc0000082, 0xc0000046, 0x4000004b, 0x80000107, 0x00000089, 0x00000014, 0x8000024b } }
    , {2, 51, 2, 65, 0, 26, { 0x00000043, 0xd0000072, 0xf0000010, 0xf000006a, 0x80000040, 0x90000070, 0xb0000053, 0x30000008, 0x00000043, 0xd0000072, 0xb0000010, 0xf0000062, 0xc0000042, 0x00000030, 0xe0000042, 0x20000060, 0xe0000041, 0x20000050, 0xc0000041, 0xe0000072, 0xa0000003, 0xc0000012, 0x60000041, 0xc0000032, 0x20000001, 0xc0000002, 0xe0000042, 0x60000042, 0x80000002, 0x00000000, 0x00000000, 0x80000000, 0x00000002, 0x00000040, 0x00000000, 0x80000040, 0x80000000, 0x00000040, 0x80000001, 0x00000060, 0x80000003, 0x40000002, 0xc0000040, 0xc0000002, 0x80000000, 0x80000000, 0x80000002, 0x00000040, 0x00000002, 0x80000000, 0x80000000, 0x80000000, 0x00000002, 0x00000040, 0x00000000, 0x80000040, 0x80000002, 0x00000000, 0x80000000, 0x80000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000004, 0x00000080, 0x00000004, 0x00000009, 0x00000105, 0x00000089, 0x00000016, 0x0000020b, 0x0000011b, 0x0000012d, 0x0000041e, 0x00000224, 0x00000050, 0x0000092e } }
    , {2, 52, 0, 65, 0, 27, { 0x0c000002, 0xc0000010, 0xb400001c, 0x3c000004, 0xbc00001a, 0x20000010, 0x2400001c, 0xec000014, 0x0c000002, 0xc0000010, 0xb400001c, 0x2c000004, 0xbc000018, 0xb0000010, 0x0000000c, 0xb8000010, 0x08000018, 0x78000010, 0x08000014, 0x70000010, 0xb800001c, 0xe8000000, 0xb0000004, 0x58000010, 0xb000000c, 0x48000000, 0xb0000000, 0xb8000010, 0x98000010, 0xa0000000, 0x00000000, 0x00000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x60000000, 0x00000018, 0xe0000000, 0x90000000, 0x30000010, 0xb0000000, 0x20000000, 0x20000000, 0xa0000000, 0x00000010, 0x80000000, 0x20000000, 0x20000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000041, 0x40000022, 0x80000005, 0xc0000082, 0xc0000046, 0x4000004b, 0x80000107, 0x00000089, 0x00000014 } }
    , {2, 53, 0, 65, 0, 28, { 0xcc000014, 0x0c000002, 0xc0000010, 0xb400001c, 0x3c000004, 0xbc00001a, 0x20000010, 0x2400001c, 0xec000014, 0x0c000002, 0xc0000010, 0xb400001c, 0x2c000004, 0xbc000018, 0xb0000010, 0x0000000c, 0xb8000010, 0x08000018, 0x78000010, 0x08000014, 0x70000010, 0xb800001c, 0xe8000000, 0xb0000004, 0x58000010, 0xb000000c, 0x48000000, 0xb0000000, 0xb8000010, 0x98000010, 0xa0000000, 0x00000000, 0x00000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x60000000, 0x00000018, 0xe0000000, 0x90000000, 0x30000010, 0xb0000000, 0x20000000, 0x20000000, 0xa0000000, 0x00000010, 0x80000000, 0x20000000, 0x20000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000041, 0x40000022, 0x80000005, 0xc0000082, 0xc0000046, 0x4000004b, 0x80000107, 0x00000089 } }
    , {2, 54, 0, 65, 0, 29, { 0x0400001c, 0xcc000014, 0x0c000002, 0xc0000010, 0xb400001c, 0x3c000004, 0xbc00001a, 0x20000010, 0x2400001c, 0xec000014, 0x0c000002, 0xc0000010, 0xb400001c, 0x2c000004, 0xbc000018, 0xb0000010, 0x0000000c, 0xb8000010, 0x08000018, 0x78000010, 0x08000014, 0x70000010, 0xb800001c, 0xe8000000, 0xb0000004, 0x58000010, 0xb000000c, 0x48000000, 0xb0000000, 0xb8000010, 0x98000010, 0xa0000000, 0x00000000, 0x00000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x60000000, 0x00000018, 0xe0000000, 0x90000000, 0x30000010, 0xb0000000, 0x20000000, 0x20000000, 0xa0000000, 0x00000010, 0x80000000, 0x20000000, 0x20000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000041, 0x40000022, 0x80000005, 0xc0000082, 0xc0000046, 0x4000004b, 0x80000107 } }
    , {2, 55, 0, 65, 0, 30, { 0x00000010, 0x0400001c, 0xcc000014, 0x0c000002, 0xc0000010, 0xb400001c, 0x3c000004, 0xbc00001a, 0x20000010, 0x2400001c, 0xec000014, 0x0c000002, 0xc0000010, 0xb400001c, 0x2c000004, 0xbc000018, 0xb0000010, 0x0000000c, 0xb8000010, 0x08000018, 0x78000010, 0x08000014, 0x70000010, 0xb800001c, 0xe8000000, 0xb0000004, 0x58000010, 0xb000000c, 0x48000000, 0xb0000000, 0xb8000010, 0x98000010, 0xa0000000, 0x00000000, 0x00000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x60000000, 0x00000018, 0xe0000000, 0x90000000, 0x30000010, 0xb0000000, 0x20000000, 0x20000000, 0xa0000000, 0x00000010, 0x80000000, 0x20000000, 0x20000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000041, 0x40000022, 0x80000005, 0xc0000082, 0xc0000046, 0x4000004b } }
    , {2, 56, 0, 65, 0, 31, { 0x2600001a, 0x00000010, 0x0400001c, 0xcc000014, 0x0c000002, 0xc0000010, 0xb400001c, 0x3c000004, 0xbc00001a, 0x20000010, 0x2400001c, 0xec000014, 0x0c000002, 0xc0000010, 0xb400001c, 0x2c000004, 0xbc000018, 0xb0000010, 0x0000000c, 0xb8000010, 0x08000018, 0x78000010, 0x08000014, 0x70000010, 0xb800001c, 0xe8000000, 0xb0000004, 0x58000010, 0xb000000c, 0x48000000, 0xb0000000, 0xb8000010, 0x98000010, 0xa0000000, 0x00000000, 0x00000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0x20000000, 0x00000010, 0x60000000, 0x00000018, 0xe0000000, 0x90000000, 0x30000010, 0xb0000000, 0x20000000, 0x20000000, 0xa0000000, 0x00000010, 0x80000000, 0x20000000, 0x20000000, 0x20000000, 0x80000000, 0x00000010, 0x00000000, 0x20000010, 0xa0000000, 0x00000000, 0x20000000, 0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000020, 0x00000001, 0x40000002, 0x40000041, 0x40000022, 0x80000005, 0xc0000082, 0xc0000046 } }
    , {0, 0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}
};
void ubc_check(const uint32_t W[80], uint32_t dvmask[1]) {
    uint32_t mask = ~((uint32_t)(0));
    mask &= (((((W[44] ^ W[45]) >> 29) & 1) - 1) | ~(DV_I_48_0_bit | DV_I_51_0_bit | DV_I_52_0_bit |
                                                     DV_II_45_0_bit | DV_II_46_0_bit | DV_II_50_0_bit | DV_II_51_0_bit));
    mask &= (((((W[49] ^ W[50]) >> 29) & 1) - 1) | ~(DV_I_46_0_bit | DV_II_45_0_bit | DV_II_50_0_bit |
                                                     DV_II_51_0_bit | DV_II_55_0_bit | DV_II_56_0_bit));
    mask &= (((((W[48] ^ W[49]) >> 29) & 1) - 1) | ~(DV_I_45_0_bit | DV_I_52_0_bit | DV_II_49_0_bit |
                                                     DV_II_50_0_bit | DV_II_54_0_bit | DV_II_55_0_bit));
    mask &= ((((W[47] ^ (W[50] >> 25)) & (1 << 4)) - (1 << 4)) | ~(DV_I_47_0_bit | DV_I_49_0_bit |
             DV_I_51_0_bit | DV_II_45_0_bit | DV_II_51_0_bit | DV_II_56_0_bit));
    mask &= (((((W[47] ^ W[48]) >> 29) & 1) - 1) | ~(DV_I_44_0_bit | DV_I_51_0_bit | DV_II_48_0_bit |
                                                     DV_II_49_0_bit | DV_II_53_0_bit | DV_II_54_0_bit));
    mask &= (((((W[46] >> 4) ^ (W[49] >> 29)) & 1) - 1) | ~(DV_I_46_0_bit | DV_I_48_0_bit |
                                                            DV_I_50_0_bit | DV_I_52_0_bit | DV_II_50_0_bit | DV_II_55_0_bit));
    mask &= (((((W[46] ^ W[47]) >> 29) & 1) - 1) | ~(DV_I_43_0_bit | DV_I_50_0_bit | DV_II_47_0_bit |
                                                     DV_II_48_0_bit | DV_II_52_0_bit | DV_II_53_0_bit));
    mask &= (((((W[45] >> 4) ^ (W[48] >> 29)) & 1) - 1) | ~(DV_I_45_0_bit | DV_I_47_0_bit |
                                                            DV_I_49_0_bit | DV_I_51_0_bit | DV_II_49_0_bit | DV_II_54_0_bit));
    mask &= (((((W[45] ^ W[46]) >> 29) & 1) - 1) | ~(DV_I_49_0_bit | DV_I_52_0_bit | DV_II_46_0_bit |
                                                     DV_II_47_0_bit | DV_II_51_0_bit | DV_II_52_0_bit));
    mask &= (((((W[44] >> 4) ^ (W[47] >> 29)) & 1) - 1) | ~(DV_I_44_0_bit | DV_I_46_0_bit |
                                                            DV_I_48_0_bit | DV_I_50_0_bit | DV_II_48_0_bit | DV_II_53_0_bit));
    mask &= (((((W[43] >> 4) ^ (W[46] >> 29)) & 1) - 1) | ~(DV_I_43_0_bit | DV_I_45_0_bit |
                                                            DV_I_47_0_bit | DV_I_49_0_bit | DV_II_47_0_bit | DV_II_52_0_bit));
    mask &= (((((W[43] ^ W[44]) >> 29) & 1) - 1) | ~(DV_I_47_0_bit | DV_I_50_0_bit | DV_I_51_0_bit |
                                                     DV_II_45_0_bit | DV_II_49_0_bit | DV_II_50_0_bit));
    mask &= (((((W[42] >> 4) ^ (W[45] >> 29)) & 1) - 1) | ~(DV_I_44_0_bit | DV_I_46_0_bit |
                                                            DV_I_48_0_bit | DV_I_52_0_bit | DV_II_46_0_bit | DV_II_51_0_bit));
    mask &= (((((W[41] >> 4) ^ (W[44] >> 29)) & 1) - 1) | ~(DV_I_43_0_bit | DV_I_45_0_bit |
                                                            DV_I_47_0_bit | DV_I_51_0_bit | DV_II_45_0_bit | DV_II_50_0_bit));
    mask &= (((((W[40] ^ W[41]) >> 29) & 1) - 1) | ~(DV_I_44_0_bit | DV_I_47_0_bit | DV_I_48_0_bit |
                                                     DV_II_46_0_bit | DV_II_47_0_bit | DV_II_56_0_bit));
    mask &= (((((W[54] ^ W[55]) >> 29) & 1) - 1) | ~(DV_I_51_0_bit | DV_II_47_0_bit | DV_II_50_0_bit |
                                                     DV_II_55_0_bit | DV_II_56_0_bit));
    mask &= (((((W[53] ^ W[54]) >> 29) & 1) - 1) | ~(DV_I_50_0_bit | DV_II_46_0_bit | DV_II_49_0_bit |
                                                     DV_II_54_0_bit | DV_II_55_0_bit));
    mask &= (((((W[52] ^ W[53]) >> 29) & 1) - 1) | ~(DV_I_49_0_bit | DV_II_45_0_bit | DV_II_48_0_bit |
                                                     DV_II_53_0_bit | DV_II_54_0_bit));
    mask &= ((((W[50] ^ (W[53] >> 25)) & (1 << 4)) - (1 << 4)) | ~(DV_I_50_0_bit | DV_I_52_0_bit |
             DV_II_46_0_bit | DV_II_48_0_bit | DV_II_54_0_bit));
    mask &= (((((W[50] ^ W[51]) >> 29) & 1) - 1) | ~(DV_I_47_0_bit | DV_II_46_0_bit | DV_II_51_0_bit |
                                                     DV_II_52_0_bit | DV_II_56_0_bit));
    mask &= ((((W[49] ^ (W[52] >> 25)) & (1 << 4)) - (1 << 4)) | ~(DV_I_49_0_bit | DV_I_51_0_bit |
             DV_II_45_0_bit | DV_II_47_0_bit | DV_II_53_0_bit));
    mask &= ((((W[48] ^ (W[51] >> 25)) & (1 << 4)) - (1 << 4)) | ~(DV_I_48_0_bit | DV_I_50_0_bit |
             DV_I_52_0_bit | DV_II_46_0_bit | DV_II_52_0_bit));
    mask &= (((((W[42] ^ W[43]) >> 29) & 1) - 1) | ~(DV_I_46_0_bit | DV_I_49_0_bit | DV_I_50_0_bit |
                                                     DV_II_48_0_bit | DV_II_49_0_bit));
    mask &= (((((W[41] ^ W[42]) >> 29) & 1) - 1) | ~(DV_I_45_0_bit | DV_I_48_0_bit | DV_I_49_0_bit |
                                                     DV_II_47_0_bit | DV_II_48_0_bit));
    mask &= (((((W[40] >> 4) ^ (W[43] >> 29)) & 1) - 1) | ~(DV_I_44_0_bit | DV_I_46_0_bit |
                                                            DV_I_50_0_bit | DV_II_49_0_bit | DV_II_56_0_bit));
    mask &= (((((W[39] >> 4) ^ (W[42] >> 29)) & 1) - 1) | ~(DV_I_43_0_bit | DV_I_45_0_bit |
                                                            DV_I_49_0_bit | DV_II_48_0_bit | DV_II_55_0_bit));
    if (mask & (DV_I_44_0_bit | DV_I_48_0_bit | DV_II_47_0_bit | DV_II_54_0_bit | DV_II_56_0_bit))
        mask &= (((((W[38] >> 4) ^ (W[41] >> 29)) & 1) - 1) | ~(DV_I_44_0_bit | DV_I_48_0_bit |
                                                                DV_II_47_0_bit | DV_II_54_0_bit | DV_II_56_0_bit));
    mask &= (((((W[37] >> 4) ^ (W[40] >> 29)) & 1) - 1) | ~(DV_I_43_0_bit | DV_I_47_0_bit |
                                                            DV_II_46_0_bit | DV_II_53_0_bit | DV_II_55_0_bit));
    if (mask & (DV_I_52_0_bit | DV_II_48_0_bit | DV_II_51_0_bit | DV_II_56_0_bit))
        mask &= (((((W[55] ^ W[56]) >> 29) & 1) - 1) | ~(DV_I_52_0_bit | DV_II_48_0_bit | DV_II_51_0_bit |
                                                         DV_II_56_0_bit));
    if (mask & (DV_I_52_0_bit | DV_II_48_0_bit | DV_II_50_0_bit | DV_II_56_0_bit))
        mask &= ((((W[52] ^ (W[55] >> 25)) & (1 << 4)) - (1 << 4)) | ~(DV_I_52_0_bit | DV_II_48_0_bit |
                 DV_II_50_0_bit | DV_II_56_0_bit));
    if (mask & (DV_I_51_0_bit | DV_II_47_0_bit | DV_II_49_0_bit | DV_II_55_0_bit))
        mask &= ((((W[51] ^ (W[54] >> 25)) & (1 << 4)) - (1 << 4)) | ~(DV_I_51_0_bit | DV_II_47_0_bit |
                 DV_II_49_0_bit | DV_II_55_0_bit));
    if (mask & (DV_I_48_0_bit | DV_II_47_0_bit | DV_II_52_0_bit | DV_II_53_0_bit))
        mask &= (((((W[51] ^ W[52]) >> 29) & 1) - 1) | ~(DV_I_48_0_bit | DV_II_47_0_bit | DV_II_52_0_bit |
                                                         DV_II_53_0_bit));
    if (mask & (DV_I_46_0_bit | DV_I_49_0_bit | DV_II_45_0_bit | DV_II_48_0_bit))
        mask &= (((((W[36] >> 4) ^ (W[40] >> 29)) & 1) - 1) | ~(DV_I_46_0_bit | DV_I_49_0_bit |
                                                                DV_II_45_0_bit | DV_II_48_0_bit));
    if (mask & (DV_I_52_0_bit | DV_II_48_0_bit | DV_II_49_0_bit))
        mask &= ((0 - (((W[53] ^ W[56]) >> 29) & 1)) | ~(DV_I_52_0_bit | DV_II_48_0_bit | DV_II_49_0_bit));
    if (mask & (DV_I_50_0_bit | DV_II_46_0_bit | DV_II_47_0_bit))
        mask &= ((0 - (((W[51] ^ W[54]) >> 29) & 1)) | ~(DV_I_50_0_bit | DV_II_46_0_bit | DV_II_47_0_bit));
    if (mask & (DV_I_49_0_bit | DV_I_51_0_bit | DV_II_45_0_bit))
        mask &= ((0 - (((W[50] ^ W[52]) >> 29) & 1)) | ~(DV_I_49_0_bit | DV_I_51_0_bit | DV_II_45_0_bit));
    if (mask & (DV_I_48_0_bit | DV_I_50_0_bit | DV_I_52_0_bit))
        mask &= ((0 - (((W[49] ^ W[51]) >> 29) & 1)) | ~(DV_I_48_0_bit | DV_I_50_0_bit | DV_I_52_0_bit));
    if (mask & (DV_I_47_0_bit | DV_I_49_0_bit | DV_I_51_0_bit))
        mask &= ((0 - (((W[48] ^ W[50]) >> 29) & 1)) | ~(DV_I_47_0_bit | DV_I_49_0_bit | DV_I_51_0_bit));
    if (mask & (DV_I_46_0_bit | DV_I_48_0_bit | DV_I_50_0_bit))
        mask &= ((0 - (((W[47] ^ W[49]) >> 29) & 1)) | ~(DV_I_46_0_bit | DV_I_48_0_bit | DV_I_50_0_bit));
    if (mask & (DV_I_45_0_bit | DV_I_47_0_bit | DV_I_49_0_bit))
        mask &= ((0 - (((W[46] ^ W[48]) >> 29) & 1)) | ~(DV_I_45_0_bit | DV_I_47_0_bit | DV_I_49_0_bit));
    mask &= ((((W[45] ^ W[47]) & (1 << 6)) - (1 << 6)) | ~(DV_I_47_2_bit | DV_I_49_2_bit |
                                                           DV_I_51_2_bit));
    if (mask & (DV_I_44_0_bit | DV_I_46_0_bit | DV_I_48_0_bit))
        mask &= ((0 - (((W[45] ^ W[47]) >> 29) & 1)) | ~(DV_I_44_0_bit | DV_I_46_0_bit | DV_I_48_0_bit));
    mask &= (((((W[44] ^ W[46]) >> 6) & 1) - 1) | ~(DV_I_46_2_bit | DV_I_48_2_bit | DV_I_50_2_bit));
    if (mask & (DV_I_43_0_bit | DV_I_45_0_bit | DV_I_47_0_bit))
        mask &= ((0 - (((W[44] ^ W[46]) >> 29) & 1)) | ~(DV_I_43_0_bit | DV_I_45_0_bit | DV_I_47_0_bit));
    mask &= ((0 - ((W[41] ^ (W[42] >> 5)) & (1 << 1))) | ~(DV_I_48_2_bit | DV_II_46_2_bit |
                                                           DV_II_51_2_bit));
    mask &= ((0 - ((W[40] ^ (W[41] >> 5)) & (1 << 1))) | ~(DV_I_47_2_bit | DV_I_51_2_bit |
                                                           DV_II_50_2_bit));
    if (mask & (DV_I_44_0_bit | DV_I_46_0_bit | DV_II_56_0_bit))
        mask &= ((0 - (((W[40] ^ W[42]) >> 4) & 1)) | ~(DV_I_44_0_bit | DV_I_46_0_bit | DV_II_56_0_bit));
    mask &= ((0 - ((W[39] ^ (W[40] >> 5)) & (1 << 1))) | ~(DV_I_46_2_bit | DV_I_50_2_bit |
                                                           DV_II_49_2_bit));
    if (mask & (DV_I_43_0_bit | DV_I_45_0_bit | DV_II_55_0_bit))
        mask &= ((0 - (((W[39] ^ W[41]) >> 4) & 1)) | ~(DV_I_43_0_bit | DV_I_45_0_bit | DV_II_55_0_bit));
    if (mask & (DV_I_44_0_bit | DV_II_54_0_bit | DV_II_56_0_bit))
        mask &= ((0 - (((W[38] ^ W[40]) >> 4) & 1)) | ~(DV_I_44_0_bit | DV_II_54_0_bit | DV_II_56_0_bit));
    if (mask & (DV_I_43_0_bit | DV_II_53_0_bit | DV_II_55_0_bit))
        mask &= ((0 - (((W[37] ^ W[39]) >> 4) & 1)) | ~(DV_I_43_0_bit | DV_II_53_0_bit | DV_II_55_0_bit));
    mask &= ((0 - ((W[36] ^ (W[37] >> 5)) & (1 << 1))) | ~(DV_I_47_2_bit | DV_I_50_2_bit |
                                                           DV_II_46_2_bit));
    if (mask & (DV_I_45_0_bit | DV_I_48_0_bit | DV_II_47_0_bit))
        mask &= (((((W[35] >> 4) ^ (W[39] >> 29)) & 1) - 1) | ~(DV_I_45_0_bit | DV_I_48_0_bit |
                                                                DV_II_47_0_bit));
    if (mask & (DV_I_48_0_bit | DV_II_48_0_bit))
        mask &= ((0 - ((W[63] ^ (W[64] >> 5)) & (1 << 0))) | ~(DV_I_48_0_bit | DV_II_48_0_bit));
    if (mask & (DV_I_45_0_bit | DV_II_45_0_bit))
        mask &= ((0 - ((W[63] ^ (W[64] >> 5)) & (1 << 1))) | ~(DV_I_45_0_bit | DV_II_45_0_bit));
    if (mask & (DV_I_47_0_bit | DV_II_47_0_bit))
        mask &= ((0 - ((W[62] ^ (W[63] >> 5)) & (1 << 0))) | ~(DV_I_47_0_bit | DV_II_47_0_bit));
    if (mask & (DV_I_46_0_bit | DV_II_46_0_bit))
        mask &= ((0 - ((W[61] ^ (W[62] >> 5)) & (1 << 0))) | ~(DV_I_46_0_bit | DV_II_46_0_bit));
    mask &= ((0 - ((W[61] ^ (W[62] >> 5)) & (1 << 2))) | ~(DV_I_46_2_bit | DV_II_46_2_bit));
    if (mask & (DV_I_45_0_bit | DV_II_45_0_bit))
        mask &= ((0 - ((W[60] ^ (W[61] >> 5)) & (1 << 0))) | ~(DV_I_45_0_bit | DV_II_45_0_bit));
    if (mask & (DV_II_51_0_bit | DV_II_54_0_bit))
        mask &= (((((W[58] ^ W[59]) >> 29) & 1) - 1) | ~(DV_II_51_0_bit | DV_II_54_0_bit));
    if (mask & (DV_II_50_0_bit | DV_II_53_0_bit))
        mask &= (((((W[57] ^ W[58]) >> 29) & 1) - 1) | ~(DV_II_50_0_bit | DV_II_53_0_bit));
    if (mask & (DV_II_52_0_bit | DV_II_54_0_bit))
        mask &= ((((W[56] ^ (W[59] >> 25)) & (1 << 4)) - (1 << 4)) | ~(DV_II_52_0_bit | DV_II_54_0_bit));
    if (mask & (DV_II_51_0_bit | DV_II_52_0_bit))
        mask &= ((0 - (((W[56] ^ W[59]) >> 29) & 1)) | ~(DV_II_51_0_bit | DV_II_52_0_bit));
    if (mask & (DV_II_49_0_bit | DV_II_52_0_bit))
        mask &= (((((W[56] ^ W[57]) >> 29) & 1) - 1) | ~(DV_II_49_0_bit | DV_II_52_0_bit));
    if (mask & (DV_II_51_0_bit | DV_II_53_0_bit))
        mask &= ((((W[55] ^ (W[58] >> 25)) & (1 << 4)) - (1 << 4)) | ~(DV_II_51_0_bit | DV_II_53_0_bit));
    if (mask & (DV_II_50_0_bit | DV_II_52_0_bit))
        mask &= ((((W[54] ^ (W[57] >> 25)) & (1 << 4)) - (1 << 4)) | ~(DV_II_50_0_bit | DV_II_52_0_bit));
    if (mask & (DV_II_49_0_bit | DV_II_51_0_bit))
        mask &= ((((W[53] ^ (W[56] >> 25)) & (1 << 4)) - (1 << 4)) | ~(DV_II_49_0_bit | DV_II_51_0_bit));
    mask &= ((((W[51] ^ (W[50] >> 5)) & (1 << 1)) - (1 << 1)) | ~(DV_I_50_2_bit | DV_II_46_2_bit));
    mask &= ((((W[48] ^ W[50]) & (1 << 6)) - (1 << 6)) | ~(DV_I_50_2_bit | DV_II_46_2_bit));
    if (mask & (DV_I_51_0_bit | DV_I_52_0_bit))
        mask &= ((0 - (((W[48] ^ W[55]) >> 29) & 1)) | ~(DV_I_51_0_bit | DV_I_52_0_bit));
    mask &= ((((W[47] ^ W[49]) & (1 << 6)) - (1 << 6)) | ~(DV_I_49_2_bit | DV_I_51_2_bit));
    mask &= ((((W[48] ^ (W[47] >> 5)) & (1 << 1)) - (1 << 1)) | ~(DV_I_47_2_bit | DV_II_51_2_bit));
    mask &= ((((W[46] ^ W[48]) & (1 << 6)) - (1 << 6)) | ~(DV_I_48_2_bit | DV_I_50_2_bit));
    mask &= ((((W[47] ^ (W[46] >> 5)) & (1 << 1)) - (1 << 1)) | ~(DV_I_46_2_bit | DV_II_50_2_bit));
    mask &= ((0 - ((W[44] ^ (W[45] >> 5)) & (1 << 1))) | ~(DV_I_51_2_bit | DV_II_49_2_bit));
    mask &= ((((W[43] ^ W[45]) & (1 << 6)) - (1 << 6)) | ~(DV_I_47_2_bit | DV_I_49_2_bit));
    mask &= (((((W[42] ^ W[44]) >> 6) & 1) - 1) | ~(DV_I_46_2_bit | DV_I_48_2_bit));
    mask &= ((((W[43] ^ (W[42] >> 5)) & (1 << 1)) - (1 << 1)) | ~(DV_II_46_2_bit | DV_II_51_2_bit));
    mask &= ((((W[42] ^ (W[41] >> 5)) & (1 << 1)) - (1 << 1)) | ~(DV_I_51_2_bit | DV_II_50_2_bit));
    mask &= ((((W[41] ^ (W[40] >> 5)) & (1 << 1)) - (1 << 1)) | ~(DV_I_50_2_bit | DV_II_49_2_bit));
    if (mask & (DV_I_52_0_bit | DV_II_51_0_bit))
        mask &= ((((W[39] ^ (W[43] >> 25)) & (1 << 4)) - (1 << 4)) | ~(DV_I_52_0_bit | DV_II_51_0_bit));
    if (mask & (DV_I_51_0_bit | DV_II_50_0_bit))
        mask &= ((((W[38] ^ (W[42] >> 25)) & (1 << 4)) - (1 << 4)) | ~(DV_I_51_0_bit | DV_II_50_0_bit));
    if (mask & (DV_I_48_2_bit | DV_I_51_2_bit))
        mask &= ((0 - ((W[37] ^ (W[38] >> 5)) & (1 << 1))) | ~(DV_I_48_2_bit | DV_I_51_2_bit));
    if (mask & (DV_I_50_0_bit | DV_II_49_0_bit))
        mask &= ((((W[37] ^ (W[41] >> 25)) & (1 << 4)) - (1 << 4)) | ~(DV_I_50_0_bit | DV_II_49_0_bit));
    if (mask & (DV_II_52_0_bit | DV_II_54_0_bit))
        mask &= ((0 - ((W[36] ^ W[38]) & (1 << 4))) | ~(DV_II_52_0_bit | DV_II_54_0_bit));
    mask &= ((0 - ((W[35] ^ (W[36] >> 5)) & (1 << 1))) | ~(DV_I_46_2_bit | DV_I_49_2_bit));
    if (mask & (DV_I_51_0_bit | DV_II_47_0_bit))
        mask &= ((((W[35] ^ (W[39] >> 25)) & (1 << 3)) - (1 << 3)) | ~(DV_I_51_0_bit | DV_II_47_0_bit));
    if (mask) {

        if (mask & DV_I_43_0_bit)
            if (
                    !((W[61] ^ (W[62] >> 5)) & (1 << 1))
                    || !(!((W[59] ^ (W[63] >> 25)) & (1 << 5)))
                    || !((W[58] ^ (W[63] >> 30)) & (1 << 0))
            )
                mask &= ~DV_I_43_0_bit;
        if (mask & DV_I_44_0_bit)
            if (
                    !((W[62] ^ (W[63] >> 5)) & (1 << 1))
                    || !(!((W[60] ^ (W[64] >> 25)) & (1 << 5)))
                    || !((W[59] ^ (W[64] >> 30)) & (1 << 0))
            )
                mask &= ~DV_I_44_0_bit;
        if (mask & DV_I_46_2_bit)
            mask &= ((~((W[40] ^ W[42]) >> 2)) | ~DV_I_46_2_bit);
        if (mask & DV_I_47_2_bit)
            if (
                    !((W[62] ^ (W[63] >> 5)) & (1 << 2))
                    || !(!((W[41]^W[43]) & (1 << 6)))
            )
                mask &= ~DV_I_47_2_bit;
        if (mask & DV_I_48_2_bit)
            if (
                    !((W[63] ^ (W[64] >> 5)) & (1 << 2))
                    || !(!((W[48] ^ (W[49] << 5)) & (1 << 6)))
            )
                mask &= ~DV_I_48_2_bit;
        if (mask & DV_I_49_2_bit)
            if (
                    !(!((W[49] ^ (W[50] << 5)) & (1 << 6)))
                    || !((W[42]^W[50]) & (1 << 1))
                    || !(!((W[39] ^ (W[40] << 5)) & (1 << 6)))
                    || !((W[38]^W[40]) & (1 << 1))
            )
                mask &= ~DV_I_49_2_bit;
        if (mask & DV_I_50_0_bit)
            mask &= ((((W[36] ^ W[37]) << 7)) | ~DV_I_50_0_bit);
        if (mask & DV_I_50_2_bit)
            mask &= ((((W[43] ^ W[51]) << 11)) | ~DV_I_50_2_bit);
        if (mask & DV_I_51_0_bit)
            mask &= ((((W[37] ^ W[38]) << 9)) | ~DV_I_51_0_bit);
        if (mask & DV_I_51_2_bit)
            if (
                    !(!((W[51] ^ (W[52] << 5)) & (1 << 6)))
                    || !(!((W[49]^W[51]) & (1 << 6)))
                    || !(!((W[37] ^ (W[37] >> 5)) & (1 << 1)))
                    || !(!((W[35] ^ (W[39] >> 25)) & (1 << 5)))
            )
                mask &= ~DV_I_51_2_bit;
        if (mask & DV_I_52_0_bit)
            mask &= ((((W[38] ^ W[39]) << 11)) | ~DV_I_52_0_bit);
        if (mask & DV_II_46_2_bit)
            mask &= ((((W[47] ^ W[51]) << 17)) | ~DV_II_46_2_bit);
        if (mask & DV_II_48_0_bit)
            if (
                    !(!((W[36] ^ (W[40] >> 25)) & (1 << 3)))
                    || !((W[35] ^ (W[40] << 2)) & (1 << 30))
            )
                mask &= ~DV_II_48_0_bit;
        if (mask & DV_II_49_0_bit)
            if (
                    !(!((W[37] ^ (W[41] >> 25)) & (1 << 3)))
                    || !((W[36] ^ (W[41] << 2)) & (1 << 30))
            )
                mask &= ~DV_II_49_0_bit;
        if (mask & DV_II_49_2_bit)
            if (
                    !(!((W[53] ^ (W[54] << 5)) & (1 << 6)))
                    || !(!((W[51]^W[53]) & (1 << 6)))
                    || !((W[50]^W[54]) & (1 << 1))
                    || !(!((W[45] ^ (W[46] << 5)) & (1 << 6)))
                    || !(!((W[37] ^ (W[41] >> 25)) & (1 << 5)))
                    || !((W[36] ^ (W[41] >> 30)) & (1 << 0))
            )
                mask &= ~DV_II_49_2_bit;
        if (mask & DV_II_50_0_bit)
            if (
                    !((W[55]^W[58]) & (1 << 29))
                    || !(!((W[38] ^ (W[42] >> 25)) & (1 << 3)))
                    || !((W[37] ^ (W[42] << 2)) & (1 << 30))
            )
                mask &= ~DV_II_50_0_bit;
        if (mask & DV_II_50_2_bit)
            if (
                    !(!((W[54] ^ (W[55] << 5)) & (1 << 6)))
                    || !(!((W[52]^W[54]) & (1 << 6)))
                    || !((W[51]^W[55]) & (1 << 1))
                    || !((W[45]^W[47]) & (1 << 1))
                    || !(!((W[38] ^ (W[42] >> 25)) & (1 << 5)))
                    || !((W[37] ^ (W[42] >> 30)) & (1 << 0))
            )
                mask &= ~DV_II_50_2_bit;
        if (mask & DV_II_51_0_bit)
            if (
                    !(!((W[39] ^ (W[43] >> 25)) & (1 << 3)))
                    || !((W[38] ^ (W[43] << 2)) & (1 << 30))
            )
                mask &= ~DV_II_51_0_bit;
        if (mask & DV_II_51_2_bit)
            if (
                    !(!((W[55] ^ (W[56] << 5)) & (1 << 6)))
                    || !(!((W[53]^W[55]) & (1 << 6)))
                    || !((W[52]^W[56]) & (1 << 1))
                    || !((W[46]^W[48]) & (1 << 1))
                    || !(!((W[39] ^ (W[43] >> 25)) & (1 << 5)))
                    || !((W[38] ^ (W[43] >> 30)) & (1 << 0))
            )
                mask &= ~DV_II_51_2_bit;
        if (mask & DV_II_52_0_bit)
            if (
                    !(!((W[59]^W[60]) & (1 << 29)))
                    || !(!((W[40] ^ (W[44] >> 25)) & (1 << 3)))
                    || !(!((W[40] ^ (W[44] >> 25)) & (1 << 4)))
                    || !((W[39] ^ (W[44] << 2)) & (1 << 30))
            )
                mask &= ~DV_II_52_0_bit;
        if (mask & DV_II_53_0_bit)
            if (
                    !((W[58]^W[61]) & (1 << 29))
                    || !(!((W[57] ^ (W[61] >> 25)) & (1 << 4)))
                    || !(!((W[41] ^ (W[45] >> 25)) & (1 << 3)))
                    || !(!((W[41] ^ (W[45] >> 25)) & (1 << 4)))
            )
                mask &= ~DV_II_53_0_bit;
        if (mask & DV_II_54_0_bit)
            if (
                    !(!((W[58] ^ (W[62] >> 25)) & (1 << 4)))
                    || !(!((W[42] ^ (W[46] >> 25)) & (1 << 3)))
                    || !(!((W[42] ^ (W[46] >> 25)) & (1 << 4)))
            )
                mask &= ~DV_II_54_0_bit;
        if (mask & DV_II_55_0_bit)
            if (
                    !(!((W[59] ^ (W[63] >> 25)) & (1 << 4)))
                    || !(!((W[57] ^ (W[59] >> 25)) & (1 << 4)))
                    || !(!((W[43] ^ (W[47] >> 25)) & (1 << 3)))
                    || !(!((W[43] ^ (W[47] >> 25)) & (1 << 4)))
            )
                mask &= ~DV_II_55_0_bit;
        if (mask & DV_II_56_0_bit)
            if (
                    !(!((W[60] ^ (W[64] >> 25)) & (1 << 4)))
                    || !(!((W[44] ^ (W[48] >> 25)) & (1 << 3)))
                    || !(!((W[44] ^ (W[48] >> 25)) & (1 << 4)))
            )
                mask &= ~DV_II_56_0_bit;
    }

    dvmask[0] = mask;
}

#ifdef SHA1DC_CUSTOM_TRAILING_INCLUDE_UBC_CHECK_C
    #include SHA1DC_CUSTOM_TRAILING_INCLUDE_UBC_CHECK_C
#endif

#endif /* MACE_CONVENIENCE_EXECUTABLE */
/****************************** SHA1DC SOURCE END *****************************/

/********************************* PARG SOURCE ********************************/

struct parg_state parg_state_default = {
    .optarg = NULL,
    .optind = 1,
    .optopt = '?',
    .nextchar = NULL
};

/* Automatic usage/help printing */
void mace_parg_usage(const char *name, const struct parg_opt *longopts) {
    assert(longopts);
    printf("\nmace builder executable: %s \n", name);
    printf("Usage: %s [TARGET] [OPTIONS]\n", name);
    for (int i = 0; longopts[i].doc; ++i) {
        if (longopts[i].val)
            printf(" -%c", longopts[i].val);

        if (longopts[i].name)
            printf(",  --%-15s", longopts[i].name);

        if (longopts[i].arg) {
            printf("[=%s]", longopts[i].arg);
            printf("%*c", MACE_USAGE_MIDCOLW - 3 - strlen(longopts[i].arg), ' ');
        } else if (longopts[i].val || longopts[i].name)
            printf("%*c", MACE_USAGE_MIDCOLW, ' ');

        if ((!longopts[i].arg) && ((longopts[i].val) || (longopts[i].name)))
            printf("");

        if (longopts[i].doc)
            printf("%s", longopts[i].doc);
        printf("\n");
    }
}

void reverse(char *v[], int i, int j) {
    while (j - i > 1) {
        char *tmp = v[i];
        v[i] = v[j - 1];
        v[j - 1] = tmp;
        ++i;
        --j;
    }
}

/* Check if state is at end of argv */
int is_argv_end(const struct parg_state *ps, int argc, char *const argv[]) {
    return ps->optind >= argc || argv[ps->optind] == NULL;
}


/* Match string at nextchar against longopts. */
int match_long(struct parg_state *ps, int argc, char *const argv[], const char *optstring,
                   const struct parg_opt *longopts, int *longindex) {
        int i, match = -1, num_match = 0;
        size_t len = strcspn(ps->nextchar, "=");

        for (i = 0; longopts[i].name; ++i) {
            if (strncmp(ps->nextchar, longopts[i].name, len) == 0) {
                match = i;
                num_match++;
                /* Take if exact match */
                if (longopts[i].name[len] == '\0') {
                    num_match = 1;
                    break;
                }
            }
        }

        /* Return '?' on no or ambiguous match */
        if (num_match != 1) {
            ps->optopt = 0;
            ps->nextchar = NULL;
            return '?';
        }

        assert(match != -1);

        if (longindex) {
            *longindex = match;
        }

        if (ps->nextchar[len] == '=') {
            /* Option argument present, check if extraneous */
            if (longopts[match].has_arg == PARG_NOARG) {
                ps->optopt = longopts[match].flag ? 0 : longopts[match].val;
                ps->nextchar = NULL;
                return optstring[0] == ':' ? ':' : '?';
            } else {
                ps->optarg = &ps->nextchar[len + 1];
            }
        } else if (longopts[match].has_arg == PARG_REQARG) {
            /* Option argument required, so return next argv element */
            if (is_argv_end(ps, argc, argv)) {
                ps->optopt = longopts[match].flag ? 0 : longopts[match].val;
                ps->nextchar = NULL;
                return optstring[0] == ':' ? ':' : '?';
            }

            ps->optarg = argv[ps->optind++];
        }

        ps->nextchar = NULL;

        if (longopts[match].flag != NULL) {
            *longopts[match].flag = longopts[match].val;
            return 0;
        }

        return longopts[match].val;
    }


/* Match nextchar against optstring */
int match_short(struct parg_state *ps, int argc, char *const argv[], const char *optstring) {
    const char *p = strchr(optstring, *ps->nextchar);

    if (p == NULL) {
        ps->optopt = *ps->nextchar++;
        return '?';
    }

    /* If no option argument, return option */
    if (p[1] != ':') {
        return *ps->nextchar++;
    }

    /* If more characters, return as option argument */
    if (ps->nextchar[1] != '\0') {
        ps->optarg = &ps->nextchar[1];
        ps->nextchar = NULL;
        return *p;
    }

    /* If option argument is optional, return option */
    if (p[2] == ':') {
        return *ps->nextchar++;
    }

    /* Option argument required, so return next argv element */
    if (is_argv_end(ps, argc, argv)) {
        ps->optopt = *ps->nextchar++;
        return optstring[0] == ':' ? ':' : '?';
    }

    ps->optarg = argv[ps->optind++];
    ps->nextchar = NULL;
    return *p;
}

/* Parse next long or short option in `argv`.
 * Check GNU getopt_long example for details:
 * https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html
 */
int parg_getopt_long(struct parg_state *ps, int argc, char *const argv[], const char *optstring,
                         const struct parg_opt *longopts, int *longindex) {
        assert(ps != NULL);
        assert(argv != NULL);
        assert(optstring != NULL);

        ps->optarg = NULL;

        if (argc < 2) {
            return -1;
        }

        /* Advance to next element if needed */
        if (ps->nextchar == NULL || *ps->nextchar == '\0') {
            if (is_argv_end(ps, argc, argv)) {
                return -1;
            }

            ps->nextchar = argv[ps->optind++];

            /* Check for argument element (including '-') */
            if (ps->nextchar[0] != '-' || ps->nextchar[1] == '\0') {
                ps->optarg = ps->nextchar;
                ps->nextchar = NULL;
                return 1;
            }

            /* Check for '--' */
            if (ps->nextchar[1] == '-') {
                if (ps->nextchar[2] == '\0') {
                    ps->nextchar = NULL;
                    return -1;
                }

                if (longopts != NULL) {
                    ps->nextchar += 2;

                    return match_long(ps, argc, argv, optstring,
                                      longopts, longindex);
                }
            }

            ps->nextchar++;
        }

        /* Match nextchar */
        return match_short(ps, argc, argv, optstring);
    }

/*
 * Reorder elements of `argv` with no special cases.
 *
 * This function assumes there is no `--` element, and the last element
 * is not an option missing a required argument.
 *
 * The algorithm is described here:
 * http://hardtoc.com/2016/11/07/reordering-arguments.html
 */
int parg_reorder_simple(int argc, char *argv[], const char *optstring,
                        const struct parg_opt *longopts) {
    struct parg_state ps;
    int change, l = 0, m = 0, r = 0;

    if (argc < 2) {
        return argc;
    }

    do {
        int c, nextind;
        ps = parg_state_default;
        nextind = ps.optind;

        /* Parse until end of argument */
        do {
            c = parg_getopt_long(&ps, argc, argv, optstring, longopts, NULL);
        } while (ps.nextchar != NULL && *ps.nextchar != '\0');

        change = 0;

        do {
            /* Find next non-option */
            for (l = nextind; c != 1 && c != -1;) {
                l = ps.optind;

                do {
                    c = parg_getopt_long(&ps, argc, argv, optstring, longopts, NULL);
                } while (ps.nextchar != NULL && *ps.nextchar != '\0');
            }

            /* Find next option */
            for (m = l; c == 1;) {
                m = ps.optind;

                do {
                    c = parg_getopt_long(&ps, argc, argv, optstring, longopts, NULL);
                } while (ps.nextchar != NULL && *ps.nextchar != '\0');
            }

            /* Find next non-option */
            for (r = m; c != 1 && c != -1;) {
                r = ps.optind;

                do {
                    c = parg_getopt_long(&ps, argc, argv, optstring, longopts, NULL);
                } while (ps.nextchar != NULL && *ps.nextchar != '\0');
            }

            /* Find next option */
            for (nextind = r; c == 1;) {
                nextind = ps.optind;

                do {
                    c = parg_getopt_long(&ps, argc, argv, optstring, longopts, NULL);
                } while (ps.nextchar != NULL && *ps.nextchar != '\0');
            }

            if (m < r) {
                change = 1;
                reverse(argv, l, m);
                reverse(argv, m, r);
                reverse(argv, l, r);
            }
        } while (c != -1);
    } while (change != 0);

    return l + (r - m);
}

/**
 * Parse next short option in `argv`.
 * Check GNU getopt example for details:
 * https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
 */
int parg_getopt(struct parg_state *ps, int argc, char *const argv[], const char *optstring) {
    return parg_getopt_long(ps, argc, argv, optstring, NULL, NULL);
}

/**
 * Reorder elements of `argv` so options appear first.
 *
 * If there are no long options, `longopts` may be `NULL`.
 *
 * The return value can be used as `argc` parameter for `parg_getopt()` and
 * `parg_getopt_long()`.
 *
 * @param argc number of elements in `argv`
 * @param argv array of pointers to command-line arguments
 * @param optstring string containing option characters
 * @param longopts array of `parg_option` structures
 * @return index of first argument in `argv` on success, `-1` on error
 */
int parg_reorder(int argc, char *argv[], const char *optstring,
                 const struct parg_opt *longopts) {
    struct parg_state ps;
    int lastind, optend, c;

    assert(argv != NULL);
    assert(optstring != NULL);

    if (argc < 2) {
        return argc;
    }

    ps = parg_state_default;

    /* Find end of normal arguments */
    do {
        lastind = ps.optind;

        c = parg_getopt_long(&ps, argc, argv, optstring, longopts, NULL);

        /* Check for trailing option with error */
        if ((c == '?' || c == ':') && is_argv_end(&ps, argc, argv)) {
            lastind = ps.optind - 1;
            break;
        }
    } while (c != -1);

    optend = parg_reorder_simple(lastind, argv, optstring, longopts);

    /* Rotate `--` or trailing option with error into position */
    if (lastind < argc) {
        reverse(argv, optend, lastind);
        reverse(argv, optend, lastind + 1);
        ++optend;
    }

    return optend;
}

int parg_zgetopt_long(struct parg_state *ps, int argc, char *const argv[], const char *optstring,
                          const struct parg_opt *longopts, int *longindex) {
        assert(ps != NULL);
        assert(argv != NULL);
        assert(optstring != NULL);

        ps->optarg = NULL;

        if (argc < 2) {
            return -1;
        }

        /* Advance to next element if needed */
        if (ps->nextchar == NULL || *ps->nextchar == '\0') {
            if (is_argv_end(ps, argc, argv)) {
                return -1;
            }

            ps->nextchar = argv[ps->optind++];

            /* Check for argument element (including '-') */
            if (ps->nextchar[0] != '-' || ps->nextchar[1] == '\0') {
                ps->optarg = ps->nextchar;
                ps->nextchar = NULL;
                return 1;
            }

            /* Check for '--' */
            if (ps->nextchar[1] == '-') {
                if (ps->nextchar[2] == '\0') {
                    ps->nextchar = NULL;
                    return -1;
                }

                if (longopts != NULL) {
                    ps->nextchar += 2;

                    return match_long(ps, argc, argv, optstring, longopts, longindex);
                }
            }
            ps->nextchar++;
        }

        /* Match nextchar */
        return match_short(ps, argc, argv, optstring);
    }

/******************************* PARG SOURCE END ******************************/

/****************************** argument parsing ******************************/
/* list of parg options to be parsed, with usage */
static struct parg_opt longopts[] = {
    // {NULL,          PARG_NOARG,  0,  0,  NULL,   "Debug options:"},
    {"always-make", PARG_NOARG,  0, 'B', NULL,   "Build targers without checking checksums."},
    {"directory",   PARG_REQARG, 0, 'C', "DIR",  "Move to directory before anything else."},
    {"cc",          PARG_REQARG, 0, 'c', "CC",   "Override C compiler."},
    {"debug",       PARG_NOARG,  0, 'd', NULL,   "Print debug info"},
    {"help",        PARG_NOARG,  0, 'h', NULL,   "display help and exit"},
    {"jobs",        PARG_REQARG, 0, 'j', "INT",  "Allow N jobs at once"},
    {"dry-run",     PARG_NOARG,  0, 'n', NULL,   "Don't build, just echo commands"},
    {"old-file",    PARG_REQARG, 0, 'o', "FILE", "Skip target/file"},
    {"silent",      PARG_NOARG,  0, 's', NULL,   "Don't echo commands"},
    {"version",     PARG_NOARG,  0, 'v', NULL,   "display version and exit"},
    {NULL,          PARG_NOARG,  0,  0,  NULL,   "Convenience executable options:"},
    {"file",        PARG_REQARG, 0, 'f', "FILE", "Specify input macefile. Defaults to macefile.c)"},
};

struct Mace_Arguments Mace_Arguments_default = {
    .user_target        = NULL,
    .user_target_hash   = 0,
    .jobs               = 1,
    .cc                 = NULL,
    .macefile           = NULL,
    .dir                = NULL,
    .debug              = false,
    .silent             = false,
    .dry_run            = false,
    .skip_checksum      = false,
};

void Mace_Arguments_Free(struct Mace_Arguments *args) {
    if (args->macefile != NULL) {
        free(args->macefile);
        args->macefile = NULL;
    }
    if (args->user_target != NULL) {
        free(args->user_target);
        args->user_target = NULL;
    }
    if (args->dir != NULL) {
        free(args->dir);
        args->dir = NULL;
    }
}

struct Mace_Arguments mace_parse_args(int argc, char *argv[]) {
    struct Mace_Arguments out_args = Mace_Arguments_default;
    struct parg_state ps = parg_state_default;
    int *longindex;
    int c;
    size_t len;

    if (argc <= 1)
        return (out_args);

    while ((c = parg_getopt_long(&ps, argc, argv, "BC:df:hj:no:sv", longopts, longindex)) != -1) {
        switch (c) {
            case 1:
                len = strlen(ps.optarg);
                out_args.user_target = calloc(len + 1, sizeof(*out_args.user_target));
                strncpy(out_args.user_target, ps.optarg, len);
                out_args.user_target_hash = mace_hash(ps.optarg);
                break;
            case 'B':
                out_args.skip_checksum = true;
                break;
            case 'C':
                len = strlen(ps.optarg);
                out_args.dir = calloc(len + 1, sizeof(*out_args.dir));
                strncpy(out_args.dir, ps.optarg, len);
                break;
            case 'd':
                out_args.debug      = true;
                break;
            case 'f': {
                len = strlen(ps.optarg);
                out_args.macefile = calloc(len + 1, sizeof(*out_args.macefile));
                strncpy(out_args.macefile, ps.optarg, len);
                break;
            }
            case 'h':
                mace_parg_usage(argv[0], longopts);
                exit(0);
                break;
            case 'j':
                out_args.jobs       = atoi(ps.optarg);
                if (out_args.jobs < 1) {
                    fprintf(stderr, "Error: Set number of jobs above 1.");
                }
                break;
            case 'n':
                out_args.dry_run    = true;
                break;
            case 'o':
                out_args.skip       = mace_hash(ps.optarg);
                break;
            case 's':
                out_args.silent     = true;
                break;
            case 'v':
                printf("mace version %s\n", MACE_VER_STRING);
                exit(0);
            case '?':
                if (ps.optopt == 'C') {
                    printf("option -C/--directory requires an argument\n");
                } else if (ps.optopt == 'o') {
                    printf("option -o/--old-file requires an argument\n");
                } else if (ps.optopt == 'j') {
                    printf("option -j/--jobs requires an argument\n");
                } else if (ps.optopt == 'f') {
                    printf("option -f/--file requires an argument\n");
                } else {
                    printf("unknown option -%c\n", ps.optopt);
                }
                exit(EPERM);
                break;
            default:
                printf("error: unhandled option -%c\n", c);
                exit(EPERM);
                break;
        }
    }

    /*Debugging*/
    for (c = ps.optind; c < argc; ++c) {
        printf("argument '%s'\n", argv[c]);
    }

    return (out_args);
}

/************************************ main ************************************/
// 1- Runs mace function, get all info from user:
//   a- Get compiler
//   b- Get targets
// 2- Builds dependency graph from targets' links, dependencies
// 3- TODO: Determine which targets need to be recompiled
// 4- Build the targets

#ifndef MACE_OVERRIDE_MAIN
int main(int argc, char *argv[]) {
    printf("START\n");

    /* --- Parse user arguments --- */
    struct Mace_Arguments args = mace_parse_args(argc, argv);

    /* --- Get cwd, alloc memory, set defaults. --- */
    mace_init();

    /* --- User function --- */
    // Sets compiler, add targets and commands.
    mace(argc, argv);

    /* --- Post-user checks: compiler set, at least one target exists. --- */
    mace_post_user(args);

    /* --- Make output directories. --- */
    mace_mkdir(obj_dir);
    mace_mkdir(build_dir);

    /* --- Compute build order using targets links list. --- */
    mace_targets_build_order();

    /* --- Perform compilation with build_order --- */
    if (mace_user_target == MACE_CLEAN_ORDER)
        mace_clean();
    else
        mace_build_targets();

    /* --- Finish --- */
    mace_free();
    Mace_Arguments_Free(&args);
    printf("FINISH\n");
    return (0);
}
#endif /* MACE_OVERRIDE_MAIN */
