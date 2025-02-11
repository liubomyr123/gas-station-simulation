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
    Car **vehicles;
} UserJsonResult;

typedef enum
{
    NOT_FOUND,
    CORRECT_VALUE,
    WRONG_VALUE,
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

VehicleType get_vehicle_type(cJSON *vehicle_p, _Bool show_logs)
{
    VehicleType result_vehicle_type = VEHICLE_NOT_FOUND;

    char *vehicle_type = NULL;
    StatusType vehicle_type_result = get_string_value(vehicle_p, &vehicle_type, "vehicle_type");
    if (vehicle_type_result == ALLOCATION_ERROR)
    {
        if (show_logs)
        {
            printf("❌ [vehicle_type] unable to allocate memory\n");
        }
    }
    if (vehicle_type_result == NOT_FOUND)
    {
        if (show_logs)
        {
            printf("❌ [vehicle_type] was not found\n");
        }
    }
    else if (vehicle_type_result == WRONG_TYPE)
    {
        if (show_logs)
        {
            printf("❌ [vehicle_type] is not string\n");
        }
    }
    else
    {
        if (strcmp(vehicle_type, "auto") == 0)
        {
            result_vehicle_type = VEHICLE_AUTO;
        }
        else if (strcmp(vehicle_type, "truck") == 0)
        {
            result_vehicle_type = VEHICLE_TRUCK;
        }
        else if (strcmp(vehicle_type, "van") == 0)
        {
            result_vehicle_type = VEHICLE_VAN;
        }
    }

    if (result_vehicle_type == VEHICLE_AUTO)
    {
        if (show_logs)
        {
            printf("✅ [vehicle_type]: 🚗\n");
        }
    }
    else if (result_vehicle_type == VEHICLE_VAN)
    {
        if (show_logs)
        {
            printf("✅ [vehicle_type]: 🚙\n");
        }
    }
    else if (result_vehicle_type == VEHICLE_TRUCK)
    {
        if (show_logs)
        {
            printf("✅ [vehicle_type]: 🚛\n");
        }
    }
    else if (result_vehicle_type == VEHICLE_NOT_FOUND)
    {
        if (show_logs)
        {
            printf("❌ vehicle_type was not found\n");
        }
    }

    free(vehicle_type);
    return result_vehicle_type;
}

int get_custom_waiting_list_count(cJSON *custom_waiting_list_item_p, int *count, _Bool show_logs)
{
    StatusType count_result = get_int_value(custom_waiting_list_item_p, count, "count");
    if (count_result == NOT_FOUND)
    {
        if (show_logs)
        {
            printf("❌ [count] was not found\n");
        }
        return 0;
    }
    else if (count_result == WRONG_TYPE)
    {
        if (show_logs)
        {
            printf("❌ [count] is not number\n");
        }
        return 0;
    }

    if (show_logs)
    {
        printf("✅ [count]: %d\n", *count);
    }
    return 1;
}

StatusType get_custom_waiting_list_fuel_needed(cJSON *custom_waiting_list_item_p, int *fuel_needed, _Bool show_logs)
{
    StatusType fuel_needed_result = get_int_value(custom_waiting_list_item_p, fuel_needed, "fuel_needed");
    if (fuel_needed_result == NOT_FOUND)
    {
        if (show_logs)
        {
            printf("❌ [fuel_needed] was not found\n");
        }
        return NOT_FOUND;
    }
    else if (fuel_needed_result == WRONG_TYPE)
    {
        if (show_logs)
        {
            printf("❌ [fuel_needed] is not number\n");
        }
        return WRONG_TYPE;
    }
    else if (*fuel_needed < 0)
    {
        if (show_logs)
        {
            printf("❌ [fuel_needed] can not be negative\n");
        }
        return WRONG_VALUE;
    }
    if (*fuel_needed == 0)
    {
        if (show_logs)
        {
            printf("❌ [fuel_needed] can not be 0\n");
        }
        return WRONG_VALUE;
    }
    else
    {
        if (show_logs)
        {
            printf("✅ [fuel_needed]: %d\n", *fuel_needed);
        }
    }
    return CORRECT_VALUE;
}

StatusType get_custom_waiting_list_wait_time_sec(cJSON *custom_waiting_list_item_p, int *wait_time_sec, _Bool show_logs)
{
    StatusType wait_time_sec_result = get_int_value(custom_waiting_list_item_p, wait_time_sec, "wait_time_sec");
    if (wait_time_sec_result == NOT_FOUND)
    {
        if (show_logs)
        {
            printf("❌ [wait_time_sec] was not found\n");
        }
        return NOT_FOUND;
    }
    else if (wait_time_sec_result == WRONG_TYPE)
    {
        if (show_logs)
        {

            printf("❌ [wait_time_sec] is not number\n");
        }
        return WRONG_TYPE;
    }
    else if (*wait_time_sec < 0 && *wait_time_sec != -1)
    {
        if (show_logs)
        {
            printf("❌ [wait_time_sec] value is less than -1. Value can be only -1 or positive\n");
        }
        return WRONG_VALUE;
    }
    else
    {
        if (show_logs)
        {
            printf("✅ [wait_time_sec]: %d\n", *wait_time_sec);
        }
    }
    return CORRECT_VALUE;
}

StatusType get_custom_waiting_list(
    cJSON *json,
    VehicleType vehicle_type,
    int default_wait_time_sec,
    int default_fuel_needed,
    Car **all_user_vehicles,
    int *count_added_vehicles)
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
    int show_logs = 0;

    if (show_logs)
    {
        printf("\n");
        printf("✅ [custom_waiting_list] length: %d\n", custom_waiting_list_length);
    }

    cJSON *custom_waiting_list_item_p = NULL;
    cJSON_ArrayForEach(custom_waiting_list_item_p, custom_waiting_list_p)
    {
        if (show_logs)
        {
            printf("\n");
            printf("--------------------------------Level 2: START\n");
        }
        int count = 0;
        if (get_custom_waiting_list_count(custom_waiting_list_item_p, &count, show_logs) == 0)
        {
            continue;
        }

        int fuel_needed = 0;
        StatusType fuel_needed_result = get_custom_waiting_list_fuel_needed(custom_waiting_list_item_p, &fuel_needed, show_logs);
        if (fuel_needed_result == WRONG_TYPE)
        {
            continue;
        }
        if (fuel_needed_result == WRONG_VALUE)
        {
            continue;
        }

        int wait_time_sec = 0;
        StatusType wait_time_sec_result = get_custom_waiting_list_wait_time_sec(custom_waiting_list_item_p, &wait_time_sec, show_logs);
        if (wait_time_sec_result == WRONG_TYPE)
        {
            continue;
        }
        if (wait_time_sec_result == WRONG_VALUE)
        {
            continue;
        }

        for (int i = 0; i < count; i++)
        {
            Car *new_vehicle = (Car *)malloc(sizeof(Car));
            if (new_vehicle == NULL)
            {
                printf("❌ Memory allocation on new vehicle failed\n");
                continue;
            }
            new_vehicle->vehicle_type = vehicle_type;
            if (fuel_needed_result == NOT_FOUND)
            {
                new_vehicle->fuel_needed = default_fuel_needed;
            }
            else
            {
                new_vehicle->fuel_needed = fuel_needed;
            }
            if (wait_time_sec_result == NOT_FOUND)
            {
                new_vehicle->wait_time_sec = default_wait_time_sec;
            }
            else
            {
                new_vehicle->wait_time_sec = wait_time_sec;
            }

            all_user_vehicles[*count_added_vehicles] = new_vehicle;
            *count_added_vehicles += 1;
        }

        if (show_logs)
        {
            printf("--------------------------------Level 2: END\n");
        }
    }

    return CORRECT_VALUE;
}

