#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// prototypes
static void do_cat(const char *path);
static void die(const char *s);

// コマンドライン引数としてファイル名を一つ以上受け取り
// その中身を標準出力へ書き込む簡略版catコマンド
int
main(int argc, char *argv[])
{
    int i;

    // 引数が足りない場合
    if (argc < 2) {

        // エラーメッセージを標準エラー出力に出してから実行を止める
        // fprintf(stderr, "%s: file name not given\n", argv[0]);
        // exit(1);

        // 標準入力からcatする
        do_cat(stdin);

    }

    // 与えられたファイルを順番に出力してく
    for (i = 1; i < argc; i++) {

        // 簡易版catの本質部分
        do_cat(argv[i]);
    }
    exit(0);
}

#define BUFFER_SIZE 2048

// 簡易版catの本質部分
static void
do_cat(const char *path)
{

    // ファイルディスクリプタ
    int fd;

    // readの吐き出し先バッファ
    unsigned char buf[BUFFER_SIZE];

    int n;

    // openでファイルを開ける
    // フラグはReaDONLY
    fd = open(path, O_RDONLY);

    // open失敗
    if (fd < 0) {
        die(path);
    }

    // ファイルを開けたのでその内容を読み込む
    for (;;) {

        // readでbufに読み込む
        // sizeof演算子はメモリサイズをバイト単位で返す
        n = read(fd, buf, sizeof buf);

        // read失敗
        if (n < 0) {
            die(path);
        }

        // readで終端文字'\0'に遭遇した（つまり読み込み終わった）
        if (n == 0) {
            break;
        }

        // bufに一時的にためておいたファイルの中身を標準出力に吐き出す
        // write失敗で-1が返る
        // 常にbufが埋まるわけではないのでreadで読み込んだ分（n）だけwriteする
        if (write(STDOUT_FILENO, buf, n) < 0) {
            die(path);
        }
    }

    // ファイルを閉じる
    if (close(fd) < 0) {
        die(path);
    }
}

// エラーを出力し終了させるヘルパー関数
static void
die(const char *s)
{
    perror(s);
    exit(1);
}
