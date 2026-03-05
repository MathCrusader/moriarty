#!/usr/bin/env bash
set -euo pipefail

# Run from repository root regardless of current working directory.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
cd "$REPO_ROOT"

# Pass through build options to the refresh tool's internal bazel invocation.
bazel run //:refresh_cc_meta -- "$@"

OUTPUT_BASE="$(bazel info output_base)"
EXEC_EXTERNAL="$REPO_ROOT/bazel-moriarty/external"
CACHE_EXTERNAL="$OUTPUT_BASE/external"

mkdir -p "$EXEC_EXTERNAL"

# Make sure clangd can resolve `external/<repo>` include paths emitted in
# compile_commands.json outside of Bazel's sandboxed execution.
if [[ ! -d "$CACHE_EXTERNAL" ]]; then
  echo "error: Bazel external repo directory not found: $CACHE_EXTERNAL" >&2
  exit 1
fi

for target in "$CACHE_EXTERNAL"/*; do
  [[ -e "$target" ]] || continue

  repo="$(basename "$target")"
  link="$EXEC_EXTERNAL/$repo"

  if [[ -e "$link" && ! -L "$link" ]]; then
    echo "warning: not replacing existing non-symlink path: $link"
    continue
  fi

  ln -sfn "$target" "$link"
done

echo "Done. compile_commands refreshed and external include links repaired."
