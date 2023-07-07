#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "curl.h"
#include "memory.h"

enum my_file_types
{
    ft_regular,
    ft_directory,
    ft_root,
    ft_unexistant,
};

enum my_file_types type_of_file(const char* path, int* num);

int parse_file_path(const char* path, int* uid, char* dir);
size_t get_file_size(const char* path);