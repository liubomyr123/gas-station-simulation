#include <stdio.h>
#include <pthread.h>
#include <limits.h>
#include <unistd.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <bits/local_lim.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#include "util_read_data_parser.h"

pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
time_t start_time = 0;

char *get_formatted_time(char *formatted_time)
{
    time_t current_time = time(NULL);
    time_t diff_time = difftime(current_time, start_time);

    struct tm *utc_time = gmtime(&diff_time);
    strftime(formatted_time, 10, "%H:%M:%S", utc_time);

    return formatted_time;
}

void print_car(VehicleType vehicle_type, int car_number, const char *message, ...)
{
    va_list args;
    va_start(args, message);

    pthread_mutex_lock(&log_lock);

// #ifdef DEBUG_
    char formatted_time[10];
    get_formatted_time(formatted_time);
    if (vehicle_type == VEHICLE_AUTO)
    {
        printf("[%s] 🚗 #%d: ", formatted_time, car_number);
    }
    else if (vehicle_type == VEHICLE_VAN)
    {
        printf("[%s] 🚙 #%d: ", formatted_time, car_number);
    }
    else if (vehicle_type == VEHICLE_TRUCK)
    {
        printf("[%s] 🚛 #%d: ", formatted_time, car_number);
    }
    else
    {
        printf("[%s] 🚗 #%d: ", formatted_time, car_number);
    }
// #else
//     if (vehicle_type == VEHICLE_AUTO)
//     {
//         printf("🚗 #%d: ", car_number);
//     }
//     else if (vehicle_type == VEHICLE_VAN)
//     {
//         printf("🚙 #%d: ", car_number);
//     }
//     else if (vehicle_type == VEHICLE_TRUCK)
//     {
//         printf("🚛 #%d: ", car_number);
//     }
//     else
//     {
//         printf("🚗 #%d: ", car_number);
//     }
// #endif
    vprintf(message, args);
    printf("\n");

    pthread_mutex_unlock(&log_lock);

    va_end(args);
}

void print_tanker(int car_number, const char *message, ...)
{
    va_list args;
    va_start(args, message);

    pthread_mutex_lock(&log_lock);

// #ifdef DEBUG_
    char formatted_time[10];
    get_formatted_time(formatted_time);
    printf("[%s] 🚚 #%d: ", formatted_time, car_number);
// #else
//     printf("🚚 #%d: ", car_number);
// #endif
    vprintf(message, args);
    printf("\n");

    pthread_mutex_unlock(&log_lock);

    va_end(args);
}

void print_debug(const char *message, ...)
{
    va_list args;
    va_start(args, message);

    char formatted_time[10];
    get_formatted_time(formatted_time);

    pthread_mutex_lock(&log_lock);

    printf("[%s] 🛠️: ", formatted_time);
    vprintf(message, args);
    printf("\n");

    pthread_mutex_unlock(&log_lock);

    va_end(args);
}

int get_os_thread_limit()
{
    int count = 0;

    struct rlimit limit;
    if (getrlimit(RLIMIT_NPROC, &limit) == 0)
    {
        if (limit.rlim_cur == RLIM_INFINITY)
        {
            return -1;
        }
        count = limit.rlim_cur;
    }

    return count;
}

void print_ram_info()
{
    long total_ram = sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE);
    struct sysinfo info;

    if (sysinfo(&info) == 0)
    {
        if (total_ram >= 1024 * 1024 * 1024)
        {
            print_debug("Total RAM: %.2f GB", (double)total_ram / (1024 * 1024 * 1024));
        }
        else if (total_ram >= 1024 * 1024)
        {
            print_debug("Total RAM: %.2f MB", (double)total_ram / (1024 * 1024));
        }
        else if (total_ram >= 1024)
        {
            print_debug("Total RAM: %.2f KB", (double)total_ram / 1024);
        }
        else
        {
            print_debug("Total RAM: %ld bytes", total_ram);
        }

        if (info.freeram >= 1024 * 1024 * 1024)
        {
            print_debug("Free RAM: %.2f GB", (double)info.freeram / (1024 * 1024 * 1024));
        }
        else if (info.freeram >= 1024 * 1024)
        {
            print_debug("Free RAM: %.2f MB", (double)info.freeram / (1024 * 1024));
        }
        else if (info.freeram >= 1024)
        {
            print_debug("Free RAM: %.2f KB", (double)info.freeram / 1024);
        }
        else
        {
            print_debug("Free RAM: %ld bytes", info.freeram);
        }
    }
    else
    {
        perror("sysinfo");
    }
}

void print_cpu_info()
{
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    print_debug("Number of CPU cores (online): %d", num_cores);

    int total_cores = sysconf(_SC_NPROCESSORS_CONF);
    print_debug("Number of CPU cores (configured): %d", total_cores);
}

void print_thread_stack_size_info()
{
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);

    size_t stack_size = 0;
    pthread_attr_getstacksize(&attributes, &stack_size);
    print_debug("Default thread stack size: %ld", stack_size);

    pthread_attr_setstacksize(&attributes, PTHREAD_STACK_MIN);

    pthread_attr_getstacksize(&attributes, &stack_size);
    print_debug("PTHREAD_STACK_MIN stack size: %ld", stack_size);
}

void print_max_number_of_threads()
{
    int os_thread_limit = get_os_thread_limit();
    print_debug("Max number of threads: %ld", os_thread_limit);
}

void print_total_simulation_time()
{
    time_t end_time = time(NULL);
    double elapsed_time = difftime(end_time, start_time);
    print_debug("⏳ Total simulation time: %.2f seconds", elapsed_time);
}

void init_attributes_with_min_stack_size(pthread_attr_t *attributes_p)
{
    int init_result = pthread_attr_init(attributes_p);
    if (init_result != 0)
    {
        switch (init_result)
        {
        case EINVAL:
        {
            print_debug("❌ Invalid settings for thread attributes (EINVAL).\n");
            break;
        }
        default:
        {
            print_debug("❌ Unknown error initializing pthread attributes: %s\n", strerror(init_result));
            break;
        }
        }
    }

    int setstacksize_result = pthread_attr_setstacksize(attributes_p, PTHREAD_STACK_MIN * 2);
    if (setstacksize_result != 0)
    {
        switch (setstacksize_result)
        {
        case EINVAL:
        {
            print_debug("❌ Invalid stack size (EINVAL). The stack size is too small or too large.\n");
            break;
        }
        default:
        {
            print_debug("❌ Unknown error setting stack size: %s\n", strerror(setstacksize_result));
            break;
        }
        }
    }
}
