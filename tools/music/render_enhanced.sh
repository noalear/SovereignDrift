#!/bin/sh
# render_enhanced.sh -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
#
# Builds the Enhanced Edition soundtrack: the eight original Stellar Frontier
# music tracks (Stardock's music from the 1997 game, released under the
# Stardock Shared Source license) re-arranged by rearrange.py -- same
# melodies, structure, tempo and length, with better instruments, layering,
# phrasing and a handful of reviewed note corrections -- then rendered with
# FluidSynth and the GeneralUser GS SoundFont by S. Christian Collins
# (https://www.schristiancollins.com), using the same synth, trim and
# loudness settings as tools/render_music.sh.
#
#   tools/music/render_enhanced.sh MID_DIR SOUNDFONT.sf2 [GAMEDATA_DIR]
#
# MID_DIR holds the game's original sf-music1..8.mid (read only).  Outputs:
#   GAMEDATA_DIR/music/sf-musicN.ogg      enhanced renders (what the game plays)
#   GAMEDATA_DIR/music_faithful/          the previous faithful renders, moved
#                                         here once, on the first run
#   tools/music/enhanced_mid/*.mid        the re-arranged MIDI files
#   tools/music/corrections.log           every note correction, for review
set -e
MID=$1; SF2=$2
HERE=$(cd "$(dirname "$0")" && pwd)
GD=${3:-$HERE/../../gamedata}
[ -n "$MID" ] && [ -f "$SF2" ] || { echo "usage: $0 MID_DIR SOUNDFONT.sf2 [GAMEDATA_DIR]"; exit 1; }
OUT=$GD/music
FAITH=$GD/music_faithful
EMID=$HERE/enhanced_mid
LOG=$HERE/corrections.log
mkdir -p "$OUT" "$EMID"

# keep the faithful renders (only once: never overwrite them with enhanced ones)
if [ ! -d "$FAITH" ]; then
  mkdir -p "$FAITH"
  for f in "$OUT"/sf-music*.ogg; do [ -f "$f" ] && mv "$f" "$FAITH/"; done
fi

{
  echo "# corrections.log -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition."
  echo "# Note corrections made by tools/music/rearrange.py to the original game MIDI files"
  echo "# (Stardock's music).  CHANGED lines were applied; KEPT lines are clashes that were"
  echo "# examined and deliberately left alone, with the reason.  Times are bar:beat.fraction."
  echo
} > "$LOG"

TMP=$(mktemp -d)
for i in 1 2 3 4 5 6 7 8; do
  n=sf-music$i
  python3 "$HERE/rearrange.py" "$MID/$n.mid" "$EMID/$n.mid" --log "$LOG"
  echo "rendering $n"
  fluidsynth -ni -q -F "$TMP/$n.wav" -r 48000 -g 0.5 -o audio.file.format=float \
    -o synth.reverb.active=1 -o synth.reverb.room-size=0.55 -o synth.reverb.damp=0.35 \
    -o synth.reverb.width=0.8 -o synth.reverb.level=0.55 \
    -o synth.chorus.active=1 -o synth.chorus.level=1.2 \
    -o synth.polyphony=512 \
    "$SF2" "$EMID/$n.mid"
  # trim the silence after the reverb tail, normalise to -16 LUFS, -1 dBTP
  ffmpeg -v error -y -i "$TMP/$n.wav" \
    -af "areverse,silenceremove=start_periods=1:start_threshold=-70dB:start_silence=0.5,areverse,loudnorm=I=-16:TP=-1.0:LRA=11" \
    -ar 48000 -c:a libvorbis -q:a 7 "$OUT/$n.ogg"
done
rm -rf "$TMP"
