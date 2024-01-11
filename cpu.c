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
#define CACHE_HIT_BONUS 1000

typedef struct {
    Board *board;
    uint64_t put;
    int move_ordering_value;
} ChildNode;

typedef int (*search_func_t)(Board *, TransposeTables *,
                             int16_t[YSIZE][UINT8_MAX], int, int, int, bool);

typedef void (*store_node_t)(Board *, ChildNode *, uint64_t, int);
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

static inline char *generate_key(Board *board) {
    char *key = (char *)malloc(8 + 8 + 1 + 1);  // black + white + mode + '\0'

    memcpy(key, &(board->black), 8);
    memcpy(key + 8, &(board->white), 8);
    key[16] = (board->mode == BLACK) ? 1 : 0;
    key[17] = '\0';
    return key;
}

static inline int calulate_move_ordering_value(
    TransposeTables *tables, char *key, Board *board,
    int16_t prepared_score_matrix[YSIZE][UINT8_MAX]) {
    int score = 0;

    if (tables->prev == NULL) {
        // 前回の探索がない
        return -evaluate(board, prepared_score_matrix);
    }

    if (zhash_exists(tables->prev->upper, key)) {
        // 前回の探索で上限値が格納された
        score =
            CACHE_HIT_BONUS - *((int *)(zhash_get(tables->prev->upper, key)));
    } else if (zhash_exists(tables->prev->lower, key)) {
        // 前回の探索で下限値が格納された
        score =
            CACHE_HIT_BONUS - *((int *)(zhash_get(tables->prev->lower, key)));
    } else {
        // 枝刈りされた
        score = -evaluate(board, prepared_score_matrix);
    }
    return score;
}

static inline int compare_child_node(const void *a, const void *b) {
    ChildNode *node_a = (ChildNode *)a;
    ChildNode *node_b = (ChildNode *)b;

    if (node_a->move_ordering_value > node_b->move_ordering_value)
        return -1;
    else if (node_a->move_ordering_value < node_b->move_ordering_value)
        return 1;
    else
        return 0;
}

static inline int search_function_process_if_passed(
    Board *board, TransposeTables *tables,
    int16_t precompute_score_matrix[YSIZE][UINT8_MAX], int depth, int alpha,
    int beta, bool is_pass, search_func_t search_func) {
    if (is_pass)
        return evaluate(board, precompute_score_matrix);
    else {
        Board tmp_board = *board;

        tmp_board.mode = -tmp_board.mode;
        return -search_func(&tmp_board, tables, precompute_score_matrix, depth,
                            -beta, -alpha, true);
    }
}

static inline size_t enumerate_nodes(
    Board *board, Validcoords *validcoords, ChildNode *child_nodes,
    TransposeTables *tables, int16_t precomputed_score_matrix[YSIZE][UINT8_MAX],
    store_node_t store_func) {
    uint64_t coords = validcoords->coords;
    size_t child_nodes_count = 0;

    for (size_t y = 0; y < YSIZE; y++) {
        for (size_t x = 0; x < XSIZE; x++, coords >>= 1) {
            if (!(coords & 1)) continue;
            if (child_nodes_count >= CHILD_NODES_MAX) {
                fprintf(stderr, "child_nodes_count >= CHILD_NODES_MAX\n");
                exit(EXIT_FAILURE);
            }

            uint64_t put = coord_to_bit(y, x);
            Board *tmp_board;
            char *key;

            if ((tmp_board = (Board *)malloc(sizeof(Board))) == NULL) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }

            *tmp_board = *board;
            key = generate_key(tmp_board);

            reverse_stones(tmp_board, validcoords, put);

            tmp_board->mode = -tmp_board->mode;

            store_func(tmp_board, &(child_nodes[child_nodes_count]), put,
                       calulate_move_ordering_value(tables, key, tmp_board,
                                                    precomputed_score_matrix));

            child_nodes_count++;
            free(key);
        }
    }
    return child_nodes_count;
}

static inline void store_board_and_ordering_value(Board *board,
                                                  ChildNode *child_node,
                                                  uint64_t unused_put,
                                                  int move_ordering_value) {
    child_node->board = board;
    child_node->move_ordering_value = move_ordering_value;

    UNUSED(unused_put);
}

static inline void store_board_and_put(Board *board, ChildNode *child_node,
                                       uint64_t put,
                                       int unused_move_ordering_value) {
    child_node->board = board;
    child_node->put = put;

    UNUSED(unused_move_ordering_value);
}

static inline void regist_score_to_table(struct ZHashTable *table, char *key,
                                         int score) {
    int *score_ptr = (int *)malloc(sizeof(int));
    *score_ptr = score;
    zhash_set(table, key, score_ptr);
}

static inline void free_child_nodes(ChildNode *child_nodes, size_t length) {
    for (size_t i = 0; i < length; i++) {
        free(child_nodes[i].board);
    }
}

