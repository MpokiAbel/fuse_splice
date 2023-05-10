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

// static int input_timeout(int filedes, unsigned int seconds)
// {
//     fd_set set;
//     struct timeval timeout;

//     /* Initialize the file descriptor set. */
//     FD_ZERO(&set);
//     FD_SET(filedes, &set);

//     /* Initialize the timeout data structure. */
//     timeout.tv_sec = seconds;
//     timeout.tv_usec = 0;

//     /* select returns 0 if timeout, 1 if input available, -1 if error. */
//     return TEMP_FAILURE_RETRY(select(FD_SETSIZE,
//                                      &set, NULL, NULL,
//                                      &timeout));
// }

int stackfs__getattr(const char *path, struct stat *stat, struct fuse_file_info *fi)
{
    (void)fi;

#ifdef ENABLE_REMOTE
    struct stackfs_data *data = (struct stackfs_data *)fuse_get_context()->private_data;

    struct requests request;
    struct server_response response;

    strcpy(request.path, path);
    request.type = GETATTR;

    if (send(data->sockfd, &request, sizeof(struct requests), 0) == -1)
        return -errno;

    if (recv(data->sockfd, &response, sizeof(struct server_response), 0) == -1)
        return -errno;

    if (response.error < 0)
        return response.error;

    *stat = response.stat;

#else
    if (lstat(path, stat) == -1)
        return -errno;
#endif

    return 0;
}

int stackfs__open(const char *path, struct fuse_file_info *fi)
{
#ifdef ENABLE_REMOTE
    struct stackfs_data *data = (struct stackfs_data *)fuse_get_context()->private_data;
    struct requests request;

    strcpy(request.path, path);
    request.type = OPEN;
    request.flags = fi->flags;

    if (send(data->sockfd, &request, sizeof(struct requests), 0) == -1)
        return -errno;

    struct server_response response;
    if (recv(data->sockfd, &response, sizeof(struct server_response), 0) == -1)
        return -errno;

    if (response.error < 0)
        return response.error;

    fi->fh = response.fh;

#else
    int fd = open(path, fi->flags);

    if (fd == -1)
        return -errno;
    fi->fh = fd;
#endif

    return 0;
}

int stackfs__opendir(const char *path, struct fuse_file_info *fi)
{

// Open directory
#ifdef ENABLE_REMOTE
    struct stackfs_data *data = (struct stackfs_data *)fuse_get_context()->private_data;

    struct requests request;
    strcpy(request.path, path);
    request.type = OPENDIR;

    if (send(data->sockfd, &request, sizeof(struct requests), 0) != sizeof(struct requests))
        return -errno;

    // sendcounter++;
    // printf("Sent in opendir request of type %d counter %d\n", request.type, sendcounter);

    struct server_response response;
    if (recv(data->sockfd, &response, sizeof(struct server_response), 0) == -1)
        return -errno;
    // recvcounter++;
    // printf("Received in opendir response of type %d counter %d\n", response.type, recvcounter);

    if (response.error < 0)
    {
        // printf("Errors in opendir error %d\n", response.error);
        return response.error;
    }
    fi->fh = response.fh;

#else

    DIR *dir = opendir(path);
    uint64_t ptr;
    int ret = 0;

    if (dir == NULL)
    {
        ret = -errno;
        printf("Failed to open directory \"%s\"\n", path);
        return ret;
    }
    ptr = (intptr_t)dir;
    fi->fh = ptr;
#endif

    return 0;
}

