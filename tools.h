#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct node {
    struct node *next;
    size_t len;
    uint8_t data[];
} Node;

extern const int sx[8];
extern const int sy[8];

extern Node *append_node(Node *node, const uint8_t *data, size_t len);
extern Node *search_node(Node *node, const uint8_t *data, size_t len);
extern void destroy_node(Node *node);

extern void flush(void);