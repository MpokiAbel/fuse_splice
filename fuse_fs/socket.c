#include "socket.h"
#include <string.h>
#include <stdio.h>

struct socket
{
    int fd;
    struct sockaddr_in serv_addr;
};

static struct socket create_socket()
{
    struct socket sock;

    sock.fd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&sock.serv_addr, 0, sizeof(sock.serv_addr));
    sock.serv_addr.sin_family = AF_INET;
    sock.serv_addr.sin_addr.s_addr = INADDR_ANY;
    sock.serv_addr.sin_port = htons(SERVER_PORT);

    return sock;
}

int do_client_connect()
{
    struct socket sock = create_socket();
    if (connect(sock.fd, (struct sockaddr *)&sock.serv_addr, sizeof(sock.serv_addr)) < 0)
    {
        perror("connect");
        return -1;
    }

    return sock.fd;
}

int do_server_connect()
{
    struct socket sock = create_socket();

    // int opt = 1;
    // if (setsockopt(sock.fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    // {
    //     perror("setsockopt");
    //     return -1;
    // }

    // Bind the socket
    if (bind(sock.fd, (struct sockaddr *)&sock.serv_addr, sizeof(sock.serv_addr)) < 0)
    {
        perror("bind");
        return -1;
    }

    // Listen to connection
    listen(sock.fd, 5);

    return sock.fd;
}
