#include "cpu.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashmap.h"
#include "mapio.h"
#include "mapmanager.h"
#include "tools.h"

#define CHILD_NODES_MAX 64
#define CACHE_HIT_BONUS 1000

typedef struct {
    Board *board;
    uint64_t put;
    int move_ordering_value;
} ChildNode;

typedef int (*search_func_t)(Board *, TransposeTables **,
                             int16_t[YSIZE][UINT8_MAX], int, int, int, bool);

typedef void (*store_node_t)(Board *, ChildNode *, uint64_t, int);
// https://note.com/nyanyan_cubetech/n/n17c169271832?magazine_key=m54104c8d2f12
const int8_t SCORE_MATRIX[YSIZE][XSIZE] = {
    {30, -12, 0, -1, -1, 0, -12, 30},     {-12, -15, -3, -3, -3, -3, -15, -12},
    {0, -3, 0, -1, -1, 0, -3, 0},         {-1, -3, -1, -1, -1, -1, -3, -1},
    {-1, -3, -1, -1, -1, -1, -3, -1},     {0, -3, 0, -1, -1, 0, -3, 0},
    {-12, -15, -3, -3, -3, -3, -15, -12}, {30, -12, 0, -1, -1, 0, -12, 30}};

unsigned long long visited_nodes = 0;
unsigned long long cache_hitted = 0;

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

static inline int evaluate(Board *board,
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

static inline int get_score_from_table(hashmap_t *table, Board *board) {
    cache_hitted++;
    int res = hashmap_get(table, board);
    return (board->mode == BLACK) ? res : -res;
}

static inline void regist_score_to_table(hashmap_t *table, Board *board,
                                         int score) {
    (board->mode == BLACK) ? hashmap_set(table, board, score)
                           : hashmap_set(table, board, -score);
}

static inline int calulate_move_ordering_value(
    TransposeTables **tables, Board *board,
    int16_t prepared_score_matrix[YSIZE][UINT8_MAX]) {
    int score = 0;

    TransposeTables *before_tables = tables[1];

    if (before_tables == NULL) {
        // 前回の探索がない
        return -evaluate(board, prepared_score_matrix);
    }

    if (hashmap_exist(before_tables->upper, board)) {
        // 前回の探索で上限値が格納された
        score =
            CACHE_HIT_BONUS - get_score_from_table(before_tables->upper, board);
    } else if (hashmap_exist(before_tables->lower, board)) {
        // 前回の探索で下限値が格納された
        score =
            CACHE_HIT_BONUS - get_score_from_table(before_tables->lower, board);
    } else {
        // 枝刈りされた
        score = -evaluate(board, prepared_score_matrix);
    }
    return score;
}

static inline int compare_child_node(const void *a, const void *b) {
    // 降順にソート
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
    Board *board, TransposeTables **tables,
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
    TransposeTables **tables,
    int16_t precomputed_score_matrix[YSIZE][UINT8_MAX],
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
            Board *tmp_board = (Board *)malloc(sizeof(Board));

            if (tmp_board == NULL) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }

            *tmp_board = *board;

            reverse_stones(tmp_board, validcoords, put);

            tmp_board->mode = -tmp_board->mode;

            store_func(tmp_board, &(child_nodes[child_nodes_count]), put,
                       calulate_move_ordering_value(tables, tmp_board,
                                                    precomputed_score_matrix));

            child_nodes_count++;
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

// hashmap には黒目線の値を格納する

static inline void free_child_nodes(ChildNode child_nodes[], size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (child_nodes[i].board != NULL) {
            free(child_nodes[i].board);
            child_nodes[i].board = NULL;
        }
    }
}

static int nega_alpha_transpose(
    Board *board, TransposeTables **tables,
    int16_t precomputed_score_matrix[YSIZE][UINT8_MAX], int depth, int alpha,
    int beta, bool is_pass) {
    Validcoords *validcoords;
    uint64_t coords;
    int max_score = -INT_MAX;
    ChildNode child_nodes[CHILD_NODES_MAX];
    size_t child_nodes_count = 0;
    int score_upper = INT_MAX, score_lower = -INT_MAX;
    TransposeTables *current_tables = tables[0];

    visited_nodes++;

    if (depth == 0) {
        return evaluate(board, precomputed_score_matrix);
    }

    // 置換表に値がある場合はそれを用いる
    if (hashmap_exist(current_tables->upper, board)) {
        score_upper = get_score_from_table(current_tables->upper, board);
        // fprintf(stderr, "nat(): upper cache hit value=%d\n", score_upper);
    }
    if (hashmap_exist(current_tables->lower, board)) {
        score_lower = get_score_from_table(current_tables->lower, board);
        // fprintf(stderr, "nat(): lower cache hit value=%d\n", score_lower);
    }

    if (score_upper == score_lower) {
        // fprintf(stderr, "nat(): minimax value found, alpha=%d\n",
        // score_upper);
        return score_upper;
    }

    validcoords = get_validcoords(board);
    coords = validcoords->coords;
    // パスの場合
    if (coords == 0) {
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
        int score;

        score =
            -nega_alpha_transpose(tmp_board, tables, precomputed_score_matrix,
                                  depth - 1, -beta, -alpha, false);
        free(tmp_validcoords);

        if (score >= beta) {
            // fprintf(stderr, "fall high\n");
            if (score > score_lower)
                regist_score_to_table(current_tables->lower, board, score);
            free_child_nodes(child_nodes, child_nodes_count);
            return score;
        }
        alpha = MAX(alpha, score);
        max_score = MAX(max_score, score);
    }

    if (max_score < alpha) {
        //   fprintf(stderr, "nat(): fall low, alpha=%d\n", alpha);
        regist_score_to_table(current_tables->upper, board, max_score);
    } else {
        //    fprintf(stderr, "nat(): minimax value found, alpha=%d\n", alpha);
        regist_score_to_table(current_tables->lower, board, max_score);
        regist_score_to_table(current_tables->upper, board, max_score);
    }

    free_child_nodes(child_nodes, child_nodes_count);

    return max_score;
}

