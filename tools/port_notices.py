#!/usr/bin/env python3
# port_notices.py -- NEW FILE (2026-10-12), Stellar Frontier Godot port.
#
# The Stardock Shared Source Stellar Frontier License (clause 4) requires
# modified files to carry prominent notices stating that the Software was
# changed and the date of the changes.  This script compares every file in
# src/slib and src/game with the original source release and
#   - adds a notice block at the top of each changed file that lacks one,
#   - adds today's date to the notice of a file changed again today,
#   - removes the notice from a file that is back to the original.
#
#   tools/port_notices.py [--date YYYY-MM-DD] [--check]
import argparse
import datetime
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BEGIN = "/* === Stellar Frontier port: MODIFIED FILE ============================== */"
END = "/* ======================================================================= */"
NOTICE_RE = re.compile(re.escape(BEGIN) + r".*?" + re.escape(END) + r"\n", re.S)

MAPS = [
    ("src/slib", ["original/slib_source/slib"]),
    ("src/game/gui", ["original/frontier_source/frontier/src/gui"]),
    ("src/game/portcpp", ["original/frontier_source/frontier/src/portCPP",
                          "original/frontier_source/frontier/src/portcpp"]),
]


def original_of(rel_dir, orig_dirs, name):
    for d in orig_dirs:
        p = os.path.join(ROOT, d, name)
        if os.path.isfile(p):
            return p
        # case-insensitive match
        full = os.path.join(ROOT, d)
        if os.path.isdir(full):
            for f in os.listdir(full):
                if f.lower() == name.lower():
                    return os.path.join(full, f)
    return None


def notice(dates):
    lines = [
        BEGIN,
        "/* This file has been changed from the original Stardock Shared Source  */",
        "/* release of Stellar Frontier, for the port to modern compilers and    */",
        "/* operating systems.  It is not the original Software.                 */",
    ]
    for i in range(0, len(dates), 4):
        chunk = ", ".join(dates[i:i + 4])
        label = "Changed on: " if i == 0 else "            "
        lines.append("/* " + (label + chunk).ljust(69) + " */")
    lines.append(END)
    return "\n".join(lines) + "\n"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--date", default=datetime.date.today().isoformat())
    ap.add_argument("--check", action="store_true", help="report only")
    a = ap.parse_args()
    changed = 0
    for rel, origs in MAPS:
        d = os.path.join(ROOT, rel)
        for name in sorted(os.listdir(d)):
            if not re.search(r"\.(c|cpp|h|hpp)$", name, re.I):
                continue
            path = os.path.join(d, name)
            orig = original_of(rel, origs, name)
            if orig is None:
                continue        # new file: carries its own "NEW FILE" header
            with open(path, "r", encoding="latin-1", newline="") as f:
                text = f.read()
            with open(orig, "r", encoding="latin-1", newline="") as f:
                otext = f.read()
            m = NOTICE_RE.search(text)
            body = NOTICE_RE.sub("", text, count=1) if m else text
            dates = re.findall(r"\d{4}-\d{2}-\d{2}", m.group(0)) if m else []
            same = body.replace("\r\n", "\n") == otext.replace("\r\n", "\n")
            if same:
                new = body
            else:
                # A file gets today's date when it first gets a notice, or
                # when its body changed since this script last stamped it.
                stamp = os.path.join(ROOT, "build", "notice_stamps", rel.replace("/", "_") + "_" + name)
                prev = None
                if os.path.exists(stamp):
                    with open(stamp, "r", encoding="latin-1", newline="") as f:
                        prev = f.read()
                if not dates or (prev is not None and prev != body and a.date not in dates):
                    dates = sorted(set(dates + [a.date]))
                new = notice(dates) + body
                if not a.check:
                    os.makedirs(os.path.dirname(stamp), exist_ok=True)
                    with open(stamp, "w", encoding="latin-1", newline="") as f:
                        f.write(body)
            if new != text:
                changed += 1
                print(("would update " if a.check else "updated ") + os.path.join(rel, name))
                if not a.check:
                    with open(path, "w", encoding="latin-1", newline="") as f:
                        f.write(new)
    print(f"{changed} file(s) {'need' if a.check else 'got'} notice changes")
    return 1 if (a.check and changed) else 0


if __name__ == "__main__":
    sys.exit(main())
