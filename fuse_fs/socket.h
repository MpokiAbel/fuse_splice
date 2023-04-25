#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define ENABLE_REMOTE 1
#define SERVER_PORT 9001

struct requests
{
    char path[256];
    int type;
    int flags;
    uint64_t fh;
    // Used for simlink
    size_t size;
};

struct server_response
{
    int bool;
    char path[256];
    size_t size;
    struct stat stat;
};

int do_client_connect();

int do_server_connect();
