#include "memory.h"
#include <string.h>

void chunk_init(CURL* curl, struct memory_struct* mem)
{
    mem->data = NULL;
    mem->len = 0;
    mem->header_len = 0;
    mem->text_len_only = 0;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, mem);
}

void chunk_free(struct memory_struct* mem)
{
    free(mem->data);
}

int extract_length(const char* fetched)
{
    char* ptr = (char*)fetched;
    while (*ptr != '{')
    {
        if (*ptr == '\r' || *ptr == '\n')
            return -1;
        ptr++;
    }
    ptr++;
    char* start = ptr;
    size_t len = 0;
    while (*ptr != '}')
    {
        ptr++;
        len++;
    }
    char* mail_len = malloc(len + 1);
    snprintf(mail_len, len + 1, "%s", start);
    int res = atoi(mail_len);
    free(mail_len);
    return res;
}

size_t write_memory_call_back(void* content, size_t size, size_t nmemb, void* userdata)
{
    size_t total_size = size * nmemb;

    struct memory_struct *mem = (struct memory_struct *)userdata;

    int body_length = extract_length(content);
    if (body_length >= 0 && !mem->text_len_only) {
        {
            char* start = strstr(content, "}") + 1;
            if (strstr(start + 2, "\r")[1] != '\n')
                return total_size;
        }
        mem->header_len = total_size;
        total_size += body_length;
    }

    char *ptr = realloc(mem->data, mem->len + total_size + 1);
    if(!ptr) {
        fprintf(stderr, "%s", "realloc failed\n");
        return 0;
    }

    mem->data = ptr;
    memset(&mem->data[mem->len], 0, total_size);
    memcpy(&(mem->data[mem->len]), content, total_size);
    mem->len += total_size;
    mem->data[mem->len] = 0;

    return total_size;
}