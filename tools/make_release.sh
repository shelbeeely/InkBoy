#!/bin/sh
set -eu

die() { echo "make_release: $*" >&2; exit 1; }

[ $# -eq 1 ] || die "usage: tools/make_release.sh <version>"
version=$1
case $version in
    ''|*[!0-9A-Za-z._-]*|[!0-9A-Za-z]*) die "version may only contain letters, digits, '.', '_' and '-', and must start with a letter or digit" ;;
esac

cd "$(dirname "$0")/.."
command -v pio >/dev/null 2>&1 || die "PlatformIO is not installed; see docs/development.md"
# esptool ships in PlatformIO's own Python environment.
pio_python="$(dirname "$(command -v pio)")/python"
[ -x "$pio_python" ] || pio_python=python3
"$pio_python" -m esptool version >/dev/null 2>&1 || die "esptool is not available in PlatformIO's Python environment"

[ -z "$(git status --porcelain)" ] || die "working tree is not clean; commit or stash your changes"
if git submodule status | grep -q '^[-+U]'; then
    die "a submodule is missing or is not at the commit recorded in this repository (see git submodule status)"
fi

env=ws154
build=.pio/build/$env
pio run -e "$env"
tools/run_tests.sh
python3 tools/check_safety.py --config "sdkconfig.$env"

image=dist/freeinkboy-$version.bin
app_image=dist/freeinkboy-$version-app.bin
for file in bootloader.bin partitions.bin firmware.bin flasher_args.json; do
    [ -f "$build/$file" ] || die "$build/$file is missing"
done
mkdir -p dist
rm -f "$image" "$app_image"
cp "$build/firmware.bin" "$app_image"
# Offsets and flash settings come from the ESP-IDF flasher arguments; PlatformIO names the images
# itself. The OTA data slot is left erased (0xFF), which boots ota_0, as ota_data_initial.bin does.
"$pio_python" -m esptool --chip esp32s3 merge-bin -o "$image" $("$pio_python" -c '
import json, sys
build = sys.argv[1]
args = json.load(open(build + "/flasher_args.json"))
names = {"bootloader/bootloader.bin": "bootloader.bin", "partition_table/partition-table.bin": "partitions.bin",
         "ota_data_initial.bin": None}
print(*[a.replace("_", "-") if a.startswith("--") else a for a in args["write_flash_args"]])
for offset, path in sorted(args["flash_files"].items(), key=lambda item: int(item[0], 16)):
    name = names.get(path, "firmware.bin" if int(offset, 16) == 0x10000 else path)
    if name: print(offset, build + "/" + name)
' "$build" | tr '\n' ' ')

if command -v sha256sum >/dev/null 2>&1; then sha256="sha256sum"; else sha256="shasum -a 256"; fi
(cd dist && $sha256 freeinkboy-*.bin > SHA256SUMS)

ls -l dist
cat dist/SHA256SUMS
