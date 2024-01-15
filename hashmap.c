#include "hashmap.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASHMAP_SIZE 32779  // 衝突を避けるために素数にする

static hashmap_t *hashmap_create_with_size(int size);
static size_t convert_key_to_index(Board *key);

static hashmap_t *hashmap_create_with_size(int size) {
    hashmap_t *hashmap = malloc(sizeof(hashmap_t));
    if (hashmap == NULL) {
        fprintf(stderr, "malloc() failed.\n");
        return NULL;
    }
    hashmap->size = size;
    hashmap->buckets = malloc(sizeof(hashmap_entry_t *) * size);
    if (hashmap->buckets == NULL) {
        fprintf(stderr, "malloc() failed.\n");
        return NULL;
    }
    for (int i = 0; i < size; i++) hashmap->buckets[i] = NULL;
    return hashmap;
}

static size_t convert_key_to_index(Board *key) {
    size_t hash = 0;

    hash = key->black;
    hash = (hash << 5) - hash + key->white;
    return hash % HASHMAP_SIZE;
}

hashmap_t *hashmap_create(void) {
    return hashmap_create_with_size(HASHMAP_SIZE);
}

void hashmap_destroy(hashmap_t *hashmap) {
    if (hashmap == NULL) return;
    for (int i = 0; i < hashmap->size; i++) {
        hashmap_entry_t *entry = hashmap->buckets[i];
        while (entry != NULL) {
            hashmap_entry_t *next = entry->next;
            free(entry);
            entry = next;
        }
    }
    free(hashmap->buckets);
    free(hashmap);
}

void hashmap_set(hashmap_t *hashmap, Board *key, int value) {
    size_t hash = convert_key_to_index(key);
    hashmap_entry_t *entry = hashmap->buckets[hash];
    board_128_t board_128 = {key->black, key->white};

    while (entry != NULL) {
        if (memcmp(&board_128, &entry->key, sizeof(board_128_t)) == 0) {
            entry->value = value;
            return;
        }
        entry = entry->next;
    }
    entry = malloc(sizeof(hashmap_entry_t));
    if (entry == NULL) {
        fprintf(stderr, "malloc() failed.\n");
        return;
    }
    entry->key = board_128;
    entry->value = value;
    entry->next = hashmap->buckets[hash];
    hashmap->buckets[hash] = entry;
}

int hashmap_get(hashmap_t *hashmap, Board *key) {
    size_t hash = convert_key_to_index(key);
    hashmap_entry_t *entry = hashmap->buckets[hash];
    board_128_t board_128 = {key->black, key->white};

    while (entry != NULL) {
        if (memcmp(&board_128, &(entry->key), sizeof(board_128_t)) == 0)
            return entry->value;
        entry = entry->next;
    }
    // 見つからなかった場合は INT_MIN を返す
    return -INT_MAX;
}

bool hashmap_exist(hashmap_t *hashmap, Board *key) {
    size_t hash = convert_key_to_index(key);
    hashmap_entry_t *entry = hashmap->buckets[hash];
    board_128_t board_128 = {key->black, key->white};

    while (entry != NULL) {
        if (memcmp(&board_128, &(entry->key), sizeof(board_128_t)) == 0)
            return true;
        entry = entry->next;
    }
    return false;
}

void hashmap_clear(hashmap_t *hashmap) {
    for (int i = 0; i < hashmap->size; i++) {
        hashmap_entry_t *entry = hashmap->buckets[i];
        while (entry != NULL) {
            hashmap_entry_t *next = entry->next;
            free(entry);
            entry = next;
        }
        hashmap->buckets[i] = NULL;
    }
}

void hashmap_remove(hashmap_t *hashmap, Board *key) {
    size_t hash = convert_key_to_index(key);
    hashmap_entry_t *entry = hashmap->buckets[hash];
    hashmap_entry_t *prev = NULL;
    board_128_t board_128 = {key->black, key->white};

    while (entry != NULL) {
        if (memcmp(&board_128, &(entry->key), sizeof(board_128_t)) == 0) {
            if (prev == NULL) {
                hashmap->buckets[hash] = entry->next;
            } else {
                prev->next = entry->next;
            }
            free(entry);
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}
