#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>

// コマンドを表現する構造体
struct command {
    int argc;               // コマンドライン引数の個数
    char **argv;            // コマンドライン引数を格納した文字列配列
    int capacity;           // コマンドライン引数を格納した文字列配列の大きさ
    int status;             // ステータス
    int pid;                // プロセスID
    struct command *next;   // 次のコマンドを表現するcommand構造体へのポインタ
};

#define IS_REDIRECT_PROCESS(command) ((command)->argc == -1)
#define PID_BUILTIN -2
#define IS_BUILTIN_PROCESS(command) ((command)->pid == PID_BUILTIN)

// 組み込みのコマンドを表現する構造体
struct builtin {
    char *name;                         // 名前
    int (*f)(int argc, char *argv[]);   // 関数へのポインタ
};

// このシェルの本質
static void prompt(void);

// コマンドを実行する関数
static int execute_commands(struct command *command);

//
static void execute_pipline(struct command *command_head);

//
static void redirect_stdout(char *path);

//
static int wait_pipeline(struct command *command_head);

//
static struct command* pipeline_tail(struct command *command_head);

//
static struct command* parse_commandline(char *commandline);

//
static void free_command(struct command *p);

//
static struct builtin* lookup_builtin(char *name);

//
static int builtin_cd_command(int argc, char *argv[]);

//
static int builtin_pwd_command(int argc, char *argv[]);

//
static int builtin_exit_command(int argc, char *argv[]);

// sizeだけの領域を確保し、そこへのポインタを返す
static void* my_malloc(size_t size);

// pointerの参照先をsizeまで拡張し、そこへのポインタを返す
static void* my_realloc(void *pointer, size_t size);

// プログラムの名前を確保しておくための文字列へのポインタ
static char *program_name;

int
main(int argc, char *argv[])
{
    program_name = argv[0];
    for(;;) {
        // ここがx-my-shellの本質
        prompt();
    }
    exit(0);
}

#define MAX_LINEBUFFER 2048

static void
prompt(void)
{
    static char buffer[MAX_LINEBUFFER];
    struct command *command;

    // 改行なしで標準出力に出力したいのでfflush
    fprintf(stdout, "(x-my-shell)> ");
    fflush(stdout);

    // 標準入力から行単位で入力を読み込んでbufferに格納する
    // 何も読み込まないでEOFに遭遇するとNULLを返す
    if (fgets(buffer, MAX_LINEBUFFER, stdin) == NULL) {
        exit(0); // Ctrl-Dで終了
    }

    // bufferの中身を解析してcommand構造体を構成し、それへのポインタをcommandへ格納
    command = parse_commandline(buffer);

    // パースに失敗するとNULLポインタが返ってくる
    if (command == NULL) {
        fprintf(stderr, "%s: syntax error\n", program_name);
        return;
    }

    if (command->argc > 0) {
        // 入力されたコマンドを実行
        execute_commands(command);
    }
    free_command(command);
}

// command構造体へのポインタを引数にとり、そこから芋づる式で各コマンドを実行していく
static int
execute_commands(struct command *command_head)
{
    int status;                     // ステータス
    int original_stdin = dup(0);    // stdinに繋がるストリームを複製
    int original_stdout = dup(1);   // stdoutに繋がるストリームを複製

    execute_pipline(command_head);
    status = wait_pipeline(command_head);
    close(0);
    dup2(original_stdin, 0);
    close(original_stdin);
    close(1);
    dup2(original_stdout, 1);
    close(original_stdout);
    return status;
}

#define IS_HEAD_PROCESS(command) ((command) == command_head)
#define IS_TAIL_PROCESS(command) (((command)->next == NULL) || IS_REDIRECT_PROCESS((command)->next))

// パイプで繋がったコマンドを逐次実行していく関数
static void
execute_pipline(struct command *command_head)
{
    struct command *command;
    int fds1[2] = {-1, -1};
    int fds2[2] = {-1, -1};

    for (command = command_head; command && !IS_REDIRECT_PROCESS(command); command = command->next) {
        fds1[0] = fds2[0];
        fds1[1] = fds2[1];

        // 末端でなければ両端とも自プロセスに繋がったパイプを生成
        if (! IS_TAIL_PROCESS(command)) {

            // pipeの呼び出しに失敗
            if (pipe(fds2) < 0) {
                perror("failed pipe");
                exit(3);
            }
        }

        // pidの設定
        // 今見ているコマンドが組み込みのコマンドならば
        if (lookup_builtin(command->argv[0]) != NULL) {
            command->pid = PID_BUILTIN;
        } else {

            // ここでforkする
            command->pid = fork();
            if (command->pid < 0) { // fork失敗
                perror("failed fork");
                exit(3);
            }
            if (command->pid > 0) { // 親プロセス
                if (fds1[0] != -1) {
                    close(fds1[0]);
                }
                if (fds1[1] != -1) {
                    close(fds1[1]);
                }
                continue;
            }
        }

        // 先頭のコマンドでなければ
        if (! IS_HEAD_PROCESS(command)) {

            // 自プロセスの標準入力へ繋がるストリームを閉じる
            close(0);

            //
            dup2(fds1[0], 0);

            //
            close(fds1[0]);

            //
            close(fds1[1]);
        }

        // 末端のコマンドでなければ
        if (! IS_TAIL_PROCESS(command)) {

            //
            close(fds2[0]);

            //
            close(1);

            //
            dup2(fds2[1], 1);

            //
            close(fds2[1]);
        }

        // 今見ているコマンドの次が存在して、かつそれがリダイレクトされていたら
        if ((command->next != NULL) && IS_REDIRECT_PROCESS(command->next)) {
            redirect_stdout(command->next->argv[0]);
        }

        // 今見ているコマンドが組み込みのコマンドでないならば
        if (! IS_BUILTIN_PROCESS(command)) {

            // コマンドを実行
            execvp(command->argv[0], command->argv);

            // エラーした時にしか返ってこない
            fprintf(stderr, "%s: command not found: %s\n", program_name, command->argv[0]);
            exit(1);
        }
    }
}

