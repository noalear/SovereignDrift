#!/bin/sh
# render_music.sh -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
#
# High-fidelity versions of the original music: the game's own MIDI files
# (mid/sf-music1..8.mid, the same notes the 1997 game played) rendered with
# FluidSynth and the GeneralUser GS SoundFont (S. Christian Collins, free
# for any music creation; https://www.schristiancollins.com), with the
# synthesizer's reverb and chorus, then loudness-normalised and encoded as
# Ogg Vorbis.  The Enhanced Edition plays game/music/<name>.ogg in place of
# the MIDI file of the same name.
#
#   tools/render_music.sh MID_DIR SOUNDFONT.sf2 OUT_DIR
set -e
MID=$1; SF2=$2; OUT=$3
mkdir -p "$OUT"
TMP=$(mktemp -d)
for f in "$MID"/*.mid; do
  n=$(basename "$f" .mid)
  echo "rendering $n"
  fluidsynth -ni -q -F "$TMP/$n.wav" -r 48000 -g 0.5 \
    -o synth.reverb.active=1 -o synth.reverb.room-size=0.55 -o synth.reverb.damp=0.35 \
    -o synth.reverb.width=0.8 -o synth.reverb.level=0.55 \
    -o synth.chorus.active=1 -o synth.chorus.level=1.2 \
    -o synth.polyphony=512 \
    "$SF2" "$f"
  # trim the silence after the reverb tail, normalise to -16 LUFS, -1 dBTP
  ffmpeg -v error -y -i "$TMP/$n.wav" \
    -af "areverse,silenceremove=start_periods=1:start_threshold=-70dB:start_silence=0.5,areverse,loudnorm=I=-16:TP=-1.0:LRA=11" \
    -ar 48000 -c:a libvorbis -q:a 7 "$OUT/$n.ogg"
done
rm -rf "$TMP"
