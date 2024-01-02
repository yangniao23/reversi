#include "mapio.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mapmanager.h"
#include "tools.h"

/*
parse_*coord() の戻り値
0から(XSIZE|YSIZE)-1 は座標
-1 はstrがNULL
-2 は範囲外
*/

static int parse_rowcoord(const char *str) {
    if (str == NULL) return -1;
    if ('1' <= str[0] && str[0] <= '1' + YSIZE) {
        return str[0] - '1';
    }
    return -2;
}

static int parse_columncoord(const char *str) {
    if (str == NULL) return -1;
    if ('A' <= str[0] && str[0] <= 'A' + XSIZE) {
        return str[0] - 'A';
    } else if ('a' <= str[0] && str[0] <= 'a' + XSIZE) {
        return str[0] - 'a';
    }
    return -2;
}

/*
-1 はmake_move()からやり直す (石の配置が変わるのでvalidcoordsの再計算が必要)
0 はコマンドなし
1 はinput_move()からやり直す
*/
static int parse_cmd(const char *str, Board *board, Flags *flags) {
    char buf[BUFSIZE], *p = NULL;
    size_t last;

    last = strlen(str) - 1;
    memcpy(buf, str, last + 1);
    if (buf[last] == '\n') {
        // スペース区切りのコマンドを取り出す
        if (strtok(buf, " ")) p = strtok(NULL, "\n");
    }

    if (str[0] == ':') {
        switch (str[1]) {
            case 's':
                flags->skip_flag = true;
                return -1;

            case 'r':
                if (read_map_file(p, board) != 0) {
                    fprintf(stderr, "read_map_file() failed.\n");
                    return 1;
                };
                dump_bitmap(board);
                flags->reset_flag = true;
                return -1;

            case 'w':
                if (write_map_file(p, board) != 0) {
                    fprintf(stderr, "write_map_file() failed.\n");
                    return 1;
                };
                if (str[2] == 'q')
                    return -1;
                else
                    return 1;

            case 'q':
                flags->quit_flag = true;
                return -1;
        }
        return -1;
    }
    return 0;
}

uint64_t input_move(Board *board, Validcoords *validcoords, Flags *flags) {
    char buf[BUFSIZE];
    int x, y;

    while (1) {
        int res;
        printf("input coords: ");
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            fprintf(stderr, "fgets() failed.\n");
            return 0;
        }
        res = parse_cmd(buf, board, flags);
        if (res == 1)
            continue;
        else if (res == -1)
            return 0;

        y = parse_rowcoord(buf);
        if (y == -1) {
            fprintf(stderr, "Invalid input.\n");
            continue;
        } else if (y == -2) {
            fprintf(stderr, "Out of range.\n");
            continue;
        }

        x = parse_columncoord(buf + 1);
        if (x == -1) {
            fprintf(stderr, "Invalid input.\n");
            continue;
        } else if (x == -2) {
            fprintf(stderr, "Out of range.\n");
            continue;
        }

        if ((validcoords->coords & coord_to_bit(y, x)) == 0) {
            fprintf(stderr, "Cannot put here.\n");
            continue;
        }
        break;
    }
    return coord_to_bit(y, x);
}

void dump_bitmap(Board *board) {
    uint64_t bit = 1;

    flush();
    putchar('\n');
    printf("  a  b  c  d  e  f  g  h\n");
    for (int y = 0; y < YSIZE; y++) {
        putchar('1' + y);
        for (int x = 0; x < XSIZE; x++) {
            if ((board->white & bit) == bit) {
                printf("|●|");
            } else if ((board->black & bit) == bit) {
                printf("|○|");
            } else {
                printf("| |");
            }
            bit <<= 1;
        }
        putchar('\n');
    }
}

void dump_coords(uint64_t coords) {
    int x, y;

    for (y = 0; y < YSIZE; y++) {
        for (x = 0; x < XSIZE; x++) {
            if (coords & 1) {
                printf("%d%c ", y + 1, 'a' + x);
            }
            coords >>= 1;
        }
    }
    printf("\n");
}

int read_map_file(const char *fname, Board *board) {
    char buf[BUFSIZE], row[XSIZE];
    FILE *fp;

    if ((fp = fopen(fname, "r")) == NULL) {
        fprintf(stderr, "fopen() failed.\n");
        return -1;
    }
    fgets(buf, sizeof(buf), fp);
    board->black = 0;
    board->white = 0;

    for (int y = 0; y < YSIZE; y++) {
        fgets(buf, sizeof(buf), fp);
        row[0] = atoi(strtok(buf, ","));
        for (int x = 1; x < XSIZE; x++) {
            row[x] = atoi(strtok(NULL, ","));
        }

        for (int x = 0; x < XSIZE; x++) {
            switch (row[x]) {
                case 0:
                    break;
                case WHITE:
                    board->white |= coord_to_bit(y, x);
                    break;
                case BLACK:
                    board->black |= coord_to_bit(y, x);
                    break;
            }
        }
    }
    fclose(fp);
    return 0;
}

int write_map_file(const char *fname, Board *board) {
    FILE *fp;
    char map[YSIZE][XSIZE];

    if ((fp = fopen(fname, "w")) == NULL) {
        fprintf(stderr, "fopen() failed.\n");
        return -1;
    }

    for (int y = 0; y < YSIZE; y++) {
        for (int x = 0; x < XSIZE; x++) {
            if ((board->white & coord_to_bit(y, x)) == coord_to_bit(y, x)) {
                map[y][x] = WHITE;
            } else if ((board->black & coord_to_bit(y, x)) ==
                       coord_to_bit(y, x)) {
                map[y][x] = BLACK;
            } else {
                map[y][x] = 0;
            }
        }
    }

    fprintf(fp, "%d,%d\n", XSIZE, YSIZE);

    for (int y = 0; y < YSIZE; y++) {
        fprintf(fp, "%d", map[y][0]);
        for (int x = 1; x < XSIZE; x++) {
            fprintf(fp, ",%d", map[y][x]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
    return 0;
}