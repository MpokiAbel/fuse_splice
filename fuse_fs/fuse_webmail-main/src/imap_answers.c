#include "imap_answers.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int parse_select_exists(const char* line, int* out)
{
    char* ptr = strstr(line, "EXISTS");
    if (!ptr)
        return -1;
    ptr--;
    *ptr = '\0';
    while (*(ptr--) != ' ')
    {

    }
    *out = atoi(ptr + 2);
    return 0;
}

int parse_list(char** line, char* res, const char* end)
{
    char* cur = *line;
    do
    {
        ++cur;
        if (cur == end)
            return -1;
    }
    while(*cur != '\r' && *cur != '\n');

    cur--;
    if (*cur != '\"') {
        return -1;
    }
    *line = cur + 2;

    size_t len;
    for (len = 0; *(--cur) != '\"'; ++len) {

    }
    strncpy(res, cur + 1, len);
    res[len] = '\0';
    return 0;
}

static char* my_strnstr(const char* haystack, const char* needle, int n)
{
    size_t needle_len = strlen(needle);
    for (int i = 0; i < n - needle_len && haystack[i] != 0; ++i)
    {
        for (int j = 0; j < needle_len; ++j) {
            if (haystack[i + j] != needle[j])
                break;
            if (j == needle_len - 1)
                return haystack + i;
        }
    }
    return NULL;
}

int get_header_attr(const char* answer, size_t len_answer, char** sub, char* str_to_search)
{
    char* start = strstr(answer, str_to_search);
    if (!start)
        return -1;

    char* end = my_strnstr(start, "\r\n", len_answer);
    if (!end)
        end = answer + len_answer - 1;
    else
        end+=2;
    size_t len = end - start;
    *sub = malloc(len + 1);
    strncpy(*sub, start, len);
    (*sub)[len] = 0;
    return len;
}