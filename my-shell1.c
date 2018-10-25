#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>

// コマンドを表す構造体
struct command {

    // コマンドライン引数を表す文字列の配列
    char **argv;

    // 配列argvの個数
    long argc;

    // メモリ上に確保されたargvの長さ
    long capacity;
};

// command構造体へのポインタを引数に、その指し示す先のコマンドを実行する
static void execute_command(struct command *command);

// コマンドを読み込んで、command構造体へのポインタを返す
static struct command* read_command(void);

// コマンドライン引数を表現する文字列を引数に、パースしてそれを格納したcommand構造体へのポインタを返す
static struct command* parse_command(char *command_line);

// command構造体を指すポインタを引数に、指し示す先のメモリ領域を解放する
static void free_command(struct command *p);

// sizs分だけメモリ上に領域を確保する
static void* my_malloc(size_t size);

// pointerで指し示す先の領域をsizeになるまで拡張する
static void* my_realloc(void *pointer, size_t size);

// このシェルを実行しているプログラムの名前my-shellを記録する
static char *program_name;

#define PROMPT "(my-shell)> "

int
main(int argc, char *argv[])
{
    program_name = argv[0];
    for(;;){

        // command構造体を指すポインタ変数を宣言
        struct command *command;

        // プロンプトを表示
        // 改行なしで出力するのでfflush
        fprintf(stdout, PROMPT);
        fflush(stdout);

        // コマンドラインを読み込んでcommand構造体を生成し、そのポインタをcommandに代入
        command = read_command();

        if (command->argc > 0) {

            // ここで入力されたコマンドラインを実行
            execute_command(command);
        }

        // 実行し終わったのでcommandの指し示す先を解放する
        free_command(command);
    }
    exit(0);
}

// コマンドを実行する関数
static void
execute_command(struct command *command)
{
    pid_t pid;

    // forkして、このプログラムの複製が誕生する
    pid = fork();

    // forkに失敗した
    if (pid < 0) {
        perror("fork(2) failed!");
        exit(1);
    }

    if (pid > 0) { // 親プロセス
        waitpid(pid, NULL, 0);
    } else { // 子プロセス

        // execvpで実行する
        // 成功したら戻らない。失敗した時だけ制御が戻ってくる
        execvp(command->argv[0], command->argv);

        // 戻ってきているということはコマンドの実行に失敗したということ
        fprintf(stderr, "%s: command not found: %s\n", program_name, command->argv[0]);
        exit(1);
    }
}

#define LINE_BUFFER_SIZE 2048

// 標準入力からコマンドラインを読み込んでそれを表現するcommand構造体へのポインタを返す関数
static struct command*
read_command(void)
{
    static char buffer[LINE_BUFFER_SIZE];

    // fgetsで行単位で入力を取得
    if (fgets(buffer, LINE_BUFFER_SIZE, stdin) == NULL) { // 一文字も読まずにEOFに遭遇したらNULLを返す
        exit(0); // Ctrl-Dでmy-shellを終了することを容認している
    }

    // 読み込んだコマンドラインをパースして適切なcommand構造体へのポインタを返す
    return parse_command(buffer);
}

#define INIT_CAPACITY 16

// 文字列を指すポインタを引数にとり、その文字列を解析してcommand構造体を生成し、それへのポインタを返すヘルパー関数
static struct command*
parse_command(char *command_line)
{
    // コマンドラインの文字列を指すポインタをpにコピー
    char *p = command_line;

    // 返すcommand構造体へのポインタを格納する変数を宣言して、メモリ領域を確保
    struct command *command;
    command = my_malloc(sizeof(struct command));

    // 変数commandの指し示す先を初期化
    command->argc = 0;
    command->argv = my_malloc(sizeof(char*) * INIT_CAPACITY);
    command->capacity = INIT_CAPACITY;

    // コマンドラインの文字列をパースしていく
    while (*p) {

        // whitespaceを終端文字に置換していくことで文字列を分断していく
        while (*p && isspace((int)*p)) {
            *p++ = '\0';
        }

        if (*p) {

            // コマンドラインの文字列が長すぎて足りないので拡張
            if (command->capacity <= command->argc + 1) {
                command->capacity *= 2;
                command->argv = my_realloc(command->argv, command->capacity);
            }

            // commandの指し示す先にあるcommand構造体へ格納していく
            command->argv[command->argc] = p;

            // argvに格納するたびにargcを更新する
            command->argc++;
        }
        while (*p && !isspace((int)*p)) {
            p++;
        }
    }

    // 末端にはNULLを入れておくことでexecvpを使える
    command->argv[command->argc] = NULL;
    return command;
}

// sizeだけの領域を確保してそこを指し示すvoidポインタを返すヘルパー関数
static void *
my_malloc(size_t size)
{
    void *p;
    p = malloc(size);
    if (!p) {
        perror("malloc failed");
        exit(1);
    }
    return p;
}

// pointerの指し示す先があればそこをsizeになるまで拡張する、指し示す先がなければsizeだけの領域を確保してその領域へのポインタを返すヘルパー関数
static void *
my_realloc(void *pointer, size_t size)
{
    void *p;
    if (!pointer) {
        return my_malloc(size);
    }

    p = realloc(pointer, size);
    if (!p) {
        perror("failed realloc");
        exit(1);
    }
    return p;
}

static void
free_command(struct command *command)
{
    free(command->argv);
    free(command);
}