int stackfs__readdir(const char *path, void *buff, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{

    (void)offset;

#ifdef ENABLE_REMOTE
    struct stackfs_data *data = (struct stackfs_data *)fuse_get_context()->private_data;

    struct requests request;
    struct server_response response;
    strcpy(request.path, path);
    request.type = READDIR;
    request.fh = fi->fh;
    request.flags = flags;

    if (send(data->sockfd, &request, sizeof(struct requests), 0) != sizeof(struct requests))
        return -errno;

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

#else

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
#endif

    return 0;
}

int stackfs__readlink(const char *path, char *buff, size_t size)
{

#ifdef ENABLE_REMOTE

    struct stackfs_data *data = (struct stackfs_data *)fuse_get_context()->private_data;

    struct requests request;
    strcpy(request.path, path);
    request.type = READLINK;
    request.size = size;

    if (send(data->sockfd, &request, sizeof(struct requests), 0) == -1)
        return -errno;

    if (recv(data->sockfd, buff, size, 0) == -1)
        return errno;

#else

    int ret = readlink(path, buff, size - 1);
    if (ret == -1)
        return -errno;

    buff[ret] = '\0';
#endif
    return 0;
}

int stackfs__releasedir(const char *path, struct fuse_file_info *fi)
{
// int ret[2];
#ifdef ENABLE_REMOTE

    struct stackfs_data *data = (struct stackfs_data *)fuse_get_context()->private_data;

    struct requests request;
    strcpy(request.path, path);
    request.fh = fi->fh;
    request.type = RELEASEDIR;

    if (send(data->sockfd, &request, sizeof(struct requests), 0) == -1)
        return -errno;

#else
    closedir((DIR *)fi->fh);
#endif
    return 0;
}

int stackfs__release(const char *path, struct fuse_file_info *fi)
{

#ifdef ENABLE_REMOTE

    struct stackfs_data *data = (struct stackfs_data *)fuse_get_context()->private_data;

    struct requests request;
    strcpy(request.path, path);
    request.fh = fi->fh;
    request.type = RELEASE;

    if (send(data->sockfd, &request, sizeof(struct requests), 0) == -1)
        return -errno;

#else

    (void)path;
    close(fi->fh);

#endif

    return 0;
}

int stackfs__access(const char *path, int mask)
{
#ifdef ENABLE_REMOTE

    struct stackfs_data *data = (struct stackfs_data *)fuse_get_context()->private_data;

    struct requests request;
    strcpy(request.path, path);

    request.type = ACCESS;
    request.mask = mask;
    if (send(data->sockfd, &request, sizeof(struct requests), 0) == -1)
        return -errno;
    // sendcounter++;
    // printf("Sent in access request of type %d counter %d\n", request.type, sendcounter);

    struct server_response response;
    if (recv(data->sockfd, &response, sizeof(struct server_response), 0) < 0)
        return -errno;
    // recvcounter++;
    // printf("Received in Access response of type %d counter %d\n", response.type, recvcounter);
    if (response.error < 0)
        return response.error;

#else

    if (access(path, mask) == -1)
        return -errno;
#endif

    return 0;
}

int stackfs__read_buf(const char *path, struct fuse_bufvec **bufp,
                      size_t size, off_t off, struct fuse_file_info *fi)
{

#ifdef ENABLE_REMOTE
    struct stackfs_data *data = (struct stackfs_data *)fuse_get_context()->private_data;

    struct requests request;
    memset(&request, 0, sizeof(struct requests));
    strcpy(request.path, path);
    request.type = READ_BUF;
    request.flags = fi->flags;
    request.fh = fi->fh;
    request.size = size;
    request.off = off;

    if (send(data->sockfd, &request, sizeof(struct requests), 0) != sizeof(struct requests))
        return -errno;

    // Receive the header
    struct server_response response;
    if (recv(data->sockfd, &response, sizeof(struct server_response), 0) == -1)
        return -errno;

    // check if there is an error returned by the server based on the data requested
    if (response.error == 1)
        return -EIO;

    printf("Hello I execute this data size is %ld for path %s\n", response.size, path);
    // Setup the buffer
    struct fuse_bufvec *buf = (struct fuse_bufvec *)malloc(sizeof(struct fuse_bufvec));
    *buf = FUSE_BUFVEC_INIT(response.size);
    buf->buf[0].flags |= FUSE_BUF_IS_FD | FUSE_BUF_FD_RETRY;
    buf->buf[0].fd = data->sockfd;
    *bufp = buf;

#else

    struct fuse_bufvec *buf = (struct fuse_bufvec *)malloc(sizeof(struct fuse_bufvec));
    *buf = FUSE_BUFVEC_INIT(size);
    buf->buf[0].pos = off;
    buf->buf[0].flags |= FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK;
    buf->buf[0].fd = fi->fh;
    *bufp = buf;
#endif

    return 0;
}

static void *stackfs__init(struct fuse_conn_info *conn, struct fuse_config *conf)
{
    (void)conf;
    if ((conn->capable & FUSE_CAP_SPLICE_WRITE) && (conn->capable & FUSE_CAP_SPLICE_MOVE) && (conn->capable & FUSE_CAP_SPLICE_READ))
        conn->want |= FUSE_CAP_SPLICE_READ | FUSE_CAP_SPLICE_WRITE | FUSE_CAP_SPLICE_MOVE;

#ifdef ENABLE_REMOTE

    struct stackfs_data *data = (struct stackfs_data *)malloc(sizeof(struct stackfs_data));
    data->sockfd = do_client_connect();
    if (data->sockfd == -1)
        return NULL;

    return (void *)data;
#else
    return NULL;

#endif
}

int stackfs__flush(const char *path, struct fuse_file_info *fi)
{

#ifdef ENABLE_REMOTE
    struct stackfs_data *data = (struct stackfs_data *)fuse_get_context()->private_data;

    struct requests request;
    strcpy(request.path, path);
    request.fh = fi->fh;
    request.type = FLUSH;

    if (send(data->sockfd, &request, sizeof(struct requests), 0) == -1)
        return -errno;

    struct server_response response;

    if (recv(data->sockfd, &response, sizeof(struct server_response), 0) == -1)
        return -errno;

    if (response.error < 0)
        return response.error;

#else
    (void)path;
    if (close(dup(fi->fh)) == -1)
        return -errno;
#endif
    return 0;
}

void stackfs__destroy(void *private_data)
{
#ifdef REMOTE_ENABLE
    struct stackfs_data *data = (struct stackfs_data *)private_data;
    close(data->sockfd);
#endif
}

int stackfs__read(const char *path, char *buf, size_t size, off_t off,
                  struct fuse_file_info *fi)
{

#ifdef ENABLE_REMOTE
    struct stackfs_data *data = (struct stackfs_data *)fuse_get_context()->private_data;

    struct requests request = {0};
    strcpy(request.path, path);
    request.type = READ;
    request.flags = fi->flags;
    request.fh = fi->fh;
    request.size = size;
    request.off = off;

    if (send(data->sockfd, &request, sizeof(struct requests), 0) == -1)
        return -errno;

    struct server_response response;
    if (recv(data->sockfd, &response, sizeof(struct server_response), 0) == -1)
        return -errno;

    if (response.error == -1)
        return -EIO;

    size_t res;
    int temp_size = 4096;
    char temp[temp_size];
    long int remain = response.size;
    long int data_read = 0;
    while (remain > 0)
    {
        res = recv(data->sockfd, temp, temp_size, 0);
        if (res <= 0)
            break;

        memcpy(buf + data_read, temp, res);
        remain -= res;
        data_read += res;
    }

    return response.size;
#else
    int res;

    (void)path;
    res = pread(fi->fh, buf, size, off);
    if (res == -1)
        res = -errno;
    return size;
#endif
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
    .flush = stackfs__flush,
    .destroy = stackfs__destroy,
    // .read = stackfs__read,

};

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &stackfs__op, NULL);
}
