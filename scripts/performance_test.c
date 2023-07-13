#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>

int main()
{
    FILE *file;
    char command[100];
    struct timeval start, end;
    double elapsed_time;

    // Specify the file to be read
    const char *filename = "/home/mpokiabel/Documents/fuse/fuse_fs/mnt/file_1.out";

    // Create the command to execute
    sprintf(command, "cat %s", filename);

    // Start the timer
    gettimeofday(&start, NULL);

    // Execute the command using system()
    int status = system(command);
    // int status = open(filename,O_RDONLY);
    // Stop the timer
    gettimeofday(&end, NULL);

    // Check if the command executed successfully
    if (status == -1)
    {   perror("Open");
        printf("Error executing the cat command.\n");
        return 1;
    }

    // Calculate the elapsed time in milliseconds
    elapsed_time = (end.tv_sec - start.tv_sec) * 1000.0;    // seconds to milliseconds
    elapsed_time += (end.tv_usec - start.tv_usec) / 1000.0; // microseconds to milliseconds

    printf("Cat operation took %.2f milliseconds.\n", elapsed_time);

    return 0;
}
