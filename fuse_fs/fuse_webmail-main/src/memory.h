#pragma once

#include <curl/curl.h>
#include <stdlib.h>

struct memory_struct {
    char *data;
    size_t len;
    size_t header_len;
    int text_len_only;
};

void chunk_init(CURL* curl, struct memory_struct* mem);
void chunk_free(struct memory_struct* mem);

size_t write_memory_call_back(void* content, size_t size, size_t nmemb, void* userdata);