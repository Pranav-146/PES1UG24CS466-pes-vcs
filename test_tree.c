#include "tree.h"

static void ok(int cond, const char *msg) {
    if (!cond) die("FAIL: %s", msg);
    printf("PASS: %s\n", msg);
}

int main(void) {
    mkdir_p(PES_OBJECTS);
    Tree t1, t2, parsed;
    tree_init(&t1);
    tree_init(&t2);
    tree_add(&t1, 0100644, "blob", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "README.md");
    tree_add(&t1, 0100755, "blob", "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb", "build.sh");
    tree_add(&t2, 0100755, "blob", "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb", "build.sh");
    tree_add(&t2, 0100644, "blob", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "README.md");
    size_t s1, s2;
    char *a = tree_serialize(&t1, &s1);
    char *b = tree_serialize(&t2, &s2);
    ok(s1 == s2 && memcmp(a, b, s1) == 0, "deterministic tree serialization");
    ok(tree_parse(a, s1, &parsed) == 0, "parse serialized tree");
    ok(parsed.count == 2, "roundtrip entry count");
    ok(strcmp(parsed.entries[0].name, "README.md") == 0, "roundtrip first path");
    char hash[PES_HASH_HEX_SIZE + 1];
    ok(object_write(OBJ_TREE, a, s1, hash) == 0, "write tree object");
    printf("Tree object: %s\n", hash);
    printf("All tree tests passed.\n");
    free(a);
    free(b);
    tree_free(&t1);
    tree_free(&t2);
    tree_free(&parsed);
    return 0;
}
