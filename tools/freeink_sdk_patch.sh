#!/bin/sh
# Regenerate freeink-sdk/patches/0001-waveshare-epaper-1.54.patch: FreeInkBoy's changes to the
# vendored FreeInk SDK, relative to the upstream commit it was taken from, ready to send upstream.
set -eu
die() { echo "freeink_sdk_patch: $*" >&2; exit 1; }
cd "$(dirname "$0")/.."
upstream=https://github.com/Free-Ink/freeink-sdk.git
commit=$(sed -n 's/^Upstream commit: `\([0-9a-f]*\)`.*/\1/p' freeink-sdk/FREEINKBOY.md)
[ -n "$commit" ] || die "no upstream commit recorded in freeink-sdk/FREEINKBOY.md"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
git clone -q "$upstream" "$work/upstream" && git -C "$work/upstream" checkout -q "$commit" || die "cannot fetch $commit"
mkdir -p "$work/a" "$work/b"
# Compare only what is vendored: the libraries (without their host tests) and the device notes.
for path in $(cd freeink-sdk && find libs docs -type f | sort); do
    mkdir -p "$work/b/$(dirname "$path")"; cp "freeink-sdk/$path" "$work/b/$path"
    if [ -f "$work/upstream/$path" ]; then
        mkdir -p "$work/a/$(dirname "$path")"; cp "$work/upstream/$path" "$work/a/$path"
    fi
done
(cd "$work" && git diff --no-index a b || true) | sed -e '/^diff --git/ s#a/[ab]/#a/#' -e '/^diff --git/ s# b/b/# b/#' \
    -e 's#^--- a/a/#--- a/#' -e 's#^+++ b/b/#+++ b/#' > freeink-sdk/patches/0001-waveshare-epaper-1.54.patch
git -C "$work/upstream" apply --check "$PWD/freeink-sdk/patches/0001-waveshare-epaper-1.54.patch" || die "patch does not apply upstream"
echo "wrote freeink-sdk/patches/0001-waveshare-epaper-1.54.patch"
