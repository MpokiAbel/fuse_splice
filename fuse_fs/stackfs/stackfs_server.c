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

const char *base_dir = "/home/mpokiabel/Documents/fuse/root";
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
    response.type = OPEN;
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
    struct server_response response = {0};
    response.type = READDIR;
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

static int send_file(int fd, int sockfd, off_t off, struct server_response *response)
{

    int error;
    int buf_size = response->size + sizeof(struct server_response);
    response->type = READ;
    char *buf = (char *)malloc(buf_size);
    bzero(buf, buf_size);

    error = pread(fd, buf + sizeof(struct server_response), response->size, off);

    response->error = error > 0 ? 0 : error;
    response->size = error < 0 ? 0 : error;

    memcpy(buf, response, sizeof(struct server_response));
    // Send only when there is something to send
    if (error)
        error = send(sockfd, buf, buf_size, 0);

    if (error == -1)
        perror("send_file send");

    free(buf);
    return error;
}

static void handle_read(int connfd, const char *path, uint64_t fh, int flags, size_t size, off_t off)
{
    struct server_response response = {0};

    response.size = size;
    strcpy(response.path, path);
    send_file(fh, connfd, off, &response);

    printf("offset %ld, size %ld, data to read %ld error %d\n", off, size, response.size, errno);
}

static void handle_access(int connfd, const char *path, int mask)
{

    struct server_response response = {0};
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
    response.type = FLUSH;
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