#ifndef PES_H
#define PES_H

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define PES_DIR ".pes"
#define PES_OBJECTS ".pes/objects"
#define PES_INDEX ".pes/index"
#define PES_HEAD ".pes/HEAD"
#define PES_MAIN_REF ".pes/refs/heads/main"
#define PES_HASH_HEX_SIZE 64
#define PES_HASH_BIN_SIZE 32

typedef enum {
    OBJ_BLOB = 1,
    OBJ_TREE = 2,
    OBJ_COMMIT = 3
} ObjectType;

typedef struct {
    int mode;
    char hash[PES_HASH_HEX_SIZE + 1];
    long long mtime;
    long long size;
    char *path;
} IndexEntry;

typedef struct {
    IndexEntry *entries;
    size_t count;
    size_t capacity;
} Index;

static inline void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

static inline const char *object_type_name(ObjectType type) {
    switch (type) {
    case OBJ_BLOB:
        return "blob";
    case OBJ_TREE:
        return "tree";
    case OBJ_COMMIT:
        return "commit";
    default:
        return "unknown";
    }
}

static inline ObjectType object_type_from_name(const char *name) {
    if (strcmp(name, "blob") == 0) return OBJ_BLOB;
    if (strcmp(name, "tree") == 0) return OBJ_TREE;
    if (strcmp(name, "commit") == 0) return OBJ_COMMIT;
    return 0;
}

static inline void bytes_to_hex(const unsigned char *bytes, char hex[PES_HASH_HEX_SIZE + 1]) {
    static const char table[] = "0123456789abcdef";
    for (int i = 0; i < PES_HASH_BIN_SIZE; i++) {
        hex[i * 2] = table[(bytes[i] >> 4) & 0xf];
        hex[i * 2 + 1] = table[bytes[i] & 0xf];
    }
    hex[PES_HASH_HEX_SIZE] = '\0';
}

static inline int hex_to_bytes(const char *hex, unsigned char bytes[PES_HASH_BIN_SIZE]) {
    for (int i = 0; i < PES_HASH_BIN_SIZE; i++) {
        unsigned int value;
        if (sscanf(hex + i * 2, "%2x", &value) != 1) return -1;
        bytes[i] = (unsigned char)value;
    }
    return 0;
}

static inline int mkdir_p(const char *path) {
    char tmp[4096];
    size_t len = strlen(path);
    if (len >= sizeof(tmp)) return -1;
    strcpy(tmp, path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0777) < 0 && errno != EEXIST) return -1;
            *p = '/';
        }
    }
    if (mkdir(tmp, 0777) < 0 && errno != EEXIST) return -1;
    return 0;
}

static inline char *pes_strdup(const char *s) {
    char *copy = malloc(strlen(s) + 1);
    if (!copy) die("out of memory");
    strcpy(copy, s);
    return copy;
}

static inline const char *pes_author(void) {
    const char *author = getenv("PES_AUTHOR");
    return author && *author ? author : "PES User <pes@localhost>";
}

int object_write(ObjectType type, const void *data, size_t size, char out_hash[PES_HASH_HEX_SIZE + 1]);
void *object_read(const char *hash, ObjectType *out_type, size_t *out_size);

#endif
