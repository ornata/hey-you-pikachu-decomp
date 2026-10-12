#!/usr/bin/env python3
"""Compile a C unit with IDO and insert declared records into ugen's binary
assembly (.G) before as1 runs.

usage (as the compiler command of asm-processor's build.py):
    python3 tools/ido_binasm_patch.py MANIFEST.json tools/ido-5.3/cc <cc args> -o OUT IN.c

The driver's own stages are reproduced exactly: `cc -K` keeps ugen's binary
assembly (IN.G) and symbol table (IN.T), the manifest's records are inserted,
and as1 is rerun with the argument list the driver itself prints for `cc -v`.
Everything else (cfe, uopt, ugen, flags, asm-processor post-processing) is
untouched.

The manifest is a JSON list of patches:
    {"function": "actionCommandLaunchHeldItem",
     "anchor": {"kind": "range_check_target", "ordinal": 1},
     "insert": [{"op": 4, "words": [0, 2, 0]}],
     "reason": "..."}
`anchor.range_check_target` names the label record targeted by the function's
N-th conditional branch with the bgeu macro opcode (a switch range check).
The records are inserted directly before that label record. Record layout is
16 bytes: u32 label/symbol, u16 op, u16 w1, then two u32 words (`.align N`
is op 4 with w2 = N).

Every patch must apply exactly once; otherwise the build fails. The patched
function's text must also equal the manifest's `expected_text_sha256` when one
is given, so a silently shifted anchor cannot produce a different object.
"""
import hashlib
import json
import os
import pathlib
import re
import shutil
import struct
import subprocess
import sys
import tempfile

REC = 16
OP_INSTRUCTION = 23
OP_LABEL = 0
OP_LOC = 28
OP_ENT = 27
OP_END = 24
BGEU_MACRO = 0x1e  # ugen instruction-record opcode field for the bgeu macro


def records(data):
    return [data[i:i + REC] for i in range(0, len(data), REC)]


def rec_op(r):
    return struct.unpack('>H', r[4:6])[0]


def rec_label(r):
    return struct.unpack('>i', r[0:4])[0]


def make_record(label, op, w1, words):
    if len(words) != 2:
        raise SystemExit('ido_binasm_patch: a record has a u32 label, u16 op, u16 w1 and two u32 words')
    return struct.pack('>iHHII', label, op, w1, *words)


def function_line(source_text, name):
    """Logical source line of the function definition, honouring #line."""
    logical = 0
    pending = None
    for line in source_text.splitlines():
        if pending is not None:
            logical = pending
            pending = None
        else:
            logical += 1
        m = re.match(r'\s*#\s*line\s+(\d+)', line)
        if m:
            pending = int(m.group(1))
            continue
        if re.match(r'^[A-Za-z_][\w\s\*]*\b' + re.escape(name) + r'\s*\(', line) and not line.rstrip().endswith(';'):
            return logical
    raise SystemExit(f'ido_binasm_patch: definition of {name} not found in the input')


def procedure_span(recs, line):
    """Records of the procedure whose .ent follows `.loc 2 line`."""
    for i, r in enumerate(recs):
        if rec_op(r) == OP_LOC and struct.unpack('>I', r[12:16])[0] == line:
            for j in range(i + 1, min(i + 8, len(recs))):
                if rec_op(recs[j]) == OP_ENT:
                    for k in range(j + 1, len(recs)):
                        if rec_op(recs[k]) == OP_END:
                            return j, k
                    break
    raise SystemExit(f'ido_binasm_patch: no procedure at line {line}')


def anchor_index(recs, start, end, anchor):
    if anchor['kind'] != 'range_check_target':
        raise SystemExit(f'ido_binasm_patch: unknown anchor kind {anchor["kind"]}')
    ordinal = anchor.get('ordinal', 1)
    seen = 0
    for i in range(start, end):
        r = recs[i]
        if rec_op(r) == OP_INSTRUCTION and struct.unpack('>H', r[6:8])[0] == BGEU_MACRO:
            seen += 1
            if seen == ordinal:
                label = rec_label(r)
                targets = [k for k in range(start, end)
                           if rec_op(recs[k]) == OP_LABEL and rec_label(recs[k]) == label]
                if len(targets) != 1:
                    raise SystemExit(f'ido_binasm_patch: label {label} defined {len(targets)} times')
                return targets[0]
    raise SystemExit('ido_binasm_patch: range-check branch not found')


def driver_as1_command(cc, args, cwd):
    """The as1 argument list the driver prints for -v, with paths rebased."""
    out = subprocess.run([cc, '-v', '-K'] + args, cwd=cwd, capture_output=True, text=True)
    for line in (out.stderr + out.stdout).splitlines():
        if re.match(r'\S*/as1 ', line):
            parts = line.split()
            parts[0] = str(pathlib.Path(cc).resolve().parent / 'as1')
            return [p for p in parts if p != '-v']
    raise SystemExit('ido_binasm_patch: the driver printed no as1 command')


