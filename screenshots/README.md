# Screenshot Capture Folder

Run this on Ubuntu 22.04 after installing the prerequisites:

```bash
bash scripts/capture_phase_outputs.sh
```

The script saves the required command transcripts here using the required IDs:

- `1A-test_objects.txt`
- `1B-object-store-find.txt`
- `2A-test_tree.txt`
- `2B-tree-object-xxd.txt`
- `3A-init-add-status.txt`
- `3B-index-cat.txt`
- `4A-log.txt`
- `4B-pes-find.txt`
- `4C-refs.txt`
- `final-integration.txt`

The GitHub Actions workflow `.github/workflows/generate-screenshots.yml` renders
PNG screenshots from these transcripts and commits them back into this folder.
