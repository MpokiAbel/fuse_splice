#include "socket.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#define bufer_size 4096

int main(int argc, char const *argv[])
{
    int sockfd = do_client_connect();
    char buf[bufer_size];

    send(sockfd, buf, bufer_size, 0);

    size_t size;
    recv(sockfd, &size, sizeof(size), 0);

    long int remain = size;
    char *data = (char *)malloc(size * sizeof(char));
    int total_bytes_read = 0;
    size_t n;

    while (remain > 0)
    {
        printf("Total bytes read %d\n", total_bytes_read);
        n = recv(sockfd, buf, bufer_size, 0);
        if (n <= 0)
        {
            break;
        }

        memcpy(data + total_bytes_read, buf, n);
        // printf("%s",buf);
        remain -= n;
        total_bytes_read += n;
        // break;
    }

    int fd;
    char *filename = "example.pdf";
    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    write(fd, data, size);
    close(fd);

    return 0;
}
