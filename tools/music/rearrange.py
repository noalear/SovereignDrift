#!/usr/bin/env python3
# rearrange.py -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
#
# "Same songs, better instruments": re-arranges the eight music tracks of
# Stellar Frontier (1997).  The music is Stardock's, from the original game
# (released under the Stardock Shared Source license, non-commercial use);
# this tool only re-voices and re-performs the game's own MIDI files.  The
# renders use the GeneralUser GS SoundFont by S. Christian Collins
# (https://www.schristiancollins.com).
#
# Every melody, chord, bar and tempo of the original is kept.  The tool:
#   * corrects a small number of probable wrong notes (harsh minor-2nd
#     clashes with out-of-key or avoid notes, stray octave outliers,
#     duplicated same-pitch notes) and logs every change for review;
#   * quantises toward the grid only where the original is loose (the game's
#     DirectMusic exports are essentially on-grid, so this rarely fires) and
#     tidies note overlaps in monophonic lines;
#   * re-performs dynamics: phrase-arched velocities, subtle downbeat
#     accents, crescendos into section changes, CC11 swells on sustained
#     parts, and gentle deterministic humanisation;
#   * re-orchestrates: better General MIDI / GS variation patches where the
#     1997 ones are thin or dated, a soft pad layer doubling the existing
#     sustained harmony, a sine sub an octave under real bass parts, tuned
#     timpani under the orchestra stabs, a GS drum kit with light kick/snare
#     layering, per-part stereo placement and reverb/chorus sends.  Only the
#     MIDI channels the original leaves unused (11-13 of the free 11-16)
#     carry the added layers; channel 10 stays drums.
#
#   rearrange.py IN.mid OUT.mid [--log corrections.log]   re-arrange one song
#   rearrange.py --analyze IN.mid [IN2.mid ...]             print an analysis
#
# Deterministic: all randomness comes from a seed derived from the song name.

import argparse
import bisect
import collections
import math
import os
import random

import mido

# --------------------------------------------------------------------------
# General MIDI names (for the analysis and the log)
GM = ("Acoustic Grand Piano,Bright Acoustic Piano,Electric Grand Piano,Honky-tonk Piano,"
      "Electric Piano 1,Electric Piano 2,Harpsichord,Clavinet,Celesta,Glockenspiel,Music Box,"
      "Vibraphone,Marimba,Xylophone,Tubular Bells,Dulcimer,Drawbar Organ,Percussive Organ,"
      "Rock Organ,Church Organ,Reed Organ,Accordion,Harmonica,Tango Accordion,Nylon Guitar,"
      "Steel Guitar,Jazz Guitar,Clean Guitar,Muted Guitar,Overdriven Guitar,Distortion Guitar,"
      "Guitar Harmonics,Acoustic Bass,Finger Bass,Pick Bass,Fretless Bass,Slap Bass 1,"
      "Slap Bass 2,Synth Bass 1,Synth Bass 2,Violin,Viola,Cello,Contrabass,Tremolo Strings,"
      "Pizzicato Strings,Orchestral Harp,Timpani,String Ensemble 1,String Ensemble 2,"
      "Synth Strings 1,Synth Strings 2,Choir Aahs,Voice Oohs,Synth Voice,Orchestra Hit,"
      "Trumpet,Trombone,Tuba,Muted Trumpet,French Horn,Brass Section,Synth Brass 1,"
      "Synth Brass 2,Soprano Sax,Alto Sax,Tenor Sax,Baritone Sax,Oboe,English Horn,Bassoon,"
      "Clarinet,Piccolo,Flute,Recorder,Pan Flute,Blown Bottle,Shakuhachi,Whistle,Ocarina,"
      "Square Lead,Saw Lead,Calliope Lead,Chiff Lead,Charang,Voice Lead,Fifths Lead,"
      "Bass+Lead,Fantasia Pad,Warm Pad,Polysynth,Choir Pad,Bowed Pad,Metallic Pad,Halo Pad,"
      "Sweep Pad,Rain FX,Soundtrack FX,Crystal FX,Atmosphere FX,Brightness FX,Goblins FX,"
      "Echoes FX,Sci-fi FX,Sitar,Banjo,Shamisen,Koto,Kalimba,Bagpipe,Fiddle,Shanai,"
      "Tinkle Bell,Agogo,Steel Drums,Woodblock,Taiko Drum,Melodic Tom,Synth Drum,"
      "Reverse Cymbal,Guitar Fret Noise,Breath Noise,Seashore,Bird Tweet,Telephone Ring,"
      "Helicopter,Applause,Gunshot").split(",")
assert len(GM) == 128
DRUM = {35: "Kick 2", 36: "Kick 1", 37: "Side Stick", 38: "Snare 1", 39: "Clap", 40: "Snare 2",
        41: "Low Floor Tom", 42: "Closed HH", 43: "High Floor Tom", 44: "Pedal HH",
        45: "Low Tom", 46: "Open HH", 47: "Low-Mid Tom", 48: "Hi-Mid Tom", 49: "Crash 1",
        50: "High Tom", 51: "Ride 1", 52: "China", 53: "Ride Bell", 54: "Tambourine",
        55: "Splash", 56: "Cowbell", 57: "Crash 2", 59: "Ride 2", 63: "Open Hi Conga",
        64: "Low Conga", 65: "High Timbale", 66: "Low Timbale", 69: "Cabasa", 75: "Claves",
        81: "Open Triangle"}
NOTE = "C C# D Eb E F F# G Ab A Bb B".split()


