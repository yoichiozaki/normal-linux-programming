#include <stdio.h>
#include <stdlib.h>

static void do_tail(FILE *f, int nlines);
static unsigned char *readline(FILE *f);

int
main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [number]\n", argv[0]);
        exit(1);
    }
    do_tail(stdin, atoi(argv[1]));
    exit(0);
}

#define INCREMENT(ptrvar) do {\
    ptrvar++;\
    if (ptrvar >= ringbuffer + nlines)\
    { ptrvar = ringbuffer; }\
} while (0)

static void
do_tail(FILE *f, int nlines)
{

    unsigned char **ringbuffer;
    unsigned char **p;
    unsigned char *line;
    int n;

    if (nlines == 0) {
        return;
    }

    ringbuffer = calloc(nlines, sizeof(char*));
    if (ringbuffer == NULL) {
        exit(1);
    }

    p = ringbuffer;
    while ((line = readline(f)) != NULL) {
        if (*p) {
            free(*p);
        }
        *p = line;
        INCREMENT(p);
    }
    if (*p == NULL) {
        p = ringbuffer;
    }
    for (n = nlines; n > 0 && *p; n--) {
        printf("%s", *p);
        free(*p);
        INCREMENT(p);
    }
    free(ringbuffer);
}

static unsigned char*
readline(FILE *f)
{
    unsigned char *buffer, *p;
    size_t buflen = BUFSIZ;
    int c;
    buffer = p = malloc(sizeof(char) * buflen);
    if (buffer == NULL) {
        exit(1);
    }
    for (;;) {
        c = getc(f);
        if (c == EOF) {
            if (buffer == p) {
                free(buffer);
                return NULL;
            }
            break;
        }
        *p++ = c;
        if (p >= buffer + buflen - 1) {
            buflen *= 2;
            buffer = realloc(buffer, buflen);
            if (buffer == NULL) {
                exit(1);
            }
        }
        if (c == '\n') {
            break;
        }
    }
    *p++ = '\0';
    return buffer;
}
