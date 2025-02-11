#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include "cjson/cJSON.h"

#define MAX_VEHICLES 100

typedef enum
{
    VEHICLE_AUTO,
    VEHICLE_TRUCK,
    VEHICLE_VAN,
    VEHICLE_NOT_FOUND,
} VehicleType;

typedef struct
{
    int id;
    VehicleType vehicle_type;
    int wait_time_sec;
    int fuel_needed;
    time_t start_wait_time_sec;
    time_t end_wait_time_sec;
    bool is_left_without_fuel;
} Car;

typedef struct
{
    int fuel_pumps_count;
    int max_vehicle_capacity;
    int initial_fuel_in_tanker;
    int fuel_transfer_rate;
    int current_vehicle_capacity;
    Car *vehicles;
} UserJsonResult;

typedef enum
{
    NOT_FOUND,
    CORRECT_VALUE,
    WRONG_TYPE,
    UNKNOWN_ERROR,
    ALLOCATION_ERROR
} StatusType;

int get_file_size(FILE *fp)
{
    int size = 0;
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        printf("Error of fseek(): SEEK_END\n");
        return -1;
    }
    size = ftell(fp);
    if (fseek(fp, 0, SEEK_SET) != 0)
    {
        printf("Error of fseek(): SEEK_SET\n");
        return -1;
    }

    return size;
}

StatusType get_file_buffer(char **buffer)
{
    FILE *fp = fopen("data.json", "r");
    if (fp == NULL)
    {
        perror("Unable to open the file");
        return UNKNOWN_ERROR;
    }

    const int size = get_file_size(fp);
    if (size < 0)
    {
        printf("Unable to get file size\n");
        fclose(fp);
        return UNKNOWN_ERROR;
    }

    (*buffer) = (char *)malloc(size * sizeof(char));
    if ((*buffer) == NULL)
    {
        printf("Unable to allocate memory for file buffer\n");
        fclose(fp);
        return ALLOCATION_ERROR;
    }
    size_t number_of_bytes_transferred = fread((*buffer), 1, size, fp);
    if (number_of_bytes_transferred != size)
    {
        printf("Expected to read %d bytes, but read only %ld bytes\n", size, number_of_bytes_transferred);
        fclose(fp);
        return UNKNOWN_ERROR;
    }
    fclose(fp);

    return CORRECT_VALUE;
}

StatusType parse_file_buffer(cJSON **json, char **buffer)
{
    (*json) = cJSON_Parse((*buffer));
    if (*json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            printf("Error: %s\n", error_ptr);
        }
        // cJSON_Delete(*json);
        return UNKNOWN_ERROR;
    }
    return CORRECT_VALUE;
}

StatusType get_int_value(cJSON *json, int *result, char *name)
{
    cJSON *int_value_p = cJSON_GetObjectItemCaseSensitive(json, name);
    if (int_value_p == NULL)
    {
        return NOT_FOUND;
    }
    if (!cJSON_IsNumber(int_value_p))
    {
        return WRONG_TYPE;
    }

    *result = int_value_p->valueint;
    return CORRECT_VALUE;
}

StatusType get_string_value(cJSON *json, char **result, char *name)
{
    cJSON *string_value_p = cJSON_GetObjectItemCaseSensitive(json, name);
    if (string_value_p == NULL)
    {
        return NOT_FOUND;
    }
    if (!cJSON_IsString(string_value_p))
    {
        return WRONG_TYPE;
    }
    size_t string_length = strlen(string_value_p->valuestring);

    *result = (char *)malloc(string_length * sizeof(char));
    if (*result == NULL) {
        return ALLOCATION_ERROR;
    }
    strcpy(*result, string_value_p->valuestring);

    return CORRECT_VALUE;
}

StatusType get_boolean_value(cJSON *json, _Bool *result, char *name)
{
    cJSON *boolean_value_p = cJSON_GetObjectItemCaseSensitive(json, name);
    if (boolean_value_p == NULL)
    {
        return NOT_FOUND;
    }
    if (!cJSON_IsBool(boolean_value_p))
    {
        return WRONG_TYPE;
    }

    if (cJSON_IsFalse(boolean_value_p))
    {
        *result = 0;
    }
    if (cJSON_IsTrue(boolean_value_p))
    {
        *result = 1;
    }
    return CORRECT_VALUE;
}

