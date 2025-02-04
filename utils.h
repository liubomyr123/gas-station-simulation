#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdarg.h>

pthread_mutex_t log_lock;
time_t start_time;

char *get_formatted_time(char *formatted_time);

void print_car(int car_number, const char *message, ...);

void print_tanker(int car_number, const char *message, ...);

void print_debug(const char *message, ...);

int get_os_thread_limit();

void print_ram_info();

void print_cpu_info();

void print_thread_stack_size_info();

void print_max_number_of_threads();

void print_total_simulation_time();

#endif
