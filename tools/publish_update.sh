#!/usr/bin/env bash
# Publish the current commit to players (see src/autoload/Updater.gd):
#   GODOT=/path/to/godot UPDATES=/path/to/Stellar-Frontier-Updates BASES=/path/to/bases \
#     tools/publish_update.sh [--new-base]
# The installer's game data is the base.  Each later build ships as one patch
# pack of the files changed since that base, so players download only that.
# A new base (and so a new installer) is cut on the first run, with
# --new-base, when project.godot, the export presets, Updater.gd or the
# engine changed (they load before any patch), or when the patch grows past
# 60 MB.  BASES keeps the base pack the patches are made against.
# Prints NEW_INSTALLER=build/StellarFrontierRemake-Setup.exe when the
# installer has to be handed out again.
set -euo pipefail
cd "$(dirname "$0")/.."
GODOT="${GODOT:-godot}"
: "${UPDATES:?set UPDATES to a checkout of Stellar-Frontier-Updates}"
: "${BASES:?set BASES to the folder that keeps base packs}"
REPO_URL="https://raw.githubusercontent.com/noalear/Stellar-Frontier-Updates/main"
INSTALLER_URL="${INSTALLER_URL:-https://drive.google.com/file/d/1xAcmV3v4agDk25Q6vmtaA_xgXHyXHVy7/view}"
MAX_PATCH=$((60 * 1024 * 1024))
VERSION=$(sed -n 's/^config\/version="\(.*\)"/\1/p' project.godot)
BUILD=$(git rev-parse --short HEAD)
FP=$( (cat project.godot export_presets.cfg src/autoload/Updater.gd; "$GODOT" --version) | sha256sum | cut -d' ' -f1)
mkdir -p "$BASES" "$UPDATES/patches"
BASE=$(cat "$BASES/current" 2>/dev/null || true)
NEW_BASE=0
if [[ "${1:-}" == "--new-base" || -z "$BASE" || ! -f "$BASES/$BASE.pck" || "$(cat "$BASES/$BASE.fp" 2>/dev/null)" != "$FP" ]]; then
	NEW_BASE=1
fi

printf '{"build": "%s", "version": "%s"}\n' "$BUILD" "$VERSION" > build.json
trap 'rm -f build.json' EXIT
"$GODOT" --headless --path . --import >/dev/null 2>&1 || true

if [[ $NEW_BASE == 0 ]]; then
	PATCH="$UPDATES/patches/$BUILD.pck"
	"$GODOT" --headless --path . --export-patch "Windows Desktop" "$PATCH" --patches "$BASES/$BASE.pck" >/dev/null 2>&1
	if (( $(stat -c %s "$PATCH") > MAX_PATCH )); then
		echo "The patch is over 60 MB: cutting a new base instead."
		rm -f "$PATCH"
		NEW_BASE=1
	fi
fi

if [[ $NEW_BASE == 1 ]]; then
	rm -rf build/windows && mkdir -p build/windows
	"$GODOT" --headless --path . --export-release "Windows Desktop" build/windows/StellarFrontier.exe >/dev/null 2>&1
	"$GODOT" --headless --path . --export-pack "Windows Desktop" "$BASES/$BUILD.pck" >/dev/null 2>&1
	makensis -V2 -DVERSION="$VERSION" -DBUILD="$BUILD" -DEXE="$PWD/build/windows/StellarFrontier.exe" \
		-DOUT="$PWD/build/StellarFrontierRemake-Setup.exe" tools/installer/StellarFrontier.nsi
	[[ -n "$BASE" && "$BASE" != "$BUILD" ]] && rm -f "$BASES/$BASE.pck" "$BASES/$BASE.fp"
	echo "$FP" > "$BASES/$BUILD.fp"
	echo "$BUILD" > "$BASES/current"
	BASE="$BUILD"
	PATCHES="{}"
else
	SHA=$(sha256sum "$PATCH" | cut -d' ' -f1)
	SIZE=$(stat -c %s "$PATCH")
	PATCHES="{\"$BASE\": {\"url\": \"$REPO_URL/patches/$BUILD.pck\", \"size\": $SIZE, \"sha256\": \"$SHA\"}}"
fi
# only the newest patch is kept: it holds every change since the base
find "$UPDATES/patches" -name '*.pck' ! -name "$BUILD.pck" -delete
cat > "$UPDATES/updates.json" <<EOF
{"latest": "$BUILD", "version": "$VERSION", "base": "$BASE", "installer": "$INSTALLER_URL",
 "published": "$(date -u +%Y-%m-%dT%H:%M:%SZ)", "patches": $PATCHES}
EOF
cat "$UPDATES/updates.json"
[[ $NEW_BASE == 1 ]] && echo "NEW_INSTALLER=build/StellarFrontierRemake-Setup.exe"
exit 0
