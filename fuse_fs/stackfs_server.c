#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdint.h>
#include <sys/sendfile.h>
#include "socket.h"

const char *base_dir = "/home";
static void
handle_getattr(int connfd, const char *path)
{
    struct server_response response = {0};

    if (lstat(path, &response.stat) < 0)
    {
        perror("handle_getattr lstat");
        response.error = -errno;
    }

    response.type = GETATTR;
    if (send(connfd, &response, sizeof(struct server_response), 0) == -1)
        perror("handle_getattr send");
}

static void handle_open(int connfd, const char *path, int flags)

{
    struct server_response response = {0};

    if ((response.fh = open(path, flags)) == -1)
    {
        perror("handle_open Open");
        response.error = -errno;
    }
    if (send(connfd, &response, sizeof(struct server_response), 0) == -1)
        perror("handle_open send");
}

static void handle_opendir(int connfd, const char *path)
{
    struct server_response response = {0};
    DIR *dir = opendir(path);
    response.fh = (uint64_t)dir;
    response.type = OPENDIR;
    if (dir == NULL)
    {
        response.error = -errno;
        perror("handle_opendir Opendir");
    }

    if (send(connfd, &response, sizeof(struct server_response), 0) == -1)
        perror("handle_opendir send");
}

static void handle_readdir(int connfd, const char *path, uint64_t fh, int flags)
{
    DIR *dir = (DIR *)fh;
    struct dirent *de;
    struct server_response response;
    size_t count = 0;
    if (!dir)
    {
        strcpy(response.path, "Error");
        if (send(connfd, &response, sizeof(struct server_response), 0) == -1)
            perror("handle_readdir send 1");
        return;
    }
    while ((de = readdir(dir)) != NULL)
    {
        strcpy(response.path, de->d_name);
        response.stat.st_ino = de->d_ino;
        response.stat.st_mode = de->d_type << 12;
        response.size = count;
        if (send(connfd, &response, sizeof(struct server_response), 0) == -1)
            perror("handle_readdir send 2");
    }
    strcpy(response.path, "End");
    if (send(connfd, &response, sizeof(struct server_response), 0) == -1)
        perror("handle_readdir send 3");
}

static void handle_readlink(int connfd, const char *path, size_t size)
{
    char buff[size];
    int ret = readlink(path, buff, size - 1);
    buff[ret] = '\0';
    if (send(connfd, buff, size, 0) == -1)
        perror("handle_readlink send");
}

static void handle_releasedir(int connfd, uint64_t fh)
{
    DIR *dir = (DIR *)(uintptr_t)fh;
    closedir(dir);
}

void send_file(int fd, int sockfd, int size, off_t off)
{
    char buf[size];
    if (pread(fd, buf, size, off) == -1)
        perror("send_file pread");
    int ret = send(sockfd, buf, size, 0);
    printf("Data sent is %d\n", ret);
}
static void handle_read(int connfd, const char *path, uint64_t fh, int flags, size_t size, off_t off)
{
    struct server_response response = {0};
    struct stat statbuf;

    if (stat(path, &statbuf) == -1)
        perror("handle_read stat");

    if (off >= statbuf.st_size)
    {
        response.error = 1;
        if (send(connfd, &response, sizeof(struct server_response), 0) == -1)
            perror("handle_read send 1");
        return;
    }

    size_t data_to_read = statbuf.st_size - off;

    if (size < data_to_read)
    {
        response.size = size;
        if (send(connfd, &response, sizeof(struct server_response), 0) == -1)
            perror("handle_read send 2");
        if (sendfile(connfd, fh, &off, size) == -1)
            perror("handle_read sendfile 1");
        // send_file(fh, connfd, size, off);
    }

    else
    {
        response.size = data_to_read;
        if (send(connfd, &response, sizeof(struct server_response), 0) == -1)
            perror("handle_read send 3");
        if (sendfile(connfd, fh, &off, data_to_read) == -1)
            perror("handle_read sendfile 2");
        // send_file(fh, connfd, data_to_read, off);
    }
}

static void handle_access(int connfd, const char *path, int mask)
{

    struct server_response response;
    response.type = ACCESS;

    if (access(path, mask) == -1)
    {
        response.error = -errno;
        perror("handle_access access");
    }

    if (send(connfd, &response, sizeof(struct server_response), 0) == -1)
        perror("handle_access send");
}

static void handle_release(int connfd, const char *path, uint64_t fh)
{
    close(fh);
}

static void handle_flush(int connfd, const char *path, uint64_t fh)
{

    (void)path;
    struct server_response response = {0};
    if (close(dup(fh)) == -1)
    {
        response.error = -errno;
        perror("handle_flush close");
    }

    if (send(connfd, &response, sizeof(struct server_response), 0) == -1)
        perror("handle_flush send");
}

static void handle_request(int connfd, struct requests *request)

{
    char path[256];
    strcpy(path, request->path);
    sprintf(request->path, "%s%s", base_dir, path);

    switch (request->type)
    {
    case GETATTR:
        handle_getattr(connfd, request->path);
        break;

    case OPEN:
        handle_open(connfd, request->path, request->flags);
        break;

    case OPENDIR:
        handle_opendir(connfd, request->path);
        break;

    case READ:
        handle_read(connfd, request->path, request->fh, request->flags, request->size, request->off);
        break;

    case READ_BUF:
        handle_read(connfd, request->path, request->fh, request->flags, request->size, request->off);
        break;

    case READDIR:
        handle_readdir(connfd, request->path, request->fh, request->flags);
        break;

    case READLINK:
        handle_readlink(connfd, request->path, request->size);
        break;

    case RELEASEDIR:
        handle_releasedir(connfd, request->fh);
        break;

    case ACCESS:
        handle_access(connfd, request->path, request->mask);
        break;

    case RELEASE:
        handle_release(connfd, request->path, request->fh);
        break;

    case FLUSH:
        handle_flush(connfd, request->path, request->fh);
        break;

    default:
        printf("Not implemented %d\n", request->type);
        break;
    }
}

int main(int argc, char const *argv[])
{
    int sockfd = do_server_connect();
    printf("Server started .....\n");
    // ToDo: make it accept multiple requests i.e connections

    struct requests recv_request = {0};
    int connfd;

    while (1)
    {
        connfd = accept(sockfd, NULL, NULL);
        if (connfd < 0)
        {
            perror("accept");
            return -1;
        }

        printf("Accepted the connection\n");
        while (1)
        {
            // sleep(5);
            if (recv(connfd, &recv_request, sizeof(struct requests), 0) <= 0)
            {
                perror("recv");
                close(connfd);
                break;
            }

            handle_request(connfd, &recv_request);
            memset(&recv_request, 0, sizeof(struct requests));
        }
    }

    return 0;
}