static int nega_alpha_transpose(
    Board *board, TransposeTables *tables,
    int16_t precomputed_score_matrix[YSIZE][UINT8_MAX], int depth, int alpha,
    int beta, bool is_pass) {
    Validcoords *validcoords;
    uint64_t coords;
    int max_score = INT_MIN;
    ChildNode child_nodes[CHILD_NODES_MAX];
    size_t child_nodes_count = 0;
    char *key = generate_key(board);
    int score_upper = beta, score_lower = alpha;

    if (depth == 0) {
        return evaluate(board, precomputed_score_matrix);
    }

    // 置換表に値がある場合はそれを用いる
    if (zhash_exists(tables->upper, key)) {
        score_upper = *((int *)zhash_get(tables->upper, key));
    }
    if (zhash_exists(tables->lower, key)) {
        score_lower = *((int *)zhash_get(tables->lower, key));
    }

    if (score_upper == score_lower) {
        free(key);
        return score_upper;
    }

    validcoords = get_validcoords(board);
    coords = validcoords->coords;
    // パスの場合
    if (coords == 0) {
        free(key);
        free(validcoords);

        return search_function_process_if_passed(
            board, tables, precomputed_score_matrix, depth, alpha, beta,
            is_pass, nega_alpha_transpose);
    }

    // 置換表の値を用いてalphaとbetaの範囲を狭める
    alpha = MAX(alpha, score_lower);
    beta = MIN(beta, score_upper);

    // 探索する
    child_nodes_count = enumerate_nodes(board, validcoords, child_nodes, tables,
                                        precomputed_score_matrix,
                                        store_board_and_ordering_value);
    free(validcoords);

    qsort(child_nodes, child_nodes_count, sizeof(ChildNode),
          compare_child_node);

    for (size_t i = 0; i < child_nodes_count; i++) {
        Board *tmp_board = child_nodes[i].board;
        Validcoords *tmp_validcoords = get_validcoords(tmp_board);
        char *tmp_key = generate_key(tmp_board);
        int score;

        score =
            -nega_alpha_transpose(tmp_board, tables, precomputed_score_matrix,
                                  depth - 1, -beta, -alpha, false);
        free(tmp_validcoords);

        if (score >= beta) {
            if (score > score_lower)
                regist_score_to_table(tables->lower, tmp_key, score);
            free_child_nodes(child_nodes, child_nodes_count);
            return score;
        }
        alpha = MAX(alpha, score);
        max_score = MAX(max_score, score);
        free(tmp_key);
    }

    // alloc and set

    if (max_score < alpha) {
        regist_score_to_table(tables->upper, key, max_score);
    } else {
        regist_score_to_table(tables->lower, key, max_score);
        regist_score_to_table(tables->upper, key, max_score);
    }

    free(key);
    free_child_nodes(child_nodes, child_nodes_count);

    return max_score;
}

static int nega_scout(Board *board, TransposeTables *tables,
                      int16_t precomputed_score_matrix[YSIZE][UINT8_MAX],
                      int depth, int alpha, int beta, bool is_pass) {
    Validcoords *validcoords;
    uint64_t coords;
    int score;
    int max_score = INT_MIN;
    ChildNode child_nodes[CHILD_NODES_MAX];
    size_t child_nodes_count = 0;

    int score_upper = alpha, score_lower = beta;
    char *key = generate_key(board);

    if (depth == 0) {
        return evaluate(board, precomputed_score_matrix);
    }
    // 置換表に値がある場合はそれを用いる
    if (zhash_exists(tables->upper, key)) {
        score_upper = *((int *)zhash_get(tables->upper, key));
    }
    if (zhash_exists(tables->lower, key)) {
        score_lower = *((int *)zhash_get(tables->lower, key));
    }

    if (score_upper == score_lower) {
        free(key);
        return score_upper;
    }

    validcoords = get_validcoords(board);
    coords = validcoords->coords;
    // パスの場合
    if (coords == 0) {
        free(key);
        free(validcoords);

        return search_function_process_if_passed(
            board, tables, precomputed_score_matrix, depth, alpha, beta,
            is_pass, nega_scout);
    }

    // 置換表の値を用いてalphaとbetaの範囲を狭める
    alpha = MAX(alpha, score_lower);
    beta = MIN(beta, score_upper);

    // 探索する
    child_nodes_count = enumerate_nodes(board, validcoords, child_nodes, tables,
                                        precomputed_score_matrix,
                                        store_board_and_ordering_value);

    qsort(child_nodes, child_nodes_count, sizeof(ChildNode),
          compare_child_node);

    // 最善手候補を通常窓で探索
    score = -nega_scout(child_nodes[0].board, tables, precomputed_score_matrix,
                        depth - 1, -beta, -alpha, false);
    if (score >= beta) {
        if (score > score_lower)
            regist_score_to_table(tables->lower, key, score);
        free(validcoords);
        free(key);
        free_child_nodes(child_nodes, child_nodes_count);
        return score;
    }

    alpha = MAX(alpha, score);
    max_score = MAX(max_score, score);

    // 残りの候補を Null Window Search で探索
    for (size_t i = 1; i < child_nodes_count; i++) {
        char *tmp_key = generate_key(child_nodes[i].board);

        score = -nega_alpha_transpose(child_nodes[i].board, tables,
                                      precomputed_score_matrix, depth - 1,
                                      -alpha - 1, -alpha, false);
        if (score >= beta) {
            if (score > score_lower)
                regist_score_to_table(tables->lower, tmp_key, score);
            free(validcoords);
            free(tmp_key);
            free(key);
            free_child_nodes(child_nodes, child_nodes_count);
            return score;
        }
        // child_nodes[0] よりも良い手が見つかったら再探索
        if (score > alpha) {
            alpha = score;
            score = -nega_scout(child_nodes[i].board, tables,
                                precomputed_score_matrix, depth - 1, -beta,
                                -alpha, false);
            if (score >= beta) {
                if (score > score_lower)
                    regist_score_to_table(tables->lower, tmp_key, score);
                free(validcoords);
                free(tmp_key);
                free(key);
                return score;
            }
        }
        alpha = MAX(alpha, score);
        max_score = MAX(max_score, score);
        free(tmp_key);
    }
    if (max_score < alpha) {
        regist_score_to_table(tables->upper, key, max_score);
    } else {
        regist_score_to_table(tables->lower, key, max_score);
        regist_score_to_table(tables->upper, key, max_score);
    }
    free(validcoords);
    free(key);
    free_child_nodes(child_nodes, child_nodes_count);
    return max_score;
}

