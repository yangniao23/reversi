#include "tools.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void hexdump(const uint8_t *data, size_t len) {
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