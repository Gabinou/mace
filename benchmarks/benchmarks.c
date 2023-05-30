/*
* benchmarks.c
*
* Copyright (C) Gabriel Taillon, 2023
*
* Benchmarks for mace C-based build system.
*
*/

#define MACE_OVERRIDE_MAIN
#include "../mace.h"
#include <stdint.h>

/********************** 0.1 MICROSECOND RESOLUTION CLOCK **********************/
//  Modified from: https://gist.github.com/ForeverZer0/0a4f80fc02b96e19380ebb7a3debbee5
#include <stdint.h>
#if defined(__linux)
#  define MICROSECOND_CLOCK
#  define HAVE_POSIX_TIMER
#  include <time.h>
#  ifdef CLOCK_MONOTONIC
#     define CLOCKID CLOCK_MONOTONIC
#  else
#     define CLOCKID CLOCK_REALTIME
#  endif
#elif defined(__APPLE__)
#  define MICROSECOND_CLOCK
#  define HAVE_MACH_TIMER
#  include <mach/mach_time.h>
#elif defined(_WIN32)
#  define MICROSECOND_CLOCK
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

extern uint64_t  tnecs_get_ns();
extern double    tnecs_get_us();

uint64_t tnecs_get_ns() {
    static uint64_t is_init = 0;
    #if defined(__APPLE__)
    static mach_timebase_info_data_t info;
    if (0 == is_init) {
        mach_timebase_info(&info);
        is_init = 1;
    }
    uint64_t now;
    now = mach_absolute_time();
    now *= info.numer;
    now /= info.denom;
    return now;
    #elif defined(__linux)
    static struct timespec linux_rate;
    if (0 == is_init) {
        clock_getres(CLOCKID, &linux_rate);
        is_init = 1;
    }
    uint64_t now;
    struct timespec spec;
    clock_gettime(CLOCKID, &spec);
    now = spec.tv_sec * 1.0e9 + spec.tv_nsec;
    return now;
    #elif defined(_WIN32)
    static LARGE_INTEGER win_frequency;
    if (0 == is_init) {
        QueryPerformanceFrequency(&win_frequency);
        is_init = 1;
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (uint64_t)((1e9 * now.QuadPart) / win_frequency.QuadPart);
    #endif
}
#ifdef MICROSECOND_CLOCK
double tnecs_get_us() {
    return (tnecs_get_ns() / 1e3);
}
#else
#  define FAILSAFE_CLOCK
#  define tnecs_get_us() (((double)clock())/CLOCKS_PER_SEC*1e6) // [us]
#  define tnecs_get_ns() (((double)clock())/CLOCKS_PER_SEC*1e9) // [ns]
#endif


#define ITERATIONS 100000

int bench_filereading() {
    struct Target sota = {
    .sources            = "benchmarks",
    .base_dir           = "..",
    .kind               = MACE_STATIC_LIBRARY,
    };
    mace_Target_sources_grow(&sota);
    uint64_t t_0, t_1;
    t_0 = tnecs_get_ns();
    mace_Target_sources_grow(&sota);
    sota._argv_objects[0] = "-ounit1.o"; 
    for (int i = 0; i < ITERATIONS; i++) {
        mace_Target_Read_ho(&sota, 0);
        sota._deps_headers_num[0] = 0;
    }
    t_1 = tnecs_get_ns();
    printf("Read .ho file %d ms\n", (t_1-t_0)/100000);

    // Target_Object_Hash_Add_nocoll(&sota, mace_hash("-ounit1.o"));
    Target_Object_Hash_Add_nocoll(&sota, mace_hash("unit1.o"));
    // mace_Target_Object_Add(&sota, "unit.c");

    for (int i = 0; i < ITERATIONS; i++) {
        mace_Target_Read_d(&sota, 0);
        sota._deps_headers_num[0] = 0;
    }
    t_1 = tnecs_get_ns();
    printf("Read .d file %d ms\n", (t_1-t_0)/100000);

    printf("Conclusion: reading .ho files is about 7x faster\n");
}

int main(int argc, char *argv[]) {
    mace_set_obj_dir("obj");
    bench_filereading();
}
