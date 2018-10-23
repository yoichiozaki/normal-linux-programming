#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

static void do_grep(regex_t *pattern, FILE *f);

int
main(int argc, char *argv[])
{
    regex_t pattern;
    int error;
    int i;

    if (argc < 2) {
        fputs("no pattern\n", stderr);
        exit(1);
    }

    error = regcomp(&pattern, argv[1], REG_EXTENDED | REG_NOSUB | REG_NEWLINE);
    if (error != 0) {
        char buffer[1024];
        regerror(error, &pattern, buffer, sizeof buffer);
        puts(buffer);
        exit(1);
    }
    if (argc == 2) {
        do_grep(&pattern, stdin);
    } else {
        for (i = 2; i < argc; i++) {
            FILE *f;
            f = fopen(argv[i], "r");
            if (!f) {
                perror(argv[i]);
                exit(1);
            }
            do_grep(&pattern, f);
            fclose(f);
        }
    }
    regfree(&pattern);
    exit(0);
}

static void
do_grep(regex_t *pattern, FILE *src)
{
    char buffer[4096];
    while (fgets(buffer, sizeof buffer, src)) {
        if (regexec(pattern, buffer, 0, NULL, 0) == 0 ) {
            fputs(buffer, stdout);
        }
    }
}

