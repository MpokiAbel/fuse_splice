#define FUSE_USE_VERSION 31
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <fuse.h>
#include "socket.h"

int stackfs__getattr(const char *path, struct stat *stat, struct fuse_file_info *fi)
{
    int res;
    (void)fi;

    if (ENABLE_REMOTE)
    {
        int sockfd = do_client_connect();

        struct requests request;
        struct server_response stat_res;

        strcpy(request.path, path);
        request.type = GETATTR;

        if (send(sockfd, &request, sizeof(struct requests), 0) != sizeof(struct requests))
        {
            perror("send");
            close(sockfd);
            return -errno;
        }
        // Receive the response from the server
        recv(sockfd, &stat_res, sizeof(struct server_response), 0);

        *stat = stat_res.stat;
        close(sockfd);
    }
    else
    {
        res = lstat(path, stat);
        if (res == -1)
            return -errno;
    }

    return 0;
}

int stackfs__open(const char *path, struct fuse_file_info *fi)
{
    int fd;
    if (ENABLE_REMOTE)
    {
        int sockfd = do_client_connect();

        struct requests request;
        int res[2];

        strcpy(request.path, path);
        request.type = OPEN;
        request.flags = fi->flags;

        if (send(sockfd, &request, sizeof(struct requests), 0) != sizeof(struct requests))
        {
            perror("send");
            close(sockfd);
            return -errno;
        }

        recv(sockfd, res, sizeof(res), 0);
        if (res[0] == 0)
            printf("There is an error\n");
        else
            fi->fh = res[1];

        close(sockfd);
    }
    else
    {

        fd = open(path, fi->flags);

        if (fd == -1)
            return -errno;
        fi->fh = fd;
    }

    return 0;
}

int stackfs__opendir(const char *path, struct fuse_file_info *fi)
{

    uint64_t ptr;
    int ret = 0;

    // Open directory
    if (ENABLE_REMOTE)
    {
        int sockfd = do_client_connect();
        struct requests request;
        strcpy(request.path, path);
        request.type = OPENDIR;

        if (send(sockfd, &request, sizeof(struct requests), 0) != sizeof(struct requests))
        {
            perror("send");
            return -errno;
        }

        int rec = recv(sockfd, &ptr, sizeof(uint64_t), 0);
        if (rec != sizeof(uint64_t))
        {
            ret = -errno;
            close(sockfd);
            return ret;
        }
        // printf("Received!! %ld\n", ptr);
        close(sockfd);
    }
    else
    {
        DIR *dir = opendir(path);
        if (dir == NULL)
        {
            ret = -errno;
            printf("Failed to open directory \"%s\"\n", path);
            return ret;
        }
        ptr = (intptr_t)dir;
    }

    // Store directory handle in fuse_file_info
    fi->fh = ptr;
    return 0;
}

int stackfs__read(const char *path, char *buff, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int res = 0;

    if (ENABLE_REMOTE)
    {
        int sockfd = do_client_connect();
        if (sockfd == -1)
        {
            return -errno;
        }

        struct requests request;
        memset(&request, 0, sizeof(struct requests));
        strcpy(request.path, path);
        request.type = READ;
        request.flags = fi->flags;
        request.fh = fi->fh;
        request.size = size;

        if (send(sockfd, &request, sizeof(struct requests), 0) != sizeof(struct requests))
        {
            perror("send");
            close(sockfd);
            return -errno;
        }

        int res = recv(sockfd, buff, size, 0);
        if (res == -1)
        {
            perror("recv");
            close(sockfd);
            return -errno;
        }
        else if (res < size)
        {
            fprintf(stderr, "Received less data than requested\n");
        }

        close(sockfd);
        return res;
    }

    return res;
    /*Create the pipe end points i.e 0 - read and 1 - write int pipefd[2];
    pipe(pipefd);

    ssize_t nwritten = splice(fi->fh, offset, pipefd[1], NULL, size, 1);
    if (nwritten == -1)
    {
        res = -errno;
    }

    ssize_t nread = splice(pipefd[0], NULL, STDOUT_FILENO, NULL, nwritten, 1);
    if (nread == -1)
    {
        res = -errno;
    }

    return 0;

    // char full_path[PATH_MAX];

    sprintf(full_path, "%s%s", base_dir, path);
    // printf("read: %s\n", path);

    res = pread(fi->fh, buff, size, offset);
    // if (res == -1)
    //     res = -errno;

    return res;

*/
}

