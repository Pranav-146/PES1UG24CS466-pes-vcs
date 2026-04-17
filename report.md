# PES-VCS Lab Report

Name: XYZ  
SRN: PES1UG24AMXXX  
GitHub repository: https://github.com/Pranav-146/PES1UG24CS466-pes-vcs  
Repository name: `PES1UG24CS466-pes-vcs`

## Overview

This project implements a small local version control system inspired by Git.
It supports repository initialization, object storage, staging, status,
commits, and commit history traversal.

The implementation follows the lab phases:

- Phase 1: content-addressable object storage
- Phase 2: tree objects and nested path snapshots
- Phase 3: text-based staging index
- Phase 4: commits, HEAD, and history
- Phase 5 and 6: written filesystem analysis questions

## Build Instructions

Target platform: Ubuntu 22.04

```bash
sudo apt update && sudo apt install -y gcc build-essential libssl-dev
export PES_AUTHOR="XYZ <PES1UG24AMXXX>"
make all
```

Run the integration test:

```bash
make test-integration
```

Run the screenshot/transcript capture workflow:

```bash
bash scripts/capture_phase_outputs.sh
```

The capture script saves the required outputs into the `screenshots/` folder.
The GitHub Actions workflow `.github/workflows/generate-screenshots.yml`
generates the same outputs on Ubuntu 22.04 and renders PNG screenshots from
those terminal transcripts.

## Phase 1: Object Storage

Implemented in `object.c`.

The object store writes full objects in this format:

```text
<type> <size>\0<data>
```

The SHA-256 hash is computed over the full object, including the header. Objects
are stored under `.pes/objects/XX/YYYY...`, where `XX` is the first two
characters of the hash.

Important behavior:

- identical content produces the same object hash
- duplicate content is stored only once
- objects are written through a temporary file and renamed atomically
- reads recompute the hash and reject corrupted content

Required captures:

- `screenshots/1A-test_objects.txt`
- `screenshots/1B-object-store-find.txt`
- `screenshots/1A_test_objects.png`
- `screenshots/1B_objects_find.png`

## Phase 2: Tree Objects

Implemented in `tree.c`.

Tree objects represent directory snapshots. The implementation builds a tree
hierarchy from staged index paths, so a path such as `src/main.c` creates a
`src` subtree containing `main.c`.

Important behavior:

- tree entries are sorted before serialization
- serialization is deterministic
- child trees are written before parent trees
- the root tree hash represents the staged project snapshot

Required captures:

- `screenshots/2A-test_tree.txt`
- `screenshots/2B-tree-object-xxd.txt`
- `screenshots/2A_test_tree.png`
- `screenshots/2B_tree_xxd.png`

## Phase 3: Index

Implemented in `index.c`.

The index is a text staging area stored at `.pes/index`. Each entry is saved as:

```text
<mode> <hash-hex> <mtime> <size> <path>
```

Important behavior:

- loading a missing index creates an empty in-memory index
- `pes add` stores the file as a blob and updates the staged entry
- index entries are sorted by path before saving
- saves use temporary-file, `fsync`, and `rename`
- `pes status` reports staged, modified, deleted, and untracked files

Required captures:

- `screenshots/3A-init-add-status.txt`
- `screenshots/3B-index-cat.txt`
- `screenshots/3A_init_add_status.png`
- `screenshots/3B_index_cat.png`

## Phase 4: Commits And History

Implemented in `commit.c`.

A commit stores the root tree hash, optional parent hash, author and committer
metadata, timestamps, and the commit message. Commits are created from the index,
not directly from the working directory.

Important behavior:

- `tree_from_index()` builds the committed snapshot
- HEAD is read to find the parent commit
- `PES_AUTHOR` supplies the author string
- the commit object is written before `.pes/refs/heads/main` is updated
- `pes log` walks the parent chain

Required captures:

- `screenshots/4A-log.txt`
- `screenshots/4B-pes-find.txt`
- `screenshots/4C-refs.txt`
- `screenshots/final-integration.txt`
- `screenshots/4A_pes_log.png`
- `screenshots/4B_find_pes_files.png`
- `screenshots/4C_refs_and_head.png`
- `screenshots/final_integration_test.png`

## Commit History Requirement

The repository contains the required phase commits:

```text
Phase 1: 5 commits
Phase 2: 5 commits
Phase 3: 5 commits
Phase 4: 5 commits
```

There is also one extra helper/report commit after the phase work.

## Q5.1 Branching And Checkout

A branch is a file inside `.pes/refs/heads/` containing a commit hash. To
implement `pes checkout <branch>`, PES-VCS would read
`.pes/refs/heads/<branch>`, load that target commit, read the commit's root tree,
and update the working directory to match that tree. It would also rewrite
`.pes/HEAD` to contain:

```text
ref: refs/heads/<branch>
```

The index must also be rewritten to match the checked-out tree.

The operation is complex because the working directory may already contain files
that need to be overwritten, deleted, created, or have their executable modes
changed. Checkout must avoid destroying uncommitted work.

## Q5.2 Dirty Working Directory Conflict Detection

For every tracked file in the current index, compare the working directory file
with the index metadata. If the file is missing, changed in size, or changed in
mtime, compute its blob hash and compare it with the hash stored in the index.
If the hash differs, the working copy is dirty.

Next, compare the current branch tree with the target branch tree. If checkout
would overwrite, delete, or replace a dirty tracked file, checkout must refuse.
If the target branch contains the same blob hash for that file, checkout does not
need to overwrite it and can proceed for that path.

## Q5.3 Detached HEAD

Detached HEAD means `.pes/HEAD` stores a commit hash directly instead of a branch
reference. New commits made in that state use the detached commit as their
parent, but no branch file automatically moves forward.

The commits can be recovered by creating a branch file that points to the
detached commit hash, for example:

```text
.pes/refs/heads/recovered
```

The user must know the commit hash or find it through a reflog-like record before
garbage collection removes it.

## Q6.1 Garbage Collection

Garbage collection can use a mark-and-sweep algorithm.

First, read every branch reference under `.pes/refs/heads/` and place those
commit hashes in a stack or queue. Maintain a hash set of reachable object
hashes. Repeatedly pop a hash, skip it if already visited, read the object, mark
it reachable, and push any referenced hashes:

- commits reference their root tree and parent commit
- trees reference blobs and subtrees
- blobs reference no other objects

After marking, scan all files in `.pes/objects/`. Any object not present in the
reachable hash set is unreachable and can be deleted.

For 100,000 commits and 50 branches, the number of visited objects is the union
of reachable objects, not `100,000 * 50`, because shared history is marked once.
In a mostly shared repository, GC visits about 100,000 commits plus all reachable
trees and blobs.

## Q6.2 GC And Concurrent Commits

Running GC concurrently with a commit is dangerous because commit creation
happens in steps. A commit operation may write blobs and trees first, then write
the commit object, then update the branch reference. If GC scans refs after the
new objects are written but before the branch ref points to the new commit, those
new objects appear unreachable and may be deleted.

That would leave the commit operation about to reference missing objects.

Real Git avoids this with conservative pruning rules. It uses lock files for ref
updates, treats reflogs and in-progress state as reachability roots, and does not
prune very new objects immediately. The grace period prevents fresh objects from
being deleted while another process is still building references to them.
