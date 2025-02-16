#ifdef __linux__
// "Let's go!"
#else
#error "Only __linux__ supported"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "util_read_data_parser.h"

static ReadDataParserResult *read_data_parser_result = NULL;
static char *default_json_file = "data.json";
static char *path = NULL;

int check_if_file_exist(char *name)
{
    DIR *dir_stream = opendir(".");
    if (dir_stream == NULL)
    {
        perror("❌ Cannot open reading dir stream.\n");
        return -1;
    }
    int found_flag = 0;
    struct dirent *dp = readdir(dir_stream);
    while (dp != NULL)
    {
        if (strncmp(dp->d_name, name, strlen(name)) == 0)
        {
            found_flag = 1;
            break;
        }
        dp = readdir(dir_stream);
    }
    if (closedir(dir_stream) != 0)
    {
        perror("❌ Cannot close reading dir stream.\n");
        return -1;
    }
    return found_flag;
}

int get_result_path(int argc, char **argv)
{
    const int default_json_file_length = strlen(default_json_file);

    if (argc == 1)
    {
        printf("🔍 Path was not specified, trying to read '%s'...\n", default_json_file);
        int if_file_exist_result = check_if_file_exist(default_json_file);
        if (if_file_exist_result == -1)
        {
            return 0;
        }
        if (if_file_exist_result == 0)
        {
            printf("❌ File '%s' was NOT found.\n", default_json_file);
            return 0;
        }
        printf("✅ File '%s' was found.\n", default_json_file);
        printf("\n");
        printf("✅ Parsing...\n");
        printf("\n");
        strncpy((path), default_json_file, default_json_file_length + 1);
        (path)[default_json_file_length] = '\0';
    }
    else
    {
        char *user_path = argv[1];
        const int user_path_length = strlen(user_path);
        char user_path_result[user_path_length + 1];

        strncpy(user_path_result, user_path, user_path_length + 1);
        user_path_result[user_path_length] = '\0';

        int if_file_exist_result = check_if_file_exist(user_path_result);
        if (if_file_exist_result == -1)
        {
            return 0;
        }
        if (if_file_exist_result == 0)
        {
            printf("❌ File '%s' was NOT found.\n", user_path_result);
            return 0;
        }
        printf("✅ File '%s' was found.\n", user_path_result);
        printf("\n");
        printf("✅ Parsing...\n");
        printf("\n");
        char *temp = (char *)realloc((path), (user_path_length + 1) * sizeof(char));
        if (temp == NULL)
        {
            printf("❌ ALLOCATION_ERROR\n");
            return 0;
        }
        strncpy((path), user_path_result, user_path_length + 1);
        (path)[user_path_length] = '\0';
    }
    return 1;
}

void clean_up_validation()
{
    printf("\n🧹 Cleaning up validation script...\n");
    if (read_data_parser_result != NULL)
    {
        clean_up_read_data_parser_result(&read_data_parser_result);
    }
    if (path != NULL)
    {
        free(path);
        printf("   └─ ✅ Cleaning allocated memory for 'path' finished!\n");
    }
}

int main(int argc, char **argv)
{
    atexit(clean_up_validation);
    const int default_json_file_length = strlen(default_json_file);
    path = (char *)malloc((default_json_file_length + 1) * sizeof(char));
    if (path == NULL)
    {
        printf("❌ ALLOCATION_ERROR\n");
        return 1;
    }

    if (get_result_path(argc, argv) == 0)
    {
        return 1;
    }

    read_data_parser_result = read_data_parser(path, true);
    if (read_data_parser_result == NULL)
    {
        printf("❌ [read_data_parser_result] is empty\n");
        return 1;
    }
    if (read_data_parser_result->json_result == NULL)
    {
        printf("❌ [read_data_parser_result->json_result] is empty\n");
        return 1;
    }
    if (read_data_parser_result->status == UNKNOWN_ERROR)
    {
        printf("❌ UNKNOWN_ERROR\n");
        return 1;
    }
    if (read_data_parser_result->status == ALLOCATION_ERROR)
    {
        printf("❌ ALLOCATION_ERROR\n");
        return 1;
    }
    if (read_data_parser_result->status == NOT_FOUND)
    {
        printf("❌ NOT_FOUND\n");
        return 1;
    }
    if (read_data_parser_result->status == WRONG_TYPE)
    {
        printf("❌ WRONG_TYPE\n");
        return 1;
    }
    if (read_data_parser_result->status == MAX_VEHICLES_ERROR)
    {
        printf("❌ MAX_VEHICLES_ERROR\n");
        return 1;
    }
    if (read_data_parser_result->status == EMPTY_VALUE)
    {
        printf("❌ EMPTY_VALUE\n");
        return 1;
    }
    if (read_data_parser_result->status == EMPTY_VEHICLE_CAPACITY_VALUE)
    {
        printf("❌ EMPTY_VEHICLE_CAPACITY_VALUE\n");
        return 1;
    }
    if (read_data_parser_result->status == WRONG_VALUE)
    {
        printf("❌ WRONG_VALUE\n");
        return 1;
    }
    if (read_data_parser_result->status == MAX_VALUE_ERROR)
    {
        printf("❌ MAX_VALUE_ERROR\n");
        return 1;
    }
    if (read_data_parser_result->status == VALIDATION_ERROR)
    {
        printf("❌ VALIDATION_ERROR\n");
        return 1;
    }
    if (read_data_parser_result->status != CORRECT_VALUE)
    {
        printf("❌ Failed to parse 'data.json' file.\n");
        return 1;
    }
    printf("\n");
    printf("✅ Successfully parsed 'data.json' file.\n");
    print_json_result(read_data_parser_result->json_result);
    return 0;
}
