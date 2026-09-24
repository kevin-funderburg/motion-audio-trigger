#!/usr/bin/env bash

# Generate docker/.env before Docker Compose is invoked.
#
# Compose reads this file for variable interpolation and passes it into the
# container. Keeping host identity and the absolute workspace path here lets
# dockerw and the devcontainer start the same environment consistently.
set -euo pipefail

DOCKER_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(realpath "$DOCKER_DIR/..")"
# Compose project names accept a limited character set, so derive a stable,
# safe identifier from the checkout directory name.
WORKSPACE_ID="$(basename "$PROJECT_DIR" | tr '[:upper:]' '[:lower:]' | tr -cs 'a-z0-9' '-')"
WORKSPACE_ID="${WORKSPACE_ID#-}"
WORKSPACE_ID="${WORKSPACE_ID%-}"

if [ -z "$WORKSPACE_ID" ]; then
    WORKSPACE_ID="workspace"
fi

# .env is local, generated state. Restrictive permissions keep it from being
# accidentally exposed if project-specific values are added later.
umask 077
cat > "$DOCKER_DIR/.env" <<EOF
BUILDER_USER=$(id -un)
BUILDER_UID=$(id -u)
BUILDER_GID=$(id -g)
PROJECT_DIR=$PROJECT_DIR
WORKSPACE_ID=$WORKSPACE_ID
EOF

# Dockerfile.devcontainer extends this base image. Create it for a fresh
# checkout, but leave an existing image alone so dockerw starts quickly.
# build.sh sets _SKIP_IMAGE_CHECK while it prepares this same environment.
if [ "${_SKIP_IMAGE_CHECK:-false}" != "true" ]; then
    IMAGE="$(id -un)-${WORKSPACE_ID}-dev"

    if docker image inspect "$IMAGE" > /dev/null 2>&1; then
        echo "Using existing $IMAGE"
    else
        echo "Image $IMAGE not found, building..."
        _SKIP_IMAGE_CHECK=true "$DOCKER_DIR/build.sh"
    fi
fi
