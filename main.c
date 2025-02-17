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
#include <semaphore.h>

#include "util_read_data_parser.h"
#include "utils.h"

typedef struct
{
    int number;
    int waiting_time;
    VehicleType vehicle_type;
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
int setup_main();
void clean_up_main();
int occupy_new_fuel_pump(int car_id, VehicleType vehicle_type);
int free_fuel_pump(int car_id, VehicleType vehicle_type);
int get_number_of_free_fuel_pumps();
int get_number_of_occupied_fuel_pumps();
void print_statistics(Car *cars, Tanker *tankers);
void print_tanker_statistics(Tanker *tankers);
void print_car_statistics(Car *cars);
int read_json();
int init_simulation_data();

pthread_mutex_t dynamic_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t dynamic_cond = PTHREAD_COND_INITIALIZER;
sem_t fuel_pump_semaphore;

static int gas_station_fuel_storage = 0;
static int total_fuel_left = 0;

static int number_of_fuel_pumps = 1;
static int number_of_cars = 1;
static int tankers_number = 1;
static int fuel_per_time = 15;
static int fuel_in_tanker = 150;
static int fuel_pump_occupied = 0;
static int *fuel_pumps_list = NULL;

static ReadDataParserResult *read_data_parser_result = NULL;

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
        pthread_mutex_lock(&dynamic_lock); // 🔒
        gas_station_fuel_storage += fuel_per_time;
        total_fuel_left -= fuel_per_time;
        tanker_data->total_fuel_deliveries++;

        printf("\n");
        print_tanker(tanker_id, "⏳ Unloading %d liters of fuel into the station...", fuel_per_time);
        print_tanker(tanker_id, "🛢️  Fuel unloaded successfully. Station storage now holds: %d liters.", gas_station_fuel_storage);

        if (total_fuel_left == 0)
        {
            print_tanker(tanker_id, "Tanker is empty...");
            print_tanker(tanker_id, "Tanker is leaving the station...");
        }
        else
        {
            print_tanker(tanker_id, "🚚 Remaining fuel in tanker: %d liters.", total_fuel_left);
            print_tanker(tanker_id, "✅ Fuel available and ready for consumption, preparing for next delivery...");
        }

        // Notify all waiting cars that fuel is available
        pthread_cond_broadcast(&dynamic_cond); // 🔔
        pthread_mutex_unlock(&dynamic_lock);   // 🔓

        // Give cars a chance to acquire the mutex, check fuel, and proceed if possible
        // This also shows that tanker is fueling during 1 second
        sleep(1);
    }
    return NULL;
}

