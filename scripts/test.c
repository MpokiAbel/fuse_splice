#define _GNU_SOURCE
#include <stdio.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <linux/stat.h>

struct spliceM
{
    unsigned long len;
    unsigned long metadata_len;
    unsigned long payload_len;
    unsigned long footer_len;
    unsigned int flags;
};

int main()
{
    int source, destination;

    if ((source = open("file_1.out", O_RDONLY)) == -1)
    {
        perror("Open file_1");
        return -1;
    }

    if ((destination = open("copied.out", O_WRONLY)) == -1)
    {
        perror("Open copied");
        return -1;
    }

    // Fill in the file data structure
    struct spliceM data;
    data.len = 1200;
    data.metadata_len = 52;
    data.payload_len = 202;
    data.footer_len = 22;
    data.flags = SPLICE_F_MOVE;

    int pipefd[2];

    if (pipe(pipefd) == -1)
        return -1;

    char *buf = (char *)malloc((data.footer_len + data.metadata_len) * sizeof(char));

    size_t amma = syscall(452, source, NULL, pipefd[1], NULL, &data, buf);
    
    if (amma > 0)
        amma = splice(pipefd[0], NULL, destination, NULL, amma, SPLICE_F_MOVE);

    printf("%s\n", buf);
    printf("%ld\n", amma);

    close(pipefd[0]);           
    close(pipefd[1]);
    close(source);
    close(destination);
    return 0;
}
