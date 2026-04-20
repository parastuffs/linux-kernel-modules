#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define DEV "/dev/leaky_dev"

void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void write_to_dev(int fd, const char *msg)
{
    ssize_t ret = write(fd, msg, strlen(msg));
    if (ret < 0)
        die("write");
    printf("[+] wrote: \"%s\"\n", msg);
}

void read_from_dev(int fd, char *buf, size_t size)
{
    memset(buf, 0, size);

    ssize_t ret = read(fd, buf, size - 1);
    if (ret < 0)
        die("read");

    buf[ret] = '\0';

    printf("[+] read (%zd bytes): \"%s\"\n", ret, buf);
}

int main(void)
{
    int fd1, fd2;
    char buffer[256];

    printf("=== Memory leak demo ===\n");

    /*
     * Step 1: Process A writes a secret
     */
    fd1 = open(DEV, O_RDWR);
    if (fd1 < 0)
        die("open fd1");

    write_to_dev(fd1, "SECRET_PASSWORD_123456");

    /*
     * Step 2: Process B opens device separately
     */
    fd2 = open(DEV, O_RDWR);
    if (fd2 < 0)
        die("open fd2");

    /*
     * Overwrite with smaller payload
     * This is where reuse without zeroing becomes visible
     */
    write_to_dev(fd2, "short");

    /*
     * Step 3: Read back buffer from fd2
     */
    printf("\n[+] Process B reading leaked memory...\n");
    read_from_dev(fd2, buffer, sizeof(buffer));

    printf("\n[!] If implemented incorrectly, you may see:\n");
    printf("    - 'short' + leftover bytes from previous allocation\n");
    printf("    - or even unrelated kernel heap data\n");

    close(fd1);
    close(fd2);

    return 0;
}