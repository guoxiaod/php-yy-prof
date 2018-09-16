#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "include/yy_structs.h"
#include "include/yy_time.h"

#define CPU_INFO_VERSION 20170301
#define YY_PROF_CPU_INFO_FILE LOCALSTATEDIR "/cache/yy_prof/cpu_info.db"

#define YY_PROF_TIME_LOG(log_level, str) \
    php_error((log_level), "YY_PROF: %s %d:%s in %s:%d", \
        (str), errno, strerror(errno), __FILE__, __LINE__)

double get_cpu_frequency();
int get_all_cpu_frequencies(yy_prof_cpu_t * cpu);
int bind_to_cpu(yy_prof_cpu_t * cpu, uint32_t cpu_id);


// MODULE INIT
int init_cpu_info(yy_prof_cpu_t * cpu) {
    cpu->cpu_num = sysconf(_SC_NPROCESSORS_CONF); 

    /* Get the cpu affinity mask. */
#ifndef __APPLE__
    if (GET_AFFINITY(0, sizeof(cpu_set_t), &cpu->prev_mask) < 0) {
        YY_PROF_TIME_LOG(E_ERROR, "getaffinity for CPU failed in init_cpu_info");
        return 0;
    }
#else
    CPU_ZERO(&cpu->prev_mask);
#endif

    cpu->cpu_frequencies = NULL;
    cpu->cur_cpu_id = 0;

    get_all_cpu_frequencies(cpu);
    return 0;
}

// MODULE SHUTDOWN
int clear_cpu_info(yy_prof_cpu_t * cpu) {
    release_cpu(cpu);

    if(cpu->cpu_frequencies != NULL) {
        pefree(cpu->cpu_frequencies, 1);
        cpu->cpu_frequencies = NULL;
    }
    return 0;
}


// BEGIN PROFILE
int bind_rand_cpu(yy_prof_cpu_t * cpu) {
    if(cpu->cpu_frequencies == NULL) {
        get_all_cpu_frequencies(cpu);
    }
    uint32_t id = rand() % cpu->av_cpu_num;
    bind_to_cpu(cpu, cpu->av_cpus[id]);
    return 0;
}

// END PROFILE
/**
 * Restore cpu affinity mask to a specified value. It returns 0 on success and
 * -1 on failure.
 *
 * @param cpu_set_t * prev_mask, previous cpu affinity mask to be restored to.
 * @return int, 0 on success, and -1 on failure.
 *
 * @author cjiang
 */
int release_cpu(yy_prof_cpu_t * cpu) {
    if (SET_AFFINITY(0, sizeof(cpu_set_t), &cpu->prev_mask) < 0) {
        YY_PROF_TIME_LOG(E_WARNING, "restore setaffinity failed");
        return -1;
    }
    /* default value ofor cur_cpu_id is 0. */
    cpu->cur_cpu_id = 0;

    return 0;
}


/**
 * This is a microbenchmark to get cpu frequency the process is running on. The
 * returned value is used to convert TSC counter values to microseconds.
 *
 * @return double.
 * @author cjiang
 */
double get_cpu_frequency() {
    struct timeval start;
    struct timeval end;

    if (gettimeofday(&start, 0)) {
        YY_PROF_TIME_LOG(E_ERROR, "gettimeofday failed in get_cpu_frequency");
        return 0.0;
    }
    uint64_t tsc_start;
    SET_TIMESTAMP_COUNTER(tsc_start);
    /* Sleep for 5 miliseconds. Comparaing with gettimeofday's  few microseconds
     * execution time, this should be enough. */
    usleep(5000);
    if (gettimeofday(&end, 0)) {
        YY_PROF_TIME_LOG(E_ERROR, "gettimeofday failed in get_cpu_frequency");
        return 0.0;
    }
    uint64_t tsc_end;
    SET_TIMESTAMP_COUNTER(tsc_end);
    return (tsc_end - tsc_start) * 1.0 / (GET_US_INTERVAL(&start, &end));
}

/**
 * Calculate frequencies for all available cpus.
 *
 * @author cjiang
 */
