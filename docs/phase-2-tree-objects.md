# Phase 2 - Tree Objects

Implemented deterministic tree serialization and index-to-tree construction.

Important details:

- Index paths such as `src/main.c` become nested tree nodes.
- Child trees are written before their parent tree.
- Entries are sorted by name before serialization.
- Tree objects are stored through the same object store as blobs.