// pathの指し示す先の文字列で表現される場所に対するストリームを生成し出力つなげる関数
static void
redirect_stdout(char *path)
{
    int fd;

    // 標準出力へのストリームを閉じる
    close(1);

    // pathに繋がるストリームを開く
    fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, 0666);

    // ストリームを開くのに失敗した
    if (fd < 0) {
        perror(path);
        return;
    }

    if (fd != 1) {
        dup2(fd, 1);
        close(fd);
    }
}

static int
wait_pipeline(struct command *command_head)
{
    struct command *command;

    for (command = command_head; command && ! IS_REDIRECT_PROCESS(command); command = command->next) {
        if (IS_BUILTIN_PROCESS(command)) {
            command->status = lookup_builtin(command->argv[0])->f(command->argc, command->argv);
        } else {
            waitpid(command->pid, &command->status, 0);
        }
    }
    return pipeline_tail(command_head)->status;
}

static struct command*
pipeline_tail(struct command *command_head)
{
    struct command *command;
    for (command = command_head; ! IS_TAIL_PROCESS(command); command = command->next) {
    }
    return command;
}

#define INIT_ARGV_SIZE 8
#define IS_IDENT_CHAR_PROCESS(c) (!isspace((int)c) && ((c) != '|') && ((c) != '>'))

static struct command*
parse_commandline(char *p)
{
    struct command *command;
    command = my_malloc(sizeof(struct command));
    command->argc = 0;
    command->argv = my_malloc(sizeof(char*) * INIT_ARGV_SIZE);
    command->capacity = INIT_ARGV_SIZE;
    command->next = NULL;
    while (*p) {
        while (*p && isspace((int)*p)) {
                *p++ = '\0';
        }
        if (! IS_IDENT_CHAR_PROCESS(*p)) {
            if (command->capacity <= command->argc) {
                command->capacity *= 2;
                command->argv = my_realloc(command->argv, command->capacity);
            }
            command->argv[command->argc] = p;
            command->argc++;
        }
        while (*p && IS_IDENT_CHAR_PROCESS(*p)) {
            p++;
        }
    }
    if (command->capacity <= command->argc) {
        command->capacity += 1;
        command->argv = my_realloc(command->argv, command->capacity);
    }
    command->argv[command->argc] = NULL;
    if (*p == '|' || *p == '>') {
        if (command == NULL || command->argc == 0) {
            goto parse_error;
        }
        if (*p == '>') {
            if (command->next->argc != 1) {
                goto parse_error;
            }
            command->next->argc = -1;
        }
        *p = '\0';
    }
    return command;

parse_error:
    if (command) {
        free_command(command);
    }
    return NULL;

}

static void
free_command(struct command *command)
{
    if (command->next != NULL) {
        free_command(command->next);
    }
    free(command->argv);
    free(command);
}

struct builtin builtins_list[] = {
    {"cd",      builtin_cd_command},
    {"pwd",     builtin_pwd_command},
    {"exit",    builtin_exit_command},
    {NULL,      NULL}
};

static struct builtin*
lookup_builtin(char *command)
{
    struct builtin *p;

    for (p = builtins_list; p->name; p++) {
        if (strcmp(command, p->name) == 0) {
            return p;
        }
    }
    return NULL;
}

static int
builtin_cd_command(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "%s: wrong arguments\n", argv[0]);
        return 1;
    }
    if (chdir(argv[1]) < 0) {
        perror(argv[1]);
        return 1;
    }
    return 0;
}

static int
builtin_pwd_command(int argc, char *argv[])
{
    char buffer[PATH_MAX];

    if (argc != 1) {
        fprintf(stderr, "%s: wrong arguments\n", argv[0]);
        return 1;
    }
    if (!getcwd(buffer, PATH_MAX)) {
        fprintf(stderr, "%s: cannot get working directory\n", argv[0]);
        return 1;
    }
    printf("%s\n", buffer);
    return 0;
}

static int
builtin_exit_command(int argc, char *argv[])
{
    if (argc != 1) {
        fprintf(stderr, "%s: too many arguments\n", argv[0]);
        return 1;
    }
    exit(0);
}

static void*
my_malloc(size_t size)
{
    void *p;
    p = calloc(1, size);
    if (!p) {
        exit(3);
    }
    return p;
}

static void*
my_realloc(void *pointer, size_t size)
{
    void *p;
    if (!pointer) {
        return my_malloc(size);
    }
    p = realloc(pointer, size);
    if (!p) {
        exit(3);
    }
    return p;
}