int get_all_cpu_frequencies(yy_prof_cpu_t * cpu) {
    int fd, valid;
    mode_t mode;
    uint32_t version;
    uint32_t id;
    size_t cpu_frequencies_size = sizeof(double) * cpu->cpu_num;
    double frequency;
    struct stat st = {0};

    cpu->av_cpu_num = 0;
    cpu->av_cpus = pemalloc(cpu->cpu_num * sizeof(short), 1);
    cpu->cpu_frequencies = pemalloc(cpu_frequencies_size, 1);
    if (cpu->cpu_frequencies == NULL) {
        YY_PROF_TIME_LOG(E_ERROR, "can not allocate memory for cpu_frequencies");
        return -1;
    }

    errno = 0;
    fd = open(YY_PROF_CPU_INFO_FILE, O_RDONLY);
    if(fd > 0) {
        errno = 0;
        if(fstat(fd, &st) == -1) {
            YY_PROF_TIME_LOG(E_ERROR, "stat YY_PROF_CPU_INFO_FILE failed");
            close(fd);
            return -1;
        }
        if(st.st_size == cpu_frequencies_size + sizeof(version)) {
            errno = 0;
            version = 0;
            if(read(fd, &version, sizeof(version)) == sizeof(version)) {
                if(version == CPU_INFO_VERSION 
                        && read(fd, cpu->cpu_frequencies, st.st_size - sizeof(version)) == cpu_frequencies_size) {
                    valid = 1;
                    for(id = 0; id < cpu->cpu_num; ++id) {
                        if(!CPU_ISSET(id, &cpu->prev_mask)) {
                            continue;
                        }
                        cpu->av_cpus[cpu->av_cpu_num ++] = id;
                        if(cpu->cpu_frequencies[id] < 1) {
                            valid = 0;
                            break;
                        }
                    }
                    if(valid) {
                        close(fd);
                        return 0;
                    }
                }
            }
            YY_PROF_TIME_LOG(E_ERROR, "read cpu frequency from file failed");
        }
    }
    close(fd);

    /* Iterate over all cpus found on the machine. */
    for (id = 0; id < cpu->cpu_num; ++ id) {
        if(!CPU_ISSET(id, &cpu->prev_mask)) {
            continue;
        }
        /* Only get the previous cpu affinity mask for the first call. */
        if (bind_to_cpu(cpu, id)) {
            clear_cpu_info(cpu);
            return -1;
        }

        /* Make sure the current process gets scheduled to the target cpu. This
         * might not be necessary though. */
        usleep(0);

        frequency = get_cpu_frequency();
        if (frequency < 1) {
            clear_cpu_info(cpu);
            return -1;
        }
        cpu->cpu_frequencies[id] = frequency;
        cpu->av_cpus[cpu->av_cpu_num++] = id;
    }
    release_cpu(cpu);

    errno = 0;
    mode = umask(0111);
    fd = open(YY_PROF_CPU_INFO_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    umask(mode);
    if(fd == -1) {
        YY_PROF_TIME_LOG(E_ERROR, "open " YY_PROF_CPU_INFO_FILE " for writing failed");
        return -1;
    }
    errno = 0;
    version = CPU_INFO_VERSION;
    if(write(fd, &version, sizeof(version)) != sizeof(version)) {
        YY_PROF_TIME_LOG(E_ERROR, "write cpu_info_version to " YY_PROF_CPU_INFO_FILE " failed"); 
        close(fd);
        return -1;
    }
    if(write(fd, cpu->cpu_frequencies, cpu_frequencies_size) != cpu_frequencies_size) {
        YY_PROF_TIME_LOG(E_ERROR, "write cpu_frequencies to " YY_PROF_CPU_INFO_FILE " failed"); 
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}


/**
 * Bind the current process to a specified CPU. This function is to ensure that
 * the OS won't schedule the process to different processors, which would make
 * values read by rdtsc unreliable.
 *
 * @param uint32_t cpu_id, the id of the logical cpu to be bound to.
 * @return int, 0 on success, and -1 on failure.
 *
 * @author cjiang
 */
int bind_to_cpu(yy_prof_cpu_t * cpu, uint32_t cpu_id) {
    cpu_set_t new_mask;

    CPU_ZERO(&new_mask);
    CPU_SET(cpu_id, &new_mask);

    if (SET_AFFINITY(0, sizeof(cpu_set_t), &new_mask) < 0) {
        YY_PROF_TIME_LOG(E_WARNING, "setaffinity in bind_to_cpu failed");
        return -1;
    }

    /* record the cpu_id the process is bound to. */
    cpu->cur_cpu_id = cpu_id;

    return 0;
}
