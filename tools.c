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

static void dump_node(Node *node) {
    if (node == NULL) return;

    hexdump(node->data, node->len);
}

Node *append_node(Node *node, const uint8_t *data, size_t len) {
    Node *new_node = malloc(sizeof(Node) + len);
    new_node->next = node;
    new_node->len = len;
    memcpy(new_node->data, data, len);
    return new_node;
}
Node *search_node(Node *node, const uint8_t *data, size_t len) {
    for (Node *entry = node; entry; entry = entry->next) {
        if (memcmp(entry->data, data, len) == 0) {
            return entry;
        }
    }
    return NULL;
}
void destroy_node(Node *node) {
    Node *next;
    while (node != NULL) {
        next = node->next;
        free(node);
        node = next;
    }
}

void flush(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}