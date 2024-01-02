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
#include "zhash.h"

#define CHILD_NODES_MAX 16384
#define CACHE_HIT_BONUS 100

typedef struct {
    Board *board;
    int move_ordering_value;
} ChildNode;

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
    uint8_t *current_player_row_value = (uint8_t *)&(CURRENT_MODE_BOARD(board));
    uint8_t *opposite_player_row_value =
        (uint8_t *)&(OPPOSITE_MODE_BOARD(board));

    for (size_t y = 0; y < YSIZE; y++) {
        score += prepared_score_matrix[y][current_player_row_value[y]] -
                 prepared_score_matrix[y][opposite_player_row_value[y]];
    }
    return score;
}

static char *generate_key(Board *board) {
    char *key = (char *)malloc(16 + 1);  // 128 bit + '\0'

    memcpy(key, &(board->black), 8);
    memcpy(key + 8, &(board->white), 8);
    key[16] = '\0';
    return key;
}

static int calulate_move_ordering_value(
    struct ZHashTable *transpose_table, char *key, ChildNode *child_node,
    int16_t prepared_score_matrix[YSIZE][UINT8_MAX]) {
    int score = 0;

    if (zhash_exists(transpose_table, key)) {
        score = CACHE_HIT_BONUS + *((int *)(zhash_get(transpose_table, key)));
    } else {
        score = -evaluate(child_node->board, prepared_score_matrix);
    }

    return score;
}

static int compare_child_node(const void *a, const void *b) {
    ChildNode *child_node_a = (ChildNode *)a;
    ChildNode *child_node_b = (ChildNode *)b;

    return child_node_b->move_ordering_value -
           child_node_a->move_ordering_value;
}

static int nega_alpha_transpose(
    Board *board, Validcoords *validcoords,
    int16_t precompute_score_matrix[YSIZE][UINT8_MAX], int depth, int alpha,
    int beta, bool is_pass) {
    static struct ZHashTable *transpose_table = NULL;
    uint64_t coords = validcoords->coords;
    int16_t max_score = SHRT_MIN;
    ChildNode child_nodes[CHILD_NODES_MAX];
    size_t child_nodes_count = 0;
    char *key;
    int *max_score_ptr;

    if (transpose_table == NULL) {
        transpose_table = zcreate_hash_table();
    }

    if (depth == 0) {
        return evaluate(board, precompute_score_matrix);
    }

    if (coords == 0) {
        if (is_pass)
            return evaluate(board, precompute_score_matrix);
        else {
            Board tmp_board = *board;
            Validcoords *tmp_validcoords;

            tmp_board.mode = -tmp_board.mode;
            tmp_validcoords = get_validcoords(&tmp_board);
            return -nega_alpha_transpose(&tmp_board, tmp_validcoords,
                                         precompute_score_matrix, depth, -beta,
                                         -alpha, true);
        }
    }

    key = generate_key(board);

    if (zhash_exists(transpose_table, key)) {
        return *((int *)zhash_get(transpose_table, key));
    }

    for (size_t y = 0; y < YSIZE; y++) {
        for (size_t x = 0; x < XSIZE; x++, coords >>= 1) {
            if (!(coords & 1)) continue;

            uint64_t put = coord_to_bit(y, x);
            Board tmp_board = *board;
            // char coord[3];

            reverse_stones(&tmp_board, validcoords, put);

            tmp_board.mode = -tmp_board.mode;

            child_nodes[child_nodes_count].board = &tmp_board;
            child_nodes[child_nodes_count].move_ordering_value =
                calulate_move_ordering_value(transpose_table, key,
                                             &(child_nodes[child_nodes_count]),
                                             precompute_score_matrix);
            child_nodes_count++;
        }
    }

    qsort(child_nodes, child_nodes_count, sizeof(ChildNode),
          compare_child_node);

    for (size_t i = 0; i < child_nodes_count; i++) {
        Board *tmp_board = child_nodes[i].board;
        Validcoords *tmp_validcoords = get_validcoords(tmp_board);
        int score;

        score = -nega_alpha_transpose(tmp_board, tmp_validcoords,
                                      precompute_score_matrix, depth - 1, -beta,
                                      -alpha, false);
        free(tmp_validcoords);

        if (score >= beta) return score;
        alpha = MAX(alpha, score);
        max_score = MAX(max_score, score);
    }

    // alloc and set
    max_score_ptr = (int *)malloc(sizeof(int));
    *max_score_ptr = max_score;

    zhash_set(transpose_table, key, max_score_ptr);

    free(key);

    return max_score;
}

uint64_t search(Board *board, Validcoords *validcoords,
                int16_t prepared_score_matrix[YSIZE][UINT8_MAX], int depth) {
    uint64_t alpha_coord = 0;
    uint64_t coords = validcoords->coords;
    int alpha = -INT_MAX;
    int beta = INT_MAX;

    for (size_t y = 0; y < YSIZE; y++) {
        for (size_t x = 0; x < XSIZE; x++, coords >>= 1) {
            if (!(coords & 1)) continue;

            int score;
            uint64_t put = coord_to_bit(y, x);
            Board tmp_board = *board;

            reverse_stones(&tmp_board, validcoords, put);
            score = -nega_alpha_transpose(&tmp_board, validcoords,
                                          prepared_score_matrix, depth, -beta,
                                          -alpha, false);

            if (score > alpha) {
                alpha = score;
                alpha_coord = put;
            }
        }
    }

    return alpha_coord;
}
