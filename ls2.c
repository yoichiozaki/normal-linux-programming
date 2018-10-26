#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <time.h>
#include <string.h>

static void do_ls(char *path);
static void die(const char *s);
static char* allocate_path_buffer(size_t required_len);

int
main(int argc, char *argv[])
{
    int i;

    if (argc < 2) {
        fprintf(stderr, "%s: no arguments\n", argv[0]);
        exit(1);
    }
    for (i = 1; i < argc; i++) {
        do_ls(argv[i]);
    }
    exit(0);
}

static void
do_ls(char *path)
{
    // pathで特定されるDIRを開く
    DIR *d = opendir(path);
    if (!d) {
        perror(path);
        exit(1);
    }
    struct dirent *ent;

    // 芋づる式にそのディレクトリに入っているエントリーを読み込んでいく
    while ((ent = readdir(d)) != NULL) {

        // 十分な長さのバッファを取っておく
        char *buffer = allocate_path_buffer(strlen(path) + 1 + strlen(ent->d_name));
        sprintf(buffer, "%s/%s", path, ent->d_name);

        struct stat st;
        if (lstat(buf, &st) < 0) {
            die(buf);
        }
        char *mtime = ctime(&st.st_mtime);
        mtime[strlen(mtime) - 1] = '\0';

        struct passwd *pw = getpwuid(st.st_uid);
        if (pw) {
            printf("%s owner=%s mtime=%s\n", ent->d_name, pw->pw_name, mtime);
        }
        else {
            printf("%s owner=%d mtime=%s\n", ent->d_name, st.st_uid, mtime);
        }
    }
    closedir(d);
}

static char *file_path = NULL;
size_t path_len = 0;

// 必要な長さのバッファを確保するヘルパー関数
static char*
allocate_path_buffer(size_t required_len)
{
    size_t len = path_len;
    while (len < required_len) {
        len += 1024;
    }
    if (len > path_len) {
        if (!file_path) {
            file_path = malloc(len);
            if (!file_path) {
                die("malloc");
            }
        }
        else {
            char *tmp = realloc(file_path, len);
            if (!tmp) {
                die("realloc");
            }
            file_path = tmp;
        }
    }
    return file_path;
}

static void
die(const char *s)
{
    perror(s);
    exit(1);
}
