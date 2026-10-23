import sys, pretty_midi
pm = pretty_midi.PrettyMIDI(sys.argv[1])
bpm = pm.get_tempo_changes()[1][0]; spb = 60/bpm
for i, ins in enumerate(pm.instruments):
    print(f"## inst {i} prog {ins.program} {pretty_midi.program_to_instrument_name(ins.program)} drum={ins.is_drum} n={len(ins.notes)}")
    if ins.is_drum: continue
    bars = {}
    for n in sorted(ins.notes, key=lambda n: (n.start, n.pitch)):
        b = n.start/spb
        bars.setdefault(int(b//4)+1, []).append(f"{b%4:g}:{pretty_midi.note_number_to_name(n.pitch)}/{round((n.end-n.start)/spb*4)/4:g}")
    for b in sorted(bars):
        print(f"  {b:3d} | " + ' '.join(bars[b]))
