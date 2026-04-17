#ifndef INDEX_H
#define INDEX_H

#include "pes.h"

void index_init(Index *index);
void index_free(Index *index);
int index_load(Index *index);
int index_save(Index *index);
int index_find(const Index *index, const char *path);
int index_add(Index *index, const char *path);
int index_remove(Index *index, const char *path);
void index_status(const Index *index);

#endif
