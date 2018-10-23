#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
    int i;

    // stdioでは独自のバッファを用意してくれるので、こちらがバッファを定義しなくて済む

    for (i = 0; i < argc; i++) {

        // FILEはファイルディスクリプタとstdioで用意するバッファの内部状態を保つ構造体
        // そのポインタを以ってストリームを表現する
        FILE *f;
        int c;

        // argv[i]のパスで指定されるファイルを開ける。返り値はFILE*
        // 失敗した場合はNULLを返す
        f = fopen(argv[i], "r");
        if (!f) {
            perror(argv[i]);
            exit(1);
        }

        // fgetcでFILE*を引数にとって1バイトづつ読み込む。ストリームが終了したらEOFを返す
        while ((c = fgetc(f)) != EOF) {

            // putcharは固定でstdoutに1バイトづつ出力する
            if (putchar(c) < 0) {
                exit(1);
            }
        }
        fclose(f);
    }
    exit(1);
}
