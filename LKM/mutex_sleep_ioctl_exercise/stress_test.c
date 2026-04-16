// stress_test.c

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>

#define DEVICE "/dev/buggy_dev"   // change to buggy_dev, fixed_dev or gold_dev

#define NUM_WRITERS 4
#define NUM_READERS 4
#define NUM_ITER 10
#define MAX_MSG 64

// ==== IOCTL (must match driver) ====
#define MY_IOCTL_MAGIC 'k'
#define IOCTL_RESET _IO(MY_IOCTL_MAGIC, 0)
#define IOCTL_SET_BLOCKING _IOW(MY_IOCTL_MAGIC, 1, int)

// ==== UTILS ====
static void rand_sleep()
{
    usleep(rand() % 1000); // 0–1ms
}

// ==== WRITER THREAD ====
void *writer_thread(void *arg)
{
    int id = *(int *)arg;
    int fd = open(DEVICE, O_RDWR);

    if (fd < 0) {
        perror("open writer");
        return NULL;
    }

    char msg[MAX_MSG];

    for (int i = 0; i < NUM_ITER; i++) {
        snprintf(msg, sizeof(msg), "W%d:%d", id, i);

        ssize_t ret = write(fd, msg, strlen(msg) + 1);

        if (ret < 0) {
            if (errno != EAGAIN)
                perror("write");
        }

        rand_sleep();
    }

    close(fd);
    return NULL;
}

// ==== READER THREAD ====
void *reader_thread(void *arg)
{
    int id = *(int *)arg;
    int fd = open(DEVICE, O_RDWR);

    if (fd < 0) {
        perror("open reader");
        return NULL;
    }

    char buf[MAX_MSG];

    for (int i = 0; i < NUM_ITER; i++) {
        ssize_t ret = read(fd, buf, sizeof(buf));

        if (ret > 0) {
            // Basic corruption detection
            if (buf[0] != 'W') {
                printf("[CORRUPTION] R%d got: %s\n", id, buf);
            }
        } else if (ret < 0) {
            if (errno != EAGAIN && errno != EINTR)
                perror("read");
        }

        rand_sleep();
    }

    close(fd);
    return NULL;
}

// ==== IOCTL THREAD ====
void *ioctl_thread(void *arg)
{
    int fd = open(DEVICE, O_RDWR);

    if (fd < 0) {
        perror("open ioctl");
        return NULL;
    }

    for (int i = 0; i < NUM_ITER; i++) {
        int mode = rand() % 2;

        if (ioctl(fd, IOCTL_SET_BLOCKING, &mode) < 0) {
            perror("ioctl SET_BLOCKING");
        }

        if (rand() % 10 == 0) {
            if (ioctl(fd, IOCTL_RESET) < 0) {
                perror("ioctl RESET");
            }
        }

        rand_sleep();
    }

    close(fd);
    return NULL;
}

// ==== MAIN ====
int main()
{
    pthread_t writers[NUM_WRITERS];
    pthread_t readers[NUM_READERS];
    pthread_t ctrl;

    int ids[NUM_WRITERS > NUM_READERS ? NUM_WRITERS : NUM_READERS];

    srand(time(NULL));

    printf("Starting stress test on %s...\n", DEVICE);

    // Spawn writers
    for (int i = 0; i < NUM_WRITERS; i++) {
        ids[i] = i;
        pthread_create(&writers[i], NULL, writer_thread, &ids[i]);
    }

    // Spawn readers
    for (int i = 0; i < NUM_READERS; i++) {
        ids[i] = i;
        pthread_create(&readers[i], NULL, reader_thread, &ids[i]);
    }

    // Spawn ioctl thread
    pthread_create(&ctrl, NULL, ioctl_thread, NULL);

    // Join all
    for (int i = 0; i < NUM_WRITERS; i++)
        pthread_join(writers[i], NULL);

    for (int i = 0; i < NUM_READERS; i++)
        pthread_join(readers[i], NULL);

    pthread_join(ctrl, NULL);

    printf("Stress test complete.\n");
    return 0;
}