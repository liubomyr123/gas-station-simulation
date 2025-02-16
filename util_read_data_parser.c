#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include "cjson/cJSON.h"
#include "util_read_data_parser.h"

// ============

//

// ============

static _Bool SHOW_LOGS = false;
static char *buffer = NULL;
static cJSON *json = NULL;

// ============

//

// ============

void print_json_result(UserJsonResult *json_result);

void clean_up();
void clean_up_json_result(UserJsonResult **json_result);
void clean_up_read_data_parser_result(ReadDataParserResult **read_data_parser_result);

VehicleType get_vehicle_type(cJSON *vehicle_p, _Bool show_logs_now, int index);

int get_file_size(FILE *fp);
int get_default_count(cJSON *vehicle_p, int *default_count, _Bool show_logs_now);
int get_default_fuel_needed(cJSON *vehicle_p, int *default_fuel_needed, _Bool show_logs_now);
int get_default_wait_time_sec(cJSON *vehicle_p, int *default_wait_time_sec, _Bool show_logs_now);
int handle_parse_file_buffer(ReadDataParserResult **read_data_parser_result);
int handle_get_file_buffer(char *path, ReadDataParserResult **read_data_parser_result);
int handle_read_data_parser_result_creation(ReadDataParserResult **read_data_parser_result);
int handle_result_vehicles(UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result);
int handle_get_all_vehicles(UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result);
int handle_randomize_vehicles(UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result);
int handle_get_limited_amount(UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result);
int handle_json_result_creation(UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result);
int handle_fuel_pumps_count(int *fuel_pumps_count, UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result);
int handle_fuel_transfer_rate(int *fuel_transfer_rate, UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result);
int handle_randomize_arrival(_Bool *randomize_arrival, UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result);
int handle_max_vehicle_capacity(int *max_vehicle_capacity, UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result);
int handle_initial_fuel_in_tanker(int *initial_fuel_in_tanker, UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result);

StatusType get_file_buffer(char **buffer, char *path);
StatusType parse_file_buffer(cJSON **json, char **buffer);
StatusType randomize_vehicles(UserJsonResult *json_result);
StatusType get_limited_amount(UserJsonResult *json_result);
StatusType get_int_value(cJSON *json, int *result, char *name);
StatusType get_array_value(cJSON *json, void **result, char *name);
StatusType get_string_value(cJSON *json, char **result, char *name);
StatusType get_boolean_value(cJSON *json, _Bool *result, char *name);
StatusType get_all_vehicles(cJSON *json, UserJsonResult *json_result);
StatusType validate_custom_waiting_list(cJSON *json, int *local_vehicle_capacity, _Bool show_logs_now);
StatusType get_custom_waiting_list_count(cJSON *custom_waiting_list_item_p, int *count, _Bool show_logs_now);
StatusType validate_vehicles(cJSON *vehicles_array_p, int *all_vehicles_length, int *valid_indexes);
StatusType get_custom_waiting_list_fuel_needed(cJSON *custom_waiting_list_item_p, int *fuel_needed, _Bool show_logs_now);
StatusType get_custom_waiting_list_wait_time_sec(cJSON *custom_waiting_list_item_p, int *wait_time_sec, _Bool show_logs_now);
StatusType get_custom_waiting_list(cJSON *json, VehicleType vehicle_type, int default_wait_time_sec, int default_fuel_needed, Vehicle **all_user_vehicles, int *count_added_vehicles, _Bool show_logs_now);

// ============

//

// ============

ReadDataParserResult *read_data_parser(char *path, _Bool show_logs)
{
    SHOW_LOGS = show_logs;
    if (path == NULL)
    {
        printf("❌ Path string to 'data.json' file is empty\n");
        return NULL;
    }

    ReadDataParserResult *read_data_parser_result = NULL;
    if (handle_read_data_parser_result_creation(&read_data_parser_result) == 0)
    {
        return NULL;
    }

    if (handle_get_file_buffer(path, &read_data_parser_result) == 0)
    {
        return read_data_parser_result;
    }

    if (handle_parse_file_buffer(&read_data_parser_result) == 0)
    {
        return read_data_parser_result;
    }

    UserJsonResult *json_result = NULL;
    if (handle_json_result_creation(&json_result, &read_data_parser_result) == 0)
    {
        return read_data_parser_result;
    }

    int fuel_pumps_count = 0;
    if (handle_fuel_pumps_count(&fuel_pumps_count, &json_result, &read_data_parser_result) == 0)
    {
        return read_data_parser_result;
    }

    _Bool randomize_arrival = 0;
    if (handle_randomize_arrival(&randomize_arrival, &json_result, &read_data_parser_result) == 0)
    {
        return read_data_parser_result;
    }

    int max_vehicle_capacity = 0;
    if (handle_max_vehicle_capacity(&max_vehicle_capacity, &json_result, &read_data_parser_result) == 0)
    {
        return read_data_parser_result;
    }

    int initial_fuel_in_tanker = 0;
    if (handle_initial_fuel_in_tanker(&initial_fuel_in_tanker, &json_result, &read_data_parser_result) == 0)
    {
        return read_data_parser_result;
    }

    int fuel_transfer_rate = 0;
    if (handle_fuel_transfer_rate(&fuel_transfer_rate, &json_result, &read_data_parser_result) == 0)
    {
        return read_data_parser_result;
    }

    if (handle_get_all_vehicles(&json_result, &read_data_parser_result) == 0)
    {
        return read_data_parser_result;
    }

    if (handle_result_vehicles(&json_result, &read_data_parser_result) == 0)
    {
        return read_data_parser_result;
    }

    if (show_logs == true)
    {
        clean_up();
    }
    return read_data_parser_result;
}

// ============

//

// ============

