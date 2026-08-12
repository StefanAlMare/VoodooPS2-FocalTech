#!/bin/bash
set -euo pipefail

FOCAL_VERSION="2.3.7"
VINPUT_VERSION="1.1.6"
BUNDLE_ID="com.stefanalmare.driver.VoodooPS2FocalTech"
ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT="$HOME/Desktop/VoodooPS2-FocalTech-Build"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/VoodooPS2-FocalTech.XXXXXX")"
SOURCE_COMMIT="$(git -C "$ROOT" rev-parse HEAD 2>/dev/null || echo unknown)"

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

say "Bootstrap pinned VoodooInput ${VINPUT_VERSION}"
rm -rf VoodooInput
mkdir -p VoodooInput/Debug VoodooInput/Release "$WORK/voodooinput"

for spec in "DEBUG:Debug" "RELEASE:Release"; do
    conf="${spec%%:*}"
    dest="${spec##*:}"
    archive="$WORK/voodooinput/VoodooInput-${VINPUT_VERSION}-${conf}.zip"
    extract="$WORK/voodooinput/${conf}"

    curl -LfsS \
        "https://github.com/acidanthera/VoodooInput/releases/download/${VINPUT_VERSION}/VoodooInput-${VINPUT_VERSION}-${conf}.zip" \
        -o "$archive" \
        || die "failed to download VoodooInput ${VINPUT_VERSION} ${conf}"

    rm -rf "$extract"
    mkdir -p "$extract"
    ditto -x -k "$archive" "$extract"
    [[ -d "$extract/VoodooInput.kext" ]] \
        || die "VoodooInput ${VINPUT_VERSION} ${conf} archive has unexpected layout"
    ditto "$extract/VoodooInput.kext" "VoodooInput/${dest}/VoodooInput.kext"
done

[[ -f "VoodooInput/Debug/VoodooInput.kext/Contents/Resources/VoodooInputMultitouch/VoodooInputMessages.h" ]] \
    || die "VoodooInput ${VINPUT_VERSION} debug SDK headers missing"

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
KEYBOARD="$BUILD/controller/VoodooPS2Keyboard.kext"
[[ -d "$CONTROLLER" ]] || die "VoodooPS2Controller.kext was not produced"
[[ -d "$KEYBOARD" ]] || die "VoodooPS2Keyboard.kext was not produced"

say "Build standalone VoodooPS2FocalTech"
cp FocalTech/VoodooPS2FocalTech.cpp VoodooPS2Trackpad/VoodooPS2Elan.cpp
cp FocalTech/VoodooPS2FocalTech.h VoodooPS2Trackpad/VoodooPS2FocalTech.h
rm -rf VoodooPS2Trackpad/Source
cp -R FocalTech/Source VoodooPS2Trackpad/Source

# The implementation fragments live one directory below the staged public
# header. Rewrite only this local include inside the isolated build tree.
python3 - <<'PY'
from pathlib import Path
p = Path("VoodooPS2Trackpad/Source/VoodooPS2FocalTech.part00.inc")
s = p.read_text()
old = '#include "VoodooPS2FocalTech.h"'
new = '#include "../VoodooPS2FocalTech.h"'
if s.count(old) != 1:
    raise SystemExit("ERROR: unexpected FocalTech header include layout")
p.write_text(s.replace(old, new, 1))
PY

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
rm -rf \
    "$PLUGINS/VoodooPS2Mouse.kext" \
    "$PLUGINS/VoodooPS2Trackpad.kext" \
    "$PLUGINS/VoodooPS2Keyboard.kext" \
    "$PLUGINS/VoodooInput.kext"

# With an explicit CONFIGURATION_BUILD_DIR Xcode builds dependency products
# next to the controller but does not reliably nest them in PlugIns. Assemble
# the final plugin layout explicitly so local and CI builds are identical.
ditto "$KEYBOARD" "$PLUGINS/VoodooPS2Keyboard.kext"
ditto "$WORK/repo/VoodooInput/Release/VoodooInput.kext" "$PLUGINS/VoodooInput.kext"

[[ -d "$PLUGINS/VoodooPS2Keyboard.kext" ]] || die "keyboard plugin missing"
[[ -d "$PLUGINS/VoodooInput.kext" ]] || die "VoodooInput plugin missing"
plutil -lint "$OUT/Kexts/VoodooPS2Controller.kext/Contents/Info.plist"
plutil -lint "$PLUGINS/VoodooPS2Keyboard.kext/Contents/Info.plist"
plutil -lint "$OUT/Kexts/VoodooPS2FocalTech.kext/Contents/Info.plist"
plutil -lint "$PLUGINS/VoodooInput.kext/Contents/Info.plist"
strings "$OUT/Kexts/VoodooPS2Controller.kext/Contents/MacOS/VoodooPS2Controller" | grep -q foclegacy \
    || die "patched controller marker not present in final binary"
strings "$OUT/Kexts/VoodooPS2FocalTech.kext/Contents/MacOS/VoodooPS2FocalTech" | grep -q ApplePS2FocalTech \
    || die "FocalTech class not present in final binary"

cat > "$OUT/INSTALL.txt" <<'EOF'
VoodooPS2-FocalTech installation

OpenCore Kernel -> Add order:
1. VoodooPS2Controller.kext
2. VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Keyboard.kext
3. VoodooPS2FocalTech.kext
4. VoodooPS2Controller.kext/Contents/PlugIns/VoodooInput.kext

FLT0101: do NOT use foclegacy=1
FLT0102: add boot-arg foclegacy=1
FLT0103: experimental; start without foclegacy=1

Validated on macOS Sequoia and macOS Tahoe on tested FLT0101/FLT0102 hardware.

IMPORTANT FOR OCA AUXILIARY TOOLS (OCAT) USERS:
If OCAT reports an update for VoodooPS2Controller.kext on a machine using this
FocalTech fork, do not let OCAT replace this fork's VoodooPS2 stack with the
stock Acidanthera build unless you intentionally want to remove FocalTech
support.

This fork uses a patched VoodooPS2Controller.kext and a separate
VoodooPS2FocalTech.kext. The patched controller can have a different file size
and hash/MD5 from Acidanthera's stock binary even when the displayed version
number is the same. OCAT may therefore show the stock package as an available
update. This difference is expected and is not an error.

Replacing this fork with the stock Acidanthera VoodooPS2 package can remove the
FocalTech compatibility path and stop the trackpad. Update this stack from the
VoodooPS2-FocalTech Releases page instead.
EOF

printf '%s\n' "$SOURCE_COMMIT" > "$OUT/SOURCE-COMMIT.txt"
printf '%s\n' "$VINPUT_VERSION" > "$OUT/VOODOOINPUT-VERSION.txt"
rm -f "$OUT/VoodooPS2-FocalTech-Kexts.zip"
(
    cd "$OUT/Kexts"
    ditto -c -k --sequesterRsrc --keepParent . "$OUT/VoodooPS2-FocalTech-Kexts.zip"
)

say "Done"
echo "$OUT"
echo "FocalTech version: $FOCAL_VERSION"
echo "VoodooInput pinned: $VINPUT_VERSION"
echo "FLT0101: no foclegacy=1"
echo "FLT0102: foclegacy=1"
echo "FLT0103: experimental"
