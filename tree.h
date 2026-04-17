#ifndef TREE_H
#define TREE_H

#include "index.h"

typedef struct {
    int mode;
    char type[16];
    char hash[PES_HASH_HEX_SIZE + 1];
    char *name;
} TreeEntry;

typedef struct {
    TreeEntry *entries;
    size_t count;
    size_t capacity;
} Tree;

void tree_init(Tree *tree);
void tree_free(Tree *tree);
int tree_add(Tree *tree, int mode, const char *type, const char *hash, const char *name);
char *tree_serialize(Tree *tree, size_t *out_size);
int tree_parse(const char *data, size_t size, Tree *tree);
int tree_from_index(const Index *index, char out_hash[PES_HASH_HEX_SIZE + 1]);

#endif
