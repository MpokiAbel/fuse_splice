#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

struct file_data
{
    int metadata;
    int payload;
    int footer;
};

int main(int argc, char const *argv[])
{
    // Open the files
    int source, destination, temp;
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
    struct file_data data;
    data.metadata = 52;
    data.payload = 202;
    data.footer = 22;

    // create the pipe
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        return -1;
    }

    // Read the payload data from the file to pipe
    int sp, x;
    off_t off_in = 0, off_out = 0;

    while (1)
    {
        off_in += data.metadata;
        printf("Reading from %ld\n", off_in);
        sp = splice(source, &off_in, pipefd[1], NULL, data.payload, SPLICE_F_NONBLOCK);

        if (sp == -1 || sp == 0)
        {
            perror("splice");
            break;
        }

        off_in += data.footer;
    }

    sp = splice(pipefd[0], NULL, destination, NULL, off_in, SPLICE_F_MOVE);

    printf("spliced data %d \n", sp);

    // printf("data spliced %d \n", sp);

    // close the file descriptors
    close(pipefd[0]);
    close(pipefd[1]);
    close(source);
    close(destination);
    close(temp);

    return 0;
}