StatusType validate_custom_waiting_list(
    cJSON *json,
    int *local_vehicle_capacity)
{
    _Bool show_logs = 0;

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

    if (show_logs)
    {
        printf("\n");
        printf("✅ [custom_waiting_list] length: %d\n", custom_waiting_list_length);
    }

    cJSON *custom_waiting_list_item_p = NULL;
    cJSON_ArrayForEach(custom_waiting_list_item_p, custom_waiting_list_p)
    {
        if (show_logs)
        {
            printf("\n");
            printf("--------------------------------Level 2: START\n");
        }
        int count = 0;
        StatusType count_result = get_int_value(custom_waiting_list_item_p, &count, "count");
        if (count_result == NOT_FOUND)
        {
            if (show_logs)
            {
                printf("❌ [count] was not found\n");
            }
            continue;
        }
        else if (count_result == WRONG_TYPE)
        {
            if (show_logs)
            {
                printf("❌ [count] is not number\n");
            }
            continue;
        }
        if (count < 0)
        {
            if (show_logs)
            {
                printf("❌ [count] can not be negative\n");
            }
            continue;
        }

        int fuel_needed = 0;
        StatusType fuel_needed_result = get_int_value(custom_waiting_list_item_p, &fuel_needed, "fuel_needed");
        if (fuel_needed_result == NOT_FOUND)
        {
            if (show_logs)
            {
                printf("❌ [fuel_needed] was not found\n");
            }
        }
        else if (fuel_needed_result == WRONG_TYPE)
        {
            if (show_logs)
            {
                printf("❌ [fuel_needed] is not number\n");
            }
            continue;
        }
        else if (fuel_needed < 0)
        {
            if (show_logs)
            {
                printf("❌ [fuel_needed] can not be negative\n");
            }
            continue;
        }
        else
        {
            if (show_logs)
            {
                printf("✅ [fuel_needed]: %d\n", fuel_needed);
            }
        }

        int wait_time_sec = 0;
        StatusType wait_time_sec_result = get_int_value(custom_waiting_list_item_p, &wait_time_sec, "wait_time_sec");
        if (wait_time_sec_result == NOT_FOUND)
        {
            if (show_logs)
            {
                printf("❌ [wait_time_sec] was not found\n");
            }
        }
        else if (wait_time_sec_result == WRONG_TYPE)
        {
            if (show_logs)
            {
                printf("❌ [wait_time_sec] is not number\n");
            }
            continue;
        }
        else if (wait_time_sec < 0 && wait_time_sec != -1)
        {
            if (show_logs)
            {
                printf("❌ [wait_time_sec] value is less than -1. Value can be only -1 or positive\n");
            }
            continue;
        }
        else
        {
            if (show_logs)
            {
                printf("✅ [wait_time_sec]: %d\n", wait_time_sec);
            }
        }

        *local_vehicle_capacity += count;
        if (show_logs)
        {
            printf("✅ [count]: %d\n", count);
            printf("--------------------------------Level 2: END\n");
        }
    }

    return CORRECT_VALUE;
}

