#pragma once
#include "reversi.h"

typedef struct _board128_t {
    uint64_t black;
    uint64_t white;
} board_128_t;

typedef struct _hashmap_entry_t {
    board_128_t key;
    int value;
    struct _hashmap_entry_t *next;
} hashmap_entry_t;

typedef struct _hashmap_t {
    hashmap_entry_t **buckets;
    int size;
} hashmap_t;

extern hashmap_t *hashmap_create(void);
extern void hashmap_destroy(hashmap_t *hashmap);
extern void hashmap_set(hashmap_t *hashmap, Board *key, int value);
extern int hashmap_get(hashmap_t *hashmap, Board *key);
extern bool hashmap_exist(hashmap_t *hashmap, Board *key);
extern void hashmap_clear(hashmap_t *hashmap);
extern void hashmap_remove(hashmap_t *hashmap, Board *key);
