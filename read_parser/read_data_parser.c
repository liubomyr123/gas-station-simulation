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
    int all_vehicles_length;
    int result_vehicles_length;
    _Bool randomize_arrival;
    Car **all_vehicles;
    Car **result_vehicles;
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

    (*buffer) = (char *)malloc((size + 1) * sizeof(char));
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
        // free(*buffer);
        fclose(fp);
        return UNKNOWN_ERROR;
    }

    // malloc will allocate size + 1 for '\n', and we need to add this '\0' at the end 
    (*buffer)[size] = '\0';
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

    // malloc will allocate only string_length, but we need to add '\0' at the end 
    *result = (char *)malloc((string_length + 1) * sizeof(char));
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

StatusType validate_vehicles(
    cJSON *vehicles_array_p,
    int *all_vehicles_length,
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

        *all_vehicles_length += local_vehicle_capacity;
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

    int all_vehicles_length = 0;
    int indexes[vehicle_length];
    StatusType vehicles_validation_result = validate_vehicles(
        vehicles_array_p,
        &all_vehicles_length,
        indexes);

    if (all_vehicles_length == 0)
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

    printf("Total valid vehicles: %d\n", all_vehicles_length);

    int count_added_vehicles = 0;
    json_result->all_vehicles = (Car **)malloc(all_vehicles_length * sizeof(Car));
    if (json_result->all_vehicles == NULL)
    {
        printf("❌ Unable to allocate memory for all_user_vehicles\n");
        return ALLOCATION_ERROR;
    }
    json_result->all_vehicles_length = all_vehicles_length;

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

        StatusType custom_waiting_list_result = get_custom_waiting_list(
            vehicle_p,
            vehicle_type,
            default_wait_time_sec,
            default_fuel_needed,
            json_result->all_vehicles,
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

                json_result->all_vehicles[count_added_vehicles] = new_vehicle;
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
    printf("✅ [randomize_arrival]: %s\n", json_result->randomize_arrival == 0 ? "false" : "true");

    // printf("\n");
    // printf("List of valid cars:\n");
    // for (int i = 0; i < json_result->all_vehicles_length; i++)
    // {
    //     if (json_result->vehicles[i]->vehicle_type == VEHICLE_VAN)
    //     {
    //         printf("🚙 #%d:\n", i + 1);
    //     }
    //     if (json_result->vehicles[i]->vehicle_type == VEHICLE_TRUCK)
    //     {
    //         printf("🚛 #%d:\n", i + 1);
    //     }
    //     if (json_result->vehicles[i]->vehicle_type == VEHICLE_AUTO)
    //     {
    //         printf("🚗 #%d:\n", i + 1);
    //     }
    //     printf("------------------------------\n");
    //     printf("🛢️  fuel_needed: %d liters\n", json_result->vehicles[i]->fuel_needed);
    //     printf("⏳ wait_time_sec: %d seconds\n", json_result->vehicles[i]->wait_time_sec);
    //     printf("------------------------------\n");
    //     printf("\n");
    // }
}

char *buffer = NULL;
cJSON *json = NULL;

void clean_up()
{
    printf("\nCleaning up...\n");
    if (buffer != NULL)
    {
        free(buffer);
        buffer = NULL;
        printf("Cleaned buffer\n");
    }
    if (json != NULL)
    {
        cJSON_Delete(json);
        printf("Cleaned json\n");
    }
}

void randomize_vehicles(UserJsonResult *json_result)
{
    printf("Program running randomizes...\n");
    srand(time(NULL));
    // printf("\nProgram will use %d cars...\n", json_result->result_vehicles_length);
    json_result->result_vehicles = (Car **)malloc(json_result->result_vehicles_length * sizeof(Car));

    // O(n*n) ❌ Custom solution
    // const int length = 10;
    // int array[length];
    // for (int i = 0; i < length; array[i] = -1, i++)
    //     ;
    // for (int i = 0; i < length; i++)
    // {
    //     _Bool flag = 0;
    //     while (flag == 0)
    //     {
    //         int rand_value = rand() % length;
    //         _Bool match = 0;
    //         for (int j = 0; j < length; j++)
    //         {
    //             if (array[j] == rand_value)
    //             {
    //                 match = 1;
    //                 break;
    //             }
    //         }
    //         if (match == 0)
    //         {
    //             array[i] = rand_value;
    //             flag = 1;
    //         }
    //         if (match == 1)
    //         {
    //             int new_rand_value = rand() % length;
    //             if (new_rand_value == rand_value)
    //             {
    //                 continue;
    //             }
    //         }
    //     }
    // }

    // O(n) ✅ Fisher-Yates Shuffle
    // const int length = 10;
    // int array[length];
    // for (int i = 0; i < length; array[i] = i, i++)
    //     ;
    // for (int i = length - 1; i >= 0; i--)
    // {
    //     int j = rand() % (i + 1);
    //     int temp = array[i];
    //     array[i] = array[j];
    //     array[j] = temp;
    // }

    // printf("Bigger length: %d\n", json_result->all_vehicles_length);
    // printf("Less length: %d\n", json_result->result_vehicles_length);

    const int bigger_length = json_result->all_vehicles_length;
    const int result_length = json_result->result_vehicles_length;

    // const int bigger_length = 7;
    // const int result_length = 5;

    if (bigger_length < result_length)
    {
        printf("Max vehicle array length is less current\n");
        return;
    }

    _Bool is_equal = bigger_length == result_length;

    int bigger_array[bigger_length];
    int result_array[result_length];

    for (int i = 0; i < bigger_length; bigger_array[i] = i, i++)
        ;

    for (int i = bigger_length - 1; i >= 0; i--)
    {
        int j = rand() % (i + 1);
        int temp = bigger_array[i];
        bigger_array[i] = bigger_array[j];
        bigger_array[j] = temp;
    }

    if (!is_equal)
    {
        // printf("NOT is_equal\n");
        for (int i = 0; i < result_length; i++)
        {
            result_array[i] = bigger_array[i];
        }
    }

    // printf("------------------\n");

    for (int j = 0; j < result_length; j++)
    {
        int index = !is_equal ? result_array[j] : bigger_array[j];

        // printf("j[%d]; index: [%d]\n", j, index);
        json_result->result_vehicles[j] = json_result->all_vehicles[index];
    }

    // printf("\n");
    // printf("List of valid cars:\n");
    // for (int i = 0; i < json_result->result_vehicles_length; i++)
    // {
    //     if (json_result->result_vehicles[i]->vehicle_type == VEHICLE_VAN)
    //     {
    //         printf("🚙 #%d:\n", i + 1);
    //     }
    //     if (json_result->result_vehicles[i]->vehicle_type == VEHICLE_TRUCK)
    //     {
    //         printf("🚛 #%d:\n", i + 1);
    //     }
    //     if (json_result->result_vehicles[i]->vehicle_type == VEHICLE_AUTO)
    //     {
    //         printf("🚗 #%d:\n", i + 1);
    //     }
    //     printf("------------------------------\n");
    //     printf("🛢️  fuel_needed: %d liters\n", json_result->result_vehicles[i]->fuel_needed);
    //     printf("⏳ wait_time_sec: %d seconds\n", json_result->result_vehicles[i]->wait_time_sec);
    //     printf("------------------------------\n");
    //     printf("\n");
    // }
}

void clean_up_json_result(UserJsonResult *json_result)
{
    if (json_result == NULL)
    {
        return;
    }
    if (json_result->all_vehicles != NULL)
    {
        for (int i = 0; i < json_result->all_vehicles_length; i++)
        {
            if (json_result->all_vehicles[i] != NULL)
            {
                free(json_result->all_vehicles[i]);
                json_result->all_vehicles[i] = NULL;
            }
        }

        free(json_result->all_vehicles);
    }
    if (json_result->result_vehicles != NULL)
    {
        free(json_result->result_vehicles);
    }
    free(json_result);
    printf("Cleaning results finished\n");
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
    json_result->all_vehicles = NULL;
    json_result->result_vehicles = NULL;

    int fuel_pumps_count = 0;
    StatusType fuel_pumps_count_result = get_int_value(json, &fuel_pumps_count, "fuel_pumps_count");
    if (fuel_pumps_count_result == NOT_FOUND)
    {
        printf("❌ [fuel_pumps_count] was not found\n");
        clean_up_json_result(json_result);
        return 1;
    }
    else if (fuel_pumps_count_result == WRONG_TYPE)
    {
        printf("❌ [fuel_pumps_count] is not number\n");
        clean_up_json_result(json_result);
        return 1;
    }
    else
    {
        // printf("✅ [fuel_pumps_count]: %d\n", fuel_pumps_count);
        json_result->fuel_pumps_count = fuel_pumps_count;
    }

    _Bool randomize_arrival = 0;
    StatusType randomize_arrival_result = get_boolean_value(json, &randomize_arrival, "randomize_arrival");
    if (randomize_arrival_result == NOT_FOUND)
    {
        printf("❓ [randomize_arrival] was not found\n");
    }
    else if (randomize_arrival_result == WRONG_TYPE)
    {
        printf("❌ [randomize_arrival] is not boolean\n");
        clean_up_json_result(json_result);
        return 1;
    }
    else
    {
        // printf("✅ [randomize_arrival]: %d\n", randomize_arrival);
        json_result->randomize_arrival = randomize_arrival;
    }

    int max_vehicle_capacity = 0;
    StatusType max_vehicle_capacity_result = get_int_value(json, &max_vehicle_capacity, "max_vehicle_capacity");
    if (max_vehicle_capacity_result == NOT_FOUND)
    {
        printf("❌ [max_vehicle_capacity] was not found\n");
        clean_up_json_result(json_result);
        return 1;
    }
    else if (max_vehicle_capacity_result == WRONG_TYPE)
    {
        printf("❌ [max_vehicle_capacity] is not number\n");
        clean_up_json_result(json_result);
        return 1;
    }
    else if (max_vehicle_capacity > MAX_VEHICLES)
    {
        printf("❌ [max_vehicle_capacity] can not be bigger than %d\n", MAX_VEHICLES);
        clean_up_json_result(json_result);
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
        clean_up_json_result(json_result);
        return 1;
    }
    else if (initial_fuel_in_tanker_result == WRONG_TYPE)
    {
        printf("❌ [initial_fuel_in_tanker] is not number\n");
        clean_up_json_result(json_result);
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
        clean_up_json_result(json_result);
        return 1;
    }
    else if (fuel_transfer_rate_result == WRONG_TYPE)
    {
        printf("❌ [fuel_transfer_rate] is not number\n");
        clean_up_json_result(json_result);
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
        clean_up_json_result(json_result);
        return 1;
    }
    if (vehicles_result == EMPTY_VEHICLE_CAPACITY_VALUE)
    {
        printf("❌ [vehicles] does not have any valid cars\n");
        clean_up_json_result(json_result);
        return 1;
    }
    if (vehicles_result == ALLOCATION_ERROR)
    {
        printf("❌ [vehicles] unable to allocate memory\n");
        clean_up_json_result(json_result);
        return 1;
    }
    if (vehicles_result == NOT_FOUND)
    {
        printf("❌ [vehicles] was not found\n");
        clean_up_json_result(json_result);
        return 1;
    }
    else if (vehicles_result == WRONG_TYPE)
    {
        printf("❌ [vehicles] is not array\n");
        clean_up_json_result(json_result);
        return 1;
    }

    // Must free each item of json_result->vehicles after use
    print_json_result(json_result);

    printf("\n");

    printf("[vehicles] length of all vehicles: %d\n", json_result->all_vehicles_length);

    printf("\n");
    if (json_result->all_vehicles_length > MAX_VEHICLES)
    {
        printf("❌ [vehicles] can not be bigger than [MAX_VEHICLES]: %d\n", MAX_VEHICLES);
    }

    _Bool is_overlap_vehicle_length = json_result->all_vehicles_length > max_vehicle_capacity;
    if (is_overlap_vehicle_length)
    {
        printf("❌ [vehicles] can not be bigger than [max_vehicle_capacity]: %d\n", max_vehicle_capacity);
        json_result->result_vehicles_length = max_vehicle_capacity;
    }
    else
    {
        printf("✅ Program will use [vehicles]/[max_vehicle_capacity] cars: %d/%d\n",
               json_result->all_vehicles_length,
               max_vehicle_capacity);
        json_result->result_vehicles_length = json_result->all_vehicles_length;
    }

    printf("\n");
    printf("[vehicles] length of result vehicles: %d\n", json_result->result_vehicles_length);
    printf("\n");

    if (randomize_arrival_result == NOT_FOUND)
    {
        // skip
    }
    else
    {
        if (randomize_arrival == 1)
        {
            if (is_overlap_vehicle_length)
            {
                printf("Program will random select %d vehicles...\n", json_result->result_vehicles_length);
            }
            else
            {
                printf("Program is going to randomize %d vehicles list...\n", json_result->result_vehicles_length);
            }
            randomize_vehicles(json_result);
        }
        else
        {
            if (is_overlap_vehicle_length)
            {
                printf("Program will random select %d vehicles...\n", json_result->result_vehicles_length);
                randomize_vehicles(json_result);
            }
        }
    }

    clean_up_json_result(json_result);
    return 0;
}

// cd ./read_parser
// make
// valgrind --tool=memcheck --leak-check=full --track-origins=yes --show-leak-kinds=all -s ./read_data_parser

// result: 
// All heap blocks were freed -- no leaks are possible
// ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
