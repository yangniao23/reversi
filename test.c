#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define XSIZE 8
#define YSIZE 8
#define BUFSIZE 81

typedef enum { WHITE = -1, BLACK = 1 } Mode;
typedef struct {
    uint64_t black;
    uint64_t white;
    Mode mode;
} Board;

static uint64_t make_validcoords(Board *board) {
    uint64_t directions[8] = {1,         -1,         XSIZE,     -XSIZE,
                              XSIZE + 1, -XSIZE - 1, XSIZE - 1, -XSIZE + 1};
    uint64_t hsentinel = 0x7e7e7e7e7e7e7e7e;
    uint64_t vsentinel = 0x00ffffffffffff00;
    uint64_t ssentinel = 0x007e7e7e7e7e7e00;
    uint64_t sentinels[8] = {hsentinel, hsentinel, vsentinel, vsentinel,
                             ssentinel, ssentinel, ssentinel, ssentinel};
    uint64_t blank = ~(board->white | board->black);
    uint64_t validcoords = 0;

    for (size_t i = 0; i < 8; i++) {
        uint64_t player = (board->mode == WHITE) ? board->white : board->black;
        uint64_t opponent =
            (board->mode == WHITE) ? board->black : board->white;
        uint64_t mask = opponent & sentinels[i];
        uint64_t tmp = mask & (player << directions[i]);

        if (tmp == 0) continue;

        for (size_t j = 0; j < XSIZE - 3; j++) {
            tmp |= sentinels[i] & (tmp << directions[i]);
        }
        validcoords |= blank & (tmp << directions[i]);
    }

    return validcoords;
}

void dump_bitmap(Board *board) {
    uint64_t bit = 1;

    // flush();
    putchar('\n');
    printf("  a  b  c  d  e  f  g  h\n");
    for (int y = 0; y < YSIZE; y++) {
        putchar('1' + y);
        for (int x = 0; x < XSIZE; x++) {
            if ((board->white & bit) == bit) {
                printf("|●|");
            } else if ((board->black & bit) == bit) {
                printf("|○|");
            } else {
                printf("| |");
            }
            bit <<= 1;
        }
        putchar('\n');
    }
}

bool is_validcoord(Board *board, uint64_t put) {
    uint64_t validcoords = make_validcoords(board);

    return (validcoords & put) == put;
}

void update_board(Board *board, uint64_t put) {
    if (board->mode == WHITE) {
        board->white |= put;
        board->black |= (put & make_validcoords(board));
    } else {
        board->black |= put;
        board->white |= (put & make_validcoords(board));
    }
}

uint64_t coord_to_bit(int y, int x) {
    uint64_t bit = 1;
    bit <<= (y * XSIZE + x);
    return bit;
}

int main(void) {
    Board board = {0x0000000810000000, 0x0000001008000000, BLACK};
    dump_bitmap(&board);
    uint64_t validcoords = make_validcoords(&board);
    uint64_t put = coord_to_bit(3, 2);
    printf("%lx\n", validcoords);
    printf("%lx\n", put);

    return 0;
}