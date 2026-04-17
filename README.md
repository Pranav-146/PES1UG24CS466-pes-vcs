# PES-VCS Lab Report

Student: Sri Pranav Gautam Buduguru
SRN: PES1UG24CS466
Repository name: `PES1UG24CS466-pes-vcs`

## Build And Test Commands

```bash
sudo apt update && sudo apt install -y gcc build-essential libssl-dev
make
make all
make test-integration
```

Author configuration:

```bash
export PES_AUTHOR="Sri Pranav Gautam Buduguru <PES1UG24CS466>"
```

The required screenshot images are saved under `screenshots/` after running:

```bash
bash scripts/capture_phase_outputs.sh
```

## Implementation Summary

Phase 1 implements the content-addressable object store in `object.c`. Objects
are stored as `<type> <size>\0<data>`, hashed with SHA-256, sharded by the first
two hex characters, and written atomically.

Phase 2 implements deterministic tree object creation in `tree.c`. Nested paths
from the index become nested tree objects, and child trees are written before
their parent.

Phase 3 implements the staging index in `index.c`. The index is a text file with
mode, object hash, mtime, size, and path fields. Saves use temp-file plus
`fsync` plus `rename`.

Phase 4 implements commits in `commit.c`. Commits snapshot the staged index,
link to the previous HEAD commit, store author metadata from `PES_AUTHOR`, and
update `.pes/refs/heads/main`.

## Screenshot Checklist

| ID | File |
| --- | --- |
| 1A | `screenshots/1A_test_objects.png` |
| 1B | `screenshots/1B_objects_find.png` |
| 2A | `screenshots/2A_test_tree.png` |
| 2B | `screenshots/2B_tree_xxd.png` |
| 3A | `screenshots/3A_init_add_status.png` |
| 3B | `screenshots/3B_index_cat.png` |
| 4A | `screenshots/4A_pes_log.png` |
| 4B | `screenshots/4B_find_pes_files.png` |
| 4C | `screenshots/4C_refs_and_head.png` |
| Final | `screenshots/final_integration_test.png` |

## Q5.1 Branching And Checkout

A branch can be implemented as a file inside `.pes/refs/heads/` containing a
commit hash. `pes checkout <branch>` would first resolve `.pes/refs/heads/<branch>`
to a target commit, read that commit object, then recursively read its root tree
to know the exact files that should exist in the working directory. `.pes/HEAD`
would be updated to `ref: refs/heads/<branch>`, and the index would be rewritten
to match the checked-out tree.

The hard part is updating the working directory safely. Checkout may need to
create files, overwrite files, delete files that do not exist in the target
tree, create directories, preserve executable modes, and refuse when doing so
would destroy uncommitted work.

## Q5.2 Dirty Working Directory Conflict Detection

For each tracked file in the current index, compare the working directory file
against the index metadata. If the file is missing, the size differs, or the
mtime differs, recompute its blob hash and compare it with the indexed hash. If
the working copy hash differs from the index hash, the file is dirty.

Then compare the current branch tree and the target branch tree. If a dirty file
would be changed, deleted, or replaced by checkout, checkout must refuse. If the
target branch has the same blob hash for that path, the dirty file does not
conflict with switching branches because checkout does not need to overwrite it.

## Q5.3 Detached HEAD

In detached HEAD state, `.pes/HEAD` contains a commit hash directly instead of a
`ref: refs/heads/...` pointer. New commits made there point to the detached HEAD
commit as their parent, but no branch name automatically moves to the new commit.

Those commits are recoverable as long as the user still has the hash, can find it
in a reflog-like record, or can discover it before garbage collection deletes it.
Recovery is done by creating a branch file such as `.pes/refs/heads/recovered`
that contains the detached commit hash.

## Q6.1 Garbage Collection

Garbage collection should use mark and sweep. First, read every branch reference
under `.pes/refs/heads/` and push those commit hashes onto a stack or queue.
Maintain a hash set of reachable object hashes. While the stack is not empty,
pop a hash, skip it if already marked, read the object, mark it reachable, and
push any hashes it references. Commits reference their tree and parent commit;
trees reference blobs and subtrees; blobs reference nothing.

After marking, scan every file in `.pes/objects/`. Any object hash not present in
the reachable hash set is unreachable and can be deleted.

With 100,000 commits and 50 branches, the traversal visits the union of all
objects reachable from those branches, not `100,000 * 50` if histories overlap.
In a mostly shared history, it is roughly 100,000 commit objects plus every tree
and blob reachable from those commits. A hash set gives expected O(1) membership
checks and prevents revisiting shared history.

## Q6.2 GC And Concurrent Commits

Concurrent garbage collection is dangerous because commit creation is not a
single instant. A commit operation may write blobs and trees first, then write the
commit object, then update the branch ref. If GC scans refs after the blobs and
trees are written but before the new commit ref exists, those new objects appear
unreachable. GC could delete a tree or blob that the new commit is about to
reference, corrupting the repository.

Real Git avoids this with conservative rules: it keeps recently created loose
objects for a grace period, uses lock files around ref updates, treats reflogs
and in-progress state as roots, and prunes only objects older than the configured
expiration time. That makes it unlikely that a concurrent writer's fresh object
is deleted.
