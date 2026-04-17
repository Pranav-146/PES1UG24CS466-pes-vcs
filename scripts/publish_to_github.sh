#!/usr/bin/env bash
set -euo pipefail

REPO_NAME="PES1UG24CS466-pes-vcs"

if ! gh auth status >/dev/null 2>&1; then
  echo "Run 'gh auth login' first, then rerun this script." >&2
  exit 1
fi

git branch -M main
gh repo create "$REPO_NAME" --public --source=. --remote=origin --push
