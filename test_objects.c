#include "pes.h"

static void ok(int cond, const char *msg) {
    if (!cond) die("FAIL: %s", msg);
    printf("PASS: %s\n", msg);
}

int main(void) {
    mkdir_p(PES_OBJECTS);
    const char payload[] = "Hello, PES objects!\n";
    char h1[PES_HASH_HEX_SIZE + 1], h2[PES_HASH_HEX_SIZE + 1];
    ok(object_write(OBJ_BLOB, payload, strlen(payload), h1) == 0, "write blob");
    ObjectType type;
    size_t size;
    char *read = object_read(h1, &type, &size);
    ok(read && type == OBJ_BLOB, "read blob type");
    ok(size == strlen(payload) && memcmp(read, payload, size) == 0, "read blob payload");
    free(read);
    ok(object_write(OBJ_BLOB, payload, strlen(payload), h2) == 0, "rewrite identical blob");
    ok(strcmp(h1, h2) == 0, "deduplicate identical content");
    char bad[PES_HASH_HEX_SIZE + 1];
    strcpy(bad, h1);
    bad[PES_HASH_HEX_SIZE - 1] = bad[PES_HASH_HEX_SIZE - 1] == '0' ? '1' : '0';
    ok(object_read(bad, &type, &size) == NULL, "reject missing/corrupt hash");
    printf("All object tests passed.\n");
    return 0;
}
