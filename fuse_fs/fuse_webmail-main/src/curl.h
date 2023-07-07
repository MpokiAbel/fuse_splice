#pragma once

#include <curl/curl.h>

char userpwd[64];
#define USER_NAME "mpoki.mwaisela@grenoble-inp.org"
#define ROOT_URL "imaps://webmail.grenoble-inp.org/"

CURL* my_curl_init();
void parse_userpwd();
int check_userpwd();