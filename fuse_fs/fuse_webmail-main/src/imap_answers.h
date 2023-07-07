#pragma once
#include <stdlib.h>

int parse_select_exists(const char* line, int* out);
int parse_list(char** line, char* name, const char* end);
int get_header_attr(const char* answer, size_t len_answer, char** res, char* str_to_search);