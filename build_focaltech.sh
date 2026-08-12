#!/bin/bash
set -euo pipefail

FOCAL_VERSION="1.0.0"
BUNDLE_ID="com.stefanalmare.driver.VoodooPS2FocalTech"
ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT="$HOME/Desktop/VoodooPS2-FocalTech-Build"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/VoodooPS2-FocalTech.XXXXXX")"

say() { printf '\n========== %s ==========\n' "$1"; }
die() { echo "ERROR: $*" >&2; exit 1; }
cleanup() { rm -rf "$WORK"; }
trap cleanup EXIT

[[ "$(uname -s)" == "Darwin" ]] || die "build must run on macOS"
for c in git rsync curl xcodebuild xcode-select python3 plutil ditto strings; do
    command -v "$c" >/dev/null 2>&1 || die "missing command: $c"
done
[[ "$(xcode-select -p 2>/dev/null || true)" != "/Library/Developer/CommandLineTools" ]] \
    || die "select full Xcode with: sudo xcode-select -s /Applications/Xcode.app/Contents/Developer"

say "Copy source to isolated build directory"
rsync -a --exclude '.git' --exclude 'build' --exclude 'DerivedData' "$ROOT/" "$WORK/repo/"
cd "$WORK/repo"

say "Apply FocalTech controller compatibility"
python3 Scripts/apply-controller-compat.py

say "Bootstrap MacKernelSDK"
rm -rf MacKernelSDK
git clone --depth=1 https://github.com/acidanthera/MacKernelSDK.git MacKernelSDK

say "Bootstrap VoodooInput"
src=$(/usr/bin/curl -Lfs https://raw.githubusercontent.com/acidanthera/VoodooInput/master/VoodooInput/Scripts/bootstrap.sh) \
    && /bin/bash -c "$src" || die "VoodooInput bootstrap failed"

BUILD="$WORK/build"
mkdir -p "$BUILD/controller" "$BUILD/focaltech"

say "Build patched VoodooPS2Controller"
xcodebuild \
    -project VoodooPS2Controller.xcodeproj \
    -target VoodooPS2Controller \
    -configuration Release \
    -jobs 1 \
    CONFIGURATION_BUILD_DIR="$BUILD/controller"

CONTROLLER="$BUILD/controller/VoodooPS2Controller.kext"
[[ -d "$CONTROLLER" ]] || die "VoodooPS2Controller.kext was not produced"

say "Build standalone VoodooPS2FocalTech"
cp FocalTech/VoodooPS2FocalTech.cpp VoodooPS2Trackpad/VoodooPS2Elan.cpp
cp FocalTech/VoodooPS2FocalTech.h VoodooPS2Trackpad/VoodooPS2FocalTech.h
rm -rf VoodooPS2Trackpad/Source
cp -R FocalTech/Source VoodooPS2Trackpad/Source

xcodebuild \
    -project VoodooPS2Controller.xcodeproj \
    -target VoodooPS2Trackpad \
    -configuration Release \
    -jobs 1 \
    CONFIGURATION_BUILD_DIR="$BUILD/focaltech" \
    PRODUCT_NAME=VoodooPS2FocalTech \
    PRODUCT_BUNDLE_IDENTIFIER="$BUNDLE_ID" \
    MODULE_NAME="$BUNDLE_ID" \
    MODULE_VERSION="$FOCAL_VERSION" \
    INFOPLIST_FILE="$WORK/repo/FocalTech/Info.plist" \
    'EXCLUDED_SOURCE_FILE_NAMES=VoodooPS2ALPSGlidePoint.cpp VoodooPS2SentelicFSP.cpp VoodooPS2SMBusDevice.cpp VoodooPS2SynapticsTouchPad.cpp'

FOCAL="$BUILD/focaltech/VoodooPS2FocalTech.kext"
[[ -d "$FOCAL" ]] || die "VoodooPS2FocalTech.kext was not produced"

say "Assemble final kext set"
rm -rf "$OUT"
mkdir -p "$OUT/Kexts"
ditto "$CONTROLLER" "$OUT/Kexts/VoodooPS2Controller.kext"
ditto "$FOCAL" "$OUT/Kexts/VoodooPS2FocalTech.kext"

PLUGINS="$OUT/Kexts/VoodooPS2Controller.kext/Contents/PlugIns"
mkdir -p "$PLUGINS"
rm -rf "$PLUGINS/VoodooPS2Mouse.kext" "$PLUGINS/VoodooPS2Trackpad.kext"

VI=""
for candidate in \
    "$WORK/repo/VoodooInput/Release/VoodooInput.kext" \
    "$WORK/repo/VoodooInput/Debug/VoodooInput.kext"; do
    [[ -d "$candidate" ]] && VI="$candidate" && break
done
[[ -n "$VI" ]] || die "cannot find bootstrapped VoodooInput.kext"
rm -rf "$PLUGINS/VoodooInput.kext"
ditto "$VI" "$PLUGINS/VoodooInput.kext"

[[ -d "$PLUGINS/VoodooPS2Keyboard.kext" ]] || die "keyboard plugin missing"
plutil -lint "$OUT/Kexts/VoodooPS2Controller.kext/Contents/Info.plist"
plutil -lint "$OUT/Kexts/VoodooPS2FocalTech.kext/Contents/Info.plist"
plutil -lint "$PLUGINS/VoodooInput.kext/Contents/Info.plist"
strings "$OUT/Kexts/VoodooPS2Controller.kext/Contents/MacOS/VoodooPS2Controller" | grep -q foclegacy \
    || die "patched controller marker not present in final binary"
strings "$OUT/Kexts/VoodooPS2FocalTech.kext/Contents/MacOS/VoodooPS2FocalTech" | grep -q ApplePS2FocalTech \
    || die "FocalTech class not present in final binary"

cat > "$OUT/INSTALL.txt" <<'EOF'
OpenCore Kernel -> Add order:
1. VoodooPS2Controller.kext
2. VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Keyboard.kext
3. VoodooPS2FocalTech.kext
4. VoodooPS2Controller.kext/Contents/PlugIns/VoodooInput.kext

FLT0101: do NOT use foclegacy=1
FLT0102: add boot-arg foclegacy=1
FLT0103: experimental; start without foclegacy=1
EOF

git rev-parse HEAD > "$OUT/SOURCE-COMMIT.txt" 2>/dev/null || true
rm -f "$OUT/VoodooPS2-FocalTech-Kexts.zip"
(
    cd "$OUT/Kexts"
    ditto -c -k --sequesterRsrc --keepParent . "$OUT/VoodooPS2-FocalTech-Kexts.zip"
)

say "Done"
echo "$OUT"
echo "FLT0101: no foclegacy=1"
echo "FLT0102: foclegacy=1"
echo "FLT0103: experimental"