int get_file_size(FILE *fp)
{
    int size = 0;
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        printf("❌ Failed to seek to end of file\n");
        return -1;
    }
    size = ftell(fp);
    if (fseek(fp, 0, SEEK_SET) != 0)
    {
        printf("❌ Failed to seek to beginning of file\n");
        return -1;
    }

    return size;
}

StatusType get_file_buffer(char **buffer, char *path)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
    {
        perror("❌ Unable to open the file.\n");
        return UNKNOWN_ERROR;
    }

    const int size = get_file_size(fp);
    if (size < 0)
    {
        printf("❌ Unable to get file size.\n");
        fclose(fp);
        return UNKNOWN_ERROR;
    }

    (*buffer) = (char *)malloc((size + 1) * sizeof(char));
    if ((*buffer) == NULL)
    {
        printf("❌ Unable to allocate memory for file buffer.\n");
        fclose(fp);
        return ALLOCATION_ERROR;
    }
    size_t number_of_bytes_transferred = fread((*buffer), 1, size, fp);
    if (number_of_bytes_transferred != (size_t)size)
    {
        printf("❌ Expected to read %d bytes, but read only %ld bytes.\n", size, number_of_bytes_transferred);
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
            printf("❌ Error on parse JSON: %s\n", error_ptr);
        }
        return UNKNOWN_ERROR;
    }
    return CORRECT_VALUE;
}

// ============

//

// ============

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
    *(result) = (char *)malloc((string_length + 1) * sizeof(char));
    if (*(result) == NULL)
    {
        return ALLOCATION_ERROR;
    }
    strcpy((*result), string_value_p->valuestring);

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

// ============

//

// ============

VehicleType get_vehicle_type(cJSON *vehicle_p, _Bool show_logs_now, int index)
{
    VehicleType result_vehicle_type = VEHICLE_NOT_FOUND;

    char *vehicle_type = NULL;
    StatusType vehicle_type_result = get_string_value(vehicle_p, &vehicle_type, "vehicle_type");
    if (vehicle_type_result == ALLOCATION_ERROR)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("📌 Vehicle #%d ❌ (Allocation error)\n", index);
        }
    }
    if (vehicle_type_result == NOT_FOUND)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("📌 Vehicle #%d ❌ (Field is required)\n", index);
        }
    }
    if (vehicle_type_result == WRONG_TYPE)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("📌 Vehicle #%d ❌ (Is not string)\n", index);
        }
    }

    if (vehicle_type == NULL)
    {
        // skip, allocation error
    }
    else if (strcmp(vehicle_type, "auto") == 0)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("📌 Vehicle #%d (%s)\n", index, AUTO_ICON);
            // printf("✅ [vehicle_type]: 🚗\n");
        }
        result_vehicle_type = VEHICLE_AUTO;
    }
    else if (strcmp(vehicle_type, "truck") == 0)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("📌 Vehicle #%d (%s)\n", index, TRUCK_ICON);
            // printf("✅ [vehicle_type]: 🚛\n");
        }
        result_vehicle_type = VEHICLE_TRUCK;
    }
    else if (strcmp(vehicle_type, "van") == 0)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("📌 Vehicle #%d (%s)\n", index, VAN_ICON);
            // printf("✅ [vehicle_type]: 🚙\n");
        }
        result_vehicle_type = VEHICLE_VAN;
    }
    else
    {
        if (strlen(vehicle_type) == 0)
        {
            if (SHOW_LOGS && show_logs_now)
            {
                printf("📌 Vehicle #%d ❌ (Empty name)\n", index);
            }
        }
        else
        {
            if (SHOW_LOGS && show_logs_now)
            {
                printf("📌 Vehicle #%d ❌ (Wrong type name)\n", index);
            }
        }
        result_vehicle_type = VEHICLE_NOT_FOUND;
    }

    free(vehicle_type);
    return result_vehicle_type;
}

// ============

//

// ============

void clean_up()
{
    if (SHOW_LOGS)
    {
        printf("\n🧹 Cleaning up...\n");
    }
    if (buffer != NULL)
    {
        free(buffer);
        buffer = NULL;
        if (SHOW_LOGS)
        {
            printf("✅ Buffer cleaned up\n");
        }
    }
    if (json != NULL)
    {
        cJSON_Delete(json);
        if (SHOW_LOGS)
        {
            printf("✅ JSON cleaned up!\n");
        }
    }
}

void clean_up_json_result(UserJsonResult **json_result)
{
    if ((*json_result) == NULL)
    {
        return;
    }
    if ((*json_result)->all_vehicles != NULL)
    {
        for (int i = 0; i < (*json_result)->all_vehicles_length; i++)
        {
            if ((*json_result)->all_vehicles[i] != NULL)
            {
                free((*json_result)->all_vehicles[i]);
                (*json_result)->all_vehicles[i] = NULL;
            }
        }

        // If we allocated new array for result_vehicles
        if ((*json_result)->result_vehicles != (*json_result)->all_vehicles)
        {
            if ((*json_result)->result_vehicles != NULL)
            {
                free((*json_result)->result_vehicles);
                (*json_result)->result_vehicles = NULL;
            }
        }

        free((*json_result)->all_vehicles);
        (*json_result)->all_vehicles = NULL;
    }
    free((*json_result));
    (*json_result) = NULL;
    if (SHOW_LOGS)
    {
        printf("✅ Cleaning data results finished!\n");
    }
}

void clean_up_read_data_parser_result(ReadDataParserResult **read_data_parser_result)
{
    if (*read_data_parser_result == NULL)
    {
        return;
    }

    clean_up_json_result(
        &((*read_data_parser_result)->json_result) //
    );
    free((*read_data_parser_result));
    *read_data_parser_result = NULL;
    if (SHOW_LOGS)
    {
        printf("✅ Cleaning user results finished!\n");
    }
}

// ============

//

// ============

