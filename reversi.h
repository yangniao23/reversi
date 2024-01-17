#pragma once

#define XSIZE 8
#define YSIZE 8
#define BUFSIZE 81

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum { WHITE = -1, BLACK = 1 } Mode;
typedef struct {
    uint64_t white;
    uint64_t black;
    Mode mode;
} Board;

typedef struct {
    uint64_t coords;
    uint64_t reverse_stones[8];
} Validcoords;

typedef struct {
    Board *board;
    uint64_t put;
    bool skip_flag;
    bool end_game_flag;
} Input_Result;
