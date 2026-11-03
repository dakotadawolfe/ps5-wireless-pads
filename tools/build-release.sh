#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
for tool in curl unzip sha256sum make xxd readelf; do
    command -v "$tool" >/dev/null || { echo "Missing build tool: $tool" >&2; exit 1; }
done

sdk_version=v0.45
sdk_sha=5fac64093d0e3e69f2584ce7935f24f703f622d9681f1495c85b9f00d1291a2c
sdk_cache="${PADBRIDGE_SDK_CACHE:-${XDG_CACHE_HOME:-$HOME/.cache}/ps5-wireless-pads}"
case "$sdk_cache" in *[[:space:]]*) echo 'PADBRIDGE_SDK_CACHE must have no spaces (SDK makefile limitation)' >&2; exit 1;; esac
sdk_root="$sdk_cache/sdk-$sdk_version"
archive="$sdk_cache/ps5-payload-sdk-$sdk_version.zip"
mkdir -p "$sdk_cache"
mkdir -p build
if [[ ! -f "$archive" ]]; then
    curl --fail --location --retry 3 \
        "https://github.com/ps5-payload-dev/sdk/releases/download/$sdk_version/ps5-payload-sdk.zip" \
        -o "$archive"
fi
printf '%s  %s\n' "$sdk_sha" "$archive" | sha256sum --check --status
if [[ ! -f "$sdk_root/ps5-payload-sdk/toolchain/prospero.mk" ]]; then
    mkdir -p "$sdk_root"
    unzip -q -o "$archive" -d "$sdk_root"
fi
export PS5_PAYLOAD_SDK="$sdk_root/ps5-payload-sdk"
case "${1:-}" in
    '') make test; make fuzz;;
    --compile-only) ;;  # For rebuilding after tests have already passed.
    *) echo "usage: $0 [--compile-only]" >&2; exit 1;;
esac
make ps5
version=$(sed -n 's/^#define PADBRIDGE_VERSION "\(.*\)"/\1/p' src/version.h | tr -d '\r')
elf="dist/PadBridge-PS5-$version.elf"
readelf --dyn-syms --wide "$elf" > build/dynamic-symbols.txt
readelf --symbols --wide build/ps5/ps5_apps.o >> build/dynamic-symbols.txt
if grep -q 'UND.*sceAppInstUtilAppInstallTitleDir' build/dynamic-symbols.txt; then
    echo '13.60-incompatible mandatory import remains' >&2
    exit 1
fi
sha256sum "$elf" > "$elf.sha256"
file "$elf"
cat "$elf.sha256"
