#!/usr/bin/env bash
# Publication hygiene gate; does not establish copyright clearance.
#
#   1. no tracked binary files at all
#   2. rom/, assets/, expected/, build/ fully untracked
#   3. sampled scan: no tracked file contains 64-byte windows of the ROM
#   4. no vendor copyright banners, local paths or credentials in tracked text
#   5. no tracked file over 1 MB
#
# Run before publishing and in every post-wave step. Exits nonzero dirty.
set -euo pipefail
cd "$(dirname "$0")/.."

fail=0

# 1 — tracked binaries (grep -I treats binary as unmatchable; -L lists
# them; empty files also match -L, so filter them back out with -s)
bins=$(git ls-files -z | xargs -0 grep -IL . 2>/dev/null | while read -r f; do [ -s "$f" ] && echo "$f"; done || true)
if [ -n "$bins" ]; then
    echo "FAIL: tracked binary files:" >&2
    echo "$bins" >&2
    fail=1
fi

# 2 — forbidden paths
for d in rom assets expected build; do
    if git ls-files --error-unmatch "$d" >/dev/null 2>&1; then
        echo "FAIL: $d/ has tracked files" >&2
        fail=1
    fi
done

# SDK implementation paths and locally supplied headers must never be tracked.
python3 - <<'PYCODE' || fail=1
import json, subprocess, sys
from pathlib import Path
tracked = set(subprocess.check_output(['git', 'ls-files', '-z']).decode().split('\0'))
headers = {e['path'] for e in json.loads(Path('docs/LOCAL_SDK_FILES.json').read_text())['files']}
prefixes = ('src/audio/', 'src/gu/', 'src/os/', 'src/libc/', 'src/libm/',
            'src/nonmatching/audio/', 'asm/', 'tools/ultralib/', 'include/PR/',
            'include/PRinternal/', 'include/sys/', 'include/libc/', 'include/gcc/')
forbidden = sorted(p for p in tracked if p in headers or p.startswith(prefixes))
if forbidden:
    print('FAIL: tracked SDK/local input paths:\n' + '\n'.join(forbidden), file=sys.stderr)
    sys.exit(1)
PYCODE

# 3 — sampled ROM-window scan over tracked files
if [ -f rom/baserom.us.z64 ]; then
    python3 - <<'EOF' || fail=1
import pathlib, random, subprocess, sys
rom = pathlib.Path("rom/baserom.us.z64").read_bytes()
tracked = subprocess.run(["git", "ls-files", "-z"], capture_output=True).stdout.split(b"\0")
random.seed(0x4E475045)  # NPGE: deterministic sample
windows = [rom[o:o+64] for o in (random.randrange(0, len(rom)-64) for _ in range(256))]
dirty = False
for f in tracked:
    if not f:
        continue
    p = pathlib.Path(f.decode())
    if not p.exists():  # tracked but deleted in the working tree
        continue
    data = p.read_bytes()
    for w in windows:
        if w in data:
            print(f"FAIL: {f.decode()} contains ROM bytes", file=sys.stderr)
            dirty = True
            break
sys.exit(1 if dirty else 0)
EOF
fi

# 4 — banners / local paths / secrets (patterns are written so this file does not match itself)
hits=$(git ls-files -z | xargs -0 grep -l -I -E "UNPUBLISHED PROPRIETAR[Y]|proprietary informatio[n] of|trade secre[t] of|Copyright[^a-z]*(19|20)[0-9][0-9][,.]? *NINTEND[O]|Copyright[^a-z]*(19|20)[0-9][0-9][,.]? *Silicon Graph[i]cs|/Use[r]s/|ghp_[A-Za-z0-9]{20}|BEGIN (RSA|OPENSSH) PRIVAT[E]" 2>/dev/null  || true)
if [ -n "$hits" ]; then
    echo "FAIL: vendor banner, local path or credential in:" >&2
    echo "$hits" >&2
    fail=1
fi

# 5 — size limit
big=$(git ls-files -z | xargs -0 ls -l 2>/dev/null | awk '$5 > 1000000 {print $9}' || true)
if [ -n "$big" ]; then
    echo "FAIL: tracked files over 1 MB:" >&2
    echo "$big" >&2
    fail=1
fi

if [ "$fail" -ne 0 ]; then exit 1; fi
echo "OK: publication hygiene checks passed (not legal clearance)"
