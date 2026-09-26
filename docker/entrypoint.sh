#!/usr/bin/env bash

set -euo pipefail

# The ESP-IDF base image's entrypoint activates its toolchain and Python
# environment. Delegate to it after this wrapper has set up the local user.
IDF_ENTRYPOINT=/opt/esp/entrypoint.sh

if [ -n "${BUILDER_UID:-}" ] && [ -n "${BUILDER_GID:-}" ] && [ -n "${BUILDER_USER:-}" ]; then
    /usr/local/bin/setup-user "$BUILDER_UID" "$BUILDER_GID" "$BUILDER_USER"
    # setpriv replaces this process with the requested user. This is preferable
    # to leaving sudo as PID 1, so a devcontainer service itself is non-root.
    # ESP-IDF's entrypoint then activates its toolchain for that user.
    exec setpriv \
        --reuid "$BUILDER_UID" \
        --regid "$BUILDER_GID" \
        --init-groups \
        env HOME="/home/$BUILDER_USER" USER="$BUILDER_USER" \
        "$IDF_ENTRYPOINT" "$@"
fi

# Permit direct `docker run` use when no host identity was supplied, while still
# activating ESP-IDF's toolchain.
exec "$IDF_ENTRYPOINT" "$@"
