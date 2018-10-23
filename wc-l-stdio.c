#include <stdio.h>
#include <stdlib.h>

static void do_wc_l(FILE *f);

int
main(int argc, char *argv[])
{
    if (argc == 1) {
        do_wc_l(stdin);
    } else {
        int i;

        for (i = 1; i < argc; i++) {
            FILE *f;

            f = fopen(argv[i], "r");
            if (!f) {
                perror(argv[i]);
                exit(1);
            }
            do_wc_l(f);
            fclose(f);
        }
    }
    exit(0);
}

static void
do_wc_l(FILE *f)
{
    unsigned long n;
    int c;

    // 直前に読んだ文字が入るprev
    int prev = '\n';

    n = 0;

    // EOFに遭遇するまで1バイトづつgetc
    while ((c = getc(f)) != EOF) {

        // '\n'に遭遇したら一行分が確定なのでインクリメント
        if (c == '\n') {
            n++;
        }
        prev = c;
    }

    // 改行なしで終わる場合もあるので最後にインクリメント
    // 末端に改行が入っていないと、上ではあくまで'\n'に遭遇したらインクリメントなので最後の改行は考慮されていない
    if (prev != '\n') {
        n++;
    }
    printf("%lu\n", n);
}
