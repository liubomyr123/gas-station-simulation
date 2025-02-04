#ifdef __linux__
// "Let's go!"
#else
#error "Only __linux__ supported"
#endif

#include <stdio.h>
#include <pthread.h>
#include <limits.h>
#include <unistd.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <stdarg.h>
#include <bits/local_lim.h>
#include <sys/sysinfo.h>

#include "utils.h"

typedef struct
{
    int number;
    int waiting_time;
    time_t start_waiting_time;
    time_t end_waiting_time;
    int fuel_required;
    bool is_left_without_fuel;
} Car;

typedef struct
{
    int fuel_total;
    int fuel_per_time;
    int number;
    int total_fuel_deliveries;
} Tanker;

void init_attributes_with_min_stack_size(pthread_attr_t *attributes_p);
void setup_main();
void clean_up_main(Car *cars, int cars_number, Tanker *tankers, int tankers_number);
int ask_user_number_of_cars();
int ask_user_amount_of_fuel();

pthread_mutex_t dynamic_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t dynamic_cond = PTHREAD_COND_INITIALIZER;

int gas_station_fuel_storage = 0;
int total_fuel_left = 0;

void *tanker(void *thread_data)
{
    Tanker *tanker_data = (Tanker *)thread_data;
    // int fuel_in_tanker = tanker_data.fuel_total;
    int fuel_per_time_default = tanker_data->fuel_per_time;
    int tanker_id = tanker_data->number;

    printf("\n");
    print_tanker(tanker_id, "Starting to unload fuel into the station... ⛽️");

    while (total_fuel_left > 0)
    {
        int fuel_per_time = (total_fuel_left < fuel_per_time_default) ? total_fuel_left : fuel_per_time_default;
        pthread_mutex_lock(&dynamic_lock);
        gas_station_fuel_storage += fuel_per_time;
        total_fuel_left -= fuel_per_time;
        tanker_data->total_fuel_deliveries++;

        printf("\n");
        print_tanker(tanker_id, "⏳ Unloading %d liters of fuel into the station...", fuel_per_time);
        sleep(2);
        print_tanker(tanker_id, "🛢️  Fuel unloaded successfully. Station storage now holds: %d liters.", gas_station_fuel_storage);

        if (total_fuel_left == 0)
        {
            print_tanker(tanker_id, "Tanker is empty...");
        }
        else
        {
            print_tanker(tanker_id, "🚚 Remaining fuel in tanker: %d liters.", total_fuel_left);
        }

        // Notify all waiting cars that fuel is available
        pthread_cond_broadcast(&dynamic_cond);

        print_tanker(tanker_id, "✅ Fuel available and ready for consumption, preparing for next delivery...");
        printf("\n");

        pthread_mutex_unlock(&dynamic_lock);

        // Give cars a chance to acquire the mutex, check fuel, and proceed if possible
        sleep(2);
    }
    return NULL;
}

