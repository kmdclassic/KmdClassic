# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Build:
# docker build --progress=plain -f Dockerfile -t kmdclassic/kmdclassic .

# Prepare data directory and config file:
# mkdir -p ./kmdclassic-data/.kmdclassic && RANDPASS=$(tr -cd '[:alnum:]' < /dev/urandom | fold -w16 | head -n1) && echo -e "txindex=1\nrpcuser=komodo\nrpcpassword=${RANDPASS}\nrpcallowip=127.0.0.1\nrpcbind=127.0.0.1\nserver=1" > ./kmdclassic-data/.kmdclassic/kmdclassic.conf
# docker run --rm -v "$(pwd)/kmdclassic-data:/data" ubuntu:18.04 bash -c 'chown -R nobody:nogroup /data'

# Run:
# docker run -d --rm -p 7770:7770 -v "$(pwd)/kmdclassic-data:/data" kmdclassic/kmdclassic -printtoconsole=1

# Run AC (for example VOTE2024):
# mkdir -p ./VOTE2024/.kmdclassic
# docker run --rm -v "$(pwd)/VOTE2024:/data" ubuntu:18.04 bash -c 'chown -R nobody:nogroup /data'
# docker run -d --rm -v "$(pwd)/VOTE2024:/data" kmdclassic/kmdclassic -printtoconsole=1 -ac_name=VOTE2024 -ac_public=1 -ac_supply=149826699 -ac_staked=10 -addnode=65.21.52.18

# Access the container after it's already running:
# docker ps --filter ancestor=kmdclassic/kmdclassic # find the id / name of container
# docker exec -it container_id_or_name /bin/bash # run bash inside the container
# docker exec -it container_id_or_name /app/kmdclassic-cli getinfo  # run RPC getinfo

# Run KMDCL with current user UID:GID:
# mkdir -p ./kmdclassic-data/.kmdclassic && RANDPASS=$(tr -cd '[:alnum:]' < /dev/urandom | fold -w16 | head -n1) && echo -e "txindex=1\nrpcuser=komodo\nrpcpassword=${RANDPASS}\nrpcallowip=127.0.0.1\nrpcbind=127.0.0.1\nserver=1" > ./kmdclassic-data/.kmdclassic/kmdclassic.conf
# docker run --rm -d --user $(id -u):$(id -g) -e HOME=/data -v "$(pwd)/kmdclassic-data:/data" kmdclassic/kmdclassic -no-ownership-check -printtoconsole

# Run AC with current user UID:GID:
# mkdir -p ./VOTE2024/.kmdclassic # pay attention that existance of .kmdclassic folder is important (!)
# docker run --rm -d --user $(id -u):$(id -g) -e HOME=/data -v "$(pwd)/VOTE2024:/data" kmdclassic/kmdclassic -no-ownership-check -printtoconsole -ac_name=VOTE2024 -ac_public=1 -ac_supply=149826699 -ac_staked=10 -addnode=65.21.52.182

# Build and Push the Multiplatform Image:
# docker buildx build --platform linux/amd64,linux/arm64 -t kmdclassic/kmdclassic:latest --push .

## Build kmdclassicd
FROM ubuntu:20.04 AS kmdclassicd-builder
LABEL maintainer="kmdclassic <kmdclassic@pm.me>"

SHELL ["/bin/bash", "-c"]

# set to true to use the tag instead of the latest main
ARG KMDCLASSIC_USE_TAG=false 
# should be used only with KMDCLASSIC_USE_TAG=true
ARG IS_RELEASE=false 
ARG KMDCLASSIC_COMMITTISH=v1.0.0-beta25
# kmdclassic <kmdclassic@pm.me> https://keys.openpgp.org/vks/v1/by-fingerprint/B0F1479BD10B508ACFBA6B58BF03ABE358F77E84
ARG KMDCLASSIC_MAINTAINER_KEYS="B0F1479BD10B508ACFBA6B58BF03ABE358F77E84"