StatusType get_array_value(cJSON *json, void **result, char *name)
{
    cJSON *array_value_p = cJSON_GetObjectItemCaseSensitive(json, name);
    if (array_value_p == NULL)
    {
        return NOT_FOUND;
    }
    if (!cJSON_IsArray(array_value_p))
    {
        return WRONG_TYPE;
    }

    *result = array_value_p;
    return CORRECT_VALUE;
}

VehicleType get_vehicle_type(cJSON *vehicle_p)
{
    VehicleType result_status = VEHICLE_NOT_FOUND;

    char *vehicle_type = NULL;
    StatusType vehicle_type_result = get_string_value(vehicle_p, &vehicle_type, "vehicle_type");
    if (vehicle_type_result == ALLOCATION_ERROR)
    {
        printf("❌ [vehicle_type] unable to allocate memory\n");
    }
    if (vehicle_type_result == NOT_FOUND)
    {
        printf("❌ [vehicle_type] was not found\n");
    }
    else if (vehicle_type_result == WRONG_TYPE)
    {
        printf("❌ [vehicle_type] is not string\n");
    }
    else
    {
        if (strcmp(vehicle_type, "auto") == 0)
        {
            result_status = VEHICLE_AUTO;
        }
        else if (strcmp(vehicle_type, "truck") == 0)
        {
            result_status = VEHICLE_TRUCK;
        }
        else if (strcmp(vehicle_type, "van") == 0)
        {
            result_status = VEHICLE_VAN;
        }
    }

    free(vehicle_type);
    return result_status;
}

StatusType get_custom_waiting_list(cJSON *json)
{
    cJSON *custom_waiting_list_p = NULL;
    StatusType custom_waiting_list_result = get_array_value(json, (void **)&custom_waiting_list_p, "custom_waiting_list");
    if (custom_waiting_list_result == NOT_FOUND)
    {
        return NOT_FOUND;
    }
    else if (custom_waiting_list_result == WRONG_TYPE)
    {
        return WRONG_TYPE;
    }

    printf("\n");
    printf("✅ [custom_waiting_list] length: %d\n", cJSON_GetArraySize(custom_waiting_list_p));

    cJSON *custom_waiting_list_item_p = NULL;
    cJSON_ArrayForEach(custom_waiting_list_item_p, custom_waiting_list_p)
    {
        printf("\n");
        printf("--------------------------------Level 2: START\n");
        int count = 0;
        StatusType count_result = get_int_value(custom_waiting_list_item_p, &count, "count");
        if (count_result == NOT_FOUND)
        {
            printf("❌ [count] was not found\n");
            continue;
        }
        else if (count_result == WRONG_TYPE)
        {
            printf("❌ [count] is not number\n");
            continue;
        }

        printf("✅ [count]: %d\n", count);

        int fuel_needed = 0;
        StatusType fuel_needed_result = get_int_value(custom_waiting_list_item_p, &fuel_needed, "fuel_needed");
        if (fuel_needed_result == NOT_FOUND)
        {
            printf("❌ [fuel_needed] was not found\n");
        }
        else if (fuel_needed_result == WRONG_TYPE)
        {
            printf("❌ [fuel_needed] is not number\n");
            continue;
        }
        else
        {
            printf("✅ [fuel_needed]: %d\n", fuel_needed);
        }

        int wait_time_sec = 0;
        StatusType wait_time_sec_result = get_int_value(custom_waiting_list_item_p, &wait_time_sec, "wait_time_sec");
        if (wait_time_sec_result == NOT_FOUND)
        {
            printf("❌ [wait_time_sec] was not found\n");
        }
        else if (wait_time_sec_result == WRONG_TYPE)
        {
            printf("❌ [wait_time_sec] is not number\n");
            continue;
        }
        else
        {
            printf("✅ [wait_time_sec]: %d\n", wait_time_sec);
        }

        printf("--------------------------------Level 2: END\n");
    }

    return CORRECT_VALUE;
}

