#define FUSE_USE_VERSION 31
#define _GNU_SOURCE
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <fuse.h>
#include <sys/types.h>
#include <sys/time.h>
#include "socket.h"

struct stackfs_data
{
    int sockfd;
};

static int input_timeout(int filedes, unsigned int seconds)
{
    fd_set set;
    struct timeval timeout;

    /* Initialize the file descriptor set. */
    FD_ZERO(&set);
    FD_SET(filedes, &set);

    /* Initialize the timeout data structure. */
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;

    /* select returns 0 if timeout, 1 if input available, -1 if error. */
    return TEMP_FAILURE_RETRY(select(FD_SETSIZE,
                                     &set, NULL, NULL,
                                     &timeout));
}

int stackfs__getattr(const char *path, struct stat *stat, struct fuse_file_info *fi)
{
    int res;
    (void)fi;

    if (ENABLE_REMOTE)
    {
        struct stackfs_data *data = (struct stackfs_data *)fuse_get_context()->private_data;

        struct requests request;
        struct server_response stat_res;

        strcpy(request.path, path);
        request.type = GETATTR;

        if (send(data->sockfd, &request, sizeof(struct requests), 0) != sizeof(struct requests))
        {
            perror("send");

            return -errno;
        }
        // Receive the response from the server
        recv(data->sockfd, &stat_res, sizeof(struct server_response), 0);

        *stat = stat_res.stat;

        // printf("Request %d received for path %s file descriptor %ld\n", request.type, request.path, request.fh);
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
        struct stackfs_data *data = (struct stackfs_data *)fuse_get_context()->private_data;

        struct requests request;
        int res[2];

        strcpy(request.path, path);
        request.type = OPEN;
        request.flags = fi->flags;

        if (send(data->sockfd, &request, sizeof(struct requests), 0) != sizeof(struct requests))
        {
            perror("send");

            return -errno;
        }

        recv(data->sockfd, res, sizeof(res), 0);
        if (res[0] == 0)
            printf("There is an error\n");
        else
            fi->fh = res[1];

        printf("Request %d received for path %s file descriptor %ld\n", request.type, request.path, fi->fh);
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
        struct stackfs_data *data = (struct stackfs_data *)fuse_get_context()->private_data;

        struct requests request;
        strcpy(request.path, path);
        request.type = OPENDIR;

        if (send(data->sockfd, &request, sizeof(struct requests), 0) != sizeof(struct requests))
        {
            perror("send");
            return -errno;
        }
        struct server_response response;
        recv(data->sockfd, &response, sizeof(struct server_response), 0);
        if (response.error < 0)
        {
            printf("Errors in opendir error %d\n", response.error);
            return response.error;
        }
        fi->fh = response.fh;
        // printf("Opendir %s with dir %ld with size %d\n", path, response.fh, size);
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
        fi->fh = ptr;
    }

    return 0;
}

