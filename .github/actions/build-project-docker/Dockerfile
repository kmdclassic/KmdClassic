FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install --no-install-recommends -y \
    build-essential pkg-config libc6-dev m4 g++-multilib autoconf automake \
    libtool ncurses-dev unzip git python3 bison zlib1g-dev wget curl \
    ca-certificates libcurl4-gnutls-dev bsdmainutils cmake mingw-w64 \
    util-linux shellcheck xvfb xauth && \
    update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix && \
    update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix && \
    rm -rf /var/lib/apt/lists/*

COPY entrypoint.sh /entrypoint.sh
ENTRYPOINT ["/bin/bash", "/entrypoint.sh"]
