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
        // dump_coords(blank & LEFT_SHIFT(tmp, directions[i]));

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