StatusType get_vehicles(cJSON *json, UserJsonResult *json_result)
{
    cJSON *vehicles_array_p = NULL;
    StatusType vehicles_array_result = get_array_value(json, (void **)&vehicles_array_p, "vehicles");
    if (vehicles_array_result == NOT_FOUND)
    {
        return NOT_FOUND;
    }
    else if (vehicles_array_result == WRONG_TYPE)
    {
        return WRONG_TYPE;
    }

    printf("\n");
    printf("✅ [vehicles] length: %d\n", cJSON_GetArraySize(vehicles_array_p));
    
    Car *vehicles = (Car *)malloc(json_result->max_vehicle_capacity * sizeof(Car));
    if (vehicles == NULL) {
        printf("❌ Unable to allocate memory for vehicles\n");
        return ALLOCATION_ERROR;
    }

    int current_vehicle_capacity = 0;

    cJSON *vehicle_p = NULL;
    cJSON_ArrayForEach(vehicle_p, vehicles_array_p)
    {
        printf("\n");
        printf("---------------------------------------------Level 1: START\n");
        VehicleType vehicle_type = get_vehicle_type(vehicle_p);
        if (vehicle_type == VEHICLE_NOT_FOUND)
        {
            printf("❌ vehicle_type was not found\n");
            continue;
        }

        if (vehicle_type == VEHICLE_AUTO)
        {
            printf("✅ [vehicle_type]: 🚗\n");
        }
        else if (vehicle_type == VEHICLE_VAN)
        {
            printf("✅ [vehicle_type]: 🚙\n");
        }
        else if (vehicle_type == VEHICLE_TRUCK)
        {
            printf("✅ [vehicle_type]: 🚛\n");
        }

        int default_fuel_needed = 0;
        StatusType default_fuel_needed_result = get_int_value(vehicle_p, &default_fuel_needed, "default_fuel_needed");
        if (default_fuel_needed_result == NOT_FOUND)
        {
            printf("❌ [default_fuel_needed] was not found\n");
            continue;
        }
        else if (default_fuel_needed_result == WRONG_TYPE)
        {
            printf("❌ [default_fuel_needed] is not number\n");
            continue;
        }

        printf("✅ [default_fuel_needed]: %d\n", default_fuel_needed);

        int default_wait_time_sec = 0;
        StatusType default_wait_time_sec_result = get_int_value(vehicle_p, &default_wait_time_sec, "default_wait_time_sec");
        if (default_wait_time_sec_result == NOT_FOUND)
        {
            printf("❌ [default_wait_time_sec] was not found\n");
            continue;
        }
        else if (default_wait_time_sec_result == WRONG_TYPE)
        {
            printf("❌ [default_wait_time_sec] is not number\n");
            continue;
        }

        printf("✅ [default_wait_time_sec]: %d\n", default_wait_time_sec);

        int default_count = 0;
        StatusType default_count_result = get_int_value(vehicle_p, &default_count, "default_count");
        if (default_count_result == NOT_FOUND)
        {
            printf("❌ [default_count] was not found\n");
            continue;
        }
        else if (default_count_result == WRONG_TYPE)
        {
            printf("❌ [default_count] is not number\n");
            continue;
        }

        printf("✅ [default_count]: %d\n", default_count);

        _Bool randomize_arrival = 0;
        StatusType randomize_arrival_result = get_boolean_value(vehicle_p, &randomize_arrival, "randomize_arrival");
        if (randomize_arrival_result == NOT_FOUND)
        {
            printf("❌ [randomize_arrival] was not found\n");
        }
        else if (randomize_arrival_result == WRONG_TYPE)
        {
            printf("❌ [randomize_arrival] is not boolean\n");
            continue;
        }
        else
        {
            printf("✅ [randomize_arrival]: %d\n", randomize_arrival);
        }

        StatusType custom_waiting_list_result = get_custom_waiting_list(vehicle_p);
        if (custom_waiting_list_result == NOT_FOUND)
        {
            printf("❌ [custom_waiting_list] was not found\n");
        }
        else if (custom_waiting_list_result == WRONG_TYPE)
        {
            printf("❌ [custom_waiting_list] is not array\n");
            continue;
        }

        if (custom_waiting_list_result == NOT_FOUND) {
            current_vehicle_capacity += default_count;
        }
        printf("---------------------------------------------Level 1: END\n");
        printf("\n");
        printf("\n");
        printf("\n");
        printf("\n");
        printf("\n");
    }

    return CORRECT_VALUE;
}

void print_json_result(UserJsonResult *json_result)
{
    printf("Data from user:\n");
    printf("✅ [fuel_pumps_count]: %d\n", json_result->fuel_pumps_count);
    printf("✅ [initial_fuel_in_tanker]: %d\n", json_result->initial_fuel_in_tanker);
    printf("✅ [fuel_transfer_rate]: %d\n", json_result->fuel_transfer_rate);
    printf("✅ [max_vehicle_capacity]: %d\n", json_result->max_vehicle_capacity);
}

char *buffer = NULL;
cJSON *json = NULL;

void clean_up() {
    printf("\nCleaning up...\n");
    if (buffer != NULL) {
        free(buffer);
        printf("Cleaned buffer\n");
    }
    if (json != NULL) {
        cJSON_Delete(json);
        printf("Cleaned json\n");
    }
}

