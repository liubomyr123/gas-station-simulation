#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
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
    EMPTY_VEHICLE_CAPACITY_VALUE,
    EMPTY_VALUE,
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
    if (*result == NULL)
    {
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

StatusType get_custom_waiting_list(
    cJSON *json,
    VehicleType vehicle_type,
    int default_wait_time_sec,
    int default_fuel_needed)
{
    if (vehicle_type == VEHICLE_NOT_FOUND)
    {
        return UNKNOWN_ERROR;
    }
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

    int custom_waiting_list_length = cJSON_GetArraySize(custom_waiting_list_p);
    if (custom_waiting_list_length == 0)
    {
        return EMPTY_VALUE;
    }

    printf("\n");
    printf("✅ [custom_waiting_list] length: %d\n", custom_waiting_list_length);

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

StatusType validate_custom_waiting_list(
    cJSON *json,
    int *local_vehicle_capacity)
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

    int custom_waiting_list_length = cJSON_GetArraySize(custom_waiting_list_p);
    if (custom_waiting_list_length == 0)
    {
        return EMPTY_VALUE;
    }

    printf("\n");
    printf("✅ [custom_waiting_list] length: %d\n", custom_waiting_list_length);

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
        if (count < 0)
        {
            printf("❌ [count] can not be negative\n");
            continue;
        }

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
        else if (fuel_needed < 0)
        {
            printf("❌ [fuel_needed] can not be negative\n");
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
        else if (wait_time_sec < 0 && wait_time_sec != -1)
        {
            printf("❌ [wait_time_sec] value is less than -1. Value can be only -1 or positive\n");
            continue;
        }
        else
        {
            printf("✅ [wait_time_sec]: %d\n", wait_time_sec);
        }

        printf("✅ [count]: %d\n", count);
        *local_vehicle_capacity += count;
        printf("--------------------------------Level 2: END\n");
    }

    return CORRECT_VALUE;
}

StatusType validate_vehicles(
    cJSON *vehicles_array_p,
    int *current_vehicle_capacity,
    int *indexes)
{
    cJSON *vehicle_p = NULL;
    int index = 0;
    my_cJSON_ArrayForEach(vehicle_p, vehicles_array_p, index)
    {
        int local_vehicle_capacity = 0;
        printf("\n");
        indexes[index] = 0;
        // printf("index = %d\n", index);
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
            printf("❓ [default_fuel_needed] was not found\n");
            continue;
        }
        else if (default_fuel_needed_result == WRONG_TYPE)
        {
            printf("❌ [default_fuel_needed] is not number\n");
            continue;
        }
        if (default_fuel_needed < 0)
        {
            printf("❌ [default_fuel_needed] can not be negative\n");
            continue;
        }
        if (default_fuel_needed == 0)
        {
            printf("❌ [default_fuel_needed] can not be 0\n");
            continue;
        }

        printf("✅ [default_fuel_needed]: %d\n", default_fuel_needed);

        int default_wait_time_sec = 0;
        StatusType default_wait_time_sec_result = get_int_value(vehicle_p, &default_wait_time_sec, "default_wait_time_sec");
        if (default_wait_time_sec_result == NOT_FOUND)
        {
            printf("❓ [default_wait_time_sec] was not found\n");
            continue;
        }
        else if (default_wait_time_sec_result == WRONG_TYPE)
        {
            printf("❌ [default_wait_time_sec] is not number\n");
            continue;
        }
        if (default_wait_time_sec < 0 && default_wait_time_sec != -1)
        {
            printf("❌ [default_wait_time_sec] value is less than -1. Value can be only -1 or positive\n");
            continue;
        }

        printf("✅ [default_wait_time_sec]: %d\n", default_wait_time_sec);

        int default_count = 0;
        StatusType default_count_result = get_int_value(vehicle_p, &default_count, "default_count");
        if (default_count_result == NOT_FOUND)
        {
            printf("❓ [default_count] was not found\n");
            continue;
        }
        else if (default_count_result == WRONG_TYPE)
        {
            printf("❌ [default_count] is not number\n");
            continue;
        }
        if (default_count_result < 0)
        {
            printf("❌ [default_count] can not be negative\n");
            continue;
        }

        printf("✅ [default_count]: %d\n", default_count);

        _Bool randomize_arrival = 0;
        StatusType randomize_arrival_result = get_boolean_value(vehicle_p, &randomize_arrival, "randomize_arrival");
        if (randomize_arrival_result == NOT_FOUND)
        {
            printf("❓ [randomize_arrival] was not found\n");
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

        StatusType custom_waiting_list_result = validate_custom_waiting_list(
            vehicle_p,
            &local_vehicle_capacity);
        if (custom_waiting_list_result == NOT_FOUND)
        {
            printf("❓ [custom_waiting_list] was not found\n");
            local_vehicle_capacity += default_count;
        }
        if (custom_waiting_list_result == EMPTY_VALUE)
        {
            printf("❌ [custom_waiting_list] is empty\n");
            local_vehicle_capacity += default_count;
        }
        else if (custom_waiting_list_result == WRONG_TYPE)
        {
            printf("❌ [custom_waiting_list] is not array\n");
            continue;
        }

        printf("\n");
        printf("Current vehicle array item has %d valid vehicles\n", local_vehicle_capacity);
        *current_vehicle_capacity += local_vehicle_capacity;
        printf("---------------------------------------------Level 1: END\n");
        // printf("Value VALID = %d\n", *current_vehicle_capacity);
        // printf("Index VALID = %d\n", index);
        indexes[index] = 1;
        printf("\n");
        printf("\n");
        printf("\n");
        printf("\n");
        printf("\n");
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

    int vehicle_length = cJSON_GetArraySize(vehicles_array_p);
    if (vehicle_length == 0)
    {
        return EMPTY_VALUE;
    }
    printf("\n");
    printf("✅ [vehicles] length: %d\n", vehicle_length);

    int current_vehicle_capacity = 0;
    int indexes[vehicle_length];
    StatusType vehicles_validation_result = validate_vehicles(
        vehicles_array_p,
        &current_vehicle_capacity,
        indexes);

    printf("\n");
    printf("Total valid vehicles: %d\n", current_vehicle_capacity);
    if (current_vehicle_capacity == 0)
    {
        return EMPTY_VEHICLE_CAPACITY_VALUE;
    }

    printf("\n");
    for (int i = 0; i < vehicle_length; i++)
    {
        if (indexes[i] == 1)
        {
            printf("✅ [%d] Vehicle array item is valid\n", i);
        }
        if (indexes[i] == 0)
        {
            printf("❌ [%d] Vehicle array item is NOT valid\n", i);
        }
    }

    Car *all_user_vehicles = (Car *)malloc(current_vehicle_capacity * sizeof(int));
    if (all_user_vehicles == NULL)
    {
        printf("❌ Unable to allocate memory for all_user_vehicles\n");
        return ALLOCATION_ERROR;
    }

    cJSON *vehicle_p = NULL;
    int index = 0;
    my_cJSON_ArrayForEach(vehicle_p, vehicles_array_p, index)
    {
        if (indexes[index] == 0)
        {
            printf("Invalid array item with index: %d. Skipping...\n", index);
            continue;
        }

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

        StatusType custom_waiting_list_result = get_custom_waiting_list(
            vehicle_p,
            vehicle_type,
            default_wait_time_sec,
            default_fuel_needed);

        if (custom_waiting_list_result == EMPTY_VALUE)
        {
            printf("❌ [custom_waiting_list] is empty\n");
        }
        if (custom_waiting_list_result == NOT_FOUND)
        {
            printf("❌ [custom_waiting_list] was not found\n");
        }
        else if (custom_waiting_list_result == WRONG_TYPE)
        {
            printf("❌ [custom_waiting_list] is not array\n");
            continue;
        }

        printf("---------------------------------------------Level 1: END\n");
        printf("\n");
        printf("\n");
        printf("\n");
        printf("\n");
        printf("\n");
    }

    free(all_user_vehicles);
    return CORRECT_VALUE;
}

void print_json_result(UserJsonResult *json_result)
{
    printf("\nData from user:\n");
    printf("✅ [fuel_pumps_count]: %d\n", json_result->fuel_pumps_count);
    printf("✅ [initial_fuel_in_tanker]: %d\n", json_result->initial_fuel_in_tanker);
    printf("✅ [fuel_transfer_rate]: %d\n", json_result->fuel_transfer_rate);
    printf("✅ [max_vehicle_capacity]: %d\n", json_result->max_vehicle_capacity);
}

char *buffer = NULL;
cJSON *json = NULL;

void clean_up()
{
    printf("\nCleaning up...\n");
    if (buffer != NULL)
    {
        free(buffer);
        printf("Cleaned buffer\n");
    }
    if (json != NULL)
    {
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
    if (vehicles_result == EMPTY_VALUE)
    {
        printf("❌ [vehicles] is empty\n");
        return 1;
    }
    if (vehicles_result == EMPTY_VEHICLE_CAPACITY_VALUE)
    {
        printf("❌ [vehicles] does not have any valid cars\n");
        return 1;
    }
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
