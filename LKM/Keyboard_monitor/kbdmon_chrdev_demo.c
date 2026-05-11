#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

struct key_event {
    uint16_t code;
    uint8_t value;
};

int main(void)
{
    int fd;
    struct key_event ev;

    fd = open("/dev/kbdmon", O_RDONLY);

    while (1) {
        read(fd, &ev, sizeof(ev));

        printf("code=%u value=%u\n",
               ev.code,
               ev.value);
    }
}