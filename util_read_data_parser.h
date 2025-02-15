#ifndef UTIL_READ_DATA_PARSER_H
#define UTIL_READ_DATA_PARSER_H

#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#include "cjson/cJSON.h"

#define MAX_VEHICLES 100
#define MAX_FUEL_PUMPS_COUNT 10
#define MAX_INITIAL_FUEL_IN_TANKER 500
#define MAX_FUEL_TRANSFER_RATE 100

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
    VALIDATION_ERROR,
    MAX_VALUE_ERROR,
    ALLOCATION_ERROR
} StatusType;

typedef struct
{
    UserJsonResult *json_result;
    StatusType status;
} ReadDataParserResult;

void print_json_result(UserJsonResult *json_result);

void clean_up_read_data_parser_result(ReadDataParserResult **read_data_parser_result);

ReadDataParserResult *read_data_parser(char *path);

#endif