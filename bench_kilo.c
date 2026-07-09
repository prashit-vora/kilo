/*** Kilo I/O Benchmark Harness ***/
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/time.h>

#define KILO_TAB_STOP 8

struct erow{
    int size;
    int rsize;
    char* chars;
    char* render;
    unsigned char* hl;
};

struct editorConfig{
    int cx, cy;
    int rx;
    int rowoff;
    int coloff;
    int screenrows;
    int screencols;
    int numrows;
    struct erow* row;
    int dirty;
    char* filename;
    char statusmsg[80];
    time_t statusmsg_time;
    struct editorSyntax* syntax;
    struct termios orig_termios;
};

struct editorSyntax{
    char* filename;
    char** filetype;
    int flags;
};

struct editorConfig E;

void die(const char* s){
    write(STDOUT_FILENO, "\x1b[2J",4);
    write(STDOUT_FILENO, "\x1b[H",3);
    perror(s);
    exit(1);
}

struct abuf {
  char *b;
  int len;
};
#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
    char *new = realloc(ab->b, ab->len + len);
    if (new == NULL) return;
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf *ab) {
    free(ab->b);
}

void editorSetStatusMessage(const char* fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}

void editorUpdateSyntax(struct erow* row){
    row->hl = realloc(row->hl, row->rsize);
    row->hl = memset(row->hl, 0, row->rsize);
    int prev_sep = 1;
    int i = 0;
    while (i < row->rsize){
        char c = row->render[i];
        unsigned char prev_hl = (i > 0) ? row->hl[i-1] : 0;
        if (isdigit(c) && (prev_sep || prev_hl == 1) ||
            (c == '.' && prev_hl == 1)){
            row->hl[i] = 1;
            i++;
            prev_sep = 0;
            continue;
        }
        prev_sep = isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
        i++;
    }
}

void editorUpdateRow(struct erow* row){
    int tabs = 0;
    int j;
    for (j = 0; j < row->size; j++)
        if (row->chars[j] == '\t') tabs++;
    free(row->render);
    row->render = malloc(row->size + tabs*(KILO_TAB_STOP - 1) + 1);
    int idx = 0;
    for (j = 0; j < row->size; j++){
        if (row->chars[j] == '\t'){
            row->render[idx++] = ' ';
            while (idx % KILO_TAB_STOP != 0) row->render[idx++] = ' ';
        }else{
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;
    editorUpdateSyntax(row);
}

void editorInsertRow(int at, char* s, size_t len){
    if (at < 0 || at > E.numrows) return;
    E.row = realloc(E.row, sizeof(struct erow) * (E.numrows + 1));
    memmove(&E.row[at + 1], &E.row[at], sizeof(struct erow) * (E.numrows - at));
    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';
    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    E.row[at].hl = NULL;
    editorUpdateRow(&E.row[at]);
    E.numrows++;
    E.dirty++;
}

char* editorRowsToString(int* buflen){
    int totlen = 0;
    int j;
    for (j = 0; j < E.numrows; j++)
        totlen += E.row[j].size + 1;
    *buflen = totlen;
    char* buf = malloc(totlen);
    char* p = buf;
    for (j = 0; j < E.numrows; j++){
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }
    return buf;
}

void editorOpen(char* filename){
    free(E.filename);
    E.filename = strdup(filename);
    FILE* fp = fopen(filename, "r");
    if (!fp) die("fopen");
    char* line = NULL;
    size_t linecap = 0;
    size_t linelen;
    while ((linelen = getline(&line, &linecap, fp)) != -1){
        while (linelen > 0 && (line[linelen-1] == '\n' || line[linelen-1] == '\r'))
            linelen--;
        editorInsertRow(E.numrows, line, linelen);
    }
    free(line);
    fclose(fp);
    E.dirty = 0;
}

void editorSave(){
    int len;
    char* buf = editorRowsToString(&len);
    int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
    if (fd != -1){
        if (ftruncate(fd,len) != -1){
            if (write(fd,buf,len) == len){
                close(fd);
                free(buf);
                E.dirty = 0;
                return;
            }
        }
        close(fd);
    }
    free(buf);
}

void freeEditor(){
    for (int j = 0; j < E.numrows; j++){
        free(E.row[j].render);
        free(E.row[j].chars);
        free(E.row[j].hl);
    }
    free(E.row);
    free(E.filename);
}

static double tv_to_double(struct timeval *tv) {
    return tv->tv_sec + tv->tv_usec / 1e6;
}

int main(int argc, char* argv[]){
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename> [--save]\n", argv[0]);
        return 1;
    }

    char* testfile = argv[1];
    int do_save = 0;
    int iterations = 5;

    for (int i = 2; i < argc; i++){
        if (strcmp(argv[i], "--save") == 0) do_save = 1;
        if (strcmp(argv[i], "--iterations") == 0 && i+1 < argc)
            iterations = atoi(argv[i+1]);
    }

    struct timeval tv1, tv2;

    printf("{\n");
    printf("  \"file\": \"%s\",\n", testfile);

    // --- BENCHMARK: Open/Read ---
    printf("  \"read_times_ms\": [\n");
    for (int iter = 0; iter < iterations; iter++) {
        // Reset editor
        memset(&E, 0, sizeof(E));
        E.filename = NULL;

        gettimeofday(&tv1, NULL);
        editorOpen(testfile);
        gettimeofday(&tv2, NULL);

        double elapsed = (tv_to_double(&tv2) - tv_to_double(&tv1)) * 1000.0;
        printf("    %s%.2f", iter > 0 ? "," : "", elapsed);

        int line_count = E.numrows;
        int char_count = 0;
        for (int j = 0; j < E.numrows; j++) char_count += E.row[j].size;

        // Cleanup for next iteration - free all rows
        for (int j = 0; j < E.numrows; j++){
            free(E.row[j].render);
            free(E.row[j].chars);
            free(E.row[j].hl);
        }
        free(E.row);
        free(E.filename);
        E.row = NULL;
        E.numrows = 0;
    }
    printf("\n  ],\n");

    // --- BENCHMARK: Write/Save ---
    if (do_save) {
        // Re-open for save test
        memset(&E, 0, sizeof(E));
        E.filename = NULL;
        editorOpen(testfile);

        printf("  \"write_times_ms\": [\n");
        for (int iter = 0; iter < iterations; iter++) {
            gettimeofday(&tv1, NULL);
            editorSave();
            gettimeofday(&tv2, NULL);
            double elapsed = (tv_to_double(&tv2) - tv_to_double(&tv1)) * 1000.0;
            printf("    %s%.2f", iter > 0 ? "," : "", elapsed);
        }
        printf("\n  ],\n");

        freeEditor();
    }

    printf("  \"end\": true\n");
    printf("}\n");

    return 0;
}