int get_default_fuel_needed(cJSON *vehicle_p, int *default_fuel_needed, _Bool show_logs)
{
    StatusType default_fuel_needed_result = get_int_value(vehicle_p, default_fuel_needed, "default_fuel_needed");
    if (default_fuel_needed_result == NOT_FOUND)
    {
        if (show_logs)
        {
            printf("❓ [default_fuel_needed] was not found\n");
        }
        return 0;
    }
    else if (default_fuel_needed_result == WRONG_TYPE)
    {
        if (show_logs)
        {
            printf("❌ [default_fuel_needed] is not number\n");
        }
        return 0;
    }
    if (*default_fuel_needed < 0)
    {
        if (show_logs)
        {
            printf("❌ [default_fuel_needed] can not be negative\n");
        }
        return 0;
    }
    if (*default_fuel_needed == 0)
    {
        if (show_logs)
        {
            printf("❌ [default_fuel_needed] can not be 0\n");
        }
        return 0;
    }

    if (show_logs)
    {
        printf("✅ [default_fuel_needed]: %d\n", *default_fuel_needed);
    }
    return 1;
}

int get_default_wait_time_sec(cJSON *vehicle_p, int *default_wait_time_sec, _Bool show_logs)
{
    StatusType default_wait_time_sec_result = get_int_value(vehicle_p, default_wait_time_sec, "default_wait_time_sec");
    if (default_wait_time_sec_result == NOT_FOUND)
    {
        if (show_logs)
        {
            printf("❓ [default_wait_time_sec] was not found\n");
        }
        return 0;
    }
    else if (default_wait_time_sec_result == WRONG_TYPE)
    {
        if (show_logs)
        {
            printf("❌ [default_wait_time_sec] is not number\n");
        }
        return 0;
    }
    if (*default_wait_time_sec < 0 && *default_wait_time_sec != -1)
    {
        if (show_logs)
        {
            printf("❌ [default_wait_time_sec] value is less than -1. Value can be only -1 or positive\n");
        }
        return 0;
    }

    if (show_logs)
    {
        printf("✅ [default_wait_time_sec]: %d\n", *default_wait_time_sec);
    }
    return 1;
}

int get_default_count(cJSON *vehicle_p, int *default_count, _Bool show_logs)
{
    StatusType default_wait_time_sec_result = get_int_value(vehicle_p, default_count, "default_count");
    if (default_wait_time_sec_result == NOT_FOUND)
    {
        if (show_logs)
        {
            printf("❓ [default_count] was not found\n");
        }
        return 0;
    }
    else if (default_wait_time_sec_result == WRONG_TYPE)
    {
        if (show_logs)
        {
            printf("❌ [default_count] is not number\n");
        }
        return 0;
    }
    if (*default_count < 0)
    {
        if (show_logs)
        {
            printf("❌ [default_count] can not be negative\n");
        }
        return 0;
    }

    if (show_logs)
    {
        printf("✅ [default_count]: %d\n", *default_count);
    }
    return 1;
}

