#!/usr/bin/env bash

set -euo pipefail

# By default, `kmdclassicd` inside the container runs as the `nobody:nogroup` user. All files
# created by the daemon in the data folder will also have these access privileges. To
# bypass the `nobody:nogroup` ownership check on the data folder, you can use the
# `-no-ownership-check` key.

nocheck=0
for arg in "$@"
do
    if [[ "$arg" == "--no-ownership-check" ]] || [[ "$arg" == "-no-ownership-check" ]]; then
        nocheck=1
        break
    fi
done

if [[ "$nocheck" -eq 0 && "$(stat -c '%u:%g' /data)" != "65534:65534" ]]; then
  echo "Folder mounted at /data is not owned by nobody:nogroup, please change its permissions on the host with 'sudo chown -R 65534:65534 path/to/komodo-data'."
  exit 1
fi

/app/fetch-params.sh
exec /app/kmdclassicd -datadir=/data/.kmdclassic "$@"
