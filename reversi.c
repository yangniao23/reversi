#include "reversi.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "mapio.h"
#include "mapmanager.h"
#include "tools.h"

#define DEPTH 10

typedef struct {
    bool black_auto_flag;
    bool white_auto_flag;
} Auto_Flags;

static void init_board(Board *board) {
    board->mode = BLACK;
    board->black = 0x0000000810000000;
    board->white = 0x0000001008000000;
}

static Auto_Flags *get_playing_mode(void) {
    char buf[BUFSIZE];
    Auto_Flags *auto_flags;

    if ((fgets(buf, sizeof(buf), stdin)) == NULL) {
        fprintf(stderr, "fgets() failed.\n");
        return NULL;
    }
    if ((auto_flags = (Auto_Flags *)malloc(sizeof(Auto_Flags))) == NULL) {
        fprintf(stderr, "malloc() failed.\n");
        return NULL;
    }
    if (strlen(buf) < 2) {
        fprintf(stderr, "invalid input.\n");
        free(auto_flags);
        return NULL;
    }

    switch (buf[0]) {
        case '0':
            auto_flags->black_auto_flag = false;
            auto_flags->white_auto_flag = false;
            break;
        case '1':
            auto_flags->black_auto_flag = true;
            auto_flags->white_auto_flag = false;
            break;
        case '2':
            auto_flags->black_auto_flag = false;
            auto_flags->white_auto_flag = true;
            break;
        case '3':
            auto_flags->black_auto_flag = true;
            auto_flags->white_auto_flag = true;
            break;
        default:
            fprintf(stderr, "invalid input.\n");
            free(auto_flags);
            return NULL;
    }
    return auto_flags;
}

static Input_Result *generate_result(Input_Result *result, Board *board,
                                     uint64_t put, bool skip_flag,
                                     bool end_game_flag) {
    if (result == NULL) return NULL;
    if ((result->board = (Board *)malloc(sizeof(Board))) == NULL) {
        fprintf(stderr, "malloc() failed.\n");
        return NULL;
    }
    memcpy(result->board, board, sizeof(Board));
    result->put = put;
    result->skip_flag = skip_flag;
    result->end_game_flag = end_game_flag;
    return result;
}

static size_t make_move(Input_Result results[], size_t turn_num, Board *board,
                        int16_t prepared_score_matrix[YSIZE][UINT8_MAX],
                        bool auto_flag, bool opposite_cpu_flag) {
    Flags input_flags = {false};
    Validcoords *validcoords;
    uint64_t put, suggested = 0;
    Input_Result *before_input_result = &results[turn_num - 1];
    Input_Result *current_input_result = &results[turn_num];

    printf("before_put: ");
    dump_coords(before_input_result->put);
    dump_bitmap(board);

    validcoords = get_validcoords(board);
    // 最初の2回で coords == 0 にはならないから NULL チェックは不要
    if (validcoords->coords == 0) {
        free(validcoords);
        if (before_input_result->skip_flag) {
            printf("game set\n");
            generate_result(current_input_result, board, 0, true, true);
            return turn_num;

        } else {
            printf("pass\n");
            generate_result(current_input_result, board, 0, true, false);
            return ++turn_num;
        }
    }

    if (auto_flag) {
        put = search(board, validcoords, prepared_score_matrix, DEPTH);
        reverse_stones(board, validcoords, put);
        free(validcoords);
        generate_result(current_input_result, board, put, false, false);
        return ++turn_num;
    }
    printf("validcoords: ");
    dump_coords(validcoords->coords);
    printf("%s turn\n", board->mode == BLACK ? "BLACK" : "WHITE");

    suggested = search(board, validcoords, prepared_score_matrix, DEPTH);

    printf("suggested: ");
    dump_coords(suggested);

    put = input_move(board, validcoords, &input_flags);
    if (put == 0) {
        free(validcoords);
        if (input_flags.reset_flag) {
            return make_move(results, turn_num, board, prepared_score_matrix,
                             auto_flag, opposite_cpu_flag);
        }
        if (input_flags.skip_flag) {
            generate_result(current_input_result, board, 0, false, false);
            return ++turn_num;
        }
        if (input_flags.quit_flag) {
            generate_result(current_input_result, board, 0, false, true);
            return turn_num;
        }  // else
        if (input_flags.undo_flag) {
            if (turn_num < 2 || (opposite_cpu_flag && turn_num < 3)) {
                fprintf(stderr, "undo is not available.\n\n");
                return make_move(results, turn_num, board,
                                 prepared_score_matrix, auto_flag,
                                 opposite_cpu_flag);
            } else {
                if (opposite_cpu_flag) {
                    // i-2手目の入力前に戻すためにi-3手目の盤面をコピーし，modeを反転させる
                    memcpy(board, results[turn_num - 3].board, sizeof(Board));
                    board->mode *= -1;
                    flush();
                    return make_move(results, turn_num - 2, board,
                                     prepared_score_matrix, auto_flag,
                                     opposite_cpu_flag);
                } else {
                    memcpy(board, before_input_result->board, sizeof(Board));
                    generate_result(current_input_result, board, 0, false,
                                    false);
                    return turn_num - 1;
                }
            }
        }
        fprintf(stderr, "input_move() failed.\n");
        return 0;
    }

    reverse_stones(board, validcoords, put);
    free(validcoords);
    generate_result(current_input_result, board, put, false, false);
    return ++turn_num;
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

int main(void) {
    Board board;
    Input_Result results[64 + 1] = {0};  // 最初の結果を results[1] とする
    int16_t prepared_score_matrix[YSIZE][UINT8_MAX];
    size_t i = 1;
    Auto_Flags *auto_flags = NULL;
    precompute_score_matrix(prepared_score_matrix,
                            (int8_t(*)[XSIZE])SCORE_MATRIX);

    init_board(&board);

    do {
        printf("select playing mode\n");
        printf("0: player(black) vs player(white)\n");
        printf("1: cpu(black) vs player(white)\n");
        printf("2: player(black) vs cpu(white)\n");
        printf("3: cpu(black) vs cpu(white)\n");
        printf(">> ");
    } while ((auto_flags = get_playing_mode()) == NULL);

    flush();

    while (1) {
        i = make_move(results, i, &board, prepared_score_matrix,
                      auto_flags->black_auto_flag, auto_flags->white_auto_flag);
        if (i == 0) {
            fprintf(stderr, "make_move() failed.\n");
            return -1;
        } else if (results[i].end_game_flag) {
            break;
        }
        board.mode *= -1;
        flush();

        i = make_move(results, i, &board, prepared_score_matrix,
                      auto_flags->white_auto_flag, auto_flags->black_auto_flag);
        if (i == 0) {
            fprintf(stderr, "make_move() failed.\n");
            return -1;
        } else if (results[i].end_game_flag) {
            break;
        }
        board.mode *= -1;
        flush();
    }

    show_winner(&board);
    free(auto_flags);
}