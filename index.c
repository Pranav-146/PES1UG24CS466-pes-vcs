#include "index.h"

#include <dirent.h>

void index_init(Index *index) {
    index->entries = NULL;
    index->count = 0;
    index->capacity = 0;
}

void index_free(Index *index) {
    for (size_t i = 0; i < index->count; i++) free(index->entries[i].path);
    free(index->entries);
    index_init(index);
}

static void index_reserve(Index *index, size_t needed) {
    if (index->capacity >= needed) return;
    size_t next = index->capacity ? index->capacity * 2 : 16;
    while (next < needed) next *= 2;
    IndexEntry *entries = realloc(index->entries, next * sizeof(IndexEntry));
    if (!entries) die("out of memory");
    index->entries = entries;
    index->capacity = next;
}

static char *normalize_path(const char *path) {
    char *copy = pes_strdup(path);
    for (char *p = copy; *p; p++) {
        if (*p == '\\') *p = '/';
    }
    return copy;
}

static int index_entry_cmp(const void *a, const void *b) {
    const IndexEntry *ea = a;
    const IndexEntry *eb = b;
    return strcmp(ea->path, eb->path);
}

int index_find(const Index *index, const char *path) {
    char *norm = normalize_path(path);
    for (size_t i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, norm) == 0) {
            free(norm);
            return (int)i;
        }
    }
    free(norm);
    return -1;
}

int index_load(Index *index) {
    index_init(index);
    FILE *fp = fopen(PES_INDEX, "r");
    if (!fp) {
        if (errno == ENOENT) return 0;
        return -1;
    }

    char *line = NULL;
    size_t cap = 0;
    ssize_t len;
    while ((len = getline(&line, &cap, fp)) != -1) {
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        char hash[PES_HASH_HEX_SIZE + 1];
        int mode;
        long long mtime, size;
        char path[4096];
        if (sscanf(line, "%o %64s %lld %lld %[^\n]", &mode, hash, &mtime, &size, path) != 5) {
            free(line);
            fclose(fp);
            return -1;
        }
        index_reserve(index, index->count + 1);
        IndexEntry *entry = &index->entries[index->count++];
        entry->mode = mode;
        strcpy(entry->hash, hash);
        entry->mtime = mtime;
        entry->size = size;
        entry->path = normalize_path(path);
    }
    free(line);
    fclose(fp);
    return 0;
}

int index_save(Index *index) {
    qsort(index->entries, index->count, sizeof(IndexEntry), index_entry_cmp);
    if (mkdir_p(PES_DIR) < 0) return -1;

    char tmp_path[128];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp.%ld", PES_INDEX, (long)getpid());
    int fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) return -1;
    FILE *fp = fdopen(fd, "w");
    if (!fp) {
        close(fd);
        unlink(tmp_path);
        return -1;
    }
    for (size_t i = 0; i < index->count; i++) {
        IndexEntry *e = &index->entries[i];
        fprintf(fp, "%06o %s %lld %lld %s\n", e->mode, e->hash, e->mtime, e->size, e->path);
    }
    fflush(fp);
    if (fsync(fd) < 0) {
        fclose(fp);
        unlink(tmp_path);
        return -1;
    }
    if (fclose(fp) != 0) {
        unlink(tmp_path);
        return -1;
    }
    if (rename(tmp_path, PES_INDEX) < 0) {
        unlink(tmp_path);
        return -1;
    }
    return 0;
}

static unsigned char *read_file_data(const char *path, size_t *out_size) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }
    long len = ftell(fp);
    if (len < 0) {
        fclose(fp);
        return NULL;
    }
    rewind(fp);
    unsigned char *buf = malloc((size_t)len);
    if (!buf && len > 0) die("out of memory");
    if (len > 0 && fread(buf, 1, (size_t)len, fp) != (size_t)len) {
        free(buf);
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    *out_size = (size_t)len;
    return buf;
}

int index_add(Index *index, const char *path) {
    struct stat st;
    if (stat(path, &st) < 0 || !S_ISREG(st.st_mode)) return -1;
    size_t size = 0;
    unsigned char *data = read_file_data(path, &size);
    if (!data && size != 0) return -1;

    char hash[PES_HASH_HEX_SIZE + 1];
    if (object_write(OBJ_BLOB, data, size, hash) < 0) {
        free(data);
        return -1;
    }
    free(data);

    char *norm = normalize_path(path);
    int pos = index_find(index, norm);
    IndexEntry *entry;
    if (pos >= 0) {
        entry = &index->entries[pos];
        free(entry->path);
    } else {
        index_reserve(index, index->count + 1);
        entry = &index->entries[index->count++];
    }
    entry->mode = (st.st_mode & S_IXUSR) ? 0100755 : 0100644;
    strcpy(entry->hash, hash);
    entry->mtime = (long long)st.st_mtime;
    entry->size = (long long)st.st_size;
    entry->path = norm;
    return 0;
}

int index_remove(Index *index, const char *path) {
    int pos = index_find(index, path);
    if (pos < 0) return -1;
    free(index->entries[pos].path);
    memmove(&index->entries[pos], &index->entries[pos + 1],
            (index->count - (size_t)pos - 1) * sizeof(IndexEntry));
    index->count--;
    return 0;
}

static int is_indexed(const Index *index, const char *path) {
    return index_find(index, path) >= 0;
}

static void scan_untracked(const Index *index, const char *dir, const char *prefix, int *printed) {
    DIR *dp = opendir(dir);
    if (!dp) return;
    struct dirent *de;
    while ((de = readdir(dp))) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
        if (strcmp(de->d_name, ".pes") == 0 || strcmp(de->d_name, ".git") == 0) continue;
        char fs_path[4096];
        char rel_path[4096];
        snprintf(fs_path, sizeof(fs_path), "%s/%s", dir, de->d_name);
        snprintf(rel_path, sizeof(rel_path), "%s%s", prefix, de->d_name);
        struct stat st;
        if (stat(fs_path, &st) < 0) continue;
        if (S_ISDIR(st.st_mode)) {
            char next_prefix[4096];
            snprintf(next_prefix, sizeof(next_prefix), "%s/", rel_path);
            scan_untracked(index, fs_path, next_prefix, printed);
        } else if (S_ISREG(st.st_mode) && !is_indexed(index, rel_path)) {
            printf("  untracked:  %s\n", rel_path);
            *printed = 1;
        }
    }
    closedir(dp);
}

void index_status(const Index *index) {
    printf("Staged changes:\n");
    if (index->count == 0) {
        printf("  (nothing to show)\n");
    } else {
        for (size_t i = 0; i < index->count; i++) printf("  staged:     %s\n", index->entries[i].path);
    }

    printf("\nUnstaged changes:\n");
    int printed = 0;
    for (size_t i = 0; i < index->count; i++) {
        const IndexEntry *e = &index->entries[i];
        struct stat st;
        if (stat(e->path, &st) < 0) {
            printf("  deleted:    %s\n", e->path);
            printed = 1;
        } else if ((long long)st.st_size != e->size || (long long)st.st_mtime != e->mtime) {
            printf("  modified:   %s\n", e->path);
            printed = 1;
        }
    }
    if (!printed) printf("  (nothing to show)\n");

    printf("\nUntracked files:\n");
    printed = 0;
    scan_untracked(index, ".", "", &printed);
    if (!printed) printf("  (nothing to show)\n");
}
