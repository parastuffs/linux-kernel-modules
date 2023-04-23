#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h> /* O_RDWR */
#include <unistd.h>

#define BUFFER_LENGTH 256
static char receive[BUFFER_LENGTH];

int main(int argc, char* argv[])
{
    int fd, ret;
    char stringToSend[BUFFER_LENGTH];

    fd = open("/dev/MyChrDevice", O_RDWR);
    if (fd < 0){
        perror("Failed to open the device...");
        return errno;
    }
    printf("Type in a short string to send to the kernel module:\n");
    scanf("%[^\n]%*c", stringToSend);
    printf("Writing message to the device [%s].\n", stringToSend);
    ret = write(fd, stringToSend, strlen(stringToSend));
    if (ret < 0){
        perror("Failed to write the message to the device.");
        return errno;
    }
    printf("Press ENTER to read back from the device...\n");
    getchar();

    printf("Reading from the device...\n");
    ret = read(fd, receive, BUFFER_LENGTH);
    if (ret < 0){
        perror("Failed to read the message from the device.");
        return errno;
    }
    printf("The received message is: [%s]\n", receive);

    if(close(fd) < 0){
        perror("Failed to close the device!\n");
        return errno;
    }


    return EXIT_SUCCESS;
}