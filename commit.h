#ifndef COMMIT_H
#define COMMIT_H

#include "tree.h"

typedef struct {
    char tree[PES_HASH_HEX_SIZE + 1];
    char parent[PES_HASH_HEX_SIZE + 1];
    char *author;
    char *committer;
    long long author_time;
    long long commit_time;
    char *message;
} Commit;

void commit_free(Commit *commit);
char *commit_serialize(const Commit *commit, size_t *out_size);
int commit_parse(const char *data, size_t size, Commit *commit);
int head_read(char out_hash[PES_HASH_HEX_SIZE + 1]);
int head_update(const char *hash);
int commit_create(const char *message, char out_hash[PES_HASH_HEX_SIZE + 1]);
int commit_walk(void);

#endif