static int nega_scout(Board *board, TransposeTables **tables,
                      int16_t precomputed_score_matrix[YSIZE][UINT8_MAX],
                      int depth, int alpha, int beta, bool is_pass) {
    Validcoords *validcoords;
    uint64_t coords;
    int score;
    int max_score = -INT_MAX;
    ChildNode child_nodes[CHILD_NODES_MAX];
    size_t child_nodes_count = 0;
    TransposeTables *current_tables = tables[0];

    int score_upper = INT_MAX, score_lower = -INT_MAX;

    visited_nodes++;

    if (depth == 0) {
        return evaluate(board, precomputed_score_matrix);
    }
    // 置換表に値がある場合はそれを用いる
    if (hashmap_exist(current_tables->upper, board)) {
        score_upper = get_score_from_table(current_tables->upper, board);
        //      fprintf(stderr, "ns(): upper cache hit value=%d\n",
        //      score_upper);
    }
    if (hashmap_exist(current_tables->lower, board)) {
        score_lower = get_score_from_table(current_tables->lower, board);
        //    fprintf(stderr, "ns(): lower cache hit value=%d\n", score_lower);
    }

    if (score_upper == score_lower) {
        //     fprintf(stderr, "ns(): minimax value found, alpha=%d\n",
        //     score_upper);
        return score_upper;
    }

    validcoords = get_validcoords(board);
    coords = validcoords->coords;
    // パスの場合
    if (coords == 0) {
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
    free(validcoords);

    qsort(child_nodes, child_nodes_count, sizeof(ChildNode),
          compare_child_node);

    // 最善手候補を通常窓で探索
    score = -nega_scout(child_nodes[0].board, tables, precomputed_score_matrix,
                        depth - 1, -beta, -alpha, false);
    if (score >= beta) {
        //        fprintf(stderr, "fall high\n");
        if (score > score_lower)
            regist_score_to_table(current_tables->lower, board, score);
        free_child_nodes(child_nodes, child_nodes_count);
        return score;
    }

    alpha = MAX(alpha, score);
    max_score = MAX(max_score, score);

    // 残りの候補を Null Window Search で探索
    for (size_t i = 1; i < child_nodes_count; i++) {
        score = -nega_alpha_transpose(child_nodes[i].board, tables,
                                      precomputed_score_matrix, depth - 1,
                                      -alpha - 1, -alpha, false);
        if (score >= beta) {
            if (score > score_lower)
                regist_score_to_table(current_tables->lower, board, score);
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
                    regist_score_to_table(current_tables->lower, board, score);
                free_child_nodes(child_nodes, child_nodes_count);
                return score;
            }
        }
        alpha = MAX(alpha, score);
        max_score = MAX(max_score, score);
    }
    if (max_score < alpha) {
        //   fprintf(stderr, "ns(): fall low, alpha=%d\n", alpha);
        regist_score_to_table(current_tables->lower, board, max_score);
    } else {
        // fprintf(stderr, "ns(): minimax value found, alpha=%d\n", alpha);
        regist_score_to_table(current_tables->lower, board, max_score);
        regist_score_to_table(current_tables->upper, board, max_score);
    }
    free_child_nodes(child_nodes, child_nodes_count);
    return max_score;
}

static void clear_tables(TransposeTables *tables) {
    if (tables == NULL) return;
    hashmap_clear(tables->upper);
    hashmap_clear(tables->lower);
}
static void free_tables(TransposeTables **tables) {
    if (tables == NULL) return;
    for (size_t i = 0; i < 2; i++) {
        hashmap_destroy(tables[i]->upper);
        hashmap_destroy(tables[i]->lower);
        free(tables[i]);
    }
    free(tables);
}

static TransposeTables **create_new_tables(void) {
    TransposeTables **tables;

    tables = (TransposeTables **)malloc(sizeof(TransposeTables *) * 2);

    if (tables == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    tables[0] = (TransposeTables *)malloc(sizeof(TransposeTables));
    tables[1] = (TransposeTables *)malloc(sizeof(TransposeTables));

    if ((tables[0] == NULL) || (tables[1] == NULL)) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < 2; i++) {
        tables[i]->upper = hashmap_create();
        tables[i]->lower = hashmap_create();
    }

    return tables;
}

static void swap_tables(TransposeTables **tables) {
    TransposeTables *tmp = tables[0];

    tables[0] = tables[1];
    tables[1] = tmp;
}

uint64_t search(Board *board, Validcoords *validcoords,
                int16_t precompute_score_matrix[YSIZE][UINT8_MAX],
                size_t depth) {
    visited_nodes = 0;
    cache_hitted = 0;
    TransposeTables **tables;
    uint64_t max_coord = 0;
    ChildNode child_nodes[CHILD_NODES_MAX];
    size_t child_nodes_count = 0;
    int alpha = -INT_MAX;
    int beta = INT_MAX;
    size_t start_depth = MAX(1, depth - 3);

    tables = create_new_tables();

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
            child_nodes[i].move_ordering_value = calulate_move_ordering_value(
                tables, child_nodes[i].board, precompute_score_matrix);
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
        fprintf(stderr, "depth: %zu, nodes: %llu\n", search_depth,
                visited_nodes);
        fprintf(stderr, "cache hit: %llu\n", cache_hitted);
        clear_tables(tables[1]);
        swap_tables(tables);
    }
    free_tables(tables);
    free_child_nodes(child_nodes, child_nodes_count);

    return max_coord;
}
