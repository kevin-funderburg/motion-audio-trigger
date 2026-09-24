#!/usr/bin/env bash

set -euo pipefail

# Build the same image used by both entry points. Skip setup's image check to
# avoid recursing when setupDockerEnv.sh invokes this script for a missing image.
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
_SKIP_IMAGE_CHECK=true "$ROOT_DIR/docker/setupDockerEnv.sh"

docker compose \
    --project-directory "$ROOT_DIR/docker" \
    -f "$ROOT_DIR/docker/compose.yaml" \
    build halloween