StatusType randomize_vehicles(UserJsonResult *json_result)
{
    if (SHOW_LOGS)
    {
        printf("✅ Program is running randomizer...\n");
    }
    srand(time(NULL));
    json_result->result_vehicles = (Vehicle **)malloc(json_result->result_vehicles_length * sizeof(Vehicle));
    if (json_result->result_vehicles == NULL)
    {
        return ALLOCATION_ERROR;
    }

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

    const int bigger_length = json_result->all_vehicles_length;
    const int result_length = json_result->result_vehicles_length;

    if (bigger_length < result_length)
    {
        printf("❌ Max vehicle array length is less current.\n");
        return WRONG_VALUE;
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
        for (int i = 0; i < result_length; i++)
        {
            result_array[i] = bigger_array[i];
        }
    }

    for (int j = 0; j < result_length; j++)
    {
        int index = !is_equal ? result_array[j] : bigger_array[j];
        json_result->result_vehicles[j] = json_result->all_vehicles[index];
    }
    return CORRECT_VALUE;
}

int handle_randomize_vehicles(UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result)
{
    StatusType randomize_result = randomize_vehicles((*json_result));
    if (randomize_result == ALLOCATION_ERROR)
    {
        printf("❌ [result_vehicles] unable to allocate memory.\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = ALLOCATION_ERROR;
        return 0;
    }
    if (randomize_result == WRONG_VALUE)
    {
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = WRONG_VALUE;
        return 0;
    }
    return 1;
}

StatusType get_limited_amount(UserJsonResult *json_result)
{
    if (SHOW_LOGS)
    {
        printf("✅ Program is selecting first %d vehicles...\n", json_result->result_vehicles_length);
    }
    json_result->result_vehicles = (Vehicle **)malloc(json_result->result_vehicles_length * sizeof(Vehicle));
    if (json_result->result_vehicles == NULL)
    {
        return ALLOCATION_ERROR;
    }

    for (int j = 0; j < json_result->result_vehicles_length; j++)
    {
        json_result->result_vehicles[j] = json_result->all_vehicles[j];
    }
    return CORRECT_VALUE;
}

int handle_get_limited_amount(UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result)
{
    StatusType limited_amount_result = get_limited_amount((*json_result));
    if (limited_amount_result == ALLOCATION_ERROR)
    {
        printf("❌ [result_vehicles] unable to allocate memory.\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = ALLOCATION_ERROR;
        return 0;
    }
    return 1;
}

// ============

//

// ============

int handle_json_result_creation(UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result)
{
    (*json_result) = (UserJsonResult *)malloc(sizeof(UserJsonResult));
    if ((*json_result) == NULL)
    {
        printf("❌ Unable to allocate memory for [json_result].\n");
        clean_up();
        (*read_data_parser_result)->status = ALLOCATION_ERROR;
        return 0;
    }
    (*json_result)->all_vehicles = NULL;
    (*json_result)->result_vehicles = NULL;
    return 1;
}

int handle_read_data_parser_result_creation(ReadDataParserResult **read_data_parser_result)
{
    (*read_data_parser_result) = (ReadDataParserResult *)malloc(sizeof(ReadDataParserResult));
    if ((*read_data_parser_result) == NULL)
    {
        printf("❌ Unable to allocate memory for [read_data_parser_result].\n");
        return 0;
    }
    (*read_data_parser_result)->status = UNKNOWN_ERROR;
    (*read_data_parser_result)->json_result = NULL;
    return 1;
}

// ============

//

// ============

int handle_fuel_pumps_count(int *fuel_pumps_count, UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result)
{
    StatusType fuel_pumps_count_result = get_int_value(json, fuel_pumps_count, "fuel_pumps_count");
    if (fuel_pumps_count_result == NOT_FOUND)
    {
        printf("❌ [fuel_pumps_count]: Field is required!\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = NOT_FOUND;
        return 0;
    }
    else if (fuel_pumps_count_result == WRONG_TYPE)
    {
        printf("❌ [fuel_pumps_count]: Invalid value! Expected a number.\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = WRONG_TYPE;
        return 0;
    }
    else if ((*fuel_pumps_count) > MAX_FUEL_PUMPS_COUNT)
    {
        printf("❌ [fuel_pumps_count]: Must be less than or equal to the maximum limit of %d.\n", MAX_FUEL_PUMPS_COUNT);
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = MAX_VALUE_ERROR;
        return 0;
    }
    else if ((*fuel_pumps_count) <= 0)
    {
        printf("❌ [fuel_pumps_count]: Must be greater than 0.\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = WRONG_VALUE;
        return 0;
    }
    (*json_result)->fuel_pumps_count = *fuel_pumps_count;
    return 1;
}

int handle_randomize_arrival(_Bool *randomize_arrival, UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result)
{
    StatusType randomize_arrival_result = get_boolean_value(json, randomize_arrival, "randomize_arrival");
    if (randomize_arrival_result == NOT_FOUND)
    {
        if (SHOW_LOGS)
        {
            printf("🔍 [randomize_arrival]: Not found. Defaulting to false.\n");
        }
    }
    else if (randomize_arrival_result == WRONG_TYPE)
    {
        printf("❌ [randomize_arrival]: Invalid value! Expected a boolean.\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = WRONG_TYPE;
        return 0;
    }
    (*json_result)->randomize_arrival = *randomize_arrival;
    return 1;
}

int handle_max_vehicle_capacity(int *max_vehicle_capacity, UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result)
{
    StatusType max_vehicle_capacity_result = get_int_value(json, max_vehicle_capacity, "max_vehicle_capacity");
    if (max_vehicle_capacity_result == NOT_FOUND)
    {
        printf("❌ [max_vehicle_capacity]: Field is required!\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = NOT_FOUND;
        return 0;
    }
    else if (max_vehicle_capacity_result == WRONG_TYPE)
    {
        printf("❌ [max_vehicle_capacity]: Invalid value! Expected a number.\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = WRONG_TYPE;
        return 0;
    }
    else if ((*max_vehicle_capacity) > MAX_VEHICLES)
    {
        printf("❌ [max_vehicle_capacity]: Must be less than or equal to the maximum limit of %d.\n", MAX_VEHICLES);
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = MAX_VALUE_ERROR;
        return 0;
    }
    else if ((*max_vehicle_capacity) <= 0)
    {
        printf("❌ [max_vehicle_capacity]: Must be greater than 0.\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = WRONG_VALUE;
        return 0;
    }
    (*json_result)->max_vehicle_capacity = *max_vehicle_capacity;
    return 1;
}

int handle_initial_fuel_in_tanker(int *initial_fuel_in_tanker, UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result)
{
    StatusType initial_fuel_in_tanker_result = get_int_value(json, initial_fuel_in_tanker, "initial_fuel_in_tanker");
    if (initial_fuel_in_tanker_result == NOT_FOUND)
    {
        printf("❌ [initial_fuel_in_tanker]: Field is required!\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = NOT_FOUND;
        return 0;
    }
    else if (initial_fuel_in_tanker_result == WRONG_TYPE)
    {
        printf("❌ [initial_fuel_in_tanker]: Invalid value! Expected a number.\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = WRONG_TYPE;
        return 0;
    }
    else if ((*initial_fuel_in_tanker) > MAX_INITIAL_FUEL_IN_TANKER)
    {
        printf("❌ [initial_fuel_in_tanker]: Must be less than or equal to the maximum limit of %d.\n", MAX_INITIAL_FUEL_IN_TANKER);
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = MAX_VALUE_ERROR;
        return 0;
    }
    else if ((*initial_fuel_in_tanker) <= 0)
    {
        printf("❌ [initial_fuel_in_tanker]: Must be greater than 0.\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = WRONG_VALUE;
        return 0;
    }
    (*json_result)->initial_fuel_in_tanker = *initial_fuel_in_tanker;
    return 1;
}

int handle_fuel_transfer_rate(int *fuel_transfer_rate, UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result)
{
    StatusType fuel_transfer_rate_result = get_int_value(json, fuel_transfer_rate, "fuel_transfer_rate");
    if (fuel_transfer_rate_result == NOT_FOUND)
    {
        printf("❌ [fuel_transfer_rate]: Field is required!\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = NOT_FOUND;
        return 0;
    }
    else if (fuel_transfer_rate_result == WRONG_TYPE)
    {
        printf("❌ [fuel_transfer_rate]: Invalid value! Expected a number.\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = WRONG_TYPE;
        return 0;
    }
    else if ((*fuel_transfer_rate) > MAX_FUEL_TRANSFER_RATE)
    {
        printf("❌ [fuel_transfer_rate]: Must be less than or equal to the maximum limit of %d.\n", MAX_FUEL_TRANSFER_RATE);
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = MAX_VALUE_ERROR;
        return 0;
    }
    else if ((*fuel_transfer_rate) <= 0)
    {
        printf("❌ [fuel_transfer_rate]: Must be greater than 0.\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = WRONG_VALUE;
        return 0;
    }
    (*json_result)->fuel_transfer_rate = *fuel_transfer_rate;

    if ((*fuel_transfer_rate) > (*json_result)->initial_fuel_in_tanker)
    {
        if (SHOW_LOGS)
        {
            printf(
                "🚨 [fuel_transfer_rate]: can not be bigger than [initial_fuel_in_tanker].\n"
                "🚨 Setting [fuel_transfer_rate] to [initial_fuel_in_tanker]...\n");
        }
        (*json_result)->fuel_transfer_rate = (*json_result)->initial_fuel_in_tanker;
    }
    return 1;
}

int handle_get_file_buffer(char *path, ReadDataParserResult **read_data_parser_result)
{
    StatusType get_file_buffer_result = get_file_buffer(&buffer, path);
    if (get_file_buffer_result == ALLOCATION_ERROR)
    {
        printf("❌ Unable to allocate memory for [buffer].\n");
        clean_up();
        (*read_data_parser_result)->status = ALLOCATION_ERROR;
        return 0;
    }
    if (get_file_buffer_result == UNKNOWN_ERROR)
    {
        printf("❌ Unable to get file [buffer].\n");
        clean_up();
        (*read_data_parser_result)->status = UNKNOWN_ERROR;
        return 0;
    }
    return 1;
}

int handle_parse_file_buffer(ReadDataParserResult **read_data_parser_result)
{
    StatusType parse_file_buffer_result = parse_file_buffer(&json, &buffer);
    if (parse_file_buffer_result == UNKNOWN_ERROR)
    {
        printf("❌ Unable to parse [buffer]\n");
        clean_up();
        (*read_data_parser_result)->status = UNKNOWN_ERROR;
        return 0;
    }
    return 1;
}

int handle_result_vehicles(UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result)
{
    (*read_data_parser_result)->json_result = *json_result;
    (*read_data_parser_result)->status = CORRECT_VALUE;

    (*json_result)->result_vehicles = (*json_result)->all_vehicles;
    (*json_result)->result_vehicles_length = (*json_result)->all_vehicles_length;

    if (SHOW_LOGS)
    {
        // printf("\n");
        // printf("✅ [vehicles] number of all vehicles: %d\n", (*json_result)->all_vehicles_length);
        // printf("\n");
    }
    if ((*json_result)->all_vehicles_length > MAX_VEHICLES)
    {
        if (SHOW_LOGS)
        {
            printf("❌ [vehicles]: Must be less than or equal to the maximum limit of %d.\n", MAX_VEHICLES);
        }
    }

    _Bool is_overlap_vehicle_length = (*json_result)->all_vehicles_length > (*json_result)->max_vehicle_capacity;
    if (is_overlap_vehicle_length)
    {
        if (SHOW_LOGS)
        {
            printf("❌ [vehicles]: Must be less than or equal to %d (max_vehicle_capacity).\n", (*json_result)->max_vehicle_capacity);
        }
        (*json_result)->result_vehicles_length = (*json_result)->max_vehicle_capacity;
    }
    else
    {
        if (SHOW_LOGS)
        {
            printf("✅ Program will use %d vehicles out of the %d total allowed.\n",
                   (*json_result)->all_vehicles_length,
                   (*json_result)->max_vehicle_capacity);
        }
        (*json_result)->result_vehicles_length = (*json_result)->all_vehicles_length;
    }

    if (SHOW_LOGS)
    {
        printf("\n");
        printf("✅ The final count of vehicles program will use is: %d\n", (*json_result)->result_vehicles_length);
        printf("\n");
    }

    if ((*json_result)->randomize_arrival == 1)
    {
        if (SHOW_LOGS)
        {
            printf("✅ Program is going to randomize %d vehicles list...\n", (*json_result)->result_vehicles_length);
        }
        if (handle_randomize_vehicles(json_result, read_data_parser_result) == 0)
        {
            return 0;
        }
        if (SHOW_LOGS)
        {
            printf("✅ Successfully randomized vehicles\n");
        }
    }
    else
    {
        if (is_overlap_vehicle_length)
        {
            if (SHOW_LOGS)
            {
                printf("✅ Program is going to select first %d vehicles...\n", (*json_result)->result_vehicles_length);
            }
            if (handle_get_limited_amount(json_result, read_data_parser_result) == 0)
            {
                return 0;
            }
            if (SHOW_LOGS)
            {
                printf("✅ Successfully selected first %d vehicles\n", (*json_result)->result_vehicles_length);
            }
        }
    }

    return 1;
}

int handle_get_all_vehicles(UserJsonResult **json_result, ReadDataParserResult **read_data_parser_result)
{
    StatusType vehicles_result = get_all_vehicles(json, *json_result);
    if (vehicles_result == VALIDATION_ERROR)
    {
        printf("❌ [vehicles]: Validation was failed.\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = VALIDATION_ERROR;
        return 0;
    }
    if (vehicles_result == NOT_FOUND)
    {
        printf("❌ [vehicles]: Field is required!\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = NOT_FOUND;
        return 0;
    }
    if (vehicles_result == WRONG_TYPE)
    {
        printf("❌ [vehicles]: Invalid value! Expected an array.\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = WRONG_TYPE;
        return 0;
    }
    if (vehicles_result == EMPTY_VALUE)
    {
        printf("❌ [vehicles]: Is empty. Must contain at least one valid object.\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = EMPTY_VALUE;
        return 0;
    }
    if (vehicles_result == EMPTY_VEHICLE_CAPACITY_VALUE)
    {
        printf("❌ [vehicles]: Does not have any valid object. Must contain at least one valid object.\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = EMPTY_VEHICLE_CAPACITY_VALUE;
        return 0;
    }
    if (vehicles_result == ALLOCATION_ERROR)
    {
        printf("❌ [vehicles]: Unable to allocate memory.\n");
        clean_up();
        clean_up_json_result(json_result);
        (*read_data_parser_result)->status = ALLOCATION_ERROR;
        return 0;
    }
    return 1;
}

// ============

//

// ============

StatusType validate_vehicles(cJSON *vehicles_array_p, int *all_vehicles_length, int *valid_indexes)
{
    if (SHOW_LOGS)
    {
        printf("✅ Program running validation...\n");
    }
    cJSON *vehicle_p = NULL;
    int index = 0;
    my_cJSON_ArrayForEach(vehicle_p, vehicles_array_p, index)
    {
        if (SHOW_LOGS)
        {
            printf("\n");
            printf("\n");
        }
        // printf("Vehicle array item #%d\n", index);
        valid_indexes[index] = 0;
        if (SHOW_LOGS)
        {
            // printf("--------------------------------------------- --------------- Level 1: START\n");
        }

        VehicleType vehicle_type = get_vehicle_type(vehicle_p, true, index);
        if (vehicle_type == VEHICLE_NOT_FOUND)
        {
            continue;
        }

        int default_fuel_needed = 0;
        if (get_default_fuel_needed(vehicle_p, &default_fuel_needed, true) == 0)
        {
            continue;
        }

        int default_wait_time_sec = 0;
        if (get_default_wait_time_sec(vehicle_p, &default_wait_time_sec, true) == 0)
        {
            continue;
        }

        int default_count = 0;
        if (get_default_count(vehicle_p, &default_count, true) == 0)
        {
            continue;
        }

        int local_vehicle_capacity = 0;
        StatusType custom_waiting_list_result = validate_custom_waiting_list(vehicle_p, &local_vehicle_capacity, true);
        if (custom_waiting_list_result == NOT_FOUND)
        {
            if (SHOW_LOGS)
            {
                printf("   └─ ❌ Custom list was not found. Using default values.\n");
            }
            local_vehicle_capacity += default_count;
        }
        if (custom_waiting_list_result == EMPTY_VALUE)
        {
            if (SHOW_LOGS)
            {
                printf("   └─ ❌ Custom list is empty. Using default values.\n");
            }
            local_vehicle_capacity += default_count;
        }
        if (custom_waiting_list_result == WRONG_TYPE)
        {
            if (SHOW_LOGS)
            {
                printf("   └─ ❌ Custom list is not array\n");
            }
            continue;
        }
        if (custom_waiting_list_result == WRONG_VALUE)
        {
            if (SHOW_LOGS)
            {
                // printf("\n");
                printf("   ❌ Did not find valid vehicles. Using default values.\n");
                printf("   ✅ Total valid vehicles in this entry: %d\n", default_count);
                // printf("--------------------------------------------- --------------- Level 1: END\n");
            }
            local_vehicle_capacity += default_count;
        }
        else
        {
            if (SHOW_LOGS)
            {
                // printf("\n");
                printf("   ✅ Total valid vehicles in this entry: %d\n", local_vehicle_capacity);
                // printf("--------------------------------------------- --------------- Level 1: END\n");
            }
        }
        *all_vehicles_length += local_vehicle_capacity;
        valid_indexes[index] = 1;
    }

    return CORRECT_VALUE;
}

StatusType validate_custom_waiting_list(cJSON *json, int *local_vehicle_capacity, _Bool show_logs_now)
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

    if (SHOW_LOGS && show_logs_now)
    {
        printf("   └─ ✅ Custom list (%d items):\n", custom_waiting_list_length);
    }

    cJSON *custom_waiting_list_item_p = NULL;
    int index = 0;
    my_cJSON_ArrayForEach(custom_waiting_list_item_p, custom_waiting_list_p, index)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            // printf("\n");
            // printf("--------------------------------Level 2: START\n");
            printf("       ─ Item #%d:\n", index);
        }
        int fuel_needed = 0;
        StatusType fuel_needed_result = get_custom_waiting_list_fuel_needed(custom_waiting_list_item_p, &fuel_needed, true);
        if (fuel_needed_result == WRONG_TYPE)
        {
            continue;
        }
        if (fuel_needed_result == WRONG_VALUE)
        {
            continue;
        }

        int wait_time_sec = 0;
        StatusType wait_time_sec_result = get_custom_waiting_list_wait_time_sec(custom_waiting_list_item_p, &wait_time_sec, true);
        if (wait_time_sec_result == WRONG_TYPE)
        {
            continue;
        }
        if (wait_time_sec_result == WRONG_VALUE)
        {
            continue;
        }

        int count = 0;
        StatusType count_result = get_custom_waiting_list_count(custom_waiting_list_item_p, &count, true);
        if (count_result != CORRECT_VALUE)
        {
            continue;
        }

        (*local_vehicle_capacity) += count;
        if (SHOW_LOGS && show_logs_now)
        {
            // printf("--------------------------------Level 2: END\n");
        }
    }

    if ((*local_vehicle_capacity) == 0)
    {
        return WRONG_VALUE;
    }

    return CORRECT_VALUE;
}

// ============

//

// ============

StatusType get_all_vehicles(cJSON *json, UserJsonResult *json_result)
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
    if (vehicles_array_p == NULL)
    {
        return ALLOCATION_ERROR;
    }

    int vehicle_length = cJSON_GetArraySize(vehicles_array_p);
    if (vehicle_length == 0)
    {
        return EMPTY_VALUE;
    }

    if (SHOW_LOGS)
    {
        printf("\n");
        printf("✅ Found %d [vehicles] objects\n", vehicle_length);
        printf("\n");
    }

    int all_vehicles_length = 0;
    int valid_indexes[vehicle_length];
    StatusType vehicles_validation_result = validate_vehicles(vehicles_array_p, &all_vehicles_length, valid_indexes);
    if (vehicles_validation_result != CORRECT_VALUE)
    {
        return VALIDATION_ERROR;
    }
    if (all_vehicles_length == 0)
    {
        return EMPTY_VEHICLE_CAPACITY_VALUE;
    }

    if (SHOW_LOGS)
    {
        printf("\n");
        printf("\n");
        printf("📋 List of analyzed vehicle elements:\n");
        printf("\n");
        for (int i = 0; i < vehicle_length; i++)
        {
            if (valid_indexes[i] == 1)
            {
                printf("✅ [%2d] Valid.\n", i);
            }
            if (valid_indexes[i] == 0)
            {
                printf("❌ [%2d] Invalid.\n", i);
            }
        }
        printf("\n");
        printf("✅ Total %d valid vehicles was found\n", all_vehicles_length);
        printf("\n");
    }

    int count_added_vehicles = 0;
    json_result->all_vehicles = (Vehicle **)malloc(all_vehicles_length * sizeof(Vehicle));
    if (json_result->all_vehicles == NULL)
    {
        return ALLOCATION_ERROR;
    }

    json_result->all_vehicles_length = all_vehicles_length;

    cJSON *vehicle_p = NULL;
    int index = 0;
    my_cJSON_ArrayForEach(vehicle_p, vehicles_array_p, index)
    {
        if (valid_indexes[index] == 0)
        {
            if (SHOW_LOGS)
            {
                // printf("\n");
                // printf("❌ [%d] Skipping...\n", index);
            }
            continue;
        }
        else
        {
            if (SHOW_LOGS)
            {
                // printf("\n");
                // printf("✅ [%d] Processing...\n", index);
            }
        }

        if (SHOW_LOGS)
        {
            // printf("--------------------------------------------- --------------- Level 1: START\n");
        }
        VehicleType vehicle_type = get_vehicle_type(vehicle_p, false, index);
        if (vehicle_type == VEHICLE_NOT_FOUND)
        {
            continue;
        }

        int default_fuel_needed = 0;
        if (get_default_fuel_needed(vehicle_p, &default_fuel_needed, false) == 0)
        {
            continue;
        }

        int default_wait_time_sec = 0;
        if (get_default_wait_time_sec(vehicle_p, &default_wait_time_sec, false) == 0)
        {
            continue;
        }

        int default_count = 0;
        if (get_default_count(vehicle_p, &default_count, false) == 0)
        {
            continue;
        }

        StatusType custom_waiting_list_result = get_custom_waiting_list(
            vehicle_p,
            vehicle_type,
            default_wait_time_sec,
            default_fuel_needed,
            json_result->all_vehicles,
            &count_added_vehicles,
            false);

        if (custom_waiting_list_result == WRONG_TYPE)
        {
            if (SHOW_LOGS)
            {
                // printf("❌ [custom_waiting_list] is not array\n");
            }
            continue;
        }
        if (custom_waiting_list_result == EMPTY_VALUE || custom_waiting_list_result == NOT_FOUND || custom_waiting_list_result == WRONG_VALUE)
        {
            for (int i = 0; i < default_count; i++)
            {
                Vehicle *new_vehicle = (Vehicle *)malloc(sizeof(Vehicle));
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
            if (SHOW_LOGS)
            {
                // printf("❌ [custom_waiting_list] is empty\n");
            }
        }
        if (custom_waiting_list_result == NOT_FOUND)
        {
            if (SHOW_LOGS)
            {
                // printf("❌ [custom_waiting_list] was not found. Field is required!\n");
            }
        }

        if (SHOW_LOGS)
        {
            // printf("--------------------------------------------- --------------- Level 1: END\n");
        }
    }

    return CORRECT_VALUE;
}

int get_default_fuel_needed(cJSON *vehicle_p, int *default_fuel_needed, _Bool show_logs_now)
{
    StatusType default_fuel_needed_result = get_int_value(vehicle_p, default_fuel_needed, "default_fuel_needed");
    if (default_fuel_needed_result == NOT_FOUND)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("   └─ ❌ Default fuel: Field is required!\n");
        }
        return 0;
    }
    else if (default_fuel_needed_result == WRONG_TYPE)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("   └─ ❌ Default fuel: Invalid value! Expected a number.\n");
        }
        return 0;
    }
    if (*default_fuel_needed < 0)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("   └─ ❌ Default fuel: Must be a positive number.\n");
        }
        return 0;
    }
    if (*default_fuel_needed == 0)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("   └─ ❌ Default fuel: Cannot be zero.\n");
        }
        return 0;
    }
    if (SHOW_LOGS && show_logs_now)
    {
        printf("   ├─ ✅ Default fuel: %d\n", *default_fuel_needed);
    }
    return 1;
}

int get_default_wait_time_sec(cJSON *vehicle_p, int *default_wait_time_sec, _Bool show_logs_now)
{
    StatusType default_wait_time_sec_result = get_int_value(vehicle_p, default_wait_time_sec, "default_wait_time_sec");
    if (default_wait_time_sec_result == NOT_FOUND)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("   └─ ❌ Default wait time: Field is required!\n");
        }
        return 0;
    }
    else if (default_wait_time_sec_result == WRONG_TYPE)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("   └─ ❌ Default wait time: Invalid value! Expected a number.\n");
        }
        return 0;
    }
    if (*default_wait_time_sec < -1)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("   └─ ❌ Default wait time: Value can be only -1 of positive.\n");
        }
        return 0;
    }
    if (SHOW_LOGS && show_logs_now)
    {
        printf("   ├─ ✅ Default wait time: %ds\n", *default_wait_time_sec);
    }
    return 1;
}

int get_default_count(cJSON *vehicle_p, int *default_count, _Bool show_logs_now)
{
    StatusType default_wait_time_sec_result = get_int_value(vehicle_p, default_count, "default_count");
    if (default_wait_time_sec_result == NOT_FOUND)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("   └─ ❌ Default count: Field is required!\n");
        }
        return 0;
    }
    else if (default_wait_time_sec_result == WRONG_TYPE)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("   └─ ❌ Default count: Invalid value! Expected a number.\n");
        }
        return 0;
    }
    if (*default_count < 0)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("   └─ ❌ Default count: Must be a positive number.\n");
        }
        return 0;
    }
    if (*default_count == 0)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("   └─ ❌ Default count: Cannot be zero.\n");
        }
        return 0;
    }
    if (SHOW_LOGS && show_logs_now)
    {
        printf("   ├─ ✅ Default count: %d\n", *default_count);
    }
    return 1;
}