def pname(p):
    return "%s%d" % (NOTE[p % 12], p // 12 - 1)


# Krumhansl-Kessler key profiles
KK_MAJ = [6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88]
KK_MIN = [6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17]
SCALE_MAJ = {0, 2, 4, 5, 7, 9, 11}
SCALE_MIN = {0, 2, 3, 5, 7, 8, 10, 9, 11}       # natural + harmonic/melodic 6, 7

PERC_PROGS = set(range(112, 128)) | {47}       # unpitched-ish melodic channels
BASS_PROGS = set(range(32, 40)) | {43, 58}

# --------------------------------------------------------------------------
# Per-song settings.  remap: original GM program -> (bank, program, note) of
# the replacement (GeneralUser GS banks: 0 = GM, 8/11/12 = GS variations).
# cap: maximum note length in beats for that replacement (stab patches).

DEFAULT_REMAP = {
    3: (0, 1, "honky-tonk piano -> bright grand piano"),
    55: (12, 48, "orchestra hit -> GS Full Orchestra stab"),
    62: (0, 61, "synth brass 1 -> brass section"),
    63: (8, 61, "synth brass 2 -> GS Brass Section 2"),
    78: (0, 73, "whistle -> flute"),
}
DEFAULT_CAP = {55: 0.55}          # seconds: the replacement sustains, the hit didn't

# Hand-reviewed corrections (applied after the automatic pass), keyed by bar,
# beat, MIDI channel (1-based) and original pitch.
MANUAL = {
    "sf-music7": [
        (18, 3, 6, "G5", "Ab5", "piccolo G5 sat a semitone under the trumpet's Ab5 for a "
         "whole bar on the Db power chord (C-pattern carried onto the Db chord); now doubles "
         "the chord fifth"),
        (62, 3, 6, "G5", "Ab5", "same figure as bar 18"),
    ],
}

SONGS = {           # kit: GS drum set; pad: (bank, program) of the pad layer;
                    # *_vol: CC7 of the added layers (tuned against the low-end
                    # and level measurements of the faithful renders)
    "sf-music1": dict(kit=8, pad=(0, 89), pad_vel=64, pad_vol=96, sub=False, expr=1.0),
    "sf-music2": dict(kit=8, pad=(0, 89), pad_vel=62, pad_vol=96, sub=True, sub_vol=66,
                      expr=1.0),
    "sf-music3": dict(kit=8, pad=(0, 89), pad_vel=64, pad_vol=96, sub=True, sub_vol=54,
                      expr=0.9),
    "sf-music4": dict(kit=8, pad=(0, 94), pad_vel=62, pad_vol=96, sub=True, sub_vol=72,
                      expr=1.0),
    "sf-music5": dict(kit=16, pad=(0, 89), pad_vel=70, pad_vol=96, sub=True, sub_vol=50,
                      expr=0.9),
    "sf-music6": dict(kit=16, pad=(0, 89), pad_vel=60, pad_vol=100, sub=True, sub_vol=74,
                      expr=0.9),
    "sf-music7": dict(kit=8, pad=(0, 89), pad_vel=64, pad_vol=110, sub=True, sub_vol=118,
                      expr=0.9),
    "sf-music8": dict(kit=8, pad=(0, 89), pad_vel=60, pad_vol=100, sub=True, sub_vol=66,
                      expr=0.9),
}

PAD_CH, SUB_CH, BOOM_CH = 10, 11, 12   # MIDI channels 11-13 (0-based 10-12)
TIMPANI = (0, 47)
SINE = (8, 80)                     # GeneralUser GS "Sine Wave"

# --------------------------------------------------------------------------


class Note(object):
    __slots__ = ("start", "end", "pitch", "vel", "ch", "track", "prog", "role",
                 "layer", "src")

    def __init__(self, start, end, pitch, vel, ch, track):
        self.start, self.end, self.pitch, self.vel = start, end, pitch, vel
        self.ch, self.track = ch, track
        self.prog, self.role, self.layer, self.src = 0, "melodic", None, None

    @property
    def dur(self):
        return self.end - self.start

    @property
    def part(self):
        return (self.track, self.ch)


class Song(object):
    def __init__(self, path):
        self.path = path
        self.name = os.path.splitext(os.path.basename(path))[0]
        mid = mido.MidiFile(path)
        self.tpb = mid.ticks_per_beat
        self.bar = self.tpb * 4                     # every song is 4/4
        self.notes, self.ctrl, self.meta = [], [], []
        self.stuck = 0
        self.end = 0
        for ti, tr in enumerate(mid.tracks):
            t = 0
            on = collections.defaultdict(list)
            for msg in tr:
                t += msg.time
                if msg.type == "note_on" and msg.velocity > 0:
                    on[(msg.channel, msg.note)].append((t, msg.velocity))
                elif msg.type in ("note_on", "note_off"):
                    k = (msg.channel, msg.note)
                    if on[k]:
                        s, v = on[k].pop(0)
                        self.notes.append(Note(s, t, msg.note, v, msg.channel, ti))
                elif msg.is_meta:
                    if msg.type in ("set_tempo", "time_signature"):
                        self.meta.append((t, msg))
                elif msg.type != "sysex":
                    self.ctrl.append((t, msg))
            self.end = max(self.end, t)
            for k, lst in on.items():
                for s, v in lst:          # unterminated note: end it at track end
                    self.notes.append(Note(s, t, k[1], v, k[0], ti))
                    self.stuck += 1
        self.notes = [n for n in self.notes if n.end > n.start]
        self.notes.sort(key=lambda n: (n.start, n.ch, n.pitch))
        # tempo (all songs have a single effective tempo at tick 0)
        tempos = [m for t, m in self.meta if m.type == "set_tempo"]
        self.tempo = tempos[-1].tempo if tempos else 500000
        # program timeline per channel
        self.progs = collections.defaultdict(list)
        for t, m in sorted(self.ctrl, key=lambda x: x[0]):
            if m.type == "program_change":
                self.progs[m.channel].append((t, m.program))
        for n in self.notes:
            n.prog = self.prog_at(n.ch, n.start)
        self.classify()

    def prog_at(self, ch, t):
        lst = self.progs.get(ch, [])
        i = bisect.bisect_right([x[0] for x in lst], t) - 1
        return lst[i][1] if i >= 0 else 0

    def sec(self, ticks):
        return ticks * self.tempo / 1e6 / self.tpb

    def barbeat(self, t):
        return "%d:%d.%02d" % (t // self.bar + 1, (t % self.bar) // self.tpb + 1,
                               (t % self.tpb) * 100 // self.tpb)

    def classify(self):
        """Role per (track, channel, program) segment."""
        seg = collections.defaultdict(list)
        for n in self.notes:
            seg[(n.track, n.ch, n.prog)].append(n)
        self.segs = seg
        for key, ns in seg.items():
            ch, prog = key[1], key[2]
            ps = sorted(n.pitch for n in ns)
            ds = sorted(n.dur for n in ns)
            med_p, med_d = ps[len(ps) // 2], ds[len(ds) // 2]
            if ch == 9:
                role = "drums"
            elif prog in PERC_PROGS:
                role = "perc"
            elif med_p < 50 and (prog in BASS_PROGS or med_p < 46):
                role = "bass"
            elif med_d >= 1.5 * self.tpb and (prog in range(40, 56) or prog in range(88, 96)
                                              or prog in range(16, 22) or prog in range(56, 64)
                                              or prog in (68, 69, 70, 71, 73)):
                role = "sustain"
            else:
                role = "melodic"
            for n in ns:
                n.role = role

    # ---------------------------------------------------------------- harmony
    def pitched(self):
        return [n for n in self.notes if n.role not in ("drums", "perc")]

    def keys(self, window_bars=8):
        """Key estimate (tonic pc, 'maj'/'min', score, pc weights) per window."""
        nb = int(math.ceil(self.end / float(self.bar)))
        res = []
        for w0 in range(0, nb, window_bars):
            a, b = w0 * self.bar, min(nb, w0 + window_bars) * self.bar
            h = [0.0] * 12
            for n in self.pitched():
                ov = min(n.end, b) - max(n.start, a)
                if ov > 0:
                    h[n.pitch % 12] += ov * (1.5 if n.role == "bass" else 1.0)
            res.append((a, b) + best_key(h) + (h,))
        return res


def best_key(h):
    if sum(h) == 0:
        return (0, "maj", 0.0)
    best = None
    for tonic in range(12):
        for mode, prof in (("maj", KK_MAJ), ("min", KK_MIN)):
            r = pearson([h[(tonic + i) % 12] for i in range(12)], prof)
            if best is None or r > best[2]:
                best = (tonic, mode, r)
    return best


def pearson(x, y):
    mx, my = sum(x) / 12.0, sum(y) / 12.0
    sx = math.sqrt(sum((a - mx) ** 2 for a in x))
    sy = math.sqrt(sum((b - my) ** 2 for b in y))
    if sx == 0 or sy == 0:
        return 0.0
    return sum((a - mx) * (b - my) for a, b in zip(x, y)) / (sx * sy)


def scale_of(tonic, mode):
    return {(tonic + i) % 12 for i in (SCALE_MAJ if mode == "maj" else SCALE_MIN)}


def keyname(tonic, mode):
    return NOTE[tonic] + ("" if mode == "maj" else "m")


# --------------------------------------------------------------------------
# Analysis report


def analyze(song):
    out = []
    w = out.append
    bpm = mido.tempo2bpm(song.tempo)
    w("## %s\n" % song.name)
    w("- Tempo %.0f BPM, 4/4, %.1f bars, %.1f s (tpb %d); %d notes; stuck notes: %d"
      % (bpm, song.end / float(song.bar), song.sec(song.end), song.tpb, len(song.notes),
         song.stuck))
    # timing grid (DirectMusic export: notes start 1 tick after the grid)
    def off(n, g):
        r = (n.start - 1) % g
        return min(r, g - r)
    N = float(len(song.notes))
    on16 = sum(1 for n in song.notes if off(n, 96) <= 2)
    ontr = sum(1 for n in song.notes if off(n, 96) > 2 and off(n, 64) <= 2)
    on32 = sum(1 for n in song.notes if off(n, 96) > 2 and off(n, 64) > 2
               and min(off(n, 48), off(n, 32)) <= 2)
    loose = [n for n in song.notes if min(off(n, g) for g in (96, 64, 48, 32)) > 2]
    w("- Timing: %.1f%% of onsets on the 16th grid, %.1f%% on 8th triplets, %.1f%% on "
      "32nds / 16th triplets (rolls); %d loose onsets (> 2 ticks off every grid). Notes "
      "start 1 tick after the grid and end 1 tick before the next grid line "
      "(DirectMusic export), so there is nothing to quantise."
      % (100 * on16 / N, 100 * ontr / N, 100 * on32 / N, len(loose)))
    w("")
    w("| ch | role | programs (first use bar) | notes | range | velocities | median len |")
    w("|---|---|---|---|---|---|---|")
    bych = collections.defaultdict(list)
    for n in song.notes:
        bych[n.ch].append(n)
    for ch in sorted(bych):
        ns = bych[ch]
        roles = collections.Counter(n.role for n in ns)
        progs = []
        for n in ns:
            if not progs or progs[-1][0] != n.prog:
                progs.append((n.prog, n.start))
        seen, pl = set(), []
        for p, t in progs:
            if p not in seen:
                seen.add(p)
                pl.append("%s (%d)" % ("kit" if ch == 9 else GM[p], t // song.bar + 1))
        vs = sorted(set(n.vel for n in ns))
        ds = sorted(n.dur for n in ns)
        if ch == 9:
            dc = collections.Counter(n.pitch for n in ns)
            rng = ", ".join("%s x%d" % (DRUM.get(p, p), c) for p, c in sorted(dc.items()))
        else:
            rng = "%s-%s" % (pname(min(n.pitch for n in ns)), pname(max(n.pitch for n in ns)))
        w("| %d | %s | %s | %d | %s | %s | %.2f beats |" % (
            ch + 1, "/".join(r for r, _ in roles.most_common()), "; ".join(pl), len(ns), rng,
            "%d-%d (%d levels)" % (vs[0], vs[-1], len(vs)), ds[len(ds) // 2] / float(song.tpb)))
    w("")
    ks = song.keys()
    w("- Key estimate per 8 bars: " + ", ".join(
        "%d-%d %s (%.2f)" % (a // song.bar + 1, b // song.bar, keyname(t, m), r)
        for a, b, t, m, r, h in ks))
    bounds = sections(song)
    w("- Section boundaries detected at bars: " + ", ".join(str(b // song.bar + 1)
                                                            for b in bounds))
    # same pitch overlaps
    dup = overlaps(song.notes)
    w("- Same-channel same-pitch overlaps/duplicates: %d" % len(dup))
    w("")
    return "\n".join(out)


def sections(song):
    """Section starts: 4-bar blocks where the instrumentation or density changes
    clearly (DirectMusic segments here are 4-bar multiples)."""
    blk = 4 * song.bar
    nb = int(math.ceil(song.end / float(blk)))
    act = [collections.Counter() for _ in range(nb)]
    dens = [0] * nb
    for n in song.notes:
        b = n.start // blk
        if b < nb:
            act[b][(n.ch, n.prog if n.ch != 9 else 0)] += n.dur
            dens[b] += 1
    res = []
    for b in range(1, nb):
        a0 = {k for k, v in act[b - 1].items() if v >= song.tpb}
        a1 = {k for k, v in act[b].items() if v >= song.tpb}
        u = a0 | a1
        j = len(a0 & a1) / float(len(u)) if u else 1.0
        d0, d1 = dens[b - 1] + 1, dens[b] + 1
        r = d1 / float(d0)
        # a section that arrives bigger: busier, or busier with new parts, or
        # re-orchestrated at the same energy
        if r >= 1.3 or (r >= 1.18 and j < 0.9) or (j <= 0.6 and r >= 0.9):
            res.append(b * blk)
    return res


def overlaps(notes):
    by = collections.defaultdict(list)
    for n in notes:
        by[(n.ch, n.pitch)].append(n)
    res = []
    for k, ns in by.items():
        ns.sort(key=lambda n: (n.start, -n.end))
        for a, b in zip(ns, ns[1:]):
            if b.start < a.end:
                res.append((a, b))
    return res


# --------------------------------------------------------------------------
# Corrections


class Log(object):
    def __init__(self, song):
        self.song, self.lines, self.changed, self.kept = song, [], 0, 0
        self.agg = {}

    def text(self):
        out = list(self.lines)
        for (track, ch, old, why), (cnt, i) in self.agg.items():
            if cnt > 1:
                out[i] += "  (x%d similar in this part)" % cnt
        return out

    def add(self, kind, n, old, new, reason):
        s = self.song
        if kind == "CHANGED":
            self.changed += 1
        else:
            self.kept += 1
            key = (n.track, n.ch, old, reason.split(" against ")[-1].split("; ")[-1][:60])
            if key in self.agg:
                self.agg[key][0] += 1
                return
            self.agg[key] = [1, len(self.lines)]
        self.lines.append("%s  %-8s track %2d ch %2d  %-9s %-4s -> %-4s  %s  [%s]" % (
            s.name, kind, n.track, n.ch + 1, s.barbeat(n.start), old, new, reason,
            GM[n.prog] if n.ch != 9 else "drums"))


def fix_manual(song, log):
    for bar, beat, ch, old, new, why in MANUAL.get(song.name, []):
        t = (bar - 1) * song.bar + int((beat - 1) * song.tpb)
        hit = [n for n in song.notes if n.ch == ch - 1 and abs(n.start - t) <= 2
               and pname(n.pitch) == old]
        for n in hit:
            q = n.pitch + ((NOTE.index(new[:-1]) - NOTE.index(old[:-1]) + 6) % 12 - 6)
            log.add("CHANGED", n, old, pname(q), "hand-reviewed: " + why)
            n.pitch = q
        if not hit:
            print("warning: manual fix not matched: %s %d:%s ch%d %s" % (song.name, bar, beat,
                                                                      ch, old))


def fix_duplicates(song, log):
    """Same channel + pitch overlapping: drop exact doubles, cut the earlier note."""
    drop = set()
    for a, b in overlaps([n for n in song.notes]):
        if id(a) in drop or id(b) in drop:
            continue
        if b.start - a.start <= 2:
            a.end = max(a.end, b.end)
            a.vel = max(a.vel, b.vel)
            drop.add(id(b))
            log.add("CHANGED", b, pname(b.pitch), "-", "duplicate of same-pitch note in "
                    "track %d, removed" % a.track)
        else:
            old = a.end
            a.end = b.start - 1
            log.add("CHANGED", a, pname(a.pitch), pname(a.pitch),
                    "overlapped next same-pitch note; shortened by %d ticks" % (old - a.end))
    song.notes = [n for n in song.notes if id(n) not in drop]


def fix_range(song, log):
    """Isolated notes more than 15 semitones from their part's median: octave fold."""
    for key, ns in song.segs.items():
        if key[1] == 9 or len(ns) < 12 or ns[0].role == "perc":
            continue
        ns = sorted(ns, key=lambda n: n.start)
        ps = sorted(n.pitch for n in ns)
        med = ps[len(ps) // 2]
        lo, hi = ps[len(ps) // 10], ps[len(ps) * 9 // 10]
        for i, n in enumerate(ns):
            dev = n.pitch - med
            if abs(dev) <= 15 or lo - 4 <= n.pitch <= hi + 4:
                continue
            nb = [m for m in ns[max(0, i - 3):i + 4] if m is not n]
            if any(abs(m.pitch - n.pitch) <= 7 for m in nb):
                continue          # part of a run up there: intentional
            if any(m.pitch == n.pitch and m.part != n.part and
                   m.start < n.end and m.end > n.start for m in song.notes if m.ch != 9):
                log.add("KEPT", n, pname(n.pitch), "", "outside the part's range (%+d st) but "
                        "doubled by another part" % dev)
                continue
            new = n.pitch
            while new > hi + 4:
                new -= 12
            while new < lo - 4:
                new += 12
            log.add("CHANGED", n, pname(n.pitch), pname(new),
                    "stray note %+d st from part median %s, octave-folded"
                    % (dev, pname(med)))
            n.pitch = new


def build_index(notes):
    notes = sorted(notes, key=lambda n: n.start)
    starts = [n.start for n in notes]
    maxdur = max([n.dur for n in notes] or [0])
    return notes, starts, maxdur


def concurrent(idx, n, min_frac=0.5):
    notes, starts, maxdur = idx
    i0 = bisect.bisect_left(starts, n.start - maxdur)
    i1 = bisect.bisect_left(starts, n.end)
    res = []
    for m in notes[i0:i1]:
        if m is n or m.part == n.part:
            continue
        ov = min(m.end, n.end) - max(m.start, n.start)
        if ov > 0 and (ov >= min_frac * n.dur or m.start <= n.start < m.end):
            res.append((m, ov))
    return res


def motif_count(line, n, prev, nxt):
    """How often this part plays the same 3-note figure with the same rhythm."""
    if prev is None or nxt is None:
        return 0
    sig = (prev.pitch - n.pitch, nxt.pitch - n.pitch, n.start - prev.start,
           nxt.start - n.start, n.pitch)
    cnt = 0
    for i in range(1, len(line) - 1):
        a, b, c = line[i - 1], line[i], line[i + 1]
        if (a.pitch - b.pitch, c.pitch - b.pitch, b.start - a.start, c.start - b.start,
                b.pitch) == sig:
            cnt += 1
    return cnt


def fix_harmony(song, log, rng):
    tpb = song.tpb
    keys = song.keys()
    pitched = song.pitched()
    idx = build_index(pitched)
    lines = collections.defaultdict(list)
    for n in pitched:
        lines[n.part].append(n)
    for p in lines:
        lines[p].sort(key=lambda n: (n.start, n.pitch))

    def window(t):
        for k in keys:
            if k[0] <= t < k[1]:
                return k
        return keys[-1]

    def neighbours(n):
        ln = lines[n.part]
        i = ln.index(n)
        prev = next(iter([m for m in reversed(ln[:i]) if m.start < n.start]), None)
        nxt = next(iter([m for m in ln[i + 1:] if m.start > n.start]), None)
        chord = [m for m in ln if m.start == n.start and m is not n]
        return prev, nxt, chord

    changes = []
    for n in pitched:
        a, b, tonic, mode, r, h = window(n.start)
        scale = scale_of(tonic, mode)
        tot = sum(h) or 1.0
        pc = n.pitch % 12
        conc = concurrent(idx, n)
        if not conc:
            continue
        clashes = [(m, ov) for m, ov in conc if (n.pitch - m.pitch) % 12 in (1, 11)]
        if not clashes:
            continue
        rel = (pc - tonic) % 12
        weight = h[pc] / tot
        prev, nxt, chord = neighbours(n)
        strong = (n.start % song.bar) in (0, 1, 2 * tpb, 2 * tpb + 1)
        short = n.dur <= tpb // 2 + 2
        desc = ", ".join("%s(ch%d)" % (pname(m.pitch), m.ch + 1) for m, _ in clashes[:3])

        # --- guards common to every case: deliberate dissonance stays
        others_pc = {m.pitch % 12 for m, ov in conc}
        if pc in others_pc:
            log.add("KEPT", n, pname(n.pitch), "", "minor 2nd against %s, but the same pitch "
                    "class is doubled in another part (deliberate)" % desc)
            continue
        lowest = min(conc, key=lambda x: x[0].pitch)[0]
        if lowest.pitch < n.pitch and (pc - lowest.pitch) % 12 in (0, 3, 4, 5, 7, 8, 9):
            log.add("KEPT", n, pname(n.pitch), "", "minor 2nd against %s, but consonant with "
                    "the bass %s (polychord / colour tone)" % (desc, pname(lowest.pitch)))
            continue
        partner_pcs = {m.pitch % 12 for m, _ in clashes}
        if nxt and nxt.start - n.end < tpb and abs(nxt.pitch - n.pitch) == 1 and \
                nxt.pitch % 12 in partner_pcs and n.dur <= 2 * tpb + 2:
            log.add("KEPT", n, pname(n.pitch), "", "minor 2nd against %s resolving by "
                    "semitone into it (appoggiatura)" % desc)
            continue
        if prev and nxt and abs(n.pitch - prev.pitch) <= 2 and abs(nxt.pitch - n.pitch) <= 2 \
                and (n.pitch - prev.pitch) * (nxt.pitch - n.pitch) > 0 and n.dur <= tpb + 2:
            log.add("KEPT", n, pname(n.pitch), "", "minor 2nd against %s; stepwise passing "
                    "tone" % desc)
            continue
        motif = motif_count(lines[n.part], n, prev, nxt)
        if motif >= 4:
            log.add("KEPT", n, pname(n.pitch), "", "minor 2nd against %s inside a figure the "
                    "part repeats %d times (composed ostinato)" % (desc, motif))
            continue
        vocab = sum(1 for m in pitched if m.part != n.part and m.pitch % 12 == pc
                    and abs(m.start - n.start) <= 2 * song.bar)
        out_of_key = pc not in scale
        if out_of_key:
            reason = "out of key (%s) and a minor 2nd/major 7th against %s" % (
                keyname(tonic, mode), desc)
            if r < 0.7:
                log.add("KEPT", n, pname(n.pitch), "", reason + "; tonality ambiguous here "
                        "(key fit %.2f), chromatic writing" % r)
                continue
            if weight >= 0.04 or vocab >= 2:
                log.add("KEPT", n, pname(n.pitch), "", reason + "; pitch class is "
                        "used by other parts nearby, deliberate colour")
                continue
            if (mode == "maj" and rel in (3, 6, 10)) or (mode == "min" and rel == 6):
                log.add("KEPT", n, pname(n.pitch), "", reason + "; blue note")
                continue
            if nxt and short and abs(nxt.pitch - n.pitch) == 1 and nxt.start - n.end < tpb:
                log.add("KEPT", n, pname(n.pitch), "", reason + "; chromatic approach tone")
                continue
            if chord and len(chord) >= 2:
                log.add("KEPT", n, pname(n.pitch), "", reason + "; inside its own chord voicing")
                continue
            # partners must be in key: otherwise it's an accidental chord, not a typo
            good = [(m, ov) for m, ov in clashes if m.pitch % 12 in scale]
            if not good:
                log.add("KEPT", n, pname(n.pitch), "", reason + "; clashing notes are also "
                        "out of key (chromatic chord)")
                continue
        else:
            # in-key avoid note: only the harshest case, a sustained minor 2nd / 9th
            # against a sustained structural note, not a suspension or passing note
            harsh = [(m, ov) for m, ov in clashes if abs(n.pitch - m.pitch) in (1, 13)
                     and m.dur >= tpb and ov >= 0.75 * n.dur
                     and (m.role in ("bass", "sustain") or m.start < n.start)]
            if not harsh or n.dur < tpb or (chord and len(chord) >= 2):
                continue
            if nxt and abs(nxt.pitch - n.pitch) <= 2 and nxt.start < max(m.end for m, _ in harsh):
                continue                     # suspension / appoggiatura resolving
            if prev and abs(prev.pitch - n.pitch) <= 2 and n.dur <= tpb + 2:
                continue                     # stepwise passing
            if mode == "min" and rel == 8 and (lowest.pitch - tonic) % 12 == 7:
                log.add("KEPT", n, pname(n.pitch), "", "b9 over the dominant in %s "
                        "(V7b9 colour)" % keyname(tonic, mode))
                continue
            m0 = harsh[0][0]
            if weight >= h[m0.pitch % 12] / tot and m0.role != "bass":
                log.add("KEPT", n, pname(n.pitch), "", "sustained minor 2nd against %s but "
                        "this pitch class is the more established one" % desc)
                continue
            good = harsh
            reason = "sustained minor 2nd/9th against %s %s (avoid note in %s)" % (
                m0.role, desc, keyname(tonic, mode))
        # choose the replacement
        concpcs = {m.pitch % 12 for m, _ in conc}
        best = None
        for d in (-1, 1, -2, 2):
            q = n.pitch + d
            if q % 12 not in scale:
                continue
            if any((q - m.pitch) % 12 in (1, 11) for m, _ in conc):
                continue
            score = (2.0 if q % 12 in concpcs else 0.0) + (1.0 if abs(d) == 1 else 0.0)
            if prev and (q - prev.pitch) * (n.pitch - prev.pitch) > 0:
                score += 0.5
            if nxt and q == nxt.pitch and nxt.start - n.end < tpb:
                score -= 0.75                # don't create a repeated note
            if best is None or score > best[0]:
                best = (score, q)
        if best is None:
            log.add("KEPT", n, pname(n.pitch), "", reason + "; no clean replacement")
            continue
        q = best[1]
        changes.append((n, q, reason + ("; weak beat" if not strong else "")
                        + ("; short" if short else "")))
    for n, q, reason in changes:
        log.add("CHANGED", n, pname(n.pitch), pname(q), reason)
        n.pitch = q


# --------------------------------------------------------------------------
# Timing


def quantize(song, strength=0.6, tol=3):
    """Pull loose onsets toward the best-fitting grid (16th or triplet)."""
    moved = 0
    for key, ns in song.segs.items():
        def dev(n, g):
            r = (n.start - 1) % g          # DirectMusic: grid + 1 tick
            return min(r, g - r)
        # 16ths, 8th triplets, 32nds (rolls), 16th triplets: a note close to
        # any of them is deliberate and stays where it is (swing included)
        grids = (96, 64, 48, 32)
        for n in ns:
            g = min(grids, key=lambda g: (dev(n, g), -g))
            d = dev(n, g)
            if d > tol:
                g = 96 if dev(n, 96) <= dev(n, 64) else 64
                r = (n.start - 1) % g
                target = n.start - r if r < g - r else n.start + (g - r)
                shift = int(round((target - n.start) * strength))
                n.start += shift
                n.end += shift
                moved += 1
    return moved


def tidy(notes, tpb):
    """Monophonic lines: no overlap smear; any channel: no same-pitch overlap."""
    lines = collections.defaultdict(list)
    for n in notes:
        lines[(n.track, n.ch, n.layer)].append(n)
    for k, ln in lines.items():
        if k[1] == 9:
            continue
        ln.sort(key=lambda n: (n.start, n.pitch))
        starts = sorted(set(n.start for n in ln))
        chords = len(ln) - len(starts)
        mono = chords <= 0.05 * len(ln)
        if not mono:
            continue
        for a, b in zip(ln, ln[1:]):
            if b.start > a.start and a.end > b.start - 1:
                a.end = max(a.start + 8, b.start - 1)
    for a, b in overlaps(notes):
        if b.start > a.start:
            a.end = max(a.start + 4, min(a.end, b.start - 1))
        else:
            b.end = min(b.end, b.start + 1)  # exact double left by a layer: neutralise
    return [n for n in notes if n.end > n.start + 1]


# --------------------------------------------------------------------------
# Dynamics


def smooth(x):
    x = max(0.0, min(1.0, x))
    return x * x * (3 - 2 * x)


def dynamics(song, expr, rng):
    tpb, bar = song.tpb, song.bar
    bounds = sections(song)
    lines = collections.defaultdict(list)
    for n in song.notes:
        lines[n.part].append(n)
    for part, ln in lines.items():
        ln.sort(key=lambda n: (n.start, n.pitch))
        drums = part[1] == 9
        # phrases: split on rests of >= 1 beat and at section boundaries
        phrases, cur, last_end = [], [], None
        for n in ln:
            if cur and (n.start - last_end >= tpb or any(cur[0].start < b <= n.start
                                                         for b in bounds)):
                phrases.append(cur)
                cur = []
            cur.append(n)
            last_end = n.end if last_end is None or not cur[:-1] else max(last_end, n.end)
        if cur:
            phrases.append(cur)
        for ph in phrases:
            t0, t1 = ph[0].start, max(n.end for n in ph)
            span = float(max(1, t1 - t0))
            if not drums and len(ph) >= 4:
                top = max(ph, key=lambda n: (n.pitch, -n.start))
                peak = min(0.85, max(0.3, (top.start - t0) / span))
            else:
                peak = 0.6
            first_down = next((n.start for n in ph if (n.start % bar) <= 2), None)
            for n in ph:
                p = (n.start - t0) / span
                if drums:
                    arch = 1.0
                elif p <= peak:
                    arch = 0.87 + 0.13 * smooth(p / peak)
                else:
                    arch = 1.0 - 0.08 * smooth((p - peak) / (1 - peak))
                arch = 1.0 - (1.0 - arch) * expr
                pos = n.start % bar
                if pos <= 2:
                    acc = 1.05
                elif abs(pos - 2 * tpb) <= 2:
                    acc = 1.02
                elif (pos % tpb) <= 2:
                    acc = 1.0
                elif abs((pos % tpb) - tpb // 2) <= 2:
                    acc = 0.96
                else:
                    acc = 0.93
                if drums:
                    acc = 1.0 + (acc - 1.0) * (0.6 if n.pitch in (42, 44, 46, 69, 75) else 0.3)
                pick = 0.9 if (first_down is not None and n.start < first_down
                               and first_down - n.start <= tpb and not drums) else 1.0
                # crescendo into the next section change (last 2 bars, 1 for drums)
                cres = 1.0
                for b in bounds:
                    lead = (2 if not drums else 1) * bar
                    if b - lead <= n.start < b:
                        cres = 0.95 + 0.09 * (n.start - (b - lead)) / float(lead)
                v = n.vel * arch * acc * pick * cres / 1.05
                v += rng.gauss(0, 3.0 if not drums else 2.5)
                n.vel = int(max(12, min(127, round(v))))


def humanize(song, rng):
    """Small deterministic timing jitter in milliseconds (drums/bass tighter)."""
    tick_per_ms = song.tpb / (song.tempo / 1e3)
    for n in song.notes:
        if n.start <= 2:
            continue                                   # loop point stays exact
        sd = 1.5 if n.ch == 9 or n.role == "bass" else 3.5
        j = max(-2 * sd, min(2 * sd, rng.gauss(0, sd))) * tick_per_ms
        j = int(round(j))
        n.start = max(0, n.start + j)
        n.end = max(n.start + 4, n.end + j)
        n.end = min(n.end, song.end)


# --------------------------------------------------------------------------
# Orchestration


def orchestrate(song, cfg, log):
    remap = dict(DEFAULT_REMAP)
    remap.update(cfg.get("remap", {}))
    cap = dict(DEFAULT_CAP)
    cap.update(cfg.get("cap", {}))
    song.remap = remap
    used = collections.Counter(n.prog for n in song.notes if n.ch != 9)
    song.swaps = ["%s -> %s" % (GM[p], remap[p][2].split("->")[-1].strip())
                  for p in sorted(used) if p in remap]
    for n in song.notes:
        if n.ch != 9 and n.prog in cap:
            c = int(cap[n.prog] * 1e6 / song.tempo * song.tpb)
            if n.dur > c:
                n.end = n.start + c
    layers = []
    # pad layer doubling the sustained harmony
    if cfg.get("pad"):
        for n in song.notes:
            if n.role == "sustain" and n.dur >= song.tpb and n.layer is None:
                p = n.pitch
                while p > 74:
                    p -= 12
                while p < 50:
                    p += 12
                m = Note(n.start, n.end, p, 0, PAD_CH, 100)
                m.vel = int(max(20, min(90, cfg["pad_vel"] * (n.vel / 110.0) ** 0.5)))
                m.layer, m.role, m.prog, m.src = "pad", "pad", cfg["pad"][1], n
                layers.append(m)
    # sub layer under the bass
    if cfg.get("sub"):
        for n in song.notes:
            # real bass instruments only (choir/strings drones get no sub)
            if n.role == "bass" and n.layer is None and n.prog in BASS_PROGS:
                # always an octave below: a same-pitch sine phase-cancels the
                # bass fundamental (measured); below E1 there is nothing to add
                p = n.pitch - 12
                if p < 28:
                    continue
                v = n.vel * 0.8 * (0.6 if n.dur >= 4 * song.tpb else 1.0)
                m = Note(n.start, n.end, p, int(min(110, v)), SUB_CH, 101)
                m.layer, m.role, m.prog, m.src = "sub", "sub", SINE[1], n
                layers.append(m)
    # stab boom: the 1997 orchestra-hit sample carried its own low thump; the
    # full-orchestra replacement doesn't, so each stab gets a timpani hit on
    # the stab chord's root (at most one per beat)
    stabs = collections.defaultdict(list)
    for n in song.notes:
        if n.ch != 9 and n.prog == 55 and n.layer is None:
            stabs[n.start // 24].append(n)
    last = -10 ** 9
    for k in sorted(stabs):
        g = stabs[k]
        t = min(n.start for n in g)
        if t - last < song.tpb - 4:
            continue
        last = t
        p = min(n.pitch for n in g)
        while p > 47:
            p -= 12
        while p < 36:
            p += 12
        v = int(min(110, max(n.vel for n in g) * cfg.get("boom", 0.8)))
        m = Note(t, t + song.tpb, p, v, BOOM_CH, 102)
        m.layer, m.role, m.prog, m.src = "stab boom", "perc", TIMPANI[1], g[0]
        layers.append(m)
    # drum reinforcement (no layering of ghost notes)
    for n in song.notes:
        if n.ch != 9:
            continue
        if n.pitch == 36 and n.vel >= 90:
            m = Note(n.start, n.end, 35, int(n.vel * 0.45), 9, n.track)
        elif n.pitch == 38 and n.vel >= 100 and (n.start % song.tpb) <= 2:
            m = Note(n.start, n.end, 40, int(n.vel * 0.40), 9, n.track)
        else:
            continue
        m.layer, m.role, m.src = "drumlayer", "drums", n
        layers.append(m)
    song.notes.extend(layers)


# pan / sends per role
ROLE_MIX = {          # (reverb CC91, chorus CC93)
    "drums": (40, 0), "perc": (55, 0), "bass": (22, 0), "sub": (0, 0),
    "melodic": (56, 18), "sustain": (66, 30), "pad": (80, 60),
}


def channel_plan(song, cfg):
    """Dominant role per channel, pan positions, sends, volume."""
    rc = collections.defaultdict(collections.Counter)
    for n in song.notes:
        rc[n.ch][n.role] += n.dur
    roles = {ch: c.most_common(1)[0][0] for ch, c in rc.items()}
    # count original CC10 automation
    pan_auto = collections.Counter()
    vals = collections.defaultdict(set)
    for t, m in song.ctrl:
        if m.type == "control_change" and m.control == 10 and t > 1:
            vals[m.channel].add(m.value)
    for ch, v in vals.items():
        if len(v) >= 3:
            pan_auto[ch] = len(v)
    # spread: melodic/sustain channels alternate around centre by register
    others = [ch for ch in sorted(roles) if roles[ch] in ("melodic", "sustain", "perc")]
    reg = {}
    for ch in others:
        ps = sorted(n.pitch for n in song.notes if n.ch == ch)
        reg[ch] = ps[len(ps) // 2]
    others.sort(key=lambda ch: reg[ch])
    spots = [44, 84, 30, 98, 54, 74, 36, 92, 64]
    pan = {}
    for i, ch in enumerate(others):
        pan[ch] = spots[i % len(spots)]
    for ch, r in roles.items():
        if r in ("bass", "sub", "drums", "pad"):
            pan[ch] = 64
    # bass notes keep their full velocity under the phrase shaping while the
    # other parts relax by ~1.5 dB; trim the bass channel to keep the balance
    vol = {ch: (cfg.get("bass_vol", 90) if r == "bass" else 100) for ch, r in roles.items()}
    vol[PAD_CH] = cfg.get("pad_vol", 88)
    vol[SUB_CH] = cfg.get("sub_vol", 72)
    vol[BOOM_CH] = cfg.get("boom_vol", 100)
    return roles, pan, pan_auto, vol


def expression(song, roles):
    """CC11 per channel = original CC11 x swell envelope over sustained chords."""
    tpb = song.tpb
    res = {}
    orig = collections.defaultdict(list)
    for t, m in sorted(song.ctrl, key=lambda x: x[0]):
        if m.type == "control_change" and m.control == 11:
            orig[m.channel].append((t, m.value))
    for ch, role in roles.items():
        if ch == 9:
            continue
        o = orig.get(ch, [])
        ot = [t for t, v in o]
        env_spans = []
        if role in ("sustain", "pad"):
            groups = collections.defaultdict(list)
            for n in song.notes:
                if n.ch == ch and n.dur >= 1.5 * tpb:
                    groups[n.start // 24].append(n)
            for g in groups.values():
                env_spans.append((min(n.start for n in g), max(n.end for n in g)))
            env_spans.sort()
        if not env_spans and not o:
            continue
        lo = 0.62 if role == "pad" else 0.80
        tail = 0.86 if role == "pad" else 0.92

        def env(t):
            e = 1.0
            for a, b in env_spans:
                if a <= t < b:
                    x = (t - a) / float(b - a)
                    pk = 0.45
                    e = lo + (1 - lo) * smooth(x / pk) if x < pk else \
                        1.0 - (1 - tail) * smooth((x - pk) / (1 - pk))
            return e

        def base(t):
            i = bisect.bisect_right(ot, t) - 1
            return o[i][1] if i >= 0 else 127

        pts = set(ot)
        for a, b in env_spans:
            pts.update(range(a, b, 48))
            pts.add(b)
        evs, lastv = [], None
        for t in sorted(pts):
            v = int(round(base(t) * env(t)))
            v = max(0, min(127, v))
            if v != lastv:
                evs.append((t, v))
                lastv = v
        res[ch] = evs
    return res


# --------------------------------------------------------------------------
# Writing


def write(song, cfg, path):
    roles, pan, pan_auto, vol = channel_plan(song, cfg)
    expr = expression(song, roles)
    remap = song.remap
    per = collections.defaultdict(list)       # ch -> [(tick, prio, msg)]
    for ch in sorted(roles):
        per[ch].append((0, 2, mido.Message("control_change", channel=ch, control=7,
                                            value=vol.get(ch, 100))))
        if ch not in pan_auto:
            per[ch].append((0, 2, mido.Message("control_change", channel=ch, control=10,
                                                value=pan[ch])))
        rv, chv = ROLE_MIX[roles[ch]]
        per[ch].append((0, 2, mido.Message("control_change", channel=ch, control=91, value=rv)))
        per[ch].append((0, 2, mido.Message("control_change", channel=ch, control=93, value=chv)))
    # programs (with GS bank select)
    for ch, lst in song.progs.items():
        if ch >= 10:
            continue
        for t, p in lst:
            if ch == 9:
                bank, prog = 0, cfg["kit"]
            elif p in remap:
                bank, prog = remap[p][0], remap[p][1]
            else:
                bank, prog = 0, p
            # bank select and program travel as an inseparable pair
            seq = len(per[ch])
            per[ch].append((t, (0, seq, 0), mido.Message("control_change", channel=ch,
                                                         control=0, value=bank)))
            per[ch].append((t, (0, seq, 1), mido.Message("program_change", channel=ch,
                                                         program=prog)))
    if 9 not in song.progs:
        per[9].append((0, 1, mido.Message("program_change", channel=9, program=cfg["kit"])))
    for ch, (bank, prog) in ((PAD_CH, cfg.get("pad")), (SUB_CH, SINE), (BOOM_CH, TIMPANI)):
        if ch in roles and prog is not None:
            per[ch].append((0, 0, mido.Message("control_change", channel=ch, control=0,
                                                value=bank)))
            per[ch].append((0, 1, mido.Message("program_change", channel=ch, program=prog)))
    # remaining original controllers (pan automation kept where present)
    for t, m in song.ctrl:
        if m.type == "program_change" or getattr(m, "channel", 0) >= 10:
            continue
        if m.type == "control_change":
            if m.control in (0, 7, 11, 32, 91, 93):
                continue
            if m.control == 10 and m.channel not in pan_auto:
                continue
        per[m.channel].append((t, 2, m))
    for ch, evs in expr.items():
        for t, v in evs:
            per[ch].append((t, 2, mido.Message("control_change", channel=ch, control=11,
                                                value=v)))
    for n in song.notes:
        per[n.ch].append((n.start, 4, mido.Message("note_on", channel=n.ch, note=n.pitch,
                                                    velocity=n.vel)))
        per[n.ch].append((n.end, 3, mido.Message("note_off", channel=n.ch, note=n.pitch,
                                                  velocity=0)))
    out = mido.MidiFile(type=1, ticks_per_beat=song.tpb)
    t0 = mido.MidiTrack()
    t0.append(mido.MetaMessage("track_name", name="Stellar Frontier %s (Enhanced Edition "
                               "re-arrangement)" % song.name, time=0))
    last = 0
    for t, m in sorted(song.meta, key=lambda x: x[0]):
        t0.append(m.copy(time=t - last))
        last = t
    t0.append(mido.MetaMessage("end_of_track", time=song.end - last))
    out.tracks.append(t0)
    names = {PAD_CH: "pad layer", SUB_CH: "sub bass layer", BOOM_CH: "stab timpani layer",
             9: "drums"}
    for ch in sorted(per):
        tr = mido.MidiTrack()
        role = roles.get(ch, "")
        tr.append(mido.MetaMessage("track_name", name=names.get(ch, "ch%d %s" % (ch + 1, role)),
                                   time=0))
        last = 0
        for t, prio, m in sorted(per[ch], key=lambda x: (x[0], x[1] if isinstance(x[1], tuple)
                                                         else (x[1], 0, 0))):
            t = min(t, song.end)
            tr.append(m.copy(time=t - last))
            last = t
        tr.append(mido.MetaMessage("end_of_track", time=song.end - last))
        out.tracks.append(tr)
    out.save(path)
    return roles, pan, pan_auto


# --------------------------------------------------------------------------


def rearrange(inp, outp, logpath=None):
    song = Song(inp)
    cfg = SONGS.get(song.name, SONGS["sf-music2"])
    rng = random.Random("stellar-frontier-ee:" + song.name)
    log = Log(song)
    fix_duplicates(song, log)
    song.classify()
    fix_range(song, log)
    fix_harmony(song, log, rng)
    fix_manual(song, log)
    moved = quantize(song)
    dynamics(song, cfg.get("expr", 1.0), rng)
    humanize(song, rng)
    orchestrate(song, cfg, log)
    song.notes = tidy(song.notes, song.tpb)
    roles, pan, pan_auto = write(song, cfg, outp)
    if logpath:
        with open(logpath, "a") as f:
            f.write("# %s: %d changed, %d reviewed and kept; %d onsets quantised\n"
                    % (song.name, log.changed, log.kept, moved))
            for line in log.text():
                f.write(line + "\n")
            f.write("\n")
    print("%s: %d corrections (%d reviewed and kept), %d quantised; swaps: %s; layers: %s"
          % (song.name, log.changed, log.kept, moved, "; ".join(song.swaps) or "none",
             ", ".join(sorted(set(n.layer for n in song.notes if n.layer))) or "none"))
    return log


def main():
    ap = argparse.ArgumentParser(description="Stellar Frontier EE music re-arranger")
    ap.add_argument("files", nargs="+")
    ap.add_argument("--analyze", action="store_true")
    ap.add_argument("--log")
    a = ap.parse_args()
    if a.analyze:
        for f in a.files:
            print(analyze(Song(f)))
        return
    if len(a.files) != 2:
        ap.error("need IN.mid OUT.mid")
    rearrange(a.files[0], a.files[1], a.log)


if __name__ == "__main__":
    main()
