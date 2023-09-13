#include "mapmanager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mapio.h"
#include "tools.h"

static Directionlist *append_directionlist(Directionlist *directionlist,
                                           int direction, int num) {
    uint8_t data[sizeof(int) * 2];

    memcpy(data, &direction, sizeof(int));
    memcpy(data + sizeof(int), &num, sizeof(int));

    return (Directionlist *)append_node((Node *)directionlist, data,
                                        sizeof(int) * 2);
}

Directionlist *check(const char map[YSIZE][XSIZE], int y0, int x0, int mode) {
    Directionlist *directions = NULL;
    int x, y;

    if (x0 < 0 || x0 > XSIZE || y0 < 0 || y0 > YSIZE) {
        fprintf(stderr, "Invalid input.\n");
        return NULL;
    }
    if (map[y0][x0] != 0) return NULL;

    for (int i = 0; i < sizeof(sx); i++) {
        int cnt = 0;

        if (x0 + sx[i] < 0 || x0 + sx[i] > XSIZE || y0 + sy[i] < 0 ||
            y0 + sy[i] > YSIZE) {
            continue;
        }
        if (map[y0 + sy[i]][x0 + sx[i]] != -1 * mode) {
            continue;
        }
        x = x0;
        y = y0;
        while (1) {
            x += sx[i];
            y += sy[i];
            if (x < 0 || x > XSIZE || y < 0 || y > YSIZE) {
                break;
            }
            if (map[y][x] == 0) {
                break;
            }
            if (map[y][x] == mode) {
                directions = append_directionlist(directions, i, cnt);
                break;
            }
            cnt++;
        }
    }
    return directions;
}