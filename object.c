#include "pes.h"

static int object_path(const char *hash, char *path, size_t path_size) {
    return snprintf(path, path_size, "%s/%c%c/%s", PES_OBJECTS, hash[0], hash[1], hash + 2) <
           (int)path_size ? 0 : -1;
}

static unsigned char *read_entire_file(const char *path, size_t *out_size) {
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
    unsigned char *buf = malloc((size_t)len + 1);
    if (!buf) die("out of memory");
    if (fread(buf, 1, (size_t)len, fp) != (size_t)len) {
        free(buf);
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    buf[len] = '\0';
    *out_size = (size_t)len;
    return buf;
}

int object_write(ObjectType type, const void *data, size_t size, char out_hash[PES_HASH_HEX_SIZE + 1]) {
    char header[128];
    int header_len = snprintf(header, sizeof(header), "%s %zu", object_type_name(type), size);
    if (header_len < 0 || header_len >= (int)sizeof(header)) return -1;
    header_len++;

    size_t full_size = (size_t)header_len + size;
    unsigned char *full = malloc(full_size);
    if (!full) die("out of memory");
    memcpy(full, header, (size_t)header_len);
    memcpy(full + header_len, data, size);

    unsigned char digest[PES_HASH_BIN_SIZE];
    SHA256(full, full_size, digest);
    bytes_to_hex(digest, out_hash);

    char dir[128];
    snprintf(dir, sizeof(dir), "%s/%c%c", PES_OBJECTS, out_hash[0], out_hash[1]);
    if (mkdir_p(dir) < 0) {
        free(full);
        return -1;
    }

    char path[4096];
    if (object_path(out_hash, path, sizeof(path)) < 0) {
        free(full);
        return -1;
    }
    if (access(path, F_OK) == 0) {
        free(full);
        return 0;
    }

    char tmp_path[4096];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp.%ld", path, (long)getpid());
    int fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0444);
    if (fd < 0) {
        free(full);
        return -1;
    }
    ssize_t written = write(fd, full, full_size);
    if (written != (ssize_t)full_size || fsync(fd) < 0) {
        close(fd);
        unlink(tmp_path);
        free(full);
        return -1;
    }
    if (close(fd) < 0) {
        unlink(tmp_path);
        free(full);
        return -1;
    }
    if (rename(tmp_path, path) < 0) {
        unlink(tmp_path);
        free(full);
        return -1;
    }

    free(full);
    return 0;
}

void *object_read(const char *hash, ObjectType *out_type, size_t *out_size) {
    char path[4096];
    if (object_path(hash, path, sizeof(path)) < 0) return NULL;

    size_t full_size = 0;
    unsigned char *full = read_entire_file(path, &full_size);
    if (!full) return NULL;

    unsigned char digest[PES_HASH_BIN_SIZE];
    char actual[PES_HASH_HEX_SIZE + 1];
    SHA256(full, full_size, digest);
    bytes_to_hex(digest, actual);
    if (strcmp(actual, hash) != 0) {
        free(full);
        errno = EINVAL;
        return NULL;
    }

    size_t header_len = 0;
    while (header_len < full_size && full[header_len] != '\0') header_len++;
    if (header_len == full_size) {
        free(full);
        errno = EINVAL;
        return NULL;
    }

    char type_name[32];
    size_t data_size = 0;
    if (sscanf((char *)full, "%31s %zu", type_name, &data_size) != 2) {
        free(full);
        errno = EINVAL;
        return NULL;
    }
    if (header_len + 1 + data_size != full_size) {
        free(full);
        errno = EINVAL;
        return NULL;
    }

    void *data = malloc(data_size + 1);
    if (!data) die("out of memory");
    memcpy(data, full + header_len + 1, data_size);
    ((unsigned char *)data)[data_size] = '\0';
    free(full);

    if (out_type) *out_type = object_type_from_name(type_name);
    if (out_size) *out_size = data_size;
    return data;
}