// ============

//

// ============

StatusType get_custom_waiting_list(cJSON *json, VehicleType vehicle_type, int default_wait_time_sec, int default_fuel_needed, Vehicle **all_user_vehicles, int *count_added_vehicles, _Bool show_logs_now)
{
    int local_vehicle_capacity = 0;
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

    if (SHOW_LOGS && show_logs_now)
    {
        // printf("\n");
        // printf("✅ [custom_waiting_list] length: %d\n", custom_waiting_list_length);
    }

    cJSON *custom_waiting_list_item_p = NULL;
    cJSON_ArrayForEach(custom_waiting_list_item_p, custom_waiting_list_p)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            // printf("\n");
            // printf("--------------------------------Level 2: START\n");
        }

        int fuel_needed = 0;
        StatusType fuel_needed_result = get_custom_waiting_list_fuel_needed(custom_waiting_list_item_p, &fuel_needed, false);
        if (fuel_needed_result == WRONG_TYPE)
        {
            continue;
        }
        if (fuel_needed_result == WRONG_VALUE)
        {
            continue;
        }

        int wait_time_sec = 0;
        StatusType wait_time_sec_result = get_custom_waiting_list_wait_time_sec(custom_waiting_list_item_p, &wait_time_sec, false);
        if (wait_time_sec_result == WRONG_TYPE)
        {
            continue;
        }
        if (wait_time_sec_result == WRONG_VALUE)
        {
            continue;
        }

        int count = 0;
        StatusType count_result = get_custom_waiting_list_count(custom_waiting_list_item_p, &count, false);
        if (count_result != CORRECT_VALUE)
        {
            continue;
        }

        local_vehicle_capacity += count;
        for (int i = 0; i < count; i++)
        {
            Vehicle *new_vehicle = (Vehicle *)malloc(sizeof(Vehicle));
            if (new_vehicle == NULL)
            {
                printf("❌ Memory allocation on new vehicle failed.\n");
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

        if (SHOW_LOGS && show_logs_now)
        {
            // printf("--------------------------------Level 2: END\n");
        }
    }

    if (local_vehicle_capacity == 0)
    {
        return WRONG_VALUE;
    }
    return CORRECT_VALUE;
}

