#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

// バッファを表現する構造体
struct strbuf {
    char *ptr;
    size_t len;
};

// traverseする関数
static void traverse(struct strbuf *buf);

// traverseの内部で呼ばれる関数。
static void traverse_function(struct strbuf *buf, int first);

// 新たにstruct strbufへのポインタを生成する関数
static struct strbuf *strbuf_new(void);

// 新たにstruct strbufを格納するメモリ領域を確保する関数
static void strbuf_realloc(struct strbuf *buf, size_t size);

// エラーメッセージを出力するヘルパー関数
static void print_error(char *s);

// このコマンドの名前を格納する
static char *program_name;

int
main(int argc, char *argv[])
{
    struct strbuf *pathbuf;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s [directory]\n", argv[0]);
        exit(1);
    }

    program_name = argv[0];
    pathbuf = strbuf_new();
    strbuf_realloc(pathbuf, strlen(argv[1]));
    strcpy(pathbuf->ptr, argv[1]);
    traverse(pathbuf);
    exit(0);
}

static void
traverse(struct strbuf *pathbuf)
{
    traverse_function(pathbuf, 1);
}

static void
traverse_function(struct strbuf *pathbuf, int first)
{
    DIR *d;
    struct dirent *ent;
    struct stat st;

    d = opendir(pathbuf->ptr);
    if (!d) {
        switch (errno) {
            case ENOTDIR: return;
            case ENOENT:
                          if (first) {
                              print_error(pathbuf->ptr);
                              exit(1);
                          } else {
                              return;
                          }
            case EACCES:
                          puts(pathbuf->ptr);
                          print_error(pathbuf->ptr);
                          return;
            default:
                          print_error(pathbuf->ptr);
                          exit(1);
        }
    }
    puts(pathbuf->ptr);
    while ((ent = readdir(d)) != NULL) {

        // 無限ループを回避するためにカレントディレクトリはパスする
        if (strcmp(ent->d_name, ".") == 0) {
            continue;
        }

        // 無限ループを回避するために一個上のディレクトリもパスする
        if (strcmp(ent->d_name, "..") == 0) {
            continue;
        }

        strbuf_realloc(pathbuf, pathbuf->len + 1 + strlen(ent->d_name) + 1);
        if (strcmp(pathbuf->ptr, "/")!= 0) {
            strcat(pathbuf->ptr, "/");
        }
        strcat(pathbuf->ptr, ent->d_name);
        if (lstat(pathbuf->ptr, &st) < 0) {
            switch (errno) {
                case ENOENT: break;
                case EACCES:
                             print_error(pathbuf->ptr);
                             break;
                default:
                             print_error(pathbuf->ptr);
                             exit(1);
            }
        } else {
            if (S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode)) {
                traverse_function(pathbuf, 0);
            } else {
                puts(pathbuf->ptr);
            }
        }
        *strchr(pathbuf->ptr, '/') = '\0';
    }
    closedir(d);
}

#define INITLEN 1024

static struct strbuf*
strbuf_new(void)
{
    struct strbuf *buf;
    buf = (struct strbuf*)malloc(sizeof(struct strbuf));
    if (!buf){
        print_error("malloc(3)");
        exit(1);
    }
    buf->ptr = malloc(INITLEN);
    if (!buf->ptr) {
        print_error("malloc(3)");
        exit(1);
    }
    buf->len = INITLEN;
    return buf;
}

// bufで指し示されるstrbuf構造体のptrの指し示すバッファの大きさをlenに拡張するヘルパー関数
static void
strbuf_realloc(struct strbuf *buf, size_t len)
{
    char *tmp;

    // 拡張する前から十分に大きかったので何もしない
    if (buf->len > len) {
        return;
    }

    // reallocで拡張
    tmp = realloc(buf->ptr, len);
    if (!tmp) {
        print_error("realloc(3)");
        exit(1);
    }
    buf->ptr = tmp;
    buf->len = len;
}

static void
print_error(char *s) {
    fprintf(stderr, "%s: %s: %s\n", program_name, s, strerror(errno));
}


