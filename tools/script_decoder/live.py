"""Compare a read-only live program capture with authenticated ROM candidates.

An equal bounded window establishes byte correspondence, not an allocation size,
exhaustive resource discovery, or the meaning of every script variable.
"""

import argparse
import hashlib
import json
from pathlib import Path

from .catalog import catalog
from .context import LocalImage, ROOT
from .__main__ import OUTPUT


def compare_candidates(image, configurations, program, raw):
    """Keep all overlay candidates, including indistinguishable short prefixes."""
    if not raw:
        raise ValueError('empty live capture cannot establish correspondence')
    rows = []
    for candidate in configurations:
        if candidate['program'] != program:
            continue
        image.allowed = {'.boot', '.ovl', candidate['section']}
        size = min(len(raw), candidate['window_bytes'])
        expected, location = image.read_address(program, size)
        mismatch = next((i for i, (a, b) in enumerate(zip(raw, expected)) if a != b), None)
        rows.append(dict(section=candidate['section'], configuration=candidate['address'],
                         compared_bytes=size, covers_candidate_window=size == candidate['window_bytes'],
                         matches=mismatch is None, first_difference=mismatch, location=location,
                         expected_sha256=hashlib.sha256(expected).hexdigest(),
                         captured_window_sha256=hashlib.sha256(raw[:size]).hexdigest()))
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--rom', type=Path, default=ROOT / 'rom/baserom.us.z64')
    parser.add_argument('--elf', type=Path, default=ROOT / 'build/pikachu.elf')
    parser.add_argument('--output', type=Path, default=OUTPUT / 'live-correspondence.json')
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(OUTPUT.resolve()):
        parser.error('outputs must stay under build/script-decoder/')
    runtime = json.loads(args.runtime.read_text())
    captures = [event for event in runtime['events'] if event['stage'] == 'program_capture']
    if runtime['status'] != 'completed' or not captures:
        parser.error('a completed runtime with live program captures is required')
    image = LocalImage(args.rom, args.elf)
    if runtime['rom_sha256'] != hashlib.sha256(image.rom).hexdigest():
        parser.error('runtime and decoder ROM provenance differ')
    census, _ = catalog(image)
    rows = []
    for capture in captures:
        if Path(capture['file']).name != capture['file']:
            parser.error('capture file must be a local basename')
        raw = (args.runtime.parent / capture['file']).read_bytes()
        if len(raw) != capture['size'] or hashlib.sha256(raw).hexdigest() != capture['sha256']:
            parser.error('capture length or digest differs from runtime observation')
        rows.append(dict(capture=capture, candidates=compare_candidates(
            image, census['configurations'], capture['program'], raw)))
    report = dict(schema_version=1,
                  scope='Byte correspondence within explicit candidate windows; equal virtual addresses remain separate overlays.',
                  runtime_sha256=hashlib.sha256(args.runtime.read_bytes()).hexdigest(),
                  elf_sha256=hashlib.sha256(args.elf.read_bytes()).hexdigest(),
                  rom_sha1=image.sha1, captures=rows)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(output)


if __name__ == '__main__':
    main()