void *car(void *thread_data)
{
    Car *car_data = (Car *)thread_data;
    int car_id = car_data->number;
    int car_waiting_time = car_data->waiting_time;
    int car_fuel_required = car_data->fuel_required;
    VehicleType vehicle_type = car_data->vehicle_type;

    sem_wait(&fuel_pump_semaphore);

    printf("\n");
    print_car(vehicle_type, car_id, "Attempting to get fuel...\n");

    car_data->start_waiting_time = time(NULL);
    pthread_mutex_lock(&dynamic_lock); // 🔒

    occupy_new_fuel_pump(car_id, vehicle_type);

    int is_time_passed = 0;
    if (total_fuel_left + gas_station_fuel_storage < car_fuel_required)
    {
        car_data->end_waiting_time = time(NULL);
        car_data->is_left_without_fuel = true;
        print_car(vehicle_type, car_id, "❌ Oh no, not enough fuel. Leaving the station...");
    }
    else
    {
        while (gas_station_fuel_storage < car_fuel_required)
        {
            if (car_waiting_time == 0)
            {
                is_time_passed = 1;
                break;
            }

            print_car(vehicle_type, car_id, "❌ Not enough fuel, waiting for delivery...");
            if (car_waiting_time > 0)
            {
                // time_t current_time = time(NULL);
                // struct timespec waiting_time;
                // waiting_time.tv_sec = current_time + car_waiting_time;
                // waiting_time.tv_nsec = 0;
                // Or
                // struct timespec waiting_time;
                // clock_gettime(CLOCK_REALTIME, &waiting_time);
                // waiting_time.tv_sec += car_waiting_time;
                // int result = pthread_cond_timedwait(&dynamic_cond, &dynamic_lock, &waiting_time);
                // if (result != 0)
                // {
                //     if (result == ETIMEDOUT)
                //     {
                //         is_time_passed = 1;
                //         break;
                //     }
                // }

                // ERROR: pthread_cond_timedwait works wrong, for some reason it skips moment when we already out of waiting time
                // So, instead just use pthread_cond_wait and check if car can still wait
                pthread_cond_wait(&dynamic_cond, &dynamic_lock);

                int waiting_time = difftime(time(NULL), car_data->start_waiting_time);
                if (waiting_time >= car_waiting_time)
                {
                    is_time_passed = 1;
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
                print_car(vehicle_type, car_id, "✅ Fuel is available. Filling up!");
            }
            else
            {
                printf("\n");
                print_car(vehicle_type, car_id, "❌ Still not enough fuel, waiting...");
            }
        }

        if (is_time_passed == 1)
        {
            car_data->end_waiting_time = time(NULL);
            car_data->is_left_without_fuel = true;
            print_car(vehicle_type, car_id, "❌ Time's up (waited %d seconds). Fuel wasn't delivered in time. Leaving the station...", car_waiting_time);
        }
        else
        {
            if (total_fuel_left + gas_station_fuel_storage < car_fuel_required)
            {
                car_data->end_waiting_time = time(NULL);
                car_data->is_left_without_fuel = true;
                print_car(vehicle_type, car_id, "❌ Not enough fuel. Leaving gas station...");
            }
            else
            {
                gas_station_fuel_storage -= car_fuel_required;
                car_data->end_waiting_time = time(NULL);
                print_car(vehicle_type, car_id, "✅ Successfully refueled %d liters. Remaining fuel at station: %d liters.", car_fuel_required, gas_station_fuel_storage);
            }
        }
    }

    double waiting_time = difftime(car_data->end_waiting_time, car_data->start_waiting_time);
    print_car(vehicle_type, car_id, "⏳ Waited for %.2f seconds\n", waiting_time);

    pthread_mutex_unlock(&dynamic_lock); // 🔓

    free_fuel_pump(car_id, vehicle_type);
    sem_post(&fuel_pump_semaphore);
    return NULL;
}

int main()
{
    if (init_simulation_data() == 0)
    {
        clean_up_main();
        return 1;
    }

    if (setup_main() == 0)
    {
        clean_up_main();
        return 1;
    }

    printf("🚗 Welcome to the fueling station simulation! 🚗\n");

    pthread_t car_threads[number_of_cars];
    Car cars[number_of_cars];

    pthread_t tanker_threads[tankers_number];
    Tanker tankers[tankers_number];

    total_fuel_left = fuel_in_tanker;

    for (int i = 0; i < number_of_cars; i++)
    {
        Vehicle *current_vehicle = read_data_parser_result->json_result->result_vehicles[i];

        int car_id = i + 1;
        cars[i].number = car_id;
        cars[i].waiting_time = current_vehicle->wait_time_sec;
        cars[i].fuel_required = current_vehicle->fuel_needed;
        cars[i].is_left_without_fuel = false;
        cars[i].vehicle_type = current_vehicle->vehicle_type;

        pthread_attr_t attributes;
        init_attributes_with_min_stack_size(&attributes);

        if (pthread_create(&car_threads[i], &attributes, car, (void *)&cars[i]) != 0)
        {
            printf("❌ Error: pthread_create for car %d\n", car_id);
        }
    }
    // Cars arrived first
    sleep(2);
    for (int i = 0; i < tankers_number; i++)
    {
        int tanker_id = i + 1;
        tankers[i].fuel_total = fuel_in_tanker;
        tankers[i].number = tanker_id;
        tankers[i].fuel_per_time = fuel_per_time;
        tankers[i].total_fuel_deliveries = 0;

        pthread_attr_t attributes;
        init_attributes_with_min_stack_size(&attributes);

        if (pthread_create(&tanker_threads[i], &attributes, tanker, (void *)&tankers[i]) != 0)
        {
            printf("❌ Error: pthread_create for tanker %d\n", tanker_id);
        }
    }

    for (int i = 0; i < number_of_cars; i++)
    {
        int car_id = i + 1;
        if (pthread_join(car_threads[i], NULL) != 0)
        {
            printf("❌ Error: pthread_join for car %d\n", car_id);
        }
    }
    for (int i = 0; i < tankers_number; i++)
    {
        int tanker_id = i + 1;
        if (pthread_join(tanker_threads[i], NULL) != 0)
        {
            printf("❌ Error: pthread_join for tanker %d\n", tanker_id);
        }
    }

    print_statistics(cars, tankers);
    clean_up_main();
    return 0;
}

int read_json()
{
    char *path = "data.json";
    read_data_parser_result = read_data_parser(path, false);
    if (
        read_data_parser_result == NULL                     //
        || read_data_parser_result->json_result == NULL     //
        || read_data_parser_result->status != CORRECT_VALUE //
    )
    {
        printf("❌ Failed to parse 'data.json' file.\n");
        printf("\n");
        printf("💡 Try running validation command. It will check the file for errors and provide detailed validation results.\n");
        clean_up_read_data_parser_result(&read_data_parser_result);
        return 0;
    }
    printf("✅ Successfully parsed 'data.json' file.\n");
    print_json_result(read_data_parser_result->json_result);
    return 1;
}

int init_simulation_data()
{
    int read_json_result = read_json();
    if (read_json_result == 0)
    {
        return 0;
    }

    number_of_fuel_pumps = read_data_parser_result->json_result->fuel_pumps_count;
    fuel_per_time = read_data_parser_result->json_result->fuel_transfer_rate;
    fuel_in_tanker = read_data_parser_result->json_result->initial_fuel_in_tanker;
    number_of_cars = read_data_parser_result->json_result->result_vehicles_length;

    fuel_pumps_list = (int *)malloc(number_of_fuel_pumps * sizeof(int));
    if (fuel_pumps_list == NULL)
    {
        printf("❌ Failed to allocate memory for fuel pumps list.\n");
        return 0;
    }

    for (int i = 0; i < number_of_fuel_pumps; i++)
    {
        fuel_pumps_list[i] = -1;
    }

    return 1;
}

int get_number_of_free_fuel_pumps()
{
    int free_fuel_pumps = 0;
    for (int i = 0; i < number_of_fuel_pumps; i++)
    {
        if (fuel_pumps_list[i] == -1)
        {
            free_fuel_pumps++;
        }
    }
    return free_fuel_pumps;
}

int get_number_of_occupied_fuel_pumps()
{
    int occupied_fuel_pumps = 0;
    for (int i = 0; i < number_of_fuel_pumps; i++)
    {
        if (fuel_pumps_list[i] != -1)
        {
            occupied_fuel_pumps++;
        }
    }
    return occupied_fuel_pumps;
}

int occupy_new_fuel_pump(int car_id, VehicleType vehicle_type)
{
    fuel_pump_occupied++;
    int occupied_fuel_pump = -1;
    for (int i = 0; i < number_of_fuel_pumps; i++)
    {
        if (fuel_pumps_list[i] == -1)
        {
            if (vehicle_type == VEHICLE_AUTO)
            {
                printf("\n⛽️ Fuel pump #%d occupied by 🚗 #%d: %d/%d pumps now in use.\n\n",
                       i + 1,
                       car_id, fuel_pump_occupied, number_of_fuel_pumps);
            }
            else if (vehicle_type == VEHICLE_VAN)
            {
                printf("\n⛽️ Fuel pump #%d occupied by 🚙 #%d: %d/%d pumps now in use.\n\n",
                       i + 1,
                       car_id, fuel_pump_occupied, number_of_fuel_pumps);
            }
            else if (vehicle_type == VEHICLE_TRUCK)
            {
                printf("\n⛽️ Fuel pump #%d occupied by 🚛 #%d: %d/%d pumps now in use.\n\n",
                       i + 1,
                       car_id, fuel_pump_occupied, number_of_fuel_pumps);
            }
            fuel_pumps_list[i] = car_id;
            occupied_fuel_pump = i;
            break;
        }
    }

    if (get_number_of_occupied_fuel_pumps() == number_of_fuel_pumps)
    {
        printf("⛽️ All fuel pumps are occupied.\n");
    }

    return occupied_fuel_pump;
}

int free_fuel_pump(int car_id, VehicleType vehicle_type)
{
    fuel_pump_occupied--;
    int released_fuel_pump = -1;
    for (int i = 0; i < number_of_fuel_pumps; i++)
    {
        if (fuel_pumps_list[i] == car_id)
        {
            if (vehicle_type == VEHICLE_AUTO)
            {
                printf("⛽️ Fuel pump #%d freed by 🚗 #%d: %d/%d pumps still occupied.\n",
                       i + 1,
                       car_id, fuel_pump_occupied, number_of_fuel_pumps);
            }
            else if (vehicle_type == VEHICLE_VAN)
            {
                printf("⛽️ Fuel pump #%d freed by 🚙 #%d: %d/%d pumps still occupied.\n",
                       i + 1,
                       car_id, fuel_pump_occupied, number_of_fuel_pumps);
            }
            else if (vehicle_type == VEHICLE_TRUCK)
            {
                printf("⛽️ Fuel pump #%d freed by 🚛 #%d: %d/%d pumps still occupied.\n",
                       i + 1,
                       car_id, fuel_pump_occupied, number_of_fuel_pumps);
            }
            fuel_pumps_list[i] = -1;
            released_fuel_pump = i;
            break;
        }
    }

    if (get_number_of_free_fuel_pumps() == number_of_fuel_pumps)
    {
        printf("⛽️ All fuel pumps are free.\n");
    }

    return released_fuel_pump;
}

void print_car_statistics(Car *cars)
{
    double total_waiting_time = 0;
    double total_fuel = 0;
    for (int i = 0; i < number_of_cars; i++)
    {
        double waiting_time = difftime(cars[i].end_waiting_time, cars[i].start_waiting_time);
        if (cars[i].is_left_without_fuel)
        {
            printf("❌ Car #%d left without refueling after: %.2f seconds\n", cars[i].number, waiting_time);
        }
        else
        {
            printf("✅ Car #%d was fully fueled %d liters after: %.2f seconds\n", cars[i].number, cars[i].fuel_required, waiting_time);
        }
        total_waiting_time += waiting_time;
        total_fuel += cars[i].fuel_required;
    }
    double average_waiting_time = total_waiting_time / number_of_cars;
    double average_fuel = total_fuel / number_of_cars;
    printf("\n");
    printf("⏳ Average car waiting time: %.2f seconds\n", average_waiting_time);
    printf("🛢️  The average fuel per car is: %.2f liters.\n", average_fuel);
    printf("\n");
}

void print_tanker_statistics(Tanker *tankers)
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
    printf("🛢️  Fuel left in storage: %d liters\n", gas_station_fuel_storage);
    printf("🚚 Total fuel deliveries: %d (%d liters each)\n", all_fuel_deliveries, fuel_per_time);
}

