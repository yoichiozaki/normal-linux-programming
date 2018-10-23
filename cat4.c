#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void do_cat(FILE *f, int escape_flag);

int
main(int argc, char *argv[])
{
    int opt;
    int escape_flag = 0;
    int i;

    while ((opt = getopt(argc, argv, "e")) != -1 ) {
        switch (opt) {
            case 'e':
                escape_flag = 1;
                break;
            case '?':
                fprintf(stderr, "Usage: %s [-e] [file ...]\n", argv[0]);
                exit(1);
        }
    }

    argc -= optind;
    argv += optind;

    if (argc == 0) {
        do_cat(stdin, escape_flag);
    } else {
        for (i = 0; i < argc; i++) {
            FILE *f;
            f = fopen(argv[i], "r");
            if (!f) {
                perror(argv[i]);
                exit(1);
            }
            do_cat(f, escape_flag);
            fclose(f);
        }
    }
    exit(0);
}

static void
do_cat(FILE *f, int escape_flag)
{
    int c;

    if (escape_flag) {
        while (( c = fgetc(f)) != EOF) {
            switch (c) {
                case '\t':
                    if (fputs("\\t", stdout) == EOF) {
                        exit(1);
                    }
                    break;
                case '\n':
                    if (fputs("$\n", stdout) == EOF) {
                        exit(1);
                    }
                    break;
                default:
                    if (putchar(c) < 0) {
                        exit(1);
                    }
                    break;
            }
        }
    } else {
        while (( c = fgetc(f)) != EOF) {
            if (putchar(c) < 0) {
                exit(1);
            }
        }
    }
}

