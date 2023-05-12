#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>

int main(int argc, char const *argv[])
{
    int fdS = open("file_1", O_RDONLY);
    int fdD = open("copied", O_WRONLY);

    struct stat fdS_stat;

    if (fstat(fdS, &fdS_stat) == -1)
        return -1;
    printf("length is %ld\n", fdS_stat.st_size);

    int pipefd[2];
    pid_t pid = getpid();

    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        return -1;
    }
    printf("Process ID %d, pipe read end %d, pipe write end %d, destination FD %d\n", pid, pipefd[0], pipefd[1], fdD);
    long off_in = 52, off_out = 0;
    size_t num_blocks = 500;
    int sp, x = 0;

    // while (x < num_blocks)
    // {
    //     sp = splice(fdS, &off_in, pipefd[1], NULL, 202, SPLICE_F_MORE);
    //     if (sp == 0)
    //     {
    //         printf("End of input\n");
    //         break;
    //     }

    //     off_in = off_in + 22 + 52;

    //     x++;
    //     printf("IN WHILE %d\n", sp);
    // }
    while (x<2)
    {
        sp = splice(fdS, &off_in, pipefd[1], NULL, fdS_stat.st_size, SPLICE_F_MORE);
        printf("spliced data size %d\n",sp);

        x++;
    }
    

    sp = splice(pipefd[0], NULL, fdD, &off_out, sp, SPLICE_F_MOVE);

    close(pipefd[0]);
    close(pipefd[1]);
    close(fdS);
    close(fdD);

    return 0;
}