void print_statistics(Car *cars, Tanker *tankers)
{
    printf("\n");
    printf("📊 STATISTICS:\n");
    printf("\n");
    print_car_statistics(cars);
    print_tanker_statistics(tankers);
    printf("\n");
}

int setup_main()
{
    start_time = time(NULL);

    if (pthread_mutex_init(&dynamic_lock, NULL) != 0)
    {
        printf("❌ Failed to init mutex.\n");
        return 0;
    }
    if (pthread_cond_init(&dynamic_cond, NULL) != 0)
    {
        printf("❌ Failed to init condition variables.\n");
        return 0;
    }
    if (sem_init(&fuel_pump_semaphore, 0, number_of_fuel_pumps) != 0)
    {
        printf("❌ Failed to init semaphore.\n");
        return 0;
    }

#ifdef DEBUG_
    print_thread_stack_size_info();
    print_max_number_of_threads();
    print_cpu_info();
    print_ram_info();
#endif

#ifdef DEBUG_
    printf("\n");
#endif

    return 1;
}

void clean_up_main()
{
    if (pthread_mutex_destroy(&dynamic_lock) != 0)
    {
        printf("❌ Failed to destroy mutex.\n");
    }
    if (pthread_cond_destroy(&dynamic_cond) != 0)
    {
        printf("❌ Failed to destroy condition variables.\n");
    }
    if (sem_destroy(&fuel_pump_semaphore) != 0)
    {
        printf("❌ Failed to destroy semaphore.\n");
    }

    clean_up_read_data_parser_result(&read_data_parser_result);

    if (fuel_pumps_list != NULL)
    {
        free(fuel_pumps_list);
        fuel_pumps_list = NULL;
    }

#ifdef DEBUG_
    print_total_simulation_time();
#endif
}
