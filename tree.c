#include "tree.h"

typedef struct TreeNode TreeNode;

struct TreeNode {
    Tree tree;
    char *name;
    TreeNode **children;
    size_t child_count;
    size_t child_capacity;
};

void tree_init(Tree *tree) {
    tree->entries = NULL;
    tree->count = 0;
    tree->capacity = 0;
}

void tree_free(Tree *tree) {
    for (size_t i = 0; i < tree->count; i++) free(tree->entries[i].name);
    free(tree->entries);
    tree_init(tree);
}

static void tree_reserve(Tree *tree, size_t needed) {
    if (tree->capacity >= needed) return;
    size_t next = tree->capacity ? tree->capacity * 2 : 8;
    while (next < needed) next *= 2;
    TreeEntry *entries = realloc(tree->entries, next * sizeof(TreeEntry));
    if (!entries) die("out of memory");
    tree->entries = entries;
    tree->capacity = next;
}

int tree_add(Tree *tree, int mode, const char *type, const char *hash, const char *name) {
    tree_reserve(tree, tree->count + 1);
    TreeEntry *e = &tree->entries[tree->count++];
    e->mode = mode;
    snprintf(e->type, sizeof(e->type), "%s", type);
    snprintf(e->hash, sizeof(e->hash), "%s", hash);
    e->name = pes_strdup(name);
    return 0;
}

static int tree_entry_cmp(const void *a, const void *b) {
    const TreeEntry *ea = a;
    const TreeEntry *eb = b;
    return strcmp(ea->name, eb->name);
}

char *tree_serialize(Tree *tree, size_t *out_size) {
    qsort(tree->entries, tree->count, sizeof(TreeEntry), tree_entry_cmp);
    size_t cap = 128;
    size_t len = 0;
    char *buf = malloc(cap);
    if (!buf) die("out of memory");
    for (size_t i = 0; i < tree->count; i++) {
        TreeEntry *e = &tree->entries[i];
        int needed = snprintf(NULL, 0, "%06o %s %s %s\n", e->mode, e->type, e->hash, e->name);
        while (len + (size_t)needed + 1 > cap) {
            cap *= 2;
            char *next = realloc(buf, cap);
            if (!next) die("out of memory");
            buf = next;
        }
        snprintf(buf + len, cap - len, "%06o %s %s %s\n", e->mode, e->type, e->hash, e->name);
        len += (size_t)needed;
    }
    *out_size = len;
    return buf;
}

int tree_parse(const char *data, size_t size, Tree *tree) {
    tree_init(tree);
    char *copy = malloc(size + 1);
    if (!copy) die("out of memory");
    memcpy(copy, data, size);
    copy[size] = '\0';
    char *save = NULL;
    for (char *line = strtok_r(copy, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
        int mode;
        char type[16], hash[PES_HASH_HEX_SIZE + 1], name[4096];
        if (sscanf(line, "%o %15s %64s %[^\n]", &mode, type, hash, name) != 4) {
            free(copy);
            tree_free(tree);
            return -1;
        }
        tree_add(tree, mode, type, hash, name);
    }
    free(copy);
    return 0;
}

static TreeNode *node_new(const char *name) {
    TreeNode *node = calloc(1, sizeof(TreeNode));
    if (!node) die("out of memory");
    tree_init(&node->tree);
    node->name = pes_strdup(name);
    return node;
}

static void node_free(TreeNode *node) {
    if (!node) return;
    tree_free(&node->tree);
    free(node->name);
    for (size_t i = 0; i < node->child_count; i++) node_free(node->children[i]);
    free(node->children);
    free(node);
}

static TreeNode *node_child(TreeNode *node, const char *name) {
    for (size_t i = 0; i < node->child_count; i++) {
        if (strcmp(node->children[i]->name, name) == 0) return node->children[i];
    }
    if (node->child_count == node->child_capacity) {
        node->child_capacity = node->child_capacity ? node->child_capacity * 2 : 4;
        TreeNode **children = realloc(node->children, node->child_capacity * sizeof(TreeNode *));
        if (!children) die("out of memory");
        node->children = children;
    }
    TreeNode *child = node_new(name);
    node->children[node->child_count++] = child;
    return child;
}

static void add_index_path(TreeNode *root, const IndexEntry *entry) {
    char *path = pes_strdup(entry->path);
    TreeNode *node = root;
    char *save = NULL;
    char *part = strtok_r(path, "/", &save);
    while (part) {
        char *next = strtok_r(NULL, "/", &save);
        if (!next) {
            tree_add(&node->tree, entry->mode, "blob", entry->hash, part);
        } else {
            node = node_child(node, part);
        }
        part = next;
    }
    free(path);
}

static int child_cmp(const void *a, const void *b) {
    const TreeNode *na = *(TreeNode * const *)a;
    const TreeNode *nb = *(TreeNode * const *)b;
    return strcmp(na->name, nb->name);
}

static int write_node(TreeNode *node, char out_hash[PES_HASH_HEX_SIZE + 1]) {
    qsort(node->children, node->child_count, sizeof(TreeNode *), child_cmp);
    for (size_t i = 0; i < node->child_count; i++) {
        char child_hash[PES_HASH_HEX_SIZE + 1];
        if (write_node(node->children[i], child_hash) < 0) return -1;
        tree_add(&node->tree, 0040000, "tree", child_hash, node->children[i]->name);
    }
    size_t size = 0;
    char *data = tree_serialize(&node->tree, &size);
    int rc = object_write(OBJ_TREE, data, size, out_hash);
    free(data);
    return rc;
}

int tree_from_index(const Index *index, char out_hash[PES_HASH_HEX_SIZE + 1]) {
    TreeNode *root = node_new("");
    for (size_t i = 0; i < index->count; i++) add_index_path(root, &index->entries[i]);
    int rc = write_node(root, out_hash);
    node_free(root);
    return rc;
}
