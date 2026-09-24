#!/usr/bin/env bash

# Create a container user that matches the invoking host user.
#
# Matching UID and GID preserves ownership of files written through the bind
# mount. This script is intentionally safe to run every time a container starts.
set -euo pipefail

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <user-id> <group-id> <user-name>" >&2
    exit 1
fi

USER_ID="$1"
GROUP_ID="$2"
USER_NAME="$3"

# Reuse a group when the numeric GID already exists. A fresh Ubuntu base does
# not normally have one, but this also keeps the script safe for future images.
if ! getent group "$GROUP_ID" > /dev/null; then
    groupadd --gid "$GROUP_ID" "$USER_NAME"
elif ! getent group "$USER_NAME" > /dev/null; then
    groupmod --new-name "$USER_NAME" "$(getent group "$GROUP_ID" | cut -d: -f1)"
fi

if ! getent passwd "$USER_ID" > /dev/null; then
    useradd --uid "$USER_ID" --gid "$GROUP_ID" --create-home --shell /bin/bash "$USER_NAME"
fi

# Set a predictable home even if the base image already had this numeric user.
USER_HOME="$(getent passwd "$USER_ID" | cut -d: -f6)"
usermod --login "$USER_NAME" --home "/home/$USER_NAME" "$(getent passwd "$USER_ID" | cut -d: -f1)"

if [ "$USER_HOME" != "/home/$USER_NAME" ]; then
    mkdir -p "/home/$USER_NAME"
    chown "$USER_ID:$GROUP_ID" "/home/$USER_NAME"
fi

# Development dependencies occasionally require apt; allow that without an
# interactive container password that has no corresponding host credential.
echo "$USER_NAME ALL=(root) NOPASSWD:ALL" > "/etc/sudoers.d/$USER_NAME"
chmod 0440 "/etc/sudoers.d/$USER_NAME"
