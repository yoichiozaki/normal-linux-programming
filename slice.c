#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

static void do_slice(regex_t *pattern, FILE *f);

int
main(int argc, char *argv[])
{
    regex_t pattern;
    int error_flag;
    int i;

    if (argc < 2) {
        fputs("no pattern\n", stderr);
        exit(1);
    }

    error_flag = regcomp(&pattern, argv[1], REG_EXTENDED | REG_NEWLINE);

    if (error_flag != 0) {
        char buffer[1024];
        regerror(error_flag, &pattern, buffer, sizeof buffer);
        puts(buffer);
        exit(1);
    }

    if (argc == 2) {
        do_slice(&pattern, stdin);
    } else {
        for (i = 2; i < argc; i++) {
            FILE *f;
            f = fopen(argv[i], "r");
            if (!f) {
                perror(argv[i]);
                exit(1);
            }
            do_slice(&pattern, f);
            fclose(f);
        }
    }
    regfree(&pattern);
    exit(0);
}

static void
do_slice(regex_t *pattern, FILE *src)
{
    char buffer[4096];
    while (fgets(buffer, sizeof buffer, src)) {
        regmatch_t matched[1];
        if (regexec(pattern, buffer, 1, matched, 0) == 0 ) {
            char *str = buffer + matched[0].rm_so;
            regoff_t len = matched[0].rm_eo - matched[0].rm_so;
            fwrite(str, len, 1, stdout);
            fputc('\n', stdout);
        }
    }
}