StatusType get_custom_waiting_list_count(cJSON *custom_waiting_list_item_p, int *count, _Bool show_logs_now)
{
    StatusType count_result = get_int_value(custom_waiting_list_item_p, count, "count");
    if (count_result == NOT_FOUND)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("            ❌ Count: Field is required!\n");
        }
        return NOT_FOUND;
    }
    if (count_result == WRONG_TYPE)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("            ❌ Count: Invalid value! Expected a number.\n");
        }
        return WRONG_TYPE;
    }
    if ((*count) <= 0)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("            ❌ Count: Must be greater than 0\n");
        }
        return WRONG_VALUE;
    }
    if (SHOW_LOGS && show_logs_now)
    {
        printf("            ✅ Count: %d\n", *count);
    }
    return CORRECT_VALUE;
}

StatusType get_custom_waiting_list_fuel_needed(cJSON *custom_waiting_list_item_p, int *fuel_needed, _Bool show_logs_now)
{
    StatusType fuel_needed_result = get_int_value(custom_waiting_list_item_p, fuel_needed, "fuel_needed");
    if (fuel_needed_result == NOT_FOUND)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("            🔎 Fuel: Not found\n");
        }
        return NOT_FOUND;
    }
    else if (fuel_needed_result == WRONG_TYPE)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("            ❌ Fuel: Invalid value! Expected a number.\n");
        }
        return WRONG_TYPE;
    }
    else if ((*fuel_needed) <= 0)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("            ❌ Fuel: Must be greater than 0\n");
        }
        return WRONG_VALUE;
    }
    if (SHOW_LOGS && show_logs_now)
    {
        printf("            ✅ Fuel: %d\n", *fuel_needed);
    }
    return CORRECT_VALUE;
}