int get_randomize_arrival(cJSON *vehicle_p, _Bool *randomize_arrival, _Bool show_logs)
{
    StatusType randomize_arrival_result = get_boolean_value(vehicle_p, randomize_arrival, "randomize_arrival");
    if (randomize_arrival_result == NOT_FOUND)
    {
        if (show_logs)
        {
            printf("❓ [randomize_arrival] was not found\n");
        }
    }
    else if (randomize_arrival_result == WRONG_TYPE)
    {
        if (show_logs)
        {
            printf("❌ [randomize_arrival] is not boolean\n");
        }
        return 0;
    }
    else
    {
        if (show_logs)
        {
            printf("✅ [randomize_arrival]: %d\n", *randomize_arrival);
        }
    }

    return 1;
}

StatusType validate_vehicles(
    cJSON *vehicles_array_p,
    int *current_vehicle_capacity,
    int *indexes)
{
    _Bool show_logs = 0;
    cJSON *vehicle_p = NULL;
    int index = 0;
    my_cJSON_ArrayForEach(vehicle_p, vehicles_array_p, index)
    {
        indexes[index] = 0;
        if (show_logs)
        {
            printf("---------------------------------------------Level 1: START\n");
        }
        VehicleType vehicle_type = get_vehicle_type(vehicle_p, show_logs);
        if (vehicle_type == VEHICLE_NOT_FOUND)
        {
            continue;
        }

        int default_fuel_needed = 0;
        if (get_default_fuel_needed(vehicle_p, &default_fuel_needed, show_logs) == 0)
        {
            continue;
        }

        int default_wait_time_sec = 0;
        if (get_default_wait_time_sec(vehicle_p, &default_wait_time_sec, show_logs) == 0)
        {
            continue;
        }

        int default_count = 0;
        if (get_default_count(vehicle_p, &default_count, show_logs) == 0)
        {
            continue;
        }

        _Bool randomize_arrival = 0;
        if (get_randomize_arrival(vehicle_p, &randomize_arrival, show_logs) == 0)
        {
            continue;
        }

        int local_vehicle_capacity = 0;
        StatusType custom_waiting_list_result = validate_custom_waiting_list(
            vehicle_p,
            &local_vehicle_capacity);
        if (custom_waiting_list_result == NOT_FOUND)
        {
            if (show_logs)
            {
                printf("❓ [custom_waiting_list] was not found\n");
            }
            local_vehicle_capacity += default_count;
        }
        if (custom_waiting_list_result == EMPTY_VALUE)
        {
            if (show_logs)
            {
                printf("❌ [custom_waiting_list] is empty\n");
            }
            local_vehicle_capacity += default_count;
        }
        else if (custom_waiting_list_result == WRONG_TYPE)
        {
            if (show_logs)
            {
                printf("❌ [custom_waiting_list] is not array\n");
            }
            continue;
        }

        *current_vehicle_capacity += local_vehicle_capacity;
        indexes[index] = 1;
        if (show_logs)
        {
            printf("\n");
            printf("Current vehicle array item has %d valid vehicles\n", local_vehicle_capacity);
            printf("---------------------------------------------Level 1: END\n");
            printf("\n");
            printf("\n");
            printf("\n");
            printf("\n");
            printf("\n");
        }
    }

    return CORRECT_VALUE;
}

void adding_default_cars()
{
}

