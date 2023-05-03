#include "socket.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char const *argv[])
{
    int sockfd = do_client_connect();
    char buf[1024];

    send(sockfd, buf, 1024, 0);
    int res;
    int counter = 0;
    while (1)
    {
        res = recv(sockfd, buf, 1024, 0);
        if (res != 1024)
        {
            printf("%s\n", buf);
            printf("Total size of file in bytes is %d\n", res + counter * 1024);
            break;
        }
        counter++;
        printf("%s", buf);
    }
}
