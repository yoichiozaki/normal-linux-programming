#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static void do_word_count(int fd, const char *path);
static void die(const char *s);

int
main(int argc, char *argv[])
{
    int i;

    if (argc < 2) {
        fprintf(stderr, "%s: file name not given\n", argv[0]);
        exit(1);
    }

    for (i = 1; i < argc; i++) {
        char *path = argv[i];
        int fd = open(path, O_RDONLY);
        if (fd < 0) {
            die(path);
        }
        do_word_count(fd, path);
        if (close(fd) < 0) {
            die(path);
        }
    }
    exit(0);
}

#define BUFFER_SIZE 2048

static void
do_word_count(int fd, const char *path)
{
    unsigned long count = 0;
    for (;;) {
        unsigned char buf[BUFFER_SIZE];
        int n = read(fd, buf, sizeof buf);
        if (n < 0) {
            die(path);
        }
        if (n == 0) {
            break;
        }
        unsigned int i;
        for (i = 0; i < BUFFER_SIZE; i++) {
            if (buf[i] == '\n') {
                count++;
            }
        }
    }
    printf("%s:\t%lu\n", path, count);
}

static void
die(const char *s)
{
    perror(s);
    exit(1);
}
