#include "mapio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tools.h"

Directionlist *input_move(Validcoord *validcoords, int *x, int *y) {
    Validcoord *entry = NULL;
    char buf[BUFSIZE];
    char *endptr, *p;

    while (1) {
        printf("Please input coordinates: ");
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            fprintf(stderr, "fgets() failed.\n");
            return NULL;
        }

        *y = strtol(strtok(buf, " "), &endptr, 10) - 1;
        if (endptr == buf) {
            fprintf(stderr, "Invalid input.\n");
            continue;
        }
        if (*y < 0 || *y >= YSIZE) {
            fprintf(stderr, "Out of range.\n");
            continue;
        }

        p = strtok(NULL, "\0");
        if (p == NULL) {
            fprintf(stderr, "Invalid input.\n");
            continue;
        }

        if ('A' <= p[0] && p[0] <= 'A' + XSIZE) {
            *x = p[0] - 'A';
        } else if ('a' <= p[0] && p[0] <= 'a' + XSIZE) {
            *x = p[0] - 'a';
        } else {
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

int input_coord(char map[YSIZE][XSIZE], int *x, int *y) {
    char buf[BUFSIZE];
    char *endptr, *p;

    x = -1;
    y = -1;
    while (1) {
        int res;
        printf("input coords: ");
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            fprintf(stderr, "fgets() failed.\n");
            return -1;
        }
        res = parse_cmd(buf, map);
        if (res == 1)
            continue;
        else if (res == -1)
            return -1;

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
        }
        break;
    }
    return 0;
}

int parse_columncoord(const char *str) {
    if (str == NULL) return -1;
    if ('A' <= str[0] && str[0] <= 'A' + XSIZE) {
        return str[0] - 'A';
    } else if ('a' <= str[0] && str[0] <= 'a' + XSIZE) {
        return str[0] - 'a';
    }
    return -1;
}

int parse_cmd(const char *str, char map[YSIZE][XSIZE]) {
    if (str[0] == ':') {
        switch (str[1]) {
            case 'r':
                read_map_file(str + 2, map);
                dump_map(map);
                return 1;
            case 'w':
                write_map_file(str + 3, map);
                if (str[2] != 'q') return 1;
            case 'q':
                return -1;
        }
        return -1;
    }
    return 0;
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
        for (int x = 0; x < XSIZE; x++) {
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