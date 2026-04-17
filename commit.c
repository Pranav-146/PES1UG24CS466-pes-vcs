#include "commit.h"

void commit_free(Commit *commit) {
    free(commit->author);
    free(commit->committer);
    free(commit->message);
    memset(commit, 0, sizeof(*commit));
}

char *commit_serialize(const Commit *commit, size_t *out_size) {
    int needed = snprintf(NULL, 0,
                          "tree %s\n%s%s%sauthor %s %lld\ncommitter %s %lld\n\n%s\n",
                          commit->tree,
                          commit->parent[0] ? "parent " : "",
                          commit->parent[0] ? commit->parent : "",
                          commit->parent[0] ? "\n" : "",
                          commit->author, commit->author_time,
                          commit->committer, commit->commit_time,
                          commit->message);
    char *buf = malloc((size_t)needed + 1);
    if (!buf) die("out of memory");
    snprintf(buf, (size_t)needed + 1,
             "tree %s\n%s%s%sauthor %s %lld\ncommitter %s %lld\n\n%s\n",
             commit->tree,
             commit->parent[0] ? "parent " : "",
             commit->parent[0] ? commit->parent : "",
             commit->parent[0] ? "\n" : "",
             commit->author, commit->author_time,
             commit->committer, commit->commit_time,
             commit->message);
    *out_size = (size_t)needed;
    return buf;
}

int commit_parse(const char *data, size_t size, Commit *commit) {
    memset(commit, 0, sizeof(*commit));
    char *copy = malloc(size + 1);
    if (!copy) die("out of memory");
    memcpy(copy, data, size);
    copy[size] = '\0';

    char *message = strstr(copy, "\n\n");
    if (!message) {
        free(copy);
        return -1;
    }
    *message = '\0';
    message += 2;
    commit->message = pes_strdup(message);
    size_t msg_len = strlen(commit->message);
    if (msg_len && commit->message[msg_len - 1] == '\n') commit->message[msg_len - 1] = '\0';

    char *save = NULL;
    for (char *line = strtok_r(copy, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
        if (strncmp(line, "tree ", 5) == 0) {
            snprintf(commit->tree, sizeof(commit->tree), "%s", line + 5);
        } else if (strncmp(line, "parent ", 7) == 0) {
            snprintf(commit->parent, sizeof(commit->parent), "%s", line + 7);
        } else if (strncmp(line, "author ", 7) == 0) {
            char *last = strrchr(line, ' ');
            if (!last) continue;
            commit->author_time = atoll(last + 1);
            *last = '\0';
            commit->author = pes_strdup(line + 7);
        } else if (strncmp(line, "committer ", 10) == 0) {
            char *last = strrchr(line, ' ');
            if (!last) continue;
            commit->commit_time = atoll(last + 1);
            *last = '\0';
            commit->committer = pes_strdup(line + 10);
        }
    }
    free(copy);
    return commit->tree[0] && commit->author && commit->committer ? 0 : -1;
}

int head_read(char out_hash[PES_HASH_HEX_SIZE + 1]) {
    FILE *fp = fopen(PES_HEAD, "r");
    if (!fp) return -1;
    char line[256];
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    line[strcspn(line, "\n")] = '\0';

    if (strncmp(line, "ref: ", 5) == 0) {
        char ref_path[4096];
        snprintf(ref_path, sizeof(ref_path), ".pes/%s", line + 5);
        FILE *rf = fopen(ref_path, "r");
        if (!rf) return -1;
        if (!fgets(out_hash, PES_HASH_HEX_SIZE + 1, rf)) {
            fclose(rf);
            return -1;
        }
        fclose(rf);
        out_hash[strcspn(out_hash, "\n")] = '\0';
        return out_hash[0] ? 0 : -1;
    }
    snprintf(out_hash, PES_HASH_HEX_SIZE + 1, "%s", line);
    return out_hash[0] ? 0 : -1;
}

int head_update(const char *hash) {
    if (mkdir_p(".pes/refs/heads") < 0) return -1;
    char tmp_path[256];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp.%ld", PES_MAIN_REF, (long)getpid());
    FILE *fp = fopen(tmp_path, "w");
    if (!fp) return -1;
    fprintf(fp, "%s\n", hash);
    fflush(fp);
    fsync(fileno(fp));
    if (fclose(fp) != 0) {
        unlink(tmp_path);
        return -1;
    }
    if (rename(tmp_path, PES_MAIN_REF) < 0) {
        unlink(tmp_path);
        return -1;
    }
    return 0;
}

int commit_create(const char *message, char out_hash[PES_HASH_HEX_SIZE + 1]) {
    Index index;
    if (index_load(&index) < 0) return -1;
    if (index.count == 0) {
        index_free(&index);
        errno = EINVAL;
        return -1;
    }

    Commit commit;
    memset(&commit, 0, sizeof(commit));
    if (tree_from_index(&index, commit.tree) < 0) {
        index_free(&index);
        return -1;
    }
    char parent[PES_HASH_HEX_SIZE + 1] = {0};
    if (head_read(parent) == 0) snprintf(commit.parent, sizeof(commit.parent), "%s", parent);
    long long now = (long long)time(NULL);
    commit.author = pes_strdup(pes_author());
    commit.committer = pes_strdup(pes_author());
    commit.author_time = now;
    commit.commit_time = now;
    commit.message = pes_strdup(message);

    size_t size = 0;
    char *data = commit_serialize(&commit, &size);
    int rc = object_write(OBJ_COMMIT, data, size, out_hash);
    free(data);
    commit_free(&commit);
    index_free(&index);
    if (rc < 0) return -1;
    return head_update(out_hash);
}

int commit_walk(void) {
    char hash[PES_HASH_HEX_SIZE + 1];
    if (head_read(hash) < 0) {
        printf("No commits yet.\n");
        return 0;
    }
    while (hash[0]) {
        ObjectType type;
        size_t size;
        char *data = object_read(hash, &type, &size);
        if (!data || type != OBJ_COMMIT) return -1;
        Commit commit;
        if (commit_parse(data, size, &commit) < 0) {
            free(data);
            return -1;
        }
        printf("commit %s\n", hash);
        printf("Author: %s\n", commit.author);
        printf("Date:   %lld\n\n", commit.author_time);
        printf("    %s\n\n", commit.message);
        snprintf(hash, sizeof(hash), "%s", commit.parent);
        commit_free(&commit);
        free(data);
    }
    return 0;
}
