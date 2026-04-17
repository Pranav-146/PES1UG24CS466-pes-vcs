# Phase 4 - Commits

Implemented commit creation and HEAD updates.

Important details:

- Commits snapshot the staged index, not the working directory.
- The root tree hash is produced by `tree_from_index`.
- The parent commit is read from HEAD when present.
- The main branch ref is updated after the commit object is written.
