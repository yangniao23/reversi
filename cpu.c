#include "cpu.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mapio.h"
#include "mapmanager.h"
#include "tools.h"

// https://note.com/nyanyan_cubetech/n/n17c169271832?magazine_key=m54104c8d2f12
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
        uint8_t current_player_row_value =
            ((CURRENT_MODE_BOARD(board) & mask) >> (y * XSIZE));
        uint8_t opposite_player_row_value =
            ((OPPOSITE_MODE_BOARD(board) & mask) >> (y * XSIZE));

        score += prepared_score_matrix[y][current_player_row_value] -
                 prepared_score_matrix[y][opposite_player_row_value];
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
            int score;
            reverse_stones(&tmp_board, validcoords, put);
            score = evaluate(&tmp_board, prepared_score_matrix);

            if (score > max_score) {
                max_score = score;
                max_coords = put;
            }
        }
    }
    return max_coords;
}

int negamax(uint64_t *return_coord, Board *board, Validcoords *validcoords,
            int16_t prepared_score_matrix[YSIZE][UINT8_MAX], int depth,
            bool is_pass) {
    uint64_t coords = validcoords->coords;
    uint64_t max_coord = 0;
    int16_t max_score = SHRT_MIN;

    if (depth == 0) {
        return evaluate(board, prepared_score_matrix);
    }

    if (coords == 0) {
        if (is_pass) {
            return evaluate(board, prepared_score_matrix);
        } else {
            Board tmp_board = *board;
            Validcoords *tmp_validcoords;

            tmp_board.mode = -tmp_board.mode;
            tmp_validcoords = get_validcoords(&tmp_board);
            return -negamax(NULL, &tmp_board, tmp_validcoords,
                            prepared_score_matrix, depth, true);
        }
    }

    for (size_t y = 0; y < YSIZE; y++) {
        for (size_t x = 0; x < XSIZE; x++, coords >>= 1) {
            if (!(coords & 1)) continue;

            uint64_t put = coord_to_bit(y, x);
            Board tmp_board = *board;
            Validcoords *tmp_validcoords;
            int score;
            // char coord[3];

            reverse_stones(&tmp_board, validcoords, put);

            tmp_board.mode = -tmp_board.mode;
            tmp_validcoords = get_validcoords(&tmp_board);
            /*
            for (int i = 3 - depth; i > 0; i--) putchar('\t');
            printf("call negamax() (mode=%s, coord=%s, depth=%d)\n",
                   (tmp_board.mode == BLACK) ? "BLACK" : "WHITE",
                   bit_to_coord(coord, 3, put), depth - 1);
            */
            score = -negamax(NULL, &tmp_board, tmp_validcoords,
                             prepared_score_matrix, depth - 1, false);
            /*
                        for (int i = 3 - depth; i > 0; i--) putchar('\t');
                        printf("return negamax() (coord=%s, score=%d)\n",
                               bit_to_coord(coord, 3, put), score);
            */
            free(tmp_validcoords);

            if (score > max_score) {
                max_score = score;
                max_coord = put;
            }
        }
    }

    if (return_coord != NULL) *return_coord = max_coord;
    return max_score;
}

int negaalpha(uint64_t *return_coord, Board *board, Validcoords *validcoords,
              int16_t prepared_score_matrix[YSIZE][UINT8_MAX], int depth,
              int alpha, int beta, bool is_pass) {
    uint64_t coords = validcoords->coords;
    uint64_t max_coord = 0;
    int16_t max_score = SHRT_MIN;

    if (depth == 0) {
        return evaluate(board, prepared_score_matrix);
    }

    if (coords == 0) {
        if (is_pass) {
            return evaluate(board, prepared_score_matrix);
        } else {
            Board tmp_board = *board;
            Validcoords *tmp_validcoords;

            tmp_board.mode = -tmp_board.mode;
            tmp_validcoords = get_validcoords(&tmp_board);
            return -negaalpha(NULL, &tmp_board, tmp_validcoords,
                              prepared_score_matrix, depth, -beta, -alpha,
                              true);
        }
    }

    for (size_t y = 0; y < YSIZE; y++) {
        for (size_t x = 0; x < XSIZE; x++, coords >>= 1) {
            if (!(coords & 1)) continue;

            uint64_t put = coord_to_bit(y, x);
            Board tmp_board = *board;
            Validcoords *tmp_validcoords;
            int score;
            // char coord[3];

            reverse_stones(&tmp_board, validcoords, put);

            tmp_board.mode = -tmp_board.mode;
            tmp_validcoords = get_validcoords(&tmp_board);
            /*
            for (int i = 3 - depth; i > 0; i--) putchar('\t');
            printf("call negaalpha() (mode=%s, coord=%s, depth=%d)\n",
                   (tmp_board.mode == BLACK) ? "BLACK" : "WHITE",
                   bit_to_coord(coord, 3, put), depth - 1);
            */
            score = -negaalpha(NULL, &tmp_board, tmp_validcoords,
                               prepared_score_matrix, depth - 1, -beta, -alpha,
                               false);
            /*
                        for (int i = 3 - depth; i > 0; i--) putchar('\t');
                        printf("return negaalpha() (coord=%s, score=%d)\n",
                               bit_to_coord(coord, 3, put), score);
            */
            free(tmp_validcoords);

            if (score >= beta) return score;
            alpha = MAX(alpha, score);
            max_score = MAX(max_score, score);
            max_coord = (MAX(max_score, score) == score) ? put : max_coord;
        }
    }

    if (return_coord != NULL) *return_coord = max_coord;
    return max_score;
}
