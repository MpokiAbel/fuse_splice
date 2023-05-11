#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
    int fdS = open("file_1", O_RDONLY);
    int fdD = open("copied", O_WRONLY);

    struct stat fdS_stat;

    if (fstat(fdS, &fdS_stat) == -1)
        return -1;
    printf("length is %ld\n", fdS_stat.st_size);

    int pipefd[2];

    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        return -1;
    }

    long off_in = 0;
    long off_out = 0;
    int sp, x;

    while (1)
    {
        off_in = off_in + 52;
        sp = splice(fdS, &off_in, pipefd[1], NULL, 202, SPLICE_F_MOVE);

        if (sp == 0)
            break;

        off_out = off_out + sp;
        off_in = off_in + 22 + 200;

        x++;
    }

    sp = splice(pipefd[0], NULL, fdD, &off_out, sp, SPLICE_F_MOVE);

    // while (x < 5)
    // {
    //     sp = splice(fdS, NULL, pipefd[1], NULL, 204+50+20+2, SPLICE_F_MOVE);
    //     x++;
    // }
    // sp = sp * (x+1);
    // sp = splice(pipefd[0], NULL, fdD, NULL, sp, SPLICE_F_MOVE);

    close(pipefd[0]);
    close(pipefd[1]);
    close(fdS);
    close(fdD);
    return 0;
}