void *car(void *thread_data)
{
    Car *car_data = (Car *)thread_data;
    int car_id = car_data->number;
    int car_waiting_time = car_data->waiting_time;
    int car_fuel_required = car_data->fuel_required;

    car_data->start_waiting_time = time(NULL);
    print_car(car_id, "Attempting to get fuel...");
    // printf("\n");
    sleep(1);
    pthread_mutex_lock(&dynamic_lock);

    int is_time_passed = 0;

    if (total_fuel_left + gas_station_fuel_storage < car_fuel_required)
    {
        car_data->end_waiting_time = time(NULL);
        car_data->is_left_without_fuel = true;
        print_car(car_id, "❌ Oh no, not enough fuel. Leaving the station...");
    }
    else
    {
        while (gas_station_fuel_storage < car_fuel_required)
        {
            print_car(car_id, "❌ Not enough fuel, waiting for delivery...");

            if (car_waiting_time > 0)
            {
                time_t current_time = time(NULL);
                struct timespec waiting_time;
                waiting_time.tv_sec = current_time + car_waiting_time;
                waiting_time.tv_nsec = 0;
                int result = pthread_cond_timedwait(&dynamic_cond, &dynamic_lock, &waiting_time);

                if (result != 0)
                {
                    if (result == ETIMEDOUT)
                    {
                        is_time_passed = 1;
                    }
                    break;
                }
            }
            else
            {
                pthread_cond_wait(&dynamic_cond, &dynamic_lock);
            }

            if (total_fuel_left + gas_station_fuel_storage < car_fuel_required)
            {
                break;
            }

            if (gas_station_fuel_storage >= car_fuel_required)
            {
                print_car(car_id, "✅ Fuel is available. Filling up!");
            }
            else
            {
                printf("\n");
                print_car(car_id, "❌ Still not enough fuel, waiting...");
            }
        }

        if (is_time_passed == 1)
        {
            car_data->end_waiting_time = time(NULL);
            car_data->is_left_without_fuel = true;
            print_car(car_id, "❌ Time's up (waited %d seconds). Fuel wasn't delivered in time. Leaving the station...", car_waiting_time);
        }
        else
        {
            if (total_fuel_left + gas_station_fuel_storage < car_fuel_required)
            {
                car_data->end_waiting_time = time(NULL);
                car_data->is_left_without_fuel = true;
                print_car(car_id, "❌ Not enough fuel. Leaving gas station...");
            }
            else
            {
                gas_station_fuel_storage -= car_fuel_required;
                car_data->end_waiting_time = time(NULL);
                print_car(car_id, "✅ Successfully refueled. Remaining fuel at station: %d liters.", gas_station_fuel_storage);
            }
        }
    }

    double waiting_time = difftime(car_data->end_waiting_time, car_data->start_waiting_time);
    print_car(car_id, "⏳ Waited for %.2f seconds", waiting_time);

    pthread_mutex_unlock(&dynamic_lock);
    return NULL;
}

int main()
{
    int cars_number = ask_user_number_of_cars();
    int fuel_in_tanker = ask_user_amount_of_fuel();

    setup_main();

    pthread_t car_threads[cars_number];
    Car cars[cars_number];

    int tankers_number = 1;
    pthread_t tanker_threads[tankers_number];
    Tanker tankers[tankers_number];

    int fuel_per_time_default = 15;
    total_fuel_left = fuel_in_tanker;

    for (int i = 0; i < cars_number; i++)
    {
        int car_id = i + 1;
        cars[i].number = car_id;
        cars[i].waiting_time = 0;
        cars[i].fuel_required = 30;

        pthread_attr_t attributes;
        init_attributes_with_min_stack_size(&attributes);

        if (pthread_create(&car_threads[i], &attributes, car, (void *)&cars[i]) != 0)
        {
            printf("Error: pthread_create for car %d\n", car_id);
        }
    }
    // Cars arrived first
    sleep(2);
    for (int i = 0; i < tankers_number; i++)
    {
        int tanker_id = i + 1;
        tankers[i].fuel_total = fuel_in_tanker;
        tankers[i].number = tanker_id;
        tankers[i].fuel_per_time = fuel_per_time_default;
        tankers[i].total_fuel_deliveries = 0;

        pthread_attr_t attributes;
        init_attributes_with_min_stack_size(&attributes);

        if (pthread_create(&tanker_threads[i], &attributes, tanker, (void *)&tankers[i]) != 0)
        {
            printf("Error: pthread_create for tanker %d\n", tanker_id);
        }
    }

    for (int i = 0; i < cars_number; i++)
    {
        int car_id = i + 1;
        if (pthread_join(car_threads[i], NULL) != 0)
        {
            printf("Error: pthread_join for car %d\n", car_id);
        }
    }
    for (int i = 0; i < tankers_number; i++)
    {
        int tanker_id = i + 1;
        if (pthread_join(tanker_threads[i], NULL) != 0)
        {
            printf("Error: pthread_join for tanker %d\n", tanker_id);
        }
    }

    clean_up_main(cars, cars_number, tankers, tankers_number);
    return 0;
}

