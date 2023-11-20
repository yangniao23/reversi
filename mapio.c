#include "mapio.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mapmanager.h"
#include "tools.h"

static int parse_columncoord(const char *str) {
    if (str == NULL) return -1;
    if ('A' <= str[0] && str[0] <= 'A' + XSIZE) {
        return str[0] - 'A';
    } else if ('a' <= str[0] && str[0] <= 'a' + XSIZE) {
        return str[0] - 'a';
    }
    return 1;
}

static int parse_cmd(const char *str, char map[YSIZE][XSIZE], Flags *flags) {
    char buf[BUFSIZE], *p;
    size_t last;

    last = strlen(str) - 1;
    memcpy(buf, str, last + 1);
    if (last >= 0 && buf[last] == '\n') {
        if (strtok(buf, " ")) p = strtok(NULL, "\n");
        if (p == NULL) return -1;
    }

    if (str[0] == ':') {
        switch (str[1]) {
            case 's':
                flags->skip_flag = true;
                return -1;

            case 'r':
                if (read_map_file(p, map) != 0) {
                    fprintf(stderr, "read_map_file() failed.\n");
                    return 1;
                };
                dump_map(map);
                flags->reset_flag = true;
                return -1;

            case 'w':
                write_map_file(p, map);
                if (str[2] != 'q') return 1;

            case 'q':
                flags->quit_flag = true;
                return -1;
        }
        return -1;
    }
    return 0;
}

Directionlist *input_move(char map[YSIZE][XSIZE], Validcoord *validcoords,
                          int *x, int *y, Flags *flags) {
    Validcoord *entry = NULL;
    char buf[BUFSIZE];
    char *endptr;

    while (1) {
        int res;
        printf("input coords: ");
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            fprintf(stderr, "fgets() failed.\n");
            return NULL;
        }
        res = parse_cmd(buf, map, flags);
        if (res == 1)
            continue;
        else if (res == -1)
            return NULL;

        *y = strtol(strtok(buf, " "), &endptr, 10) - 1;
        if (endptr == buf) {
            fprintf(stderr, "Invalid input.\n");
            continue;
        } else if (*y < 0 || *y >= YSIZE) {
            fprintf(stderr, "Out of range.\n");
            continue;
        }

        *x = parse_columncoord(strtok(NULL, "\0"));
        if (*x == -1) {
            fprintf(stderr, "Invalid input.\n");
            continue;
        } else if (*x == 1) {
            fprintf(stderr, "Out of range.\n");
            continue;
        }

        entry = search_validcoord(validcoords, *y, *x);
        if (entry != NULL)
            break;
        else
            fprintf(stderr, "Cannot put here.\n");
    }
    return entry->directionlist;
}

void dump_map(char map[YSIZE][XSIZE]) {
    flush();
    putchar('\n');
    printf("  a  b  c  d  e  f  g  h\n");
    for (int y = 0; y < YSIZE; y++) {
        putchar('1' + y);
        for (int x = 0; x < XSIZE; x++) {
            switch (map[y][x]) {
                case 0:
                    printf("| |");
                    break;
                case WHITE:
                    printf("|●|");
                    break;
                case BLACK:
                    printf("|○|");
                    break;
            }
        }
        putchar('\n');
    }
}

int read_map_file(const char *fname, char map[YSIZE][XSIZE]) {
    char buf[BUFSIZE];
    FILE *fp;

    if ((fp = fopen(fname, "r")) == NULL) {
        fprintf(stderr, "fopen() failed.\n");
        return -1;
    }
    fgets(buf, sizeof(buf), fp);

    for (int y = 0; y < YSIZE; y++) {
        fgets(buf, sizeof(buf), fp);
        map[y][0] = atoi(strtok(buf, ","));
        for (int x = 1; x < XSIZE; x++) {
            map[y][x] = atoi(strtok(NULL, ","));
        }
    }
    fclose(fp);
    return 0;
}

int write_map_file(const char *fname, char map[YSIZE][XSIZE]) {
    FILE *fp;

    if ((fp = fopen(fname, "w")) == NULL) {
        fprintf(stderr, "fopen() failed.\n");
        return -1;
    }
    fprintf(fp, "%d,%d\n", XSIZE, YSIZE);

    for (int y = 0; y < YSIZE; y++) {
        for (int x = 0; x < XSIZE; x++) {
            fprintf(fp, ",%d", map[y][x]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
    return 0;
}