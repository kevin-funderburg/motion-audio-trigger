#!/usr/bin/env bash

set -euo pipefail

if [ -n "${BUILDER_UID:-}" ] && [ -n "${BUILDER_GID:-}" ] && [ -n "${BUILDER_USER:-}" ]; then
    /usr/local/bin/setup-user "$BUILDER_UID" "$BUILDER_GID" "$BUILDER_USER"
    # setpriv replaces this process with the requested user. This is preferable
    # to leaving sudo as PID 1, so a devcontainer service itself is non-root.
    exec setpriv \
        --reuid "$BUILDER_UID" \
        --regid "$BUILDER_GID" \
        --init-groups \
        env HOME="/home/$BUILDER_USER" USER="$BUILDER_USER" "$@"
fi

# Permit direct `docker run` use when no host identity was supplied.
exec "$@"