RUN set -euxo pipefail \
    && export DEBIAN_FRONTEND=noninteractive \
    && apt-get update \
    && apt-get -y --no-install-recommends install apt-utils \
    && apt-get -y --no-install-recommends dist-upgrade \
    && apt-get -y --no-install-recommends install autoconf automake \
      bsdmainutils build-essential ca-certificates cmake curl dirmngr fakeroot \
      git g++ gnupg2 libc6-dev libgomp1 libtool m4 ncurses-dev \
      pkg-config python3 zlib1g-dev \
    && for file in /usr/bin/aarch64-linux-gnu-*; do \
        ln -s "$file" "/usr/bin/$(basename "$file" | sed 's/aarch64-linux-gnu-/aarch64-unknown-linux-gnu-/')" || true; \
    done \
    && git clone https://github.com/kmdclassic/kmdclassic.git \
    && cd /kmdclassic \
    && ( \
      if [ "$KMDCLASSIC_USE_TAG" = "true" ]; then \
        git checkout "${KMDCLASSIC_COMMITTISH}"; \
      else \
        git checkout main; \
      fi \
    ) \
    && if [ "$IS_RELEASE" = "true" ]; then \
      read -a keys <<< "$KMDCLASSIC_MAINTAINER_KEYS" \
      && for key in "${keys[@]}"; do \
        gpg2 --batch --keyserver keyserver.ubuntu.com --keyserver-options timeout=15 --recv "$key" || \
        gpg2 --batch --keyserver hkps://keys.openpgp.org --keyserver-options timeout=15 --recv "$key" || \
        gpg2 --batch --keyserver hkp://p80.pool.sks-keyservers.net:80 --keyserver-options timeout=15 --recv-keys "$key" || \
        gpg2 --batch --keyserver hkp://ipv4.pool.sks-keyservers.net --keyserver-options timeout=15 --recv-keys "$key" || \
        gpg2 --batch --keyserver hkp://pgp.mit.edu:80 --keyserver-options timeout=15 --recv-keys "$key"; \
      done \
      && if git verify-tag -v "${KMDCLASSIC_COMMITTISH}"; then \
        echo "Valid signed tag"; \
      else \
        echo "Not a valid signed tag"; \
        exit 1; \
      fi \
      && ( gpgconf --kill dirmngr || true ) \
      && ( gpgconf --kill gpg-agent || true ); \
    fi \
    && export MAKEFLAGS="-j $(($(nproc)-1))" && ./zcutil/build-no-qt.sh

## Build Final Image
FROM ubuntu:20.04

LABEL maintainer="kmdclassic <kmdclassic@pm.me>"

SHELL ["/bin/bash", "-c"]

WORKDIR /app

# Copy kmdclassicd and fetch-params.sh
COPY --from=kmdclassicd-builder /kmdclassic/src/kmdclassicd /kmdclassic/src/kmdclassic-cli /kmdclassic/zcutil/fetch-params.sh /app/
# Copy entrypoint script
COPY entrypoint.sh /app

# Install runtime dependencies and set up home folder for nobody user.
# As it is best practice to not run as root even inside a container,
# we run as nobody and change the home folder to "/data".

RUN set -euxo pipefail \
    && export DEBIAN_FRONTEND=noninteractive \
    && apt-get update \
    && apt-get -y --no-install-recommends install apt-utils \
    && apt-get -y --no-install-recommends dist-upgrade \
    && apt-get -y --no-install-recommends install ca-certificates curl libgomp1 \
    && apt-get -y clean && apt-get -y autoclean \
    && rm -rf /var/{lib/apt/lists/*,cache/apt/archives/*.deb,tmp/*,log/*} /tmp/* \
    && mkdir -p /data \
    && for path in /data /app; do chown -R nobody:nogroup $path && chmod 2755 $path; done \
    && for file in /app/{fetch-params.sh,kmdclassicd,kmdclassic-cli}; do chmod 755 $file; done \
    && sed -i 's|nobody:/nonexistent|nobody:/data|' /etc/passwd

VOLUME ["/data"]

USER nobody

ENTRYPOINT ["/app/entrypoint.sh"]

