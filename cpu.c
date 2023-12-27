#include "cpu.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mapio.h"
#include "mapmanager.h"

const int8_t SCORE_MATRIX[YSIZE][XSIZE] = {
    {30, -12, 0, -1, -1, 0, -12, 30},     {-12, -15, -3, -3, -3, -3, -15, -12},
    {0, -3, 0, -1, -1, 0, -3, 0},         {-1, -3, -1, -1, -1, -1, -3, -1},
    {-1, -3, -1, -1, -1, -1, -3, -1},     {0, -3, 0, -1, -1, 0, -3, 0},
    {-12, -15, -3, -3, -3, -3, -15, -12}, {30, -12, 0, -1, -1, 0, -12, 30}};

void precompute_score_matrix(int16_t dst[YSIZE][UINT8_MAX],
                             int8_t score_matrix[YSIZE][XSIZE]) {
    // score_matrix の組み合わせを全て試す
    for (size_t y = 0; y < YSIZE; y++) {
        for (uint8_t bit = 0; bit < UINT8_MAX; bit++) {
            int16_t score = 0;

            for (size_t x = 0; x < XSIZE; x++) {
                if (bit & (1 << x)) {
                    score += score_matrix[y][x];
                }
            }
            dst[y][bit] = score;
        }
    }
}

static int evaluate(Board *board,
                    int16_t prepared_score_matrix[YSIZE][UINT8_MAX]) {
    int score = 0;
    for (size_t y = 0; y < YSIZE; y++) {
        uint64_t mask = ((uint64_t)0xff << (y * XSIZE));
        uint8_t value = ((CURRENT_MODE_BOARD(board) & mask) >> (y * XSIZE));

        score += prepared_score_matrix[y][value];
    }
    return score;
}

uint64_t search(Board *board, Validcoords *validcoords,
                int16_t prepared_score_matrix[YSIZE][UINT8_MAX]) {
    int16_t max_score = SHRT_MIN;
    uint64_t max_coords = 0;
    uint64_t coords = validcoords->coords;

    for (size_t y = 0; y < YSIZE; y++) {
        for (size_t x = 0; x < XSIZE; x++, coords >>= 1) {
            if (!(coords & 1)) continue;

            uint64_t put = coord_to_bit(y, x);
            Board tmp_board = *board;
            reverse_stones(&tmp_board, validcoords, put);
            int16_t score = evaluate(&tmp_board, prepared_score_matrix);

            if (score > max_score) {
                max_score = score;
                max_coords = put;
            }
        }
    }
    return max_coords;
}