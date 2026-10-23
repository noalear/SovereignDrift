import os, sys, tempfile
sys.path.insert(0, sys.argv[1])
tempfile.tempdir = sys.argv[2]
import synth_orch as O
KEEP = ['horn_sus','horn_stac','tbn_sus','tbn_stac','tpt_sus','tpt_stac','tuba_sus','tuba_stac',
        'vln_sus','vln_spic','vln_trem','vla_sus','vla_spic','vc_sus','vc_spic','vc_trem','cb_sus','cb_spic']
for k in list(O.ARTS):
    if k not in KEEP: del O.ARTS[k]
idx = O.load_index()
for k,v in idx.items():
    bad=[r for r in v if r['det'] is not None and abs(r['det']-r['midi'])>0.5]
    print(k, len(v), 'bad', len(bad), 'range', min(r['midi'] for r in v), max(r['midi'] for r in v), 'layers', sorted({r['v'] for r in v}))