TransposeTables *update_new_tables(TransposeTables *prev) {
    TransposeTables *tables =
        (TransposeTables *)malloc(sizeof(TransposeTables));
    if (prev != NULL) {
        free_tables(prev->prev);
        prev->prev = NULL;
    }
    tables->prev = prev;
    tables->upper = zcreate_hash_table();
    tables->lower = zcreate_hash_table();

    return tables;
}

void free_tables(TransposeTables *tables) {
    if (tables == NULL) return;
    if (tables->prev != NULL) free_tables(tables->prev);
    zfree_hash_table(tables->upper);
    zfree_hash_table(tables->lower);
    free(tables);
}

uint64_t search(Board *board, Validcoords *validcoords,
                int16_t precompute_score_matrix[YSIZE][UINT8_MAX],
                size_t depth) {
    TransposeTables *tables = update_new_tables(NULL);
    uint64_t max_coord = 0;
    ChildNode child_nodes[CHILD_NODES_MAX];
    size_t child_nodes_count = 0;
    int alpha = -INT_MAX;
    int beta = INT_MAX;
    size_t start_depth = MAX(1, depth - 3);
    char *key = generate_key(board);

    child_nodes_count =
        enumerate_nodes(board, validcoords, child_nodes, tables,
                        precompute_score_matrix, store_board_and_put);
    // 1手ずつ探索
    for (size_t search_depth = start_depth; search_depth <= depth;
         ++search_depth) {
        int score;
        alpha = -INT_MAX;
        beta = INT_MAX;
        for (size_t i = 0; i < child_nodes_count; ++i) {
            char *tmp_key = generate_key(child_nodes[i].board);

            child_nodes[i].move_ordering_value = calulate_move_ordering_value(
                tables, tmp_key, child_nodes[i].board, precompute_score_matrix);
            free(tmp_key);
        }
        qsort(child_nodes, child_nodes_count, sizeof(ChildNode),
              compare_child_node);

        // 最善手候補を通常窓で探索
        score =
            -nega_scout(child_nodes[0].board, tables, precompute_score_matrix,
                        search_depth - 1, -beta, -alpha, false);
        alpha = score;
        max_coord = child_nodes[0].put;

        // 残りの候補を Null Window Search で探索
        for (size_t i = 1; i < child_nodes_count; i++) {
            score = -nega_alpha_transpose(
                child_nodes[i].board, tables, precompute_score_matrix,
                search_depth - 1, -alpha - 1, -alpha, false);
            // child_nodes[0] よりも良い手が見つかったら再探索
            if (score > alpha) {
                alpha = score;
                score = -nega_scout(child_nodes[i].board, tables,
                                    precompute_score_matrix, search_depth - 1,
                                    -beta, -alpha, false);
                max_coord = child_nodes[i].put;
            }
            alpha = MAX(alpha, score);
        }
        update_new_tables(tables);
    }
    free_tables(tables);
    free(key);
    free_child_nodes(child_nodes, child_nodes_count);

    return max_coord;
}
