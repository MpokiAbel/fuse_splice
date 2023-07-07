#include "curl.h"
#include <termios.h>
#include "memory.h"

CURL* my_curl_init()
{
    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);
    curl_easy_setopt(curl, CURLOPT_URL, ROOT_URL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_call_back);
    return curl;
}

void parse_userpwd()
{
    char username[32], pwd[32];
    //printf("Enter your username : ");
    //scanf("%s", username);
    printf("Enter your password : ");

    struct termios term, term_orig;
    tcgetattr(fileno(stdin), &term);
    term_orig = term;
    term.c_lflag &= ~ECHO;
    tcsetattr(fileno(stdin), TCSANOW, &term);

    scanf("%s", pwd);

    tcsetattr(fileno(stdin), TCSANOW, &term_orig);

    snprintf(userpwd, 64, "%s:%s", USER_NAME, pwd);
}

int check_userpwd()
{
    CURL* curl = my_curl_init();
    struct memory_struct mem;
    chunk_init(curl, &mem);
    CURLcode code = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    chunk_free(&mem);
    return code != CURLE_LOGIN_DENIED;
}
