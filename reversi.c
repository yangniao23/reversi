#include "reversi.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mapio.h"
#include "tools.h"

static Validcoord *append_validcoord(Validcoord *validcoord, int y, int x,
                                     Directionlist *p) {
    uint8_t data[2 * sizeof(int) + sizeof(Directionlist *)];

    memcpy(data, &y, sizeof(int));
    memcpy(data + sizeof(int), &x, sizeof(int));
    memcpy(data + 2 * sizeof(int), &p, sizeof(Directionlist *));

    return (Validcoord *)append_node((Node *)validcoord, data, sizeof(data));
}

Validcoord *search_validcoord(Validcoord *validcoord, int y, int x) {
    uint8_t data[2 * sizeof(int)];

    memcpy(data, &y, sizeof(int));
    memcpy(data + sizeof(int), &x, sizeof(int));

    return (Validcoord *)search_node((Node *)validcoord, data, 2 * sizeof(int));
}

void destroy_validcoord(Validcoord *validcoord) {
    Validcoord *next;
    while (validcoord != NULL) {
        next = validcoord->next;
        destroy_node((Node *)validcoord->directionlist);
        free(validcoord);
        validcoord = next;
    }
}

static void init(char map[YSIZE][XSIZE]) {
    for (int i = 0; i < YSIZE; i++) {
        for (int j = 0; j < XSIZE; j++) {
            map[i][j] = 0;
        }
    }
    map[YSIZE / 2 - 1][XSIZE / 2 - 1] = WHITE;
    map[YSIZE / 2 - 1][XSIZE / 2] = BLACK;
    map[YSIZE / 2][XSIZE / 2 - 1] = BLACK;
    map[YSIZE / 2][XSIZE / 2] = WHITE;

    dump_map(map);
}

static void update_map(char map[YSIZE][XSIZE], Directionlist *directions, int y,
                       int x, int mode) {
    for (Directionlist *entry = directions; entry != NULL;
         entry = entry->next) {
        int direction = entry->direction;
        int num = entry->num;
        for (int i = 0; i <= num; i++) {
            map[y + sy[direction] * i][x + sx[direction] * i] = mode;
        }
    }
    dump_map(map);
}

static int make_move(char map[YSIZE][XSIZE], bool *flag, int mode) {
    Validcoord *validcoords = NULL;
    Directionlist *directions = NULL;
    int x, y;

    for (int i = 0; i < YSIZE; i++) {
        for (int j = 0; j < XSIZE; j++) {
            Directionlist *p = check(map, i, j, mode);
            if (p != NULL) {
                validcoords = append_validcoord(validcoords, i, j, p);
                *flag = true;
            }
        }
    }

    if (validcoords == NULL) {
        if (*flag) {
            printf("pass\n");
            *flag = false;
            return 0;
        } else {
            printf("game set\n");
            return 1;
        }
    }

    if (mode == BLACK) {
        printf("BLACK turn\n");
    } else {
        printf("WHITE turn\n");
    }
    directions = input_move(validcoords, &x, &y);
    if (directions == NULL) {
        fprintf(stderr, "input_move() failed.\n");
        return -1;
    }
    update_map(map, directions, y, x, mode);
    destroy_validcoord(validcoords);
    return 0;
}

int check_winner(char map[YSIZE][XSIZE]) {
    int cnt;
    for (int y = 0; y < YSIZE; y++) {
        for (int x = 0; x < XSIZE; x++) {
            cnt += map[y][x];
        }
    }
    if (cnt > 0) {
        return BLACK;
    } else if (cnt < 0) {
        return WHITE;
    } else {
        return 0;
    }
}

int main(int argc, const char *argv[]) {
    char map[YSIZE][XSIZE];
    bool flag = true;
    int winner;

    init(map);
    while (flag) {
        int res = make_move(map, &flag, BLACK);
        if (res == -1) {
            fprintf(stderr, "make_move() failed.\n");
            return -1;
        } else if (flag == false && res == 1) {
            break;
        }

        if (make_move(map, &flag, WHITE) == -1) {
            fprintf(stderr, "make_move() failed.\n");
            return -1;
        }
    }

    winner = check_winner(map);
    if (winner == BLACK) {
        printf("BLACK WIN\n");
    } else if (winner == WHITE) {
        printf("WHITE WIN\n");
    } else {
        printf("DRAW\n");
    }
}