int main()
{
    atexit(clean_up);
    StatusType get_file_buffer_result = get_file_buffer(&buffer);
    if (get_file_buffer_result == ALLOCATION_ERROR)
    {
        printf("❌ Unable to allocate memory for buffer\n");
        return 1;
    }
    if (get_file_buffer_result == UNKNOWN_ERROR)
    {
        printf("❌ Unable to get file buffer\n");
        return 1;
    }

    StatusType parse_file_buffer_result = parse_file_buffer(&json, &buffer);
    if (parse_file_buffer_result == UNKNOWN_ERROR)
    {
        printf("❌ Unable to parse file buffer\n");
        return 1;
    }

    UserJsonResult *json_result = (UserJsonResult *)malloc(sizeof(UserJsonResult));
    if (json_result == NULL)
    {
        printf("❌ Unable to allocate memory for json_result\n");
        return 1;
    }

    int fuel_pumps_count = 0;
    StatusType fuel_pumps_count_result = get_int_value(json, &fuel_pumps_count, "fuel_pumps_count");
    if (fuel_pumps_count_result == NOT_FOUND)
    {
        printf("❌ [fuel_pumps_count] was not found\n");
        return 1;
    }
    else if (fuel_pumps_count_result == WRONG_TYPE)
    {
        printf("❌ [fuel_pumps_count] is not number\n");
        return 1;
    }
    else
    {
        // printf("✅ [fuel_pumps_count]: %d\n", fuel_pumps_count);
        json_result->fuel_pumps_count = fuel_pumps_count;
    }

    int max_vehicle_capacity = 0;
    StatusType max_vehicle_capacity_result = get_int_value(json, &max_vehicle_capacity, "max_vehicle_capacity");
    if (max_vehicle_capacity_result == NOT_FOUND)
    {
        printf("❌ [max_vehicle_capacity] was not found\n");
        return 1;
    }
    else if (max_vehicle_capacity_result == WRONG_TYPE)
    {
        printf("❌ [max_vehicle_capacity] is not number\n");
        return 1;
    }
    else if (max_vehicle_capacity > MAX_VEHICLES)
    {
        printf("❌ [max_vehicle_capacity] can not be bigger than %d\n", MAX_VEHICLES);
        return 1;
    }
    else
    {
        // printf("✅ [max_vehicle_capacity]: %d\n", max_vehicle_capacity);
        json_result->max_vehicle_capacity = max_vehicle_capacity;
    }

    int initial_fuel_in_tanker = 0;
    StatusType initial_fuel_in_tanker_result = get_int_value(json, &initial_fuel_in_tanker, "initial_fuel_in_tanker");
    if (initial_fuel_in_tanker_result == NOT_FOUND)
    {
        printf("❌ [initial_fuel_in_tanker] was not found\n");
        return 1;
    }
    else if (initial_fuel_in_tanker_result == WRONG_TYPE)
    {
        printf("❌ [initial_fuel_in_tanker] is not number\n");
        return 1;
    }
    else
    {
        // printf("✅ [initial_fuel_in_tanker]: %d\n", initial_fuel_in_tanker);
        json_result->initial_fuel_in_tanker = initial_fuel_in_tanker;
    }

    int fuel_transfer_rate = 0;
    StatusType fuel_transfer_rate_result = get_int_value(json, &fuel_transfer_rate, "fuel_transfer_rate");
    if (fuel_transfer_rate_result == NOT_FOUND)
    {
        printf("❌ [fuel_transfer_rate] was not found\n");
        return 1;
    }
    else if (fuel_transfer_rate_result == WRONG_TYPE)
    {
        printf("❌ [fuel_transfer_rate] is not number\n");
        return 1;
    }
    else
    {
        // printf("✅ [fuel_transfer_rate]: %d\n", fuel_transfer_rate);
        json_result->fuel_transfer_rate = fuel_transfer_rate;
    }

    StatusType vehicles_result = get_vehicles(json, json_result);
    if (vehicles_result == ALLOCATION_ERROR)
    {
        printf("❌ [vehicles] unable to allocate memory\n");
        return 1;
    }
    if (vehicles_result == NOT_FOUND)
    {
        printf("❌ [vehicles] was not found\n");
        return 1;
    }
    else if (vehicles_result == WRONG_TYPE)
    {
        printf("❌ [vehicles] is not array\n");
        return 1;
    }

    print_json_result(json_result);

    return 0;
}
