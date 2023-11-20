#include "cpu.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int minmaxscore(char map[YSIZE][XSIZE], int y, int x, int mode,
                       int depth, const Validcoord *validcoords,
                       const int score) {
    if (validcoords == NULL) return score;
    
}