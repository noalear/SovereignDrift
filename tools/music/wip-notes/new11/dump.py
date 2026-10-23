import sys, pretty_midi
pm = pretty_midi.PrettyMIDI(sys.argv[1])
NN = ['C','C#','D','Eb','E','F','F#','G','Ab','A','Bb','B']
def nm(p): return f"{NN[p%12]}{p//12-1}"
bt = pm.get_beats(); spb = bt[1]-bt[0]
for i, ins in enumerate(pm.instruments):
    if ins.is_drum: continue
    print(f"### inst {i} prog {ins.program} {pretty_midi.program_to_instrument_name(ins.program)} ch? n={len(ins.notes)}")
    bars = {}
    for n in ins.notes:
        b = n.start/spb; bar = int(b//4)+1
        bars.setdefault(bar, []).append(f"{b%4:.2f}:{nm(n.pitch)}/{(n.end-n.start)/spb:.2f}")
    for bar in sorted(bars):
        print(f"  {bar:3d} " + " ".join(bars[bar]))
