#ifndef __YY_PROF_TIME_H__
#define __YY_PROF_TIME_H__


#include <time.h>
#include "include/yy_structs.h"

/**
 * Get time stamp counter (TSC) value via 'rdtsc' instruction.
 *
 * @return 64 bit unsigned integer
 * @author cjiang
 */
#ifdef TIME_USE_CLOCK
    #define SET_TIMESTAMP_COUNTER(tc) \
        do { \
            struct timespec t = {0, 0}; \
            if (clock_gettime(CLOCK_MONOTONIC, &t) == 0) { \
                (tc) = t.tv_sec * 1e6 + t.tv_nsec / 1000; \
            } \
        } while (0);
#else
    #define SET_TIMESTAMP_COUNTER(tc) \
        do { \
            uint32_t __a,__d; \
            uint64_t val; \
            asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
            (val) = ((uint64_t)__a) | (((uint64_t)__d)<<32); \
            tc = val; \
        } while (0)
#endif

#define GET_US_FROM_TSC(count, cpu_frequencies) ((count) / (cpu_frequencies))
#define GET_TSC_FROM_US(usec, cpu_frequencies) ((uint64_t) ((usecs) * (cpu_frequencies)))
#define GET_US_INTERVAL(start, end) \
    (((end)->tv_sec - (start)->tv_sec) * 1000000  + ((end)->tv_usec - (start)->tv_usec))

/*
inline uint64_t cycle_timer() {
    uint32_t __a,__d;
    uint64_t val;
    asm volatile("rdtsc" : "=a" (__a), "=d" (__d));
    (val) = ((uint64_t)__a) | (((uint64_t)__d)<<32);
    return val;
}
inline double get_us_from_tsc(uint64_t count, double cpu_frequency) {
    return count / cpu_frequency;
}

inline uint64_t get_tsc_from_us(uint64_t usecs, double cpu_frequency) {
    return (uint64_t) (usecs * cpu_frequency);
}

inline long get_us_interval(struct timeval *start, struct timeval *end) {
    return (((end->tv_sec - start->tv_sec) * 1000000)
            + (end->tv_usec - start->tv_usec));
}

inline void incr_us_interval(struct timeval *start, uint64_t incr) {
    incr += (start->tv_sec * 1000000 + start->tv_usec);
    start->tv_sec  = incr / 1000000;
    start->tv_usec = incr % 1000000;
    return;
}
*/


// MODULE INIT
int init_cpu_info(yy_prof_cpu_t * cpu);
// MODULE SHUTDOWN
int clear_cpu_info(yy_prof_cpu_t * cpu);

// BEGIN PROFILE
int bind_rand_cpu(yy_prof_cpu_t * cpu);
// END PROFILE
int release_cpu(yy_prof_cpu_t * cpu);

#endif
