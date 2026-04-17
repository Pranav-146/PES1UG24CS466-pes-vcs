# Phase 1 - Object Store

Implemented content-addressed storage using SHA-256 over the complete object
payload: `<type> <size>\0<data>`.

Important details:

- Objects are stored under `.pes/objects/xx/yyyy...`.
- Identical content maps to the same object path.
- Writes use a temporary file, `fsync`, and `rename`.
- Reads recompute SHA-256 and reject mismatched content.