StatusType get_vehicles(cJSON *json, UserJsonResult *json_result)
{
    _Bool show_logs = 0;
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

    if (show_logs)
    {
        printf("\n");
        printf("✅ [vehicles] length: %d\n", vehicle_length);
    }

    int current_vehicle_capacity = 0;
    int indexes[vehicle_length];
    StatusType vehicles_validation_result = validate_vehicles(
        vehicles_array_p,
        &current_vehicle_capacity,
        indexes);

    if (current_vehicle_capacity == 0)
    {
        return EMPTY_VEHICLE_CAPACITY_VALUE;
    }

    printf("\n");
    printf("List of analyzed vehicles elements:\n");
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
    printf("\n");

    printf("Total valid vehicles: %d\n", current_vehicle_capacity);
    
    int count_added_vehicles = 0;
    json_result->vehicles = (Car **)malloc(current_vehicle_capacity * sizeof(Car));
    if (json_result->vehicles == NULL)
    {
        printf("❌ Unable to allocate memory for all_user_vehicles\n");
        return ALLOCATION_ERROR;
    }
    json_result->current_vehicle_capacity = current_vehicle_capacity;

    cJSON *vehicle_p = NULL;
    int index = 0;
    my_cJSON_ArrayForEach(vehicle_p, vehicles_array_p, index)
    {
        if (indexes[index] == 0)
        {
            if (show_logs)
            {
                printf("Invalid array item with index: %d. Skipping...\n", index);
            }
            continue;
        }

        if (show_logs)
        {
            printf("\n");
            printf("---------------------------------------------Level 1: START\n");
        }
        VehicleType vehicle_type = get_vehicle_type(vehicle_p, show_logs);
        if (vehicle_type == VEHICLE_NOT_FOUND)
        {
            continue;
        }

        int default_fuel_needed = 0;
        if (get_default_fuel_needed(vehicle_p, &default_fuel_needed, show_logs) == 0)
        {
            continue;
        }

        int default_wait_time_sec = 0;
        if (get_default_wait_time_sec(vehicle_p, &default_wait_time_sec, show_logs) == 0)
        {
            continue;
        }

        int default_count = 0;
        if (get_default_count(vehicle_p, &default_count, show_logs) == 0)
        {
            continue;
        }

        _Bool randomize_arrival = 0;
        if (get_randomize_arrival(vehicle_p, &randomize_arrival, show_logs) == 0)
        {
            continue;
        }

        StatusType custom_waiting_list_result = get_custom_waiting_list(
            vehicle_p,
            vehicle_type,
            default_wait_time_sec,
            default_fuel_needed,
            json_result->vehicles,
            &count_added_vehicles);

        if (custom_waiting_list_result == EMPTY_VALUE || custom_waiting_list_result == NOT_FOUND)
        {
            for (int i = 0; i < default_count; i++)
            {
                Car *new_vehicle = (Car *)malloc(sizeof(Car));
                if (new_vehicle == NULL)
                {
                    printf("❌ Memory allocation on new vehicle failed\n");
                    continue;
                }
                new_vehicle->vehicle_type = vehicle_type;
                new_vehicle->fuel_needed = default_fuel_needed;
                new_vehicle->wait_time_sec = default_wait_time_sec;

                json_result->vehicles[count_added_vehicles] = new_vehicle;
                count_added_vehicles++;
            }
        }
        if (custom_waiting_list_result == EMPTY_VALUE)
        {
            if (show_logs)
            {
                printf("❌ [custom_waiting_list] is empty\n");
            }
        }
        if (custom_waiting_list_result == NOT_FOUND)
        {
            if (show_logs)
            {
                printf("❌ [custom_waiting_list] was not found\n");
            }
        }
        else if (custom_waiting_list_result == WRONG_TYPE)
        {
            if (show_logs)
            {
                printf("❌ [custom_waiting_list] is not array\n");
            }
            continue;
        }

        if (show_logs)
        {
            printf("---------------------------------------------Level 1: END\n");
            printf("\n");
            printf("\n");
            printf("\n");
            printf("\n");
            printf("\n");
        }
    }

    return CORRECT_VALUE;
}

void print_json_result(UserJsonResult *json_result)
{
    printf("\nData from user:\n");
    printf("✅ [fuel_pumps_count]: %d\n", json_result->fuel_pumps_count);
    printf("✅ [initial_fuel_in_tanker]: %d\n", json_result->initial_fuel_in_tanker);
    printf("✅ [fuel_transfer_rate]: %d\n", json_result->fuel_transfer_rate);
    printf("✅ [max_vehicle_capacity]: %d\n", json_result->max_vehicle_capacity);

    printf("\n");
    printf("List of valid cars:\n");
    for (int i = 0; i < json_result->current_vehicle_capacity; i++)
    {
        if (json_result->vehicles[i]->vehicle_type == VEHICLE_VAN)
        {
            printf("🚙 #%d:\n", i+1);
        }
        if (json_result->vehicles[i]->vehicle_type == VEHICLE_TRUCK)
        {
            printf("🚛 #%d:\n", i+1);
        }
        if (json_result->vehicles[i]->vehicle_type == VEHICLE_AUTO)
        {
            printf("🚗 #%d:\n", i+1);
        }
        printf("------------------------------\n");
        printf("🛢️  fuel_needed: %d liters\n", json_result->vehicles[i]->fuel_needed);
        printf("⏳ wait_time_sec: %d seconds\n", json_result->vehicles[i]->wait_time_sec);
        printf("------------------------------\n");
        printf("\n");
    }
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

    // Must free each item of json_result->vehicles after use
    print_json_result(json_result);

    return 0;
}