def text_sha256(obj, function):
    """SHA-256 of the function's words with relocated fields masked, so the
    hash is the same for IDO (section+addend) and GNU (symbol) relocations."""
    out = subprocess.run(['mips-linux-gnu-objdump', '-dr', '-z', obj], capture_output=True, text=True).stdout
    words = []
    relocs = {}
    cur = None
    for l in out.splitlines():
        m = re.match(r'[0-9a-f]+ <(\w+)>:', l)
        if m:
            cur = m.group(1)
            continue
        if cur != function:
            continue
        m = re.match(r'\s*([0-9a-f]+):\s+([0-9a-f]{8})\s', l)
        if m:
            words.append((int(m.group(1), 16), int(m.group(2), 16)))
            continue
        m = re.match(r'\s*([0-9a-f]+):\s+(R_MIPS_\w+)', l)
        if m:
            relocs[int(m.group(1), 16)] = m.group(2)
    masked = []
    for addr, word in words:
        kind = relocs.get(addr)
        if kind == 'R_MIPS_26':
            word &= 0xfc000000
        elif kind in ('R_MIPS_HI16', 'R_MIPS_LO16', 'R_MIPS_GPREL16', 'R_MIPS_16'):
            word &= 0xffff0000
        masked.append('%08x' % word)
    return hashlib.sha256(' '.join(masked).encode()).hexdigest()


def main():
    if len(sys.argv) < 4:
        raise SystemExit(__doc__)
    manifest = json.loads(pathlib.Path(sys.argv[1]).read_text())
    cc = str(pathlib.Path(sys.argv[2]).resolve())
    args = sys.argv[3:]
    out_path = pathlib.Path(args[args.index('-o') + 1]).resolve()
    source = pathlib.Path(args[-1]).resolve()
    source_text = source.read_text(errors='replace')
    with tempfile.TemporaryDirectory(prefix='ido_binasm_patch') as tmp:
        base = source.stem
        # cc runs inside the temporary directory (that is where -K keeps the
        # intermediates), so every relative include path is made absolute.
        rebased = []
        expect_dir = False
        for a in args:
            if expect_dir:
                rebased.append(str(pathlib.Path(a).resolve()))
                expect_dir = False
            elif a == '-I':
                rebased.append(a)
                expect_dir = True
            elif a.startswith('-I') and not pathlib.Path(a[2:]).is_absolute():
                rebased.append('-I' + str(pathlib.Path(a[2:]).resolve()))
            else:
                rebased.append(a)
        rebased[rebased.index('-o') + 1] = str(out_path)
        rebased[-1] = str(source)
        as1_cmd = driver_as1_command(cc, rebased, tmp)
        g_path = pathlib.Path(tmp) / (base + '.G')
        t_path = pathlib.Path(tmp) / (base + '.T')
        if not g_path.exists() or not t_path.exists():
            raise SystemExit('ido_binasm_patch: cc -K left no .G/.T')
        recs = records(g_path.read_bytes())
        inserts = []
        for patch in manifest:
            line = function_line(source_text, patch['function'])
            start, end = procedure_span(recs, line)
            idx = anchor_index(recs, start, end, patch['anchor'])
            new = [make_record(r.get('label', 0), r['op'], r.get('w1', 0), r['words']) for r in patch['insert']]
            inserts.append((idx, new))
        for idx, new in sorted(inserts, reverse=True):
            recs[idx:idx] = new
        patched = pathlib.Path(tmp) / (base + '.patched.G')
        patched.write_bytes(b''.join(recs))
        cmd = []
        for p in as1_cmd:
            if p == base + '.G':
                cmd.append(str(patched))
            elif p == base + '.T':
                cmd.append(str(t_path))
            elif p.endswith('.o') and pathlib.Path(p).name == out_path.name:
                cmd.append(str(out_path))
            else:
                cmd.append(p)
        if str(patched) not in cmd or str(out_path) not in cmd:
            raise SystemExit(f'ido_binasm_patch: could not rebase the as1 command: {cmd}')
        subprocess.run(cmd, cwd=tmp, check=True)
        for patch in manifest:
            expected = patch.get('expected_text_sha256')
            if expected and text_sha256(str(out_path), patch['function']) != expected:
                raise SystemExit(f'ido_binasm_patch: {patch["function"]} text differs from the declared hash')
        shutil.rmtree(tmp, ignore_errors=True)
        os.makedirs(tmp, exist_ok=True)


if __name__ == '__main__':
    main()
