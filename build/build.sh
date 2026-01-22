#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Get the workspace root (parent of build/)
WORKSPACE_ROOT="$(dirname "$SCRIPT_DIR")"

# Build the Docker image
docker build -f "$SCRIPT_DIR/Dockerfile.moriarty-builder" -t moriarty-builder "$SCRIPT_DIR"

# Run the container with the workspace mounted
docker run --rm -v "$WORKSPACE_ROOT:/workspace" moriarty-builder
