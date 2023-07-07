#include "path_type.h"
#include "imap_answers.h"

bool is_directory(const char* path)
{
    size_t path_len = strlen(path);
    CURL* curl = my_curl_init();
    struct memory_struct mem;
    chunk_init(curl, &mem);

    size_t request_len = strlen("SELECT CC") + path_len;
    char* request = malloc(request_len + 1);
    snprintf(request, request_len, "SELECT \"%s\"", path + 1);
    request[request_len] = '\0';
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, request);

    CURLcode code = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    free(request);
    chunk_free(&mem);
    return code == CURLE_OK;
}

int search_for_slash_i(const char* path, int i)
{
    int n = 0, res = 0;
    while (path[res] != '\0' )
    {
        if (path[res] == '/' && ++n == i)
            return res;
        res++;
    }
    return -1;
}

#define RETURN(res) do { out = res; goto end; } while(0)

bool is_reg_file(const char* path)
{
    int mail_uid;
    char directory[32];
    if (parse_file_path(path, &mail_uid, directory) < 0)
        return false;
    size_t directory_len = strlen(directory);

    bool out;

    struct memory_struct mem;
    CURL* curl = my_curl_init();
    chunk_init(curl, &mem);
    size_t len = strlen(ROOT_URL) + directory_len + 2;
    char* new_url = malloc(len);
    snprintf(new_url, len, "%s%s", ROOT_URL, path);

    curl_easy_setopt(curl, CURLOPT_URL, new_url);
    free(new_url);

    size_t request_len = strlen("SELECT CC") + directory_len;
    char* request = malloc(request_len + 1);
    snprintf(request, request_len, "SELECT \"%s\"", directory);
    request[request_len - 1] = '\"';
    request[request_len] = '\0';
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, request);
    CURLcode code = curl_easy_perform(curl);
    if (code != CURLE_OK)
        RETURN(false);
    int n_mails_in_dir;
    if (parse_select_exists(mem.data, &n_mails_in_dir) < 0)
    {
        RETURN(false);
    }

    out = mail_uid <= n_mails_in_dir;

end:
    chunk_free(&mem);
    free(request);
    curl_easy_cleanup(curl);
    return out;
}

enum my_file_types type_of_file(const char* path, int* num)
{
    if (strcmp("/", path) == 0) {
        if (!num)
            return ft_root;
        struct memory_struct mem;

        CURL* curl = my_curl_init();
        chunk_init(curl, &mem);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "LIST \"\" \"*\"");
        CURLcode code = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if (code != CURLE_OK)
            exit(EXIT_FAILURE);
        *num = 0;

        char* ptr = mem.data;
        while (*ptr)
        {
            if ('\n' == *ptr)
                num++;
            ptr++;
        }

        chunk_free(&mem);
        return ft_root;
    }

    if (is_directory(path))
        return ft_directory;
    if (is_reg_file(path))
        return ft_regular;
    return ft_unexistant;
}

int parse_file_path(const char* path, int* uid, char* dir)
{
    int index = search_for_slash_i(path, 2);
    if (index < 0)
        return -1;

    strncpy(dir, path + 1, index - 1);
    dir[index - 1] = '\0';
    char str_mail[6];
    strncpy(str_mail, path + index + 1, 6);
    str_mail[5] = '\0';
    if (strcmp("mail#", str_mail) != 0)
        return -1;

    int index_first_digit = index + 6;
    for (char* ptr = (char*)path + index_first_digit; *ptr != '\0'; ptr++) {

        if (*ptr < '0' || *ptr > '9')
            return -1;
    }
    *uid = atoi(path + index_first_digit);
    return 0;
}


size_t get_file_size(const char* path)
{
    int mail_uid;
    char directory[32];
    if (parse_file_path(path, &mail_uid, directory) < 0)
        return 0;
    char mail_uid_str[11];
    snprintf(mail_uid_str, 11, "%d", mail_uid);
    size_t directory_len = strlen(directory),
        uid_len = strlen(mail_uid_str);

    struct memory_struct* mem = malloc(sizeof(struct memory_struct));
    CURL* curl = my_curl_init();
    mem->data = NULL;
    mem->text_len_only = 1;
    mem->len = 0;
    mem->header_len = 0;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, mem);
    size_t len = strlen(ROOT_URL) + directory_len + 2;
    char* new_url = malloc(len);
    snprintf(new_url, len, "%s%s", ROOT_URL, path);

    curl_easy_setopt(curl, CURLOPT_URL, new_url);
    free(new_url);

    size_t request_len = strlen("FETCH C BODY.PEEK[TEXT]") + uid_len;
    char* request = malloc(request_len + 1);
    snprintf(request, request_len, "FETCH %d BODY.PEEK[TEXT]", mail_uid);
    request[request_len] = '\0';
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, request);
    CURLcode code = curl_easy_perform(curl);
    size_t res;
    if (code != CURLE_OK)
        res = 0;
    else
    {
        char str_to_find[] = "FETCH (BODY[TEXT] {";
        char* n = strstr(mem->data, str_to_find);
        if (!n) {
            res = 0;
            goto end;
        }
        n += strlen(str_to_find);

        size_t len_res = (mem->data + mem->len - n - 2);
        char* str = malloc(len_res);
        snprintf(str, len_res, "%s", n);
        res = atoi(str);
        free(str);
    }

end:
    curl_easy_cleanup(curl);
    chunk_free(mem);
    free(request);
    free(mem);
    return res;
}