#define _GNU_SOURCE
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ptrace.h>

int main(int argc, char const *argv[])
{
    pid_t pid;
    int pipefd, fileout;

    printf("Enter a process ID: ");
    scanf("%d", &pid);

    printf("Enter pipe read fd: ");
    scanf("%d", &pipefd);

    printf("Enter file to write fd: ");
    scanf("%d", &fileout);

    int pidfd = syscall(SYS_pidfd_open, pid, 0);
    int piperead = syscall(SYS_pidfd_getfd, pidfd, pipefd, 0);
    int out = syscall(SYS_pidfd_getfd, pidfd, fileout, 0);

    if (piperead == -1 || out == -1)
    {
        perror("");
        return -1;
    }

    printf("Pidfd %d, piperead %d, out %d\n", pidfd, piperead, out);
    int sp;
    char buf[1024];
    while (1)
    {
        sp = splice(piperead, NULL, out, NULL, 202, SPLICE_F_MOVE);
    }
    printf("%s\n", buf);

    return 0;
}
