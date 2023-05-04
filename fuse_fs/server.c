#include "socket.h"
#include <fcntl.h>
#include <sys/sendfile.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

void handle_request(int sockfd)
{

    int fd = open("../resources/bmc.pdf", O_RDONLY);

    struct stat statf;
    stat("../resources/bmc.pdf", &statf);
    printf("file size %ld\n", statf.st_size);

    size_t size = statf.st_size;
    send(sockfd, &size, sizeof(size), 0);

    printf("Sent %ld\n", size);
    sendfile(sockfd, fd, NULL, size);
};

int main(int argc, char const *argv[])
{
    int sockfd = do_server_connect();
    char buf[1024];
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
            n = recv(connfd, &buf, sizeof(buf), 0);
            if (n <= 0)
            {
                perror("recv connefd closed goes to accept");
                close(connfd);
                break;
            }

            handle_request(connfd);
        }
    }

    return 0;
}
