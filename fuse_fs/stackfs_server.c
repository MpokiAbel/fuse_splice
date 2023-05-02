#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdint.h>
#include "socket.h"

const char *base_dir = "";
static void
handle_getattr(int connfd, const char *path)
{
    struct server_response stbuf = {0};

    if (lstat(path, &stbuf.stat) < 0)
        stbuf.error = -errno;

    send(connfd, &stbuf, sizeof(stbuf), 0);
}

static void handle_open(int connfd, const char *path, int flags)

{
    struct server_response response = {0};

    if ((response.fh = open(path, flags)) == -1)
        response.error = -errno;
    send(connfd, &response, sizeof(struct server_response), 0);
}

static void handle_opendir(int connfd, const char *path)
{
    struct server_response response = {0};
    DIR *dir = opendir(path);
    response.fh = (uint64_t)dir;
    if (dir == NULL)
        response.error = -errno;

    send(connfd, &response, sizeof(struct server_response), 0);
}

static void handle_readdir(int connfd, const char *path, uint64_t fh, int flags)
{
    DIR *dir = (DIR *)fh;
    struct dirent *de;
    struct server_response respose;
    size_t count = 0;
    if (!dir)
    {
        strcpy(respose.path, "Error");
        send(connfd, &respose, sizeof(struct server_response), 0);
        return;
    }
    while ((de = readdir(dir)) != NULL)
    {
        strcpy(respose.path, de->d_name);
        respose.stat.st_ino = de->d_ino;
        respose.stat.st_mode = de->d_type << 12;
        respose.size = count;
        send(connfd, &respose, sizeof(struct server_response), 0);
    }
    strcpy(respose.path, "End");
    send(connfd, &respose, sizeof(struct server_response), 0);
}

static void handle_readlink(int connfd, const char *path, size_t size)
{
    char buff[size];
    int ret = readlink(path, buff, size - 1);
    buff[ret] = '\0';
    send(connfd, buff, size, 0);
}

static void handle_releasedir(int connfd, uint64_t fh)
{
    DIR *dir = (DIR *)(uintptr_t)fh;
    closedir(dir);
}

static void handle_read(int connfd, const char *path, uint64_t fh, int flags, size_t size)
{
    int pipefd[2];
    size_t sp;

    // create a pipe and check for errors
    pipe(pipefd);
    sp = splice(fh, NULL, pipefd[1], NULL, size, SPLICE_F_MOVE);
    sp = splice(pipefd[0], NULL, connfd, NULL, sp, SPLICE_F_MOVE);
    printf("Size of data sent %ld\n", sp);
    close(pipefd[1]);
    close(pipefd[0]);
}

static void handle_access(int connfd, const char *path, int mask)
{

    struct server_response response;
    if (access(path, mask) == -1)
        response.error = -errno;

    send(connfd, &response, sizeof(struct server_response), 0);
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
        response.error = -errno;

    send(connfd, &response, sizeof(struct server_response), 0);
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

        // case READ:
        //     handle_read(connfd, request->path, request->fh, request->flags, request->size);
        //     break;

    case READ_BUF:
        handle_read(connfd, request->path, request->fh, request->flags, request->size);
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
    int n;

    while (1)
    {
        int connfd = accept(sockfd, NULL, NULL);
        if (connfd < 0)
        {
            perror("accept");
            return -1;
        }

        printf("Accepted the connection\n");
        while (1)
        {
            n = recv(connfd, &recv_request, sizeof(struct requests), 0);
            // printf("Request %d received for path %s file descriptor %ld\n", recv_request.type, recv_request.path, recv_request.fh);

            // sleep(5);
            if (n <= 0)
            {
                perror("recv connefd closed goes to accept");
                close(connfd);
                break;
            }

            handle_request(connfd, &recv_request);
            memset(&recv_request, 0, sizeof(struct requests));
        }
    }

    return 0;
}