int stackfs__readdir(const char *path, void *buff, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{

    (void)offset;
    (void)fi;

    // char full_path[PATH_MAX];

    // sprintf(full_path, "%s%s", base_dir, path);
    // printf("readdir: %s\n", path);
    if (ENABLE_REMOTE)
    {
        int sockfd = do_client_connect();

        struct requests request;
        struct server_response response;
        strcpy(request.path, path);
        request.type = READDIR;

        if (send(sockfd, &request, sizeof(struct requests), 0) != sizeof(struct requests))
        {
            perror("send");
            close(sockfd);
            return -errno;
        }

        int n = 0, count = 1;

        while (n < count)
        {
            recv(sockfd, &response, sizeof(response), 0);
            filler(buff, response.path, &response.stat, 0, FUSE_FILL_DIR_PLUS);

            n++;
            if (n == 1)
            {
                count = response.size - 1;
            }
        }

        close(sockfd);
    }
    else
    {
        DIR *dp;
        struct dirent *de;
        dp = opendir(path);
        if (dp == NULL)
            return -errno;

        while ((de = readdir(dp)) != NULL)
        {
            struct stat st;

            memset(&st, 0, sizeof(st));
            st.st_ino = de->d_ino;
            st.st_mode = de->d_type << 12;

            if (filler(buff, de->d_name, &st, 0, FUSE_FILL_DIR_PLUS))
                break;
        }
    }

    return 0;
}

int stackfs__readlink(const char *path, char *buff, size_t size)
{
    int ret;

    if (ENABLE_REMOTE)
    {
        int sockfd = do_client_connect();
        struct requests request;
        strcpy(request.path, path);
        request.type = READLINK;
        request.size = size;

        if (send(sockfd, &request, sizeof(struct requests), 0) != sizeof(struct requests))
        {
            perror("send");
            close(sockfd);
            return -errno;
        }

        recv(sockfd, buff, size, 0);
        close(sockfd);
    }
    else
    {
        ret = readlink(path, buff, size - 1);
        if (ret == -1)
            return -errno;

        buff[ret] = '\0';
    }
    return 0;
}

int stackfs__releasedir(const char *path, struct fuse_file_info *fi)
{
    int ret;
    if (ENABLE_REMOTE)
    {
        int sockfd = do_client_connect();

        struct requests request;
        strcpy(request.path, path);
        request.fh = fi->fh;
        request.type = RELEASEDIR;

        if (send(sockfd, &request, sizeof(struct requests), 0) != sizeof(struct requests))
        {
            perror("send");
            close(sockfd);
            return -errno;
        }

        recv(sockfd, &ret, sizeof(ret), 0);
        close(sockfd);
    }
    else
    {
        ret = closedir((DIR *)fi->fh);
    }
    return 0;
}

int stackfs__read_buf(const char *path, struct fuse_bufvec **bufp,
                      size_t size, off_t off, struct fuse_file_info *fi)
{
    struct fuse_bufvec *buf = (struct fuse_bufvec *)malloc(sizeof(struct fuse_bufvec));
    *buf = FUSE_BUFVEC_INIT(size);
    buf->buf[0].pos = off;
    buf->buf[0].flags |= FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK;
    buf->buf[0].fd = fi->fh;
    *bufp = buf;

    return 0;
}

static void *stackfs__init(struct fuse_conn_info *conn, struct fuse_config *conf)
{
    (void)conf;
    if ((conn->capable & FUSE_CAP_SPLICE_WRITE) && (conn->capable & FUSE_CAP_SPLICE_MOVE) && (conn->capable & FUSE_CAP_SPLICE_READ))
        conn->want |= FUSE_CAP_SPLICE_READ | FUSE_CAP_SPLICE_WRITE | FUSE_CAP_SPLICE_MOVE;
    return NULL;
}

static struct fuse_operations stackfs__op = {
    .init = stackfs__init,
    .getattr = stackfs__getattr,
    .opendir = stackfs__opendir,
    .open = stackfs__open,
    .read = stackfs__read,
    .readdir = stackfs__readdir,
    .readlink = stackfs__readlink,
    .releasedir = stackfs__releasedir,
    .read_buf = stackfs__read_buf,

};

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &stackfs__op, NULL);
}
