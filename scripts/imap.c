#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 1024

int main()
{
    SSL_CTX *ctx;
    SSL *ssl;
    int sock;
    struct sockaddr_in server;
    char server_address[] = "194.254.242.54"; // webmail.grenoble-inp.org
    int server_port = 993;
    char buffer[MAX_BUFFER_SIZE] = {0};

    // Initialize the SSL library
    SSL_library_init();
    ERR_load_crypto_strings();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    // Create an SSL context
    ctx = SSL_CTX_new(TLSv1_2_client_method());
    if (ctx == NULL)
    {
        fprintf(stderr, "Failed to create SSL context\n");
        return 1;
    }

    // Create a socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("Socket creation failed");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);

    // Convert server address from string to network address structure
    if (inet_pton(AF_INET, server_address, &(server.sin_addr)) <= 0)
    {
        perror("Invalid address / Address not supported");
        return 1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Connection failed");
        return 1;
    }

    // Create an SSL object and attach it to the socket
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);

    // Perform the SSL handshake
    if (SSL_connect(ssl) != 1)
    {
        fprintf(stderr, "SSL handshake failed\n");
        return 1;
    }

    // Receive the server's welcome message
    memset(buffer, 0, sizeof(buffer));
    SSL_read(ssl, buffer, sizeof(buffer));
    printf("%s\n", buffer);

    // Send IMAP commands
    char login_command[] = "a1 LOGIN mpoki.mwaisela@grenoble-inp.org Cl@ssof21_Udsm\r\n";
    SSL_write(ssl, login_command, strlen(login_command));
    memset(buffer, 0, sizeof(buffer));
    SSL_read(ssl, buffer, sizeof(buffer));
    printf("%s\n", buffer);

    char list_command[] = "a2 LIST \"\" \"*\"\r\n";
    SSL_write(ssl, list_command, strlen(list_command));
    memset(buffer, 0, sizeof(buffer));
    SSL_read(ssl, buffer, sizeof(buffer));
    printf("%s\n", buffer);

    char select_command[] = "a3 SELECT INBOX\r\n";
    SSL_write(ssl, select_command, strlen(select_command));
    memset(buffer, 0, sizeof(buffer));
    SSL_read(ssl, buffer, sizeof(buffer));
    printf("%s\n", buffer);

    char fetch_command[] = "a4 FETCH 1 BODY\r\n";
    SSL_write(ssl, fetch_command, strlen(fetch_command));
    memset(buffer, 0, sizeof(buffer));
    SSL_read(ssl, buffer, sizeof(buffer));
    printf("%s\n", buffer);

    // Close the SSL connection and free resources
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sock);
    SSL_CTX_free(ctx);

    return 0;
}
