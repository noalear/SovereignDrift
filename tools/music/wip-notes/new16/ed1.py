p='/tmp/claude-0/-home-claude/bdcf4380-7c02-523f-bd93-ae04cd7c35ec/scratchpad/work/tools/music/edm/new16_neurofunk.py'; s=open(p).read()
a=s.index('#   F3  sf-music8 (second half), the low choir')
b=s.index('# New: everything else')
s=s[:a]+'''#   F3  sf-music8 (second half), the rising closing line of ch 2 (program change
#       to contrabass at bar 84), bars 84-86: F#4 A4 (beats 4, 4.5) | B4 C#5 |
#       D5 B4 | F#5, moved down a fourth: C# E | F# G# | A F# | C#.  It is the
#       lead's answer phrase over prog B (C# C# F#m/A F#m/A: a V -> i line, the
#       E natural pickup a blue note against C#) in the breakdown and drop 2,
#       varied the second time (A B A G# | F#) so it lands on the tonic.
'''+s[b:]
s=s.replace('''# New: everything else -- 172 BPM, the form''','''# New: everything else -- 172 BPM, prog C (F#m C# D C#, i V bVI V, used in the
# second half of the drops and the outro, ending on C# so the final F#m hit
# closes it), the form''')
s=s.replace("""        'C': ['F#m', 'C#', 'D', 'C#']}            # F3, orig sf-music8 bars 61-64""","""        'C': ['F#m', 'C#', 'D', 'C#']}            # new (i V bVI V)""")
s=s.replace("prog C (F3) ends on C#","prog C ends on C#")
a=s.index('# breakdown lead: F2 augmented'); b=s.index('# F1 top voice, one per bar')
s=s[:a]+'''# breakdown lead: F2 augmented and developed (prog A), then F3 (prog B)
LEAD_BD = [(2, 'F#5', 6), (8, 'A5', 4), (12, 'C#6', 1.5), (13.5, 'C#6', 1.5), (15, 'B5', 3),
           (18, 'F#5', 4), (22, 'A5', 2), (24, 'C#6', 1.5), (25.5, 'D#6', 1.5), (27, 'C#6', 1), (28, 'B5', 3.5)]
# F3 (beats from the first bar of prog B)
F3_LINE = [(0, 'F5', 3), (3, 'C#5', .5), (3.5, 'E5', .5), (4, 'F#5', 2), (6, 'G#5', 2), (8, 'A5', 2),
           (10, 'F#5', 2), (12, 'C#6', 4)]
F3_VAR = [(0, 'G#5', 3), (3, 'C#5', .5), (3.5, 'E5', .5), (4, 'F#5', 2), (6, 'G#5', 2), (8, 'A5', 1.5),
          (9.5, 'B5', .5), (10, 'A5', 1), (11, 'G#5', 1), (12, 'F#5', 2.5)]
'''+s[b:]
s=s.replace("""lead(LEAD_BD, 64, vel=0.85)
""","""lead(LEAD_BD, 64, vel=0.85)
lead(F3_LINE, 72, vel=0.85)
lead(F3_VAR, 76, vel=0.85)
""")
s=s.replace("""motif(88, vel=0.9)
motif(92, vel=0.8)
""","""motif(88, vel=0.9)
motif(92, vel=0.8)
lead(F3_LINE, 88, vel=0.6)
lead(F3_VAR, 92, vel=0.65)
""")
open(p,'w').write(s)