int stackfs__readdir(const char *path, void *buff, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{

    (void)offset;

    if (ENABLE_REMOTE)
    {
        struct stackfs_data *data = (struct stackfs_data *)fuse_get_context()->private_data;

        struct requests request;
        struct server_response response;
        strcpy(request.path, path);
        request.type = READDIR;
        request.fh = fi->fh;
        request.flags = flags;

        if (send(data->sockfd, &request, sizeof(struct requests), 0) != sizeof(struct requests))
        {
            perror("send");
            return -errno;
        }

        while (1)
        {
            if (recv(data->sockfd, &response, sizeof(struct server_response), 0) < 0)
                return -errno;
            else if (strcmp(response.path, "End") == 0)
                break;
            else if (strcmp(response.path, "Error") == 0)
                return -EBADF;
            filler(buff, response.path, &response.stat, 0, FUSE_FILL_DIR_PLUS);
        }
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
        struct stackfs_data *data = (struct stackfs_data *)fuse_get_context()->private_data;

        struct requests request;
        strcpy(request.path, path);
        request.type = READLINK;
        request.size = size;

        if (send(data->sockfd, &request, sizeof(struct requests), 0) != sizeof(struct requests))
        {
            perror("send");
            return -errno;
        }

        recv(data->sockfd, buff, size, 0);
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
    // int ret[2];
    if (ENABLE_REMOTE && path != NULL)
    {
        struct stackfs_data *data = (struct stackfs_data *)fuse_get_context()->private_data;

        struct requests request;
        strcpy(request.path, path);
        request.fh = fi->fh;
        request.type = RELEASEDIR;

        if (send(data->sockfd, &request, sizeof(struct requests), 0) != sizeof(struct requests))
        {
            perror("send");
            return -errno;
        }
    }
    else
        closedir((DIR *)fi->fh);

    return 0;
}

int stackfs__release(const char *path, struct fuse_file_info *fi)
{

    if (ENABLE_REMOTE)
    {

        struct stackfs_data *data = (struct stackfs_data *)fuse_get_context()->private_data;

        struct requests request;
        strcpy(request.path, path);
        request.fh = fi->fh;
        request.type = RELEASE;

        if (send(data->sockfd, &request, sizeof(struct requests), 0) != sizeof(struct requests))
        {
            perror("send");
            return -errno;
        }
    }
    else
    {
        (void)path;
        close(fi->fh);
    }

    return 0;
}

int stackfs__access(const char *path, int mask)
{
    if (ENABLE_REMOTE && path != NULL)
    {

        struct stackfs_data *data = (struct stackfs_data *)fuse_get_context()->private_data;

        struct requests request;
        strcpy(request.path, path);

        request.type = ACCESS;
        request.mask = mask;

        if (send(data->sockfd, &request, sizeof(struct requests), 0) != sizeof(struct requests))
            return -errno;

        struct server_response response;
        if (recv(data->sockfd, &response, sizeof(struct server_response), 0) < 0)
            return -errno;

        if (response.error < 0)
            return response.error;
    }
    else
    {
        if (access(path, mask) == -1)
            return -errno;
    }

    return 0;
}

int stackfs__read_buf(const char *path, struct fuse_bufvec **bufp,
                      size_t size, off_t off, struct fuse_file_info *fi)
{

    if (ENABLE_REMOTE)
    {
        struct stackfs_data *data = (struct stackfs_data *)fuse_get_context()->private_data;

        struct requests request;
        memset(&request, 0, sizeof(struct requests));
        strcpy(request.path, path);
        request.type = READ_BUF;
        request.flags = fi->flags;
        request.fh = fi->fh;
        request.size = size;

        if (send(data->sockfd, &request, sizeof(struct requests), 0) != sizeof(struct requests))
        {
            perror("send");
            return -errno;
        }

        /* Wait until data becomes available in the socket
            Todo: Errors are not handled yet
        */
        input_timeout(data->sockfd, 5);

        // Get the size of data received in the socket
        size_t socketsize;
        ioctl(data->sockfd, FIONREAD, &socketsize);

        // Setup the buffer
        struct fuse_bufvec *buf = (struct fuse_bufvec *)malloc(sizeof(struct fuse_bufvec));
        *buf = FUSE_BUFVEC_INIT(socketsize);
        buf->buf[0].flags |= FUSE_BUF_IS_FD;
        buf->buf[0].fd = data->sockfd;
        *bufp = buf;
        // printf("In file system %d and size:%ld and socketsize %ld\n", data->sockfd, size, socketsize);
    }

    else
    {
        struct fuse_bufvec *buf = (struct fuse_bufvec *)malloc(sizeof(struct fuse_bufvec));
        *buf = FUSE_BUFVEC_INIT(size);
        buf->buf[0].pos = off;
        buf->buf[0].flags |= FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK;
        buf->buf[0].fd = fi->fh;
        *bufp = buf;
    }

    return 0;
}

static void *stackfs__init(struct fuse_conn_info *conn, struct fuse_config *conf)
{
    (void)conf;
    if ((conn->capable & FUSE_CAP_SPLICE_WRITE) && (conn->capable & FUSE_CAP_SPLICE_MOVE) && (conn->capable & FUSE_CAP_SPLICE_READ))
    {
        conn->want |= FUSE_CAP_SPLICE_READ | FUSE_CAP_SPLICE_WRITE | FUSE_CAP_SPLICE_MOVE;
    }

    struct stackfs_data *data = (struct stackfs_data *)malloc(sizeof(struct stackfs_data));
    data->sockfd = do_client_connect();
    if (data->sockfd == -1)
    {
        return NULL;
    }
    return (void *)data;
}

void stackfs__destroy(void *private_data)
{

    struct stackfs_data *data = (struct stackfs_data *)private_data;
    close(data->sockfd);
}

static struct fuse_operations stackfs__op = {
    .init = stackfs__init,
    .access = stackfs__access,
    .getattr = stackfs__getattr,
    .opendir = stackfs__opendir,
    .open = stackfs__open,
    .readdir = stackfs__readdir,
    .readlink = stackfs__readlink,
    .releasedir = stackfs__releasedir,
    .release = stackfs__release,
    .read_buf = stackfs__read_buf,
    .destroy = stackfs__destroy,

};

int main(int argc, char *argv[])
{

    return fuse_main(argc, argv, &stackfs__op, NULL);
}
