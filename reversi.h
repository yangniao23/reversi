#pragma once

#define XSIZE 8
#define YSIZE 8
#define BUFSIZE 81
#define WHITE -1
#define BLACK 1

#include <stdbool.h>
#include <stddef.h>

typedef struct directionlist {
    struct directionlist *next;
    size_t len;
    int direction;
    int num;
} Directionlist;

typedef struct validcoord {
    struct validcoord *next;
    size_t len;
    int y;
    int x;
    Directionlist *directionlist;
} Validcoord;

extern Validcoord *append_validcoord(Validcoord *validcoord, int y, int x,
                                     Directionlist *p);
extern Validcoord *search_validcoord(Validcoord *validcoord, int y, int x);
extern void destroy_validcoord(Validcoord *validcoord);
extern int find_validcoords(const char map[YSIZE][XSIZE],
                            Validcoord *validcoords, int mode, bool *flag);