#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

int main()
{   
    FILE *file;
    char command[100];
    struct timeval start, end;
    double elapsed_time;
    struct stat statbuf;

    // Specify the file to be read
    const char *filename = "/home/mpokiabel/Documents/fuse/fuse_fs/mnt/file_1.out";
    int fd = open(filename, O_RDONLY);
    int error = stat(filename, &statbuf);

    if (error == -1 || fd == -1)
    {
        printf("Failed to Open or Statbuf\n");
        return -1;
    }

    char *buf = (char *)malloc(statbuf.st_size);
    printf("Size to read %ld \n", statbuf.st_size);

    if (buf == NULL)
    {
        printf("There is not enough memory \n");
        return -1;
    }

    // Start the timer
    gettimeofday(&start, NULL);

    error = read(fd, buf, statbuf.st_size);

    // Stop the timer
    gettimeofday(&end, NULL);

    // Check if the command executed successfully
    if (error == -1)
    {
        printf("Read operation failed\n");
        free(buf);
        return -1;
    }

    // Calculate the elapsed time in milliseconds
    elapsed_time = (end.tv_sec - start.tv_sec) * 1000.0;    // seconds to milliseconds
    elapsed_time += (end.tv_usec - start.tv_usec) / 1000.0; // microseconds to milliseconds

    printf("Read operation took %.2f milliseconds.\n", elapsed_time);

    free(buf);
    return 0;
}