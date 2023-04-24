#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

int main(int argc, char const *argv[])
{
    int fd_in, fd_out;
    int sp;

    /*create a pipe where read is on 0 and write is on 1 index of the array
    pipe uses a unidirectional communication hence there is one end to write
    and another end to read
    */
    int pipefd[2];
    pipe(pipefd);

    fd_in = open("./root/test", O_RDONLY);
    fd_out = open("./root/out", O_WRONLY);
    if (fd_in < 0 | fd_out < 0)
    {
        perror("open");
    }
    // for splicing, one of the file descriptors must refers to a pipe file descriptor
    sp = splice(STDIN_FILENO, NULL, pipefd[1], NULL, 4096, SPLICE_F_MOVE);
    sp = splice(pipefd[0], NULL, fd_out, NULL, sp, SPLICE_F_MOVE);
    perror("splice");

    close(fd_in);
    close(fd_out);
    close(pipefd[0]);
    close(pipefd[1]);
    return 0;
}
