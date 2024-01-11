#include "evaluate.h"

#include <stdio.h>

#include "mapmanager.h"
#include "tools.h"

#define SCORE_MAX 6400

static inline int evaluate_upper_side(uint8_t upper_side) {
    int score = 0;
    return score;
}

static inline int calculate_side_evaluation_value(Board *board) {
    uint64_t current_mode_board = CURRENT_MODE_BOARD(board);
    uint8_t upper_side = current_mode_board & 0xff;
    uint8_t lower_side = (current_mode_board >> 56) & 0xff;
    uint8_t left_side = 0, right_side = 0;

    for (size_t i = 0; i < YSIZE; i++) {
        left_side |= ((current_mode_board >> (i * XSIZE)) & 0x01) << i;
        right_side |= ((current_mode_board >> (i * XSIZE + 7)) & 0x01) << i;
    }
}

int evaluate(Board *board) {}