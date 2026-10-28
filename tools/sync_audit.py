"""Compare the host's and a joiner's dumps from tools/sync_audit.gd.

For every value the host had at time t, the joiner passes when one of its
dumps from t to t+LAG seconds shows the same (floats within TOL).  Prints
each field the joiner got wrong, how often, and an example.

    python3 tools/sync_audit.py /tmp/sync_host.jsonl /tmp/sync_client.jsonl
"""
import json
import sys
from collections import defaultdict

LAG = 4       # seconds: stats and colonies go every 2 s, snapshots 20 times a second
TOL = 0.06


def load(path):
    out = {}
    with open(path) as f:
        for line in f:
            line = line.strip()
            if line:
                d = json.loads(line)
                out[int(d["t"])] = d
    return out


def flatten(v, prefix, out):
    if isinstance(v, dict):
        for k, x in v.items():
            flatten(x, f"{prefix}.{k}" if prefix else str(k), out)
    elif isinstance(v, list):
        for i, x in enumerate(v):
            flatten(x, f"{prefix}[{i}]", out)
    else:
        out[prefix] = v


def same(a, b):
    if isinstance(a, bool) or isinstance(b, bool):
        return a == b
    if isinstance(a, (int, float)) and isinstance(b, (int, float)):
        return abs(a - b) <= TOL * max(1.0, abs(a))
    return a == b


def general(path):
    """ships.12.hull -> ships.*.hull; planets[3].owner -> planets[*].owner"""
    parts = path.split(".")
    if parts[0] == "ships" and len(parts) > 2:
        parts[1] = "*"
    out = ".".join(parts)
    if out.startswith("planets["):
        out = "planets[*]" + out[out.index("]") + 1:]
    return out


def main():
    host, client = load(sys.argv[1]), load(sys.argv[2])
    bad = defaultdict(int)
    seen = defaultdict(int)
    example = {}
    for t, hd in sorted(host.items()):
        later = [client[u] for u in range(t - 2, t + LAG + 1) if u in client]   # the two clocks differ by a second or so
        if not later:
            continue
        hf = {}
        flatten(hd, "", hf)
        cfs = []
        for cd in later:
            cf = {}
            flatten(cd, "", cf)
            cfs.append(cf)
        for k, v in hf.items():
            if k in ("t",) or k.startswith("round.elapsed"):
                continue
            g = general(k)
            seen[g] += 1
            if not any(k in cf and same(v, cf[k]) for cf in cfs):
                bad[g] += 1
                example.setdefault(g, (t, k, v, cfs[0].get(k, "missing")))
    if not bad:
        print("in sync: every field the host showed reached the joiner")
        return
    print("%-34s %8s  example (t, field: host -> joiner)" % ("field", "missed"))
    for g in sorted(bad, key=lambda x: -bad[x] / seen[x]):
        t, k, v, c = example[g]
        print("%-34s %3d/%-4d  t=%d %s: %r -> %r" % (g, bad[g], seen[g], t, k, v, c))


if __name__ == "__main__":
    main()
