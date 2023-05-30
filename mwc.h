
#include <assert.h>
#include <stdio.h>
#include <string.h>

/**************************** Target struct ***********************************/
// Contains all information necessary to compile target
struct Target {
    char * includes;          /* directories,           ' ' separated */
    char * sources;           /* files, glob patterns,  ' ' separated */
    char * sources_exclude;   /* files, glob patterns,  ' ' separated */
    char * base_dir;          /* directory,                           */
    char * dependencies;      /* targets,               ' ' separated */
    char * links;             /* libraries,             ' ' separated */ 
    char * flags;             /* passed as is to compiler             */ 
    char * message_pre_build;
    char * command_pre_build;
    char * message_post_build;
    char * command_post_build;
    int kind;
};

/* --- EXAMPLE TARGET --- */ 
// Use struct Designated Initializer, guaranteeing unitialized values to 0. 
/*
* struct Target tnecs = {
*     .includes           = "",
*     .sources            = "",
*     .sources_exclude    = "",
*     .base_dir           = "",
*     .dependencies       = "",
*     .kind               = MWC_LIBRARY,
* }; 
*/
/******************************** Phony struct ********************************/
// Builds dependencies, then runs command.
struct PHONY {
    char * command;
    char * dependencies;
};

/******************************* MWC_ADD_TARGET *******************************/
// 1- Saves target name hash
// 2- Builds list of sources
// All added targets are built
#define MWC_ADD_TARGET(a)

/******************************** MWC_ADD_PHONY *******************************/
// Phony targets are only built when called explicitely e.g. <./build> install
// Default phony: 'clean' removes all targets.
#define MWC_ADD_PHONY(a)

/****************************** MWC_SET_COMPILER ******************************/
// 1- Save compiler name string
#define MWC_SET_COMPILER(a)

/******************************* MWC_SET_OBJDIR *******************************/
// Sets where the object files will be placed during build. 
#define MWC_SET_OBJDIR(a)

enum MWC_TARGET_KIND {
    MWC_EXECUTABLE      = 1,
    MWC_LIBRARY         = 2,
};

/**************************** mwc_target_dependency ***************************/
// Build target dependency graph from target links 
void mwc_target_dependency(struct Target * targets, size_t len) {

}

/********************************* mwc_build **********************************/
void mwc_build(struct Target * targets, size_t len) {
    
}

/************************************ mwc *************************************/
// User-implemented function. 
// SHOULD:
// 1- Set compiler
// 2- Add targets
extern int mwc(int argc, char *argv[]);

/************************************ main ************************************/
// 1- Run mwc, get all info from user:
//   a- Get compiler 
//   a- Get targets
// 2- Build dependency graph from targets
// 3- Determine which targets need to be recompiled
// 4- Build the targets

// if `mwc clean` is called (clean target), rm all targets  

int main(int argc, char *argv[]) {
    mwc(argc, argv);

    mwc_target_dependency();    
    mwc_build();    
    return(0);
}