void print_car_statistics(Car *cars, int cars_number)
{
    double total_waiting_time = 0;
    for (int i = 0; i < cars_number; i++)
    {
        double waiting_time = difftime(cars[i].end_waiting_time, cars[i].start_waiting_time);
        if (cars[i].is_left_without_fuel) {
            printf("❌ Car #%d left without refueling after: %.2f seconds\n", cars[i].number, waiting_time);
        } else {
            printf("✅ Car #%d was fully fueled after: %.2f seconds\n", cars[i].number, waiting_time);
        }
        total_waiting_time += waiting_time;
    }
    double average_waiting_time = total_waiting_time / cars_number;
    printf("\n");
    printf("⏳ Average car waiting time: %.2f seconds\n", average_waiting_time);
    printf("\n");
}

void print_tanker_statistics(Tanker *tankers, int tankers_number)
{
    int all_tankers_fuel = 0;
    int all_fuel_deliveries = 0;
    int all_fuel_per_time = 0;
    for (int i = 0; i < tankers_number; i++)
    {
        int current_fuel_total = tankers[i].fuel_total;
        all_tankers_fuel += current_fuel_total;

        int current_fuel_deliveries = tankers[i].total_fuel_deliveries;
        all_fuel_deliveries += current_fuel_deliveries;

        int current_fuel_per_time = tankers[i].fuel_per_time;
        all_fuel_per_time += current_fuel_per_time;
    }
    printf("🔥 Total fuel consumed: %d liters\n", all_tankers_fuel - gas_station_fuel_storage);
    printf("🚚 Total fuel deliveries: %d\n", all_fuel_deliveries);
    printf("💧 Tankers refuel 15 liters at a time\n");
}

void clean_up_main(Car *cars, int cars_number, Tanker *tankers, int tankers_number)
{
    pthread_mutex_destroy(&dynamic_lock);
    pthread_cond_destroy(&dynamic_cond);

    printf("\n");
    printf("📊 STATISTICS:\n");
    printf("\n");
    print_car_statistics(cars, cars_number);
    print_tanker_statistics(tankers, tankers_number);
    printf("\n");

#ifdef DEBUG_
    print_total_simulation_time();
#endif
}

int ask_user_number_of_cars()
{
    int number_of_cars = 5;
    int result;

    printf("Please, write amount of cars in range 0 < X < 10: ");
    while (1)
    {
        result = scanf("%d", &number_of_cars);

        if (result == 1 && number_of_cars > 0 && number_of_cars < 10)
        {
            break;
        }
        else
        {
            while (getchar() != '\n')
            {
            };

            printf("Invalid input! Please, write amount of cars in range 0 < X < 10: ");
        }
    }

    return number_of_cars;
}

int ask_user_amount_of_fuel()
{
    int amount_of_fuel = 150;
    int result;

    printf("Please, write amount of fuel in range 0 < X < 500: ");
    while (1)
    {
        result = scanf("%d", &amount_of_fuel);

        if (result == 1 && amount_of_fuel > 0 && amount_of_fuel < 500)
        {
            break;
        }
        else
        {
            while (getchar() != '\n')
            {
            };

            printf("Invalid input! Please, write amount of fuel in range 0 < X < 500: ");
        }
    }

    return amount_of_fuel;
}

void setup_main()
{
    start_time = time(NULL);

    pthread_mutex_init(&dynamic_lock, NULL);
    pthread_cond_init(&dynamic_cond, NULL);

#ifdef DEBUG_
    print_thread_stack_size_info();
    print_max_number_of_threads();
    print_cpu_info();
    print_ram_info();
#endif

#ifdef DEBUG_
    printf("\n");
#endif
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
            print_debug("Invalid settings for thread attributes (EINVAL).\n");
            break;
        }
        default:
        {
            print_debug("Unknown error initializing pthread attributes: %s\n", strerror(init_result));
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
            print_debug("Invalid stack size (EINVAL). The stack size is too small or too large.\n");
            break;
        }
        default:
        {
            print_debug("Unknown error setting stack size: %s\n", strerror(setstacksize_result));
            break;
        }
        }
    }
}