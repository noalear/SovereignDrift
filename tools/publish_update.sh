#!/usr/bin/env bash
# Publish the current commit to players (see src/autoload/Updater.gd):
#   GODOT=/path/to/godot UPDATES=/path/to/SFUpdater BASES=/path/to/bases \
#     tools/publish_update.sh [--new-base]
# The installer's game data is the base.  Each later build ships as one patch
# pack of the files changed since that base, so players download only that.
# A patch over 90 MB goes up in 90 MB parts (GitHub refuses files over
# 100 MB) that the game joins after download.
# A new base (and so a new installer) is cut on the first run or with
# --new-base.  Without it the script stops (exit 2) when Updater.gd, the
# autoload list, the display or rendering settings or the engine changed
# (they load before any patch), or when the patch grows past 500 MB.
# BASES keeps the base pack the patches are made against.
# Prints NEW_INSTALLER=build/SovereignDrift-Setup.exe when the
# installer has to be handed out again.
set -euo pipefail
cd "$(dirname "$0")/.."
GODOT="${GODOT:-godot}"
: "${UPDATES:?set UPDATES to a checkout of noalear/SFUpdater}"
: "${BASES:?set BASES to the folder that keeps base packs}"
REPO_URL="https://raw.githubusercontent.com/noalear/SFUpdater/main"
INSTALLER_URL="${INSTALLER_URL:-https://drive.google.com/file/d/1xAcmV3v4agDk25Q6vmtaA_xgXHyXHVy7/view}"
MAX_PATCH=$((500 * 1024 * 1024))
PART=$((90 * 1024 * 1024))
VERSION=$(sed -n 's/^config\/version="\(.*\)"/\1/p' project.godot)
BUILD=$(git rev-parse --short HEAD)
# what a patch can't change: the updater itself, the autoload list, the
# window and renderer setup, the engine (new input actions are fine: the
# patched Keybinds adds them from project_settings.cfg)
FP=$( (cat src/autoload/Updater.gd; awk '/^\[/{keep = ($0 ~ /^\[(autoload|display|rendering)\]/)} keep' project.godot; "$GODOT" --version) | sha256sum | cut -d' ' -f1)
mkdir -p "$BASES" "$UPDATES/patches"
BASE=$(cat "$BASES/current" 2>/dev/null || true)
NEW_BASE=0
if [[ "${1:-}" == "--new-base" || -z "$BASE" ]]; then
	NEW_BASE=1
elif [[ ! -f "$BASES/$BASE.pck" || "$(cat "$BASES/$BASE.fp" 2>/dev/null)" != "$FP" ]]; then
	# a new base means every player reinstalls: only on purpose
	echo "This build can't be a patch over $BASE (Updater.gd, the autoload, display or rendering settings, or the engine changed, or the base pack is missing)."
	echo "Run again with --new-base to cut a new installer."
	exit 2
fi

printf '{"build": "%s", "version": "%s"}\n' "$BUILD" "$VERSION" > build.json
cp project.godot project_settings.cfg
trap 'rm -f build.json project_settings.cfg' EXIT
"$GODOT" --headless --path . --import >/dev/null 2>&1 || true

if [[ $NEW_BASE == 0 ]]; then
	PATCH="$UPDATES/patches/$BUILD.pck"
	rm -f "$PATCH" "$PATCH".[0-9][0-9]
	"$GODOT" --headless --path . --export-patch "Windows Desktop" "$PATCH" --patches "$BASES/$BASE.pck" >/dev/null 2>&1
	if (( $(stat -c %s "$PATCH") > MAX_PATCH )); then
		rm -f "$PATCH"
		echo "The patch is over 500 MB: time for a new installer. Run again with --new-base."
		exit 2
	fi
fi

if [[ $NEW_BASE == 1 ]]; then
	rm -rf build/windows && mkdir -p build/windows
	"$GODOT" --headless --path . --export-release "Windows Desktop" build/windows/StellarFrontier.exe >/dev/null 2>&1
	"$GODOT" --headless --path . --export-pack "Windows Desktop" "$BASES/$BUILD.pck" >/dev/null 2>&1
	makensis -V2 -DVERSION="$VERSION" -DBUILD="$BUILD" -DEXE="$PWD/build/windows/StellarFrontier.exe" \
		-DOUT="$PWD/build/SovereignDrift-Setup.exe" tools/installer/StellarFrontier.nsi
	[[ -n "$BASE" && "$BASE" != "$BUILD" ]] && rm -f "$BASES/$BASE.pck" "$BASES/$BASE.fp"
	echo "$FP" > "$BASES/$BUILD.fp"
	echo "$BUILD" > "$BASES/current"
	BASE="$BUILD"
	PATCHES="{}"
else
	SHA=$(sha256sum "$PATCH" | cut -d' ' -f1)
	SIZE=$(stat -c %s "$PATCH")
	ENTRY="\"url\": \"$REPO_URL/patches/$BUILD.pck\", \"size\": $SIZE, \"sha256\": \"$SHA\""
	if (( SIZE > PART )); then
		split -b "$PART" -d -a 2 "$PATCH" "$PATCH."
		rm -f "$PATCH"
		PARTS=""
		for f in "$PATCH".[0-9][0-9]; do
			PARTS+="${PARTS:+, }{\"url\": \"$REPO_URL/patches/$(basename "$f")\", \"size\": $(stat -c %s "$f"), \"sha256\": \"$(sha256sum "$f" | cut -d' ' -f1)\"}"
		done
		ENTRY="\"size\": $SIZE, \"sha256\": \"$SHA\", \"parts\": [$PARTS]"
	fi
	PATCHES="{\"$BASE\": {$ENTRY}}"
fi
# only the newest patch is kept: it holds every change since the base
find "$UPDATES/patches" -name '*.pck*' ! -name "$BUILD.pck" ! -name "$BUILD.pck.[0-9][0-9]" -delete
cat > "$UPDATES/updates.json" <<EOF
{"latest": "$BUILD", "version": "$VERSION", "base": "$BASE", "installer": "$INSTALLER_URL",
 "published": "$(date -u +%Y-%m-%dT%H:%M:%SZ)", "patches": $PATCHES}
EOF
cat "$UPDATES/updates.json"
[[ $NEW_BASE == 1 ]] && echo "NEW_INSTALLER=build/SovereignDrift-Setup.exe"
exit 0
