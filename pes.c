#include "commit.h"

static int pes_init(void) {
    if (mkdir_p(PES_OBJECTS) < 0) return -1;
    if (mkdir_p(".pes/refs/heads") < 0) return -1;
    FILE *head = fopen(PES_HEAD, "w");
    if (!head) return -1;
    fprintf(head, "ref: refs/heads/main\n");
    fclose(head);
    FILE *ref = fopen(PES_MAIN_REF, "a");
    if (!ref) return -1;
    fclose(ref);
    printf("Initialized empty PES repository in .pes\n");
    return 0;
}

static void usage(void) {
    fprintf(stderr, "usage: pes <init|add|status|commit|log> [args]\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage();
        return 1;
    }
    if (strcmp(argv[1], "init") == 0) return pes_init() == 0 ? 0 : 1;
    if (strcmp(argv[1], "add") == 0) {
        if (argc < 3) die("pes add requires at least one path");
        Index index;
        if (index_load(&index) < 0) die("could not load index");
        for (int i = 2; i < argc; i++) {
            if (index_add(&index, argv[i]) < 0) die("could not add %s", argv[i]);
            printf("added %s\n", argv[i]);
        }
        int rc = index_save(&index);
        index_free(&index);
        return rc == 0 ? 0 : 1;
    }
    if (strcmp(argv[1], "status") == 0) {
        Index index;
        if (index_load(&index) < 0) die("could not load index");
        index_status(&index);
        index_free(&index);
        return 0;
    }
    if (strcmp(argv[1], "commit") == 0) {
        const char *message = NULL;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) message = argv[++i];
        }
        if (!message) die("pes commit requires -m <message>");
        char hash[PES_HASH_HEX_SIZE + 1];
        if (commit_create(message, hash) < 0) die("commit failed");
        printf("[%s] %s\n", hash, message);
        return 0;
    }
    if (strcmp(argv[1], "log") == 0) return commit_walk() == 0 ? 0 : 1;
    usage();
    return 1;
}
