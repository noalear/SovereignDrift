#!/bin/sh
# package_windows.sh -- NEW FILE (2026-10-12), Stellar Frontier Godot port.
#
# Builds the Windows release: the Godot application (64-bit) with the sfport
# extension, the original programs as 32-bit processes in game/, the game
# data, the license and the source code.
#
#   tools/package_windows.sh GODOT_EDITOR_BINARY OUT_DIR
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
GODOT=$1
OUT=$2
[ -n "$GODOT" ] && [ -n "$OUT" ] || { echo "usage: $0 GODOT_BINARY OUT_DIR"; exit 2; }
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
cd "$ROOT"

python3 tools/port_notices.py
make -j16 HOST=mingw sfcore
make -j16 godot/addons/sfport/bin/sfport.windows.x86_64.dll

rm -rf "$OUT/StellarFrontier"
mkdir -p "$OUT/StellarFrontier/game"
# SF_TEMPLATE: a slim export template (see tools/build_template.sh); the
# official one is 109 MB, too big for some ways of sending the game.
if [ -n "$SF_TEMPLATE" ]; then
  cp godot/export_presets.cfg "$OUT/export_presets.cfg.orig"
  sed -i "0,/^custom_template\/release=.*/s||custom_template/release=\"$SF_TEMPLATE\"|" godot/export_presets.cfg
fi
"$GODOT" --headless --path godot --export-release "Windows Desktop" "$OUT/StellarFrontier/StellarFrontierEnhanced.exe"
if [ -n "$SF_TEMPLATE" ]; then
  mv "$OUT/export_presets.cfg.orig" godot/export_presets.cfg
fi

for p in frontier server nexus; do
  cp "build/mingw/sfcore/$p.exe" "$OUT/StellarFrontier/game/$p.exe"
  i686-w64-mingw32-strip "$OUT/StellarFrontier/game/$p.exe"
done
for d in binwin dat bmp wav mid music music_remix music_remix_alt music_new doc zap icons; do
  cp -R "gamedata/$d" "$OUT/StellarFrontier/game/$d"
done
rm -f "$OUT"/StellarFrontier/game/binwin/*.log "$OUT"/StellarFrontier/game/dat/*.log \
      "$OUT"/StellarFrontier/game/binwin/sf.prf
cp original/stellar_frontier_shared_source_eula.txt "$OUT/StellarFrontier/"
cp tools/README-windows.txt "$OUT/StellarFrontier/README.txt"

# notices of the third-party code (Godot and its components, godot-cpp,
# zlib, the Liberation fonts the port's text rendering is made from)
N="$OUT/StellarFrontier/third-party-notices.txt"
"$GODOT" --headless --script tools/godot_licenses.gd -- "$N"
{
  printf '\n\n=== godot-cpp ===\n'; cat third_party/godot-cpp/LICENSE.md
  printf '\n\n=== zlib ===\n'; sed -n '/Copyright (C) 1995/,/madler@alumni.caltech.edu/p' third_party/zlib/zlib.h
  printf '\n\n=== Liberation fonts ===\n'; cat third_party/liberation/LICENSE
} >> "$N"

# the source code, as the license requires
python3 - "$ROOT" "$OUT/StellarFrontier/source-code.zip" <<'EOF'
import os, sys, zipfile
root, out = sys.argv[1], sys.argv[2]
os.chdir(root)
skip_dirs = {'.godot', 'bin', 'gen', '__pycache__'}
with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for top in ['src', 'tools', 'godot', 'Makefile', 'third_party/zlib', 'third_party/liberation']:
        if os.path.isfile(top):
            z.write(top, 'sfport/' + top)
            continue
        for d, dirs, fs in os.walk(top):
            dirs[:] = [x for x in dirs if x not in skip_dirs]
            for f in sorted(fs):
                if f.endswith(('.o', '.d', '.pyc', '.so', '.dll', '.a')):
                    continue
                p = os.path.join(d, f)
                z.write(p, 'sfport/' + p)
    z.write('original/stellar_frontier_shared_source_eula.txt',
            'sfport/stellar_frontier_shared_source_eula.txt')
EOF

cd "$OUT"
rm -f StellarFrontier.zip
python3 - <<'EOF'
import os, zipfile
with zipfile.ZipFile('StellarFrontier.zip', 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for d, _, fs in os.walk('StellarFrontier'):
        for f in sorted(fs):
            z.write(os.path.join(d, f))
print('zip bytes:', os.path.getsize('StellarFrontier.zip'))
EOF
echo "package ready in $OUT"
