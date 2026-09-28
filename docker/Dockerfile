FROM espressif/idf:v6.1

ENV DEBIAN_FRONTEND=noninteractive

# Ubuntu images may include an `ubuntu` user with UID 1000. Remove it so the
# host-matching user can safely claim that common UID without inheriting groups.
RUN if id -u ubuntu > /dev/null 2>&1; then userdel --remove ubuntu; fi

# Add project-specific utilities to the ESP-IDF build image. ESP-IDF, its
# toolchains, and QEMU are supplied by the base image.
RUN apt-get update && apt-get install -y --no-install-recommends \
    bash-completion \
    ca-certificates \
    curl \
    fd-find \
    file \
    git \
    iputils-ping \
    jq \
    less \
    lsof \
    make \
    nano \
    netcat-openbsd \
    openssh-client \
    procps \
    python3 \
    python3-flask \
    python3-pip \
    python3-venv \
    pipewire-bin \
    ripgrep \
    sudo \
    tio \
    tree \
    vim \
    && rm -rf /var/lib/apt/lists/*

COPY setupUser.sh /usr/local/bin/setup-user
COPY entrypoint.sh /usr/local/bin/entrypoint

RUN chmod +x /usr/local/bin/setup-user /usr/local/bin/entrypoint

ENTRYPOINT ["/usr/local/bin/entrypoint"]
# The Compose service replaces this with `sleep infinity`; dockerw replaces it
# with the command supplied on its command line.
CMD ["bash"]
