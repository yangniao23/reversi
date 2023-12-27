#include "reversi.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "mapio.h"
#include "mapmanager.h"
#include "tools.h"

static void init(Board *board) {
    board->mode = BLACK;
    board->black = 0x0000000810000000;
    board->white = 0x0000001008000000;
}

static int make_move(Board *board, bool *flag,
                     int16_t prepared_score_matrix[YSIZE][UINT8_MAX]) {
    Flags input_flags = {false};
    Validcoords *validcoords;
    uint64_t put, suggested;

    dump_bitmap(board);

    validcoords = get_validcoords(board);
    if (validcoords->coords == 0) {
        free(validcoords);
        if (*flag) {
            printf("pass\n");
            *flag = false;
            return 0;
        } else {
            printf("game set\n");
            return 1;
        }
    }
    dump_coords(validcoords->coords);
    printf("%s turn\n", board->mode == BLACK ? "BLACK" : "WHITE");

    suggested = search(board, validcoords, prepared_score_matrix);

    printf("suggested: ");
    dump_coords(suggested);

    put = input_move(board, validcoords, &input_flags);
    if (put == 0) {
        if (input_flags.reset_flag) {
            input_flags.reset_flag = false;
            return make_move(board, flag, prepared_score_matrix);
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
    reverse_stones(board, validcoords, put);
    free(validcoords);
    return 0;
}

static int popcount(uint64_t bit) {
    int count = 0;
    while (bit != 0) {
        bit &= bit - 1;
        count++;
    }
    return count;
}

static void show_winner(Board *board) {
    int black = popcount(board->black);
    int white = popcount(board->white);

    printf("BLACK: %d\n", black);
    printf("WHITE: %d\n", white);
    if (black > white) {
        printf("BLACK WIN\n");
    } else if (black < white) {
        printf("WHITE WIN\n");
    } else {
        printf("DRAW\n");
    }
}

int main(int argc, const char *argv[]) {
    Board board;
    bool flag = true;

    int16_t prepared_score_matrix[YSIZE][UINT8_MAX];
    precompute_score_matrix(prepared_score_matrix,
                            (int8_t(*)[XSIZE])SCORE_MATRIX);

    init(&board);
    while (1) {
        int res = make_move(&board, &flag, prepared_score_matrix);
        if (res == -1) {
            fprintf(stderr, "make_move() failed.\n");
            return -1;
        } else if (res == 1) {
            break;
        }

        board.mode *= -1;

        res = make_move(&board, &flag, prepared_score_matrix);
        if (res == -1) {
            fprintf(stderr, "make_move() failed.\n");
            return -1;
        } else if (res == 1) {
            break;
        }
        board.mode *= -1;
    }

    show_winner(&board);
}