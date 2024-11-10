#!/usr/bin/env bash

BDB_FILE="./depends/packages/bdb.mk"

if [[ ! -f "$BDB_FILE" ]]; then
    echo "Error: The file $BDB_FILE does not exist."
    echo "Please check the path and try again."
    exit 1
fi

if git diff --quiet "$BDB_FILE"; then
    echo "You are trying to build KomodoOcean for the Apple Silicon (arm64) chipset."
    echo "As it’s become known (https://github.com/zcash/zcash/issues/6977), the current version of Berkeley DB 6.2.23 will not work properly in this configuration."
    echo "Would you like to upgrade it to 6.2.32? Please, backup your wallet.dat if you agree to upgrade to Berkeley DB 6.2.32."

    read -p "Do you want to proceed with the upgrade? (yes/no): " user_response

    if [[ "$OSTYPE" == "darwin"* ]]; then
        SED_INPLACE="sed -i ''"
    else
        SED_INPLACE="sed -i"
    fi

    if [[ "$user_response" == "yes" ]]; then
        $SED_INPLACE 's/$(package)_version=6.2.23/$(package)_version=6.2.32/' "$BDB_FILE"
        $SED_INPLACE 's/$(package)_sha256_hash=47612c8991aa9ac2f6be721267c8d3cdccf5ac83105df8e50809daea24e95dc7/$(package)_sha256_hash=a9c5e2b004a5777aa03510cfe5cd766a4a3b777713406b02809c17c8e0e7a8fb/' "$BDB_FILE"

        echo "Berkeley DB version updated to 6.2.32 successfully."
    else
        echo "No changes made. Berkeley DB remains at version 6.2.23."
    fi
else
    echo "The file $BDB_FILE already contains the changes. No further modification is needed."
fi

make -C ${PWD}/depends v=1 NO_PROTON=1 HOST=arm64-apple-darwin -j$(nproc --all)
./autogen.sh
# -Wno-deprecated-builtins -Wno-enum-constexpr-conversion
CXXFLAGS="-g0 -O2 -Wno-unknown-warning-option" \
CONFIG_SITE="$PWD/depends/arm64-apple-darwin/share/config.site" ./configure --disable-tests --disable-bench --with-gui=qt5 --disable-bip70 --host=arm64-apple-darwin
make -j$(nproc --all) # V=1
