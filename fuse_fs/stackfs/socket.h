#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define ENABLE_REMOTE 1
#define SERVER_PORT 9000

// Request Sent
#define GETATTR 1
#define OPEN 2
#define OPENDIR 3
#define READ 4
#define READDIR 5
#define READLINK 6
#define RELEASEDIR 7
#define READ_BUF 8
#define RELEASE 9
#define ACCESS 10
#define FLUSH 11

struct requests
{
    char path[256];
    int type;
    int flags;
    uint64_t fh;
    size_t size;
    off_t off;
    int mask;
};

struct server_response
{
    int error;
    char path[256];
    size_t size;
    struct stat stat;
    uint64_t fh;
    int type;
};

int do_client_connect();

int do_server_connect();
