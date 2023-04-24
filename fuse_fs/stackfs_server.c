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

const char *base_dir = "./root";
static void
handle_getattr(int connfd, const char *path)
{
    struct server_response stbuf;

    int res = lstat(path, &stbuf.stat);

    // printf("getattr: %s\n", path);
    if (res < 0)
    {
        printf("handle_getatt failed\n");
        stbuf.bool = 0;
    }
    else
        stbuf.bool = 1;

    send(connfd, &stbuf, sizeof(stbuf), 0);
}

static void handle_open(int connfd, const char *path, int flags)

{
    int res[2] = {0};

    res[1] = open(path, flags);
    printf("open: %s\n", path);
    if (res[1] >= 0)
        res[0] = 1;

    send(connfd, res, sizeof(res), 0);
}

static void handle_opendir(int connfd, const char *path)
{
    DIR *dir = opendir(path);
    uint64_t ptr = (uint64_t)dir;
    // printf("opendir: %s: %ld\n", path, ptr);
    if (dir == NULL)
    {
        int i = 0;
        printf("Opendir failed on the server");
        send(connfd, &i, sizeof(int), 0);
    }

    else
        send(connfd, &ptr, sizeof(intptr_t), 0);
}

static void handle_readdir(int connfd, const char *path)
{
    DIR *dir;
    struct dirent *de;
    struct server_response res;
    size_t count = 0;

    // printf("readdir: %s\n", path);
    dir = opendir(path);
    if (dir == NULL)
    {
        perror("opendir");
        return;
    }

    while ((de = readdir(dir)) != NULL)
    {
        count++;
    }

    dir = opendir(path);
    if (!dir)
    {
        perror("opendir");
        return;
    }
    while ((de = readdir(dir)) != NULL)
    {
        strcpy(res.path, de->d_name);
        res.stat.st_ino = de->d_ino;
        res.stat.st_mode = de->d_type << 12;
        res.size = count;
        send(connfd, &res, sizeof(res), 0);
    }

    closedir(dir);
}

static void handle_readlink(int connfd, const char *path, size_t size)
{
    char buff[size];

    int ret = readlink(path, buff, size - 1);

    buff[ret] = '\0';
    // printf("readlink: %s\n", path);

    send(connfd, buff, size, 0);
}

static void handle_releasedir(int connfd, uint64_t fh)
{

    int ret = closedir((DIR *)fh);
    printf("releasedir: \n");

    send(connfd, &ret, sizeof(ret), 0);
}

static void handle_read(int connfd, const char *path, size_t size)
{

    int pipefd[2];
    pipe(pipefd);

    int fd = open(path, O_RDONLY);

    int sp = splice(fd, NULL, pipefd[1], NULL, size, SPLICE_F_MOVE);

    sp = splice(pipefd[0], NULL, connfd, NULL, sp, SPLICE_F_MOVE);
}

static void handle_request(int connfd, struct requests *request)
{
    char path[256];
    strcpy(path, request->path);
    sprintf(request->path, "%s%s", base_dir, path);

    switch (request->type)
    {
    case 1:
        handle_getattr(connfd, request->path);
        break;

    case 2:
        handle_open(connfd, request->path, request->flags);
        break;

    case 3:
        handle_opendir(connfd, request->path);
        break;

    case 4:
        handle_read(connfd,request->path,request->size);
        break;

    case 5:
        handle_readdir(connfd, request->path);
        break;

    case 6:
        handle_readlink(connfd, request->path, request->size);
        break;

    case 7:
        handle_releasedir(connfd, request->fh);
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

    printf("Entering the while\n");
    while (1)
    {
        int connfd = accept(sockfd, NULL, NULL);
        if (connfd < 0)
        {
            perror("accept");
            return -1;
        }
        while (1)
        {
            n = recv(connfd, &recv_request, sizeof(struct requests), 0);
            if (n <= 0)
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