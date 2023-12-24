#include "reversi.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mapio.h"
#include "mapmanager.h"
#include "tools.h"

Validcoord *append_validcoord(Validcoord *validcoord, int y, int x,
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

static void init(Board *board) {
    memset(board, 0, sizeof(Board));
    board->mode = BLACK;
    board->black = 0x0000000810000000;
    board->white = 0x0000001008000000;
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

int find_validcoords(const char map[YSIZE][XSIZE], Validcoord *validcoords,
                     int mode, bool *flag) {
    for (int i = 0; i < YSIZE; i++) {
        for (int j = 0; j < XSIZE; j++) {
            Directionlist *p = check(map, i, j, mode);
            if (p != NULL) {
                validcoords = append_validcoord(validcoords, i, j, p);
                if (flag != NULL) *flag = true;
            }
        }
    }
}

static int make_move(Board *board, bool *flag) {
    int x, y;
    Flags input_flags = {false};

    find_validcoords(map, validcoords, mode, flag);
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
    directions = input_move(map, validcoords, &x, &y, &input_flags);
    if (directions == NULL) {
        if (input_flags.reset_flag) {
            input_flags.reset_flag = false;
            return make_move(map, flag, mode);
        }
        if (input_flags.skip_flag) {
            input_flags.skip_flag = false;
            return 0;
        }
        if (input_flags.quit_flag) {
            input_flags.quit_flag = false;
            return 1;
        } else {
            fprintf(stderr, "input_move() failed.\n");
            return -1;
        }
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
    Board board;
    bool flag = true;
    int winner;

    init(&board);
    while (1) {
        int res = make_move(&board, &flag);
        if (res == -1) {
            fprintf(stderr, "make_move() failed.\n");
            return -1;
        } else if (res == 1) {
            break;
        }

        board.mode *= -1;

        res = make_move(&board, &flag);
        if (res == -1) {
            fprintf(stderr, "make_move() failed.\n");
            return -1;
        } else if (res == 1) {
            break;
        }
        board.mode *= -1;
    }

    winner = check_winner(&board);
    if (winner == BLACK) {
        printf("BLACK WIN\n");
    } else if (winner == WHITE) {
        printf("WHITE WIN\n");
    } else {
        printf("DRAW\n");
    }
}