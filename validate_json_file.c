#ifdef __linux__
// "Let's go!"
#else
#error "Only __linux__ supported"
#endif

#include <stdio.h>
#include "util_read_data_parser.h"

int main()
{
    char *path = "data.json";
    ReadDataParserResult *read_data_parser_result = read_data_parser(path, true);
    if (read_data_parser_result == NULL)
    {
        printf("❌ [read_data_parser_result] is empty\n");
        clean_up_read_data_parser_result(&read_data_parser_result);
        return 1;
    }
    if (read_data_parser_result->json_result == NULL)
    {
        printf("❌ [read_data_parser_result->json_result] is empty\n");
        clean_up_read_data_parser_result(&read_data_parser_result);
        return 1;
    }
    if (read_data_parser_result->status == UNKNOWN_ERROR)
    {
        printf("❌ UNKNOWN_ERROR\n");
        clean_up_read_data_parser_result(&read_data_parser_result);
        return 1;
    }
    if (read_data_parser_result->status == ALLOCATION_ERROR)
    {
        printf("❌ ALLOCATION_ERROR\n");
        clean_up_read_data_parser_result(&read_data_parser_result);
        return 1;
    }
    if (read_data_parser_result->status == NOT_FOUND)
    {
        printf("❌ NOT_FOUND\n");
        clean_up_read_data_parser_result(&read_data_parser_result);
        return 1;
    }
    if (read_data_parser_result->status == WRONG_TYPE)
    {
        printf("❌ WRONG_TYPE\n");
        clean_up_read_data_parser_result(&read_data_parser_result);
        return 1;
    }
    if (read_data_parser_result->status == MAX_VEHICLES_ERROR)
    {
        printf("❌ MAX_VEHICLES_ERROR\n");
        clean_up_read_data_parser_result(&read_data_parser_result);
        return 1;
    }
    if (read_data_parser_result->status == EMPTY_VALUE)
    {
        printf("❌ EMPTY_VALUE\n");
        clean_up_read_data_parser_result(&read_data_parser_result);
        return 1;
    }
    if (read_data_parser_result->status == EMPTY_VEHICLE_CAPACITY_VALUE)
    {
        printf("❌ EMPTY_VEHICLE_CAPACITY_VALUE\n");
        clean_up_read_data_parser_result(&read_data_parser_result);
        return 1;
    }
    if (read_data_parser_result->status == WRONG_VALUE)
    {
        printf("❌ WRONG_VALUE\n");
        clean_up_read_data_parser_result(&read_data_parser_result);
        return 1;
    }
    if (read_data_parser_result->status == MAX_VALUE_ERROR)
    {
        printf("❌ MAX_VALUE_ERROR\n");
        clean_up_read_data_parser_result(&read_data_parser_result);
        return 1;
    }
    if (read_data_parser_result->status == VALIDATION_ERROR)
    {
        printf("❌ VALIDATION_ERROR\n");
        clean_up_read_data_parser_result(&read_data_parser_result);
        return 1;
    }
    if (read_data_parser_result->status != CORRECT_VALUE)
    {
        printf("❌ Failed to parse 'data.json' file.\n");
        clean_up_read_data_parser_result(&read_data_parser_result);
        return 1;
    }
    printf("\n");
    printf("✅ Successfully parsed 'data.json' file.\n");
    print_json_result(read_data_parser_result->json_result);
    clean_up_read_data_parser_result(&read_data_parser_result);
    return 0;
}
