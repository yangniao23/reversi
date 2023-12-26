#include "tools.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int sx[] = {1, 1, 0, -1, -1, -1, 0, 1};
const int sy[] = {0, 1, 1, 1, 0, -1, -1, -1};

static void hexdump(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%02x ", data[i]);
        if (i % 16 == 15) {
            putchar('\n');
        }
    }
    putchar('\n');
}

void flush(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}