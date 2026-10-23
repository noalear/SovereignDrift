#!/usr/bin/env python3
"""Mechanical, behaviour-preserving type rewrite for the Stellar Frontier port.

The 1997 code was written for 32-bit compilers where `long` is exactly 32 bits.
On 64-bit Linux `long` is 64 bits, which silently changes checksums, MD5,
bitmap/packet layouts and overflow behaviour.  This script rewrites the *type*
keyword `long` to `slong` (int32_t) and `unsigned long` to `ulong` (uint32_t)
so the code behaves identically on every platform.

Comments and string/char literals are left untouched.  `long long` and
`long double` are left alone.
"""
import re
import sys

WORD = re.compile(r"\b(unsigned\s+long\s+int|unsigned\s+long|signed\s+long\s+int|"
                  r"signed\s+long|long\s+long|long\s+double|long\s+int|long)\b")

def rewrite_code(seg: str) -> str:
    def rep(m):
        t = re.sub(r"\s+", " ", m.group(1))
        if t in ("long long", "long double"):
            return m.group(1)
        if t.startswith("unsigned"):
            return "ulong"
        return "slong"
    return WORD.sub(rep, seg)

def process(text: str) -> str:
    out = []
    i, n = 0, len(text)
    code_start = 0
    while i < n:
        c = text[i]
        two = text[i:i + 2]
        if two == "//" or two == "/*" or c == '"' or c == "'":
            out.append(rewrite_code(text[code_start:i]))
            if two == "//":
                j = text.find("\n", i)
                j = n if j < 0 else j
            elif two == "/*":
                j = text.find("*/", i + 2)
                j = n if j < 0 else j + 2
            else:
                q = c
                j = i + 1
                while j < n and text[j] != q:
                    if text[j] == "\\":
                        j += 1
                    elif text[j] == "\n":  # unterminated literal; stop at EOL
                        break
                    j += 1
                j = min(j + 1, n)
            out.append(text[i:j])
            i = j
            code_start = i
            continue
        if c == "#" and (i == 0 or text[i - 1] == "\n"):
            # preprocessor line: still code (typedefs in macros), handled normally
            pass
        i += 1
    out.append(rewrite_code(text[code_start:]))
    return "".join(out)

if __name__ == "__main__":
    changed = 0
    for path in sys.argv[1:]:
        with open(path, "r", encoding="latin-1", newline="") as f:
            src = f.read()
        dst = process(src)
        if dst != src:
            with open(path, "w", encoding="latin-1", newline="") as f:
                f.write(dst)
            changed += 1
    print(f"rewrote {changed} files")
