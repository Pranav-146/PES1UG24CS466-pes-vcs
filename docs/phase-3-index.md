# Phase 3 - Index

Implemented the text-based staging area.

Important details:

- Missing `.pes/index` loads as an empty index.
- Entries are persisted as `<mode> <hash> <mtime> <size> <path>`.
- Saves are atomic through temp-file, `fsync`, and `rename`.
- `pes add` stores a blob and updates or inserts the staged entry.
