#ifndef UTIL_READ_DATA_PARSER_H
#define UTIL_READ_DATA_PARSER_H

#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#include "cjson/cJSON.h"

#define MAX_VEHICLES 100

#define my_cJSON_ArrayForEach(element, array, index) for (element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next, index++)

typedef enum
{
    VEHICLE_AUTO,
    VEHICLE_TRUCK,
    VEHICLE_VAN,
    VEHICLE_NOT_FOUND,
} VehicleType;

typedef struct
{
    VehicleType vehicle_type;
    int wait_time_sec;
    int fuel_needed;
} Vehicle;

typedef struct
{
    int fuel_pumps_count;
    int max_vehicle_capacity;
    int initial_fuel_in_tanker;
    int fuel_transfer_rate;
    int all_vehicles_length;
    int result_vehicles_length;
    bool randomize_arrival;
    Vehicle **all_vehicles;
    Vehicle **result_vehicles;
} UserJsonResult;

typedef enum
{
    NOT_FOUND,
    CORRECT_VALUE,
    WRONG_VALUE,
    WRONG_TYPE,
    MAX_VEHICLES_ERROR,
    UNKNOWN_ERROR,
    EMPTY_VEHICLE_CAPACITY_VALUE,
    EMPTY_VALUE,
    ALLOCATION_ERROR
} StatusType;

typedef struct
{
    UserJsonResult *json_result;
    StatusType status;
} ReadDataParserResult;

int get_file_size(FILE *fp);

StatusType get_file_buffer(char **buffer, char *path);

StatusType parse_file_buffer(cJSON **json, char **buffer);

StatusType get_int_value(cJSON *json, int *result, char *name);

StatusType get_string_value(cJSON *json, char **result, char *name);

StatusType get_boolean_value(cJSON *json, bool *result, char *name);

StatusType get_array_value(cJSON *json, void **result, char *name);

VehicleType get_vehicle_type(cJSON *vehicle_p, bool show_logs);

int get_custom_waiting_list_count(cJSON *custom_waiting_list_item_p, int *count, bool show_logs);

StatusType get_custom_waiting_list_fuel_needed(cJSON *custom_waiting_list_item_p, int *fuel_needed, bool show_logs);

StatusType get_custom_waiting_list_wait_time_sec(cJSON *custom_waiting_list_item_p, int *wait_time_sec, bool show_logs);

StatusType get_custom_waiting_list(
    cJSON *json,
    VehicleType vehicle_type,
    int default_wait_time_sec,
    int default_fuel_needed,
    Vehicle **all_user_vehicles,
    int *count_added_vehicles);

StatusType validate_custom_waiting_list(cJSON *json, int *local_vehicle_capacity);

int get_default_fuel_needed(cJSON *vehicle_p, int *default_fuel_needed, bool show_logs);

int get_default_wait_time_sec(cJSON *vehicle_p, int *default_wait_time_sec, bool show_logs);

int get_default_count(cJSON *vehicle_p, int *default_count, bool show_logs);

StatusType validate_vehicles(cJSON *vehicles_array_p, int *all_vehicles_length, int *indexes);

StatusType get_vehicles(cJSON *json, UserJsonResult *json_result);

void print_json_result(UserJsonResult *json_result);

void clean_up();

StatusType randomize_vehicles(UserJsonResult *json_result);

void clean_up_json_result(UserJsonResult **json_result);

int handle_randomize_vehicles(StatusType randomize_result, UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result);

void clean_up_read_data_parser_result(ReadDataParserResult **read_data_parser_result);

StatusType get_limited_amount(UserJsonResult *json_result);

ReadDataParserResult *read_data_parser(char *path);

#endif