StatusType get_custom_waiting_list_wait_time_sec(cJSON *custom_waiting_list_item_p, int *wait_time_sec, _Bool show_logs_now)
{
    StatusType wait_time_sec_result = get_int_value(custom_waiting_list_item_p, wait_time_sec, "wait_time_sec");
    if (wait_time_sec_result == NOT_FOUND)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("            🔎 Wait time: Not found\n");
        }
        return NOT_FOUND;
    }
    else if (wait_time_sec_result == WRONG_TYPE)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("            ❌ Wait time: Invalid value! Expected a number.\n");
        }
        return WRONG_TYPE;
    }
    else if (*wait_time_sec < -1)
    {
        if (SHOW_LOGS && show_logs_now)
        {
            printf("            ❌ Wait time: Value can be only -1 of positive.\n");
        }
        return WRONG_VALUE;
    }
    if (SHOW_LOGS && show_logs_now)
    {
        printf("            ✅ Wait time: %d\n", *wait_time_sec);
    }
    return CORRECT_VALUE;
}

// ============

//

// ============

void print_json_result(UserJsonResult *json_result)
{
    printf("\n");
    printf("🛠️  Simulation Constraints:\n");
    printf("   🚘 Maximum number of vehicles allowed: %d.\n", MAX_VEHICLES);
    printf("   ⛽ Maximum fuel pumps allowed: %d.\n", MAX_FUEL_PUMPS_COUNT);
    printf("   🛢️  Tanker can hold up to %d liters of fuel.\n", MAX_INITIAL_FUEL_IN_TANKER);
    printf("   🔄 Fuel Transfer Speed: Max %d liters/sec", MAX_FUEL_TRANSFER_RATE);

    printf("\n");
    printf("\n");
    printf("📋 JSON data: \n");
    if (json_result == NULL)
    {
        printf("   └─ ❌ JSON data is empty.\n");
        return;
    }
    printf("   ├─ ✅ Fuel pumps count: %d\n", json_result->fuel_pumps_count);
    printf("   ├─ ✅ Initial tanker fuel: %d\n", json_result->initial_fuel_in_tanker);
    printf("   ├─ ✅ Fuel transfer rate: %d\n", json_result->fuel_transfer_rate);
    printf("   ├─ ✅ Max vehicle capacity: %d\n", json_result->max_vehicle_capacity);
    printf("   ├─ ✅ Randomized arrival: %s\n", json_result->randomize_arrival == 0 ? "false" : "true");

    if (json_result->result_vehicles == NULL)
    {
        printf("   └─ ❌ List of cars is empty.\n");
        return;
    }

    printf("   └─ ✅ Total vehicles: %d\n", json_result->result_vehicles_length);\

    printf("\n");
    printf("📋 List of cars:\n");
    for (int i = 0; i < json_result->result_vehicles_length; i++)
    {
        if (json_result->result_vehicles[i] == NULL)
        {
            printf("❌ json_result->result_vehicles[i] is NULL.\n");
            continue;
        }
        if (json_result->result_vehicles[i]->vehicle_type == VEHICLE_VAN)
        {

            printf("   ├─ ✅ #%d(🚙):\n", i + 1);
        }
        if (json_result->result_vehicles[i]->vehicle_type == VEHICLE_TRUCK)
        {

            printf("   ├─ ✅ #%d(🚛):\n", i + 1);
        }
        if (json_result->result_vehicles[i]->vehicle_type == VEHICLE_AUTO)
        {

            printf("   ├─ ✅ #%d(🚗):\n", i + 1);
        }
        if (i == json_result->result_vehicles_length - 1)
        {
            printf("   ├   ├─ 🛢️  Fuel: %d liters\n", json_result->result_vehicles[i]->fuel_needed);
            printf("   ├   └─ ⏳ Wait: %d seconds\n", json_result->result_vehicles[i]->wait_time_sec);    
            printf("   \n");
        }
        else
        {
            printf("   ├   ├─ 🛢️  Fuel: %d liters\n", json_result->result_vehicles[i]->fuel_needed);
            printf("   ├   └─ ⏳ Wait: %d seconds\n", json_result->result_vehicles[i]->wait_time_sec);    
            printf("   ├\n");
        }
    }
}
