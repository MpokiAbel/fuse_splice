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

    if ((temp = open("./", O_TMPFILE | O_RDWR, S_IRWXU)) == -1)
    {
        perror("Open TEMP");
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

    // Get the pipe size
    int pipe_size = fcntl(pipefd[1], F_GETPIPE_SZ);
    if (pipe_size == -1)
    {
        perror("fcntl");
        return -1;
    }

    printf("Size of the pipe: %d bytes\n", pipe_size);

    // Read the payload data from the file to temp
    int sp, x;
    off_t off_in = 0, off_out = 0;
    long blocks = pipe_size / (data.metadata + data.payload + data.footer);
    while (blocks > 0)
    {
        off_in += data.metadata;
        sp = copy_file_range(source, &off_in, temp, &off_out, data.payload, 0);
        if (sp == -1 || sp == 0)
        {
            perror("copy_file_range");
            break;
        }

        off_in += data.footer;

        x++;
        blocks--;
    }

    sp = splice(temp, NULL, pipefd[1], NULL, data.payload * x, SPLICE_F_MOVE);
    sp = splice(pipefd[0], NULL, destination, NULL, sp, SPLICE_F_MOVE);

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
