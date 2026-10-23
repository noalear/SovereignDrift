#!/bin/sh
# make_gamedata.sh -- NEW FILE (2026-10-12), Stellar Frontier Godot port.
#
# Lays out a game install directory the way the original 1.10a installer
# did (binwin/, dat/, bmp/, wav/, mid/, doc/) from the released source
# tree, and rebuilds the bitmap archives the original build appended to
# frontier.exe, server.exe and spy.exe (src/build.bat: zap + zapexe).
#
#   tools/make_gamedata.sh ORIGINAL_FRONTIER_DIR OUT_DIR ZAP_TOOL
#
# ORIGINAL_FRONTIER_DIR is frontier_source/frontier from the source release.
# The archives go to OUT_DIR/zap/{frontier,server,spy}.zap and the window
# icons to OUT_DIR/icons/.
set -e
SRC=$(cd "$1" && pwd)
mkdir -p "$2"
OUT=$(cd "$2" && pwd)
ZAP=$(cd "$(dirname "$3")" && pwd)/$(basename "$3")

for d in binwin dat bmp wav mid doc; do
  rm -rf "$OUT/$d"
  cp -R "$SRC/$d" "$OUT/$d"
done
chmod -R u+w "$OUT"

mkdir -p "$OUT/zap" "$OUT/icons"
cd "$SRC/src/gui"
"$ZAP" "$OUT/zap/frontier.zap" egg.bmp
"$ZAP" "$OUT/zap/spy.zap" blue_spy.bmp blue_close.bmp blue_listbox.bmp blue_scroll.bmp \
  blue_up.bmp blue_down.bmp blue_slider.bmp blue_font.bmp blue_info.bmp blue_refresh.bmp
"$ZAP" "$OUT/zap/server.zap" blue_server.bmp blue_close.bmp blue_playerlist.bmp \
  blue_scroll.bmp blue_up.bmp blue_down.bmp blue_slider.bmp blue_font.bmp blue_start.bmp \
  blue_stop.bmp blue_console.bmp blue_nexusreg.bmp blue_16fps.bmp blue_11fps.bmp \
  blue_scenariolist.bmp blue_warn.bmp blue_kick.bmp blue_textbox.bmp blue_minimize.bmp \
  blue_lag.bmp blue_prompt.bmp blue_yes.bmp blue_no.bmp
cp sf_WIN32.ico "$OUT/icons/frontier.ico"
cp server_WIN32.ico "$OUT/icons/server.ico"
cp spy_WIN32.ico "$OUT/icons/spy.ico"
echo "game data ready in $OUT"
