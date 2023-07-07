#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <termios.h>
#include <curl/curl.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "memory.h"
#include "imap_answers.h"
#include "curl.h"
#include "path_type.h"

noreturn void report(const char* msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

char userpwd[64];

#define GET_HEADER_ATTR(target, raw)  if ((attr_len = get_header_attr(mem.data, mem.len, &target, raw)) > 0)  \
{  \
if (buffer)  memcpy(buffer + res + offset, target, attr_len);  \
free(target);  \
res += attr_len;  \
}

static int read_header(const char* url, int seq_num, char* buffer, off_t offset)
{
    struct memory_struct mem;

    CURL* curl = my_curl_init();
    chunk_init(curl, &mem);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    size_t request_len = strlen("UID FETCH %d BODY[HEADER]") + 11;
    char* request = malloc(request_len);
    snprintf(request, request_len, "FETCH %d BODY[HEADER]", seq_num);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, request);
    curl_easy_perform(curl);

    int res = 0;
    char* receivers, *date, *sender, *subject, *Cc;
    int attr_len;
    GET_HEADER_ATTR(date, "Date: ")
    GET_HEADER_ATTR(receivers, "To: ")

    if ((attr_len = get_header_attr(mem.data, mem.len, &Cc, "Cc: ")) > 0)
    {
        if (buffer)
            memcpy(buffer + res + offset, Cc, attr_len);
        free(Cc);
        res += attr_len;
    }

    GET_HEADER_ATTR(sender, "From: ")
    GET_HEADER_ATTR(subject, "Subject: ")
    chunk_free(&mem);
    curl_easy_cleanup(curl);
    free(request);
    return res;
}

static int partage_getattr(const char* path, struct stat* buf, struct fuse_file_info* fi)
{
    /*
     * could be
     * /.Trash-1002
     * /.xdg-volume-info
     * /autorun.inf
     */
    (void)fi;

    buf->st_size = 4096;
    int num = 0;
    enum my_file_types t = type_of_file(path, &num);
    switch (t) {
        case ft_directory:
            buf->st_mode = S_IFDIR | 0755;
            buf->st_nlink = 2;
            break;
        case ft_regular:
            buf->st_nlink = 1;
            buf->st_mode = S_IFREG | 0444;


            int seq_num;
            char dir[32];
            if (parse_file_path(path, &seq_num, dir) < 0)
                return -ENOENT;

            size_t new_url_len = strlen(ROOT_URL) + strlen(dir) + 2;
            char* new_url = malloc(new_url_len);
            snprintf(new_url, new_url_len, "%s%s", ROOT_URL, path);

            buf->st_size = (long)get_file_size(path) + read_header(new_url, seq_num, NULL, 0);
            free(new_url);
            break;
        case ft_root:
            buf->st_mode = S_IFDIR |0755;
            buf->st_nlink = 1 + num;
            break;
        case ft_unexistant:
            return -ENOENT;
    }

    buf->st_uid = getuid();
    buf->st_gid = getgid();
    buf->st_atime = time(NULL);
    buf->st_mtime = time(NULL);
    buf->st_blocks = 1;
    buf->st_blksize = 512;
    buf->st_dev = 0;

    return 0;
}

static int partage_access(const char* path, int mod)
{
    (void)path;
    (void)mod;
    return 0;
}

static int partage_read(const char* path, char* buffer, size_t len, off_t offset, struct fuse_file_info* fi)
{
    (void)fi;
    if (type_of_file(path, NULL) != ft_regular)
        return -ENOENT;

    int seq_num;
    char dir[32];
    if (parse_file_path(path, &seq_num, dir) < 0)
        return -ENOENT;

    size_t new_url_len = strlen(ROOT_URL) + strlen(dir) + 2;
    char* new_url = malloc(new_url_len);
    snprintf(new_url, new_url_len, "%s%s", ROOT_URL, path);

    int header_len = read_header(new_url, seq_num, buffer, offset);

    CURL* curl = my_curl_init();
    struct memory_struct mem;
    chunk_init(curl, &mem);
    int res;
    curl_easy_setopt(curl, CURLOPT_URL, new_url);

    size_t request_len = strlen("UID FETCH %d BODY[TEXT]") + 11;
    char* request = malloc(request_len);
    snprintf(request, request_len, "FETCH %d BODY[TEXT]", seq_num);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, request);

    CURLcode code = curl_easy_perform(curl);
    if (code == CURLE_QUOTE_ERROR || code == CURLE_COULDNT_CONNECT)
    {
        res = -ENOENT;
        goto end;
    }

    if (mem.len <= (size_t)offset) {
        res = 0;
        goto end;
    }

    size_t bytes_to_read = (len > mem.len - offset) ? mem.len - offset : len;
    memcpy(buffer + header_len, mem.data + offset + mem.header_len, bytes_to_read);
    res = (int)bytes_to_read + header_len;

end:
    chunk_free(&mem);
    free(new_url);
    free(request);
    curl_easy_cleanup(curl);
    return res;
}

static int partage_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
    (void)offset;
    (void)fi;
    (void)flags;

    filler(buffer, ".", NULL, 0, 0);
    filler(buffer, "..", NULL, 0, 0);
    CURL *curl = my_curl_init();
    struct memory_struct mem;
    chunk_init(curl, &mem);
    int res = 0;

    if (strcmp(path, "/") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "LIST \"\" \"*\"");
        CURLcode code = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if (code != CURLE_OK)
        {
            chunk_free(&mem);
            report("readdir on root failed");
        }

        char *line = mem.data, *end = mem.data + mem.len;
        char name[16];
        for (;;) {
            if (parse_list(&line, name, end) < 0)
                break;
            filler(buffer, name, NULL, 0, 0);
        }
    }
    else if (type_of_file(path, NULL) == ft_directory)
    {
        size_t request_len = strlen("SELECT CC") + strlen(path);
        char* request = malloc(request_len + 1);
        snprintf(request, request_len, "SELECT \"%s\"", path + 1);
        request[request_len] = '\0';
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, request);
        CURLcode code = curl_easy_perform(curl);
        free(request);

        int num_entry;
        //write(file_log, mem.data, mem.len);
        if (code != CURLE_OK || parse_select_exists(mem.data, &num_entry) < 0)
            return -ENOENT;

        for (int i = 0; i < (num_entry & 31); ++i) {
            char n[16];
            snprintf(n, 16, "mail#%d", i + 1);
            filler(buffer, n, NULL, 0, 0);
        }

        curl_easy_cleanup(curl);
    }
    else
        res = -ENOENT;
    chunk_free(&mem);

    return res;
}

static int partage_open(const char* path, struct fuse_file_info* fi)
{
    (void)fi;
    return (type_of_file(path, NULL) != ft_regular) ? -ENOENT : 0;
}

static int partage_release(const char* path, struct fuse_file_info* fi)
{
    (void)fi;
    return (type_of_file(path, NULL) != ft_regular) ? -ENOENT : 0;
}

struct fuse_operations partage_operation = {
        .getattr = partage_getattr,
        .access = partage_access,
        .read = partage_read,
        .readdir = partage_readdir,
        .open = partage_open,
        .release = partage_release,
};/*

int copy_volume_info()
{
    int fd = open("mount/.xdg-volume-info", O_CREAT | O_RDWR);

    write(fd, )
}*/

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "usage : %s -f <mountpoint>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    parse_userpwd();
    if (!check_userpwd())
    {
        fprintf(stderr, "%s\n", "wrong user/pwd");
        exit(EXIT_FAILURE);
    }
    int out = fuse_main(argc, argv, &partage_operation, NULL);
    return out;
}