#include "mapmanager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mapio.h"
#include "tools.h"

#define LEFT_SHIFT(x, y) ((y > 0) ? (x << y) : (x >> -y))
#define RIGHT_SHIFT(x, y) ((y > 0) ? (x >> y) : (x << -y))

uint64_t coord_to_bit(int y, int x) {
    uint64_t bit = 1;
    bit <<= (y * XSIZE + x);
    return bit;
}

char *bit_to_coord(char *coord, size_t length, uint64_t bit) {
    if (length < 3) return NULL;
    for (size_t y = 0; y < YSIZE; y++) {
        for (size_t x = 0; x < XSIZE; x++) {
            if (bit & coord_to_bit(y, x)) {
                coord[0] = 'a' + x;
                coord[1] = '1' + y;
                coord[2] = '\0';
                return coord;
            }
        }
    }
    return NULL;
}

Validcoords *get_validcoords(Board *board) {
    int directions[8] = {1,         -1,         XSIZE,     -XSIZE,
                         XSIZE + 1, -XSIZE - 1, XSIZE - 1, -XSIZE + 1};
    uint64_t hsentinel = 0x7e7e7e7e7e7e7e7e;
    uint64_t vsentinel = 0x00ffffffffffff00;
    uint64_t ssentinel = 0x007e7e7e7e7e7e00;
    uint64_t sentinels[8] = {hsentinel, hsentinel, vsentinel, vsentinel,
                             ssentinel, ssentinel, ssentinel, ssentinel};
    uint64_t blank = ~(board->white | board->black);
    Validcoords *validcoords = malloc(sizeof(Validcoords));

    validcoords->coords = 0;
    for (size_t i = 0; i < 8; i++) validcoords->reverse_stones[i] = 0;

    for (size_t i = 0; i < 8; i++) {
        // 返せる可能性がある石だけを取り出す
        uint64_t mask = OPPOSITE_MODE_BOARD(board) & sentinels[i];
        // directionの方向に返せる石があるかどうか
        uint64_t tmp =
            mask & LEFT_SHIFT(CURRENT_MODE_BOARD(board), directions[i]);

        if (tmp == 0) {
            validcoords->reverse_stones[i] = 0;
            continue;
        }

        for (size_t j = 0; j < XSIZE - 3; j++) {
            // 返し続ける
            tmp |= mask & LEFT_SHIFT(tmp, directions[i]);
        }
        validcoords->reverse_stones[i] = tmp;

        validcoords->coords |= blank & LEFT_SHIFT(tmp, directions[i]);
    }

    return validcoords;
}

static uint64_t get_reversed_stones(Validcoords *validcoords, uint64_t put) {
    int directions[8] = {1,         -1,         XSIZE,     -XSIZE,
                         XSIZE + 1, -XSIZE - 1, XSIZE - 1, -XSIZE + 1};
    uint64_t reverse = 0;

    if ((validcoords->coords & put) == 0) return 0;

    for (size_t i = 0; i < 8; i++) {
        uint64_t tmp =
            validcoords->reverse_stones[i] & RIGHT_SHIFT(put, directions[i]);
        for (size_t j = 0; j < XSIZE - 3; j++) {
            tmp |= validcoords->reverse_stones[i] &
                   RIGHT_SHIFT(tmp, directions[i]);
        }
        reverse |= tmp;
    }

    return reverse;
}

uint64_t reverse_stones(Board *board, Validcoords *validcoords, uint64_t put) {
    uint64_t reverse = get_reversed_stones(validcoords, put);

    CURRENT_MODE_BOARD(board) ^= put | reverse;
    OPPOSITE_MODE_BOARD(board) ^= reverse;

    return reverse;
}

Board *copy_board(Board *dst, Board *src) {
    dst->mode = src->mode;
    dst->white = src->white;
    dst->black = src->black;
    return dst;
}