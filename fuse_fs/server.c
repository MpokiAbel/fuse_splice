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
    printf("file size %ld", statf.st_size);
    sendfile(sockfd, fd, NULL, statf.st_size);
    printf("Data Sent\n");
    char buf[] = {"End"};
    send(sockfd, buf, sizeof(buf), 0);
    printf("Data Sent\n");
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
            // printf("Request %d received for path %s file descriptor %ld\n", recv_request.type, recv_request.path, recv_request.fh);

            // sleep(5);
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
