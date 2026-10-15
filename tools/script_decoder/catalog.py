"""Catalog bounded VM configuration candidates from an authenticated local ROM.

The scan recognizes a retained layout, not a C type or proof of live allocation.
Decoded scripts remain local. Reports preserve candidate status and overlay identity.
"""

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import struct
import sys

from .context import ROOT, LocalImage, native_layouts, opcode_names
from .formats import decode_vm, integer_operand
from .__main__ import OUTPUT, number
from tools.source_quality import code_only
from tools.scene_catalog import configuration_locations
from .messages import message_posts


def companion_sites(image):
    """Follow only scenario zero through all 23 authenticated scene configurations."""
    locations = configuration_locations(code_only((ROOT / 'src/ovl/area_terrain_maze.c').read_text()),
                                        code_only((ROOT / 'include/area_catalog.h').read_text()))
    table = image.symbol('gAreaConfigurations')
    image.allowed = {'.boot', '.ovl'}
    pointers, _ = image.read_address(table[1], 23 * 4)
    result = []
    for area_index, (base, offset) in enumerate(locations):
        section = '.' + base.removesuffix('_DATA_START')
        image.allowed = {'.boot', '.ovl', section}
        configuration = image.symbol(base)[1] + offset
        if struct.unpack_from('>I', pointers, area_index * 4)[0] != configuration:
            raise ValueError('linked scene catalog disagrees with source overlay selection')
        raw, _ = image.read_address(configuration, 0x7C)
        scripts = struct.unpack_from('>I', raw, 0x6C)[0]
        if scripts:
            raw, _ = image.read_address(scripts, 4)
            script = struct.unpack('>I', raw)[0]
            result.append(dict(section=section, address=script, area_index=area_index,
                               scene_configuration=configuration, scenario=0, actor_kind=0x25,
                               source='src/ovl/player_companion.c:pikachuSpawnForArea'))
    return result


def candidates(raw, base, commands):
    """Find aligned prefixes whose local program precedes the prefix and shares the known table."""
    result = []
    for table_offset in range(8, len(raw) - 11, 4):
        if int.from_bytes(raw[table_offset:table_offset + 4], 'big') != commands:
            continue
        offset = table_offset - 8
        program, strings, table, metadata, capacity = struct.unpack_from('>4Ii', raw, offset)
        address = base + offset
        if not base <= program < address:
            continue
        if strings and not base <= strings < base + len(raw):
            continue
        result.append(dict(address=address, program=program, string_table=strings,
                           commands=table, unresolved_word=metadata, stack_capacity=capacity,
                           window_bytes=address - program))
    return result


def creation_sites(code):
    """Locate exact symbol/address arguments; expressions and interior pointers remain unbound."""
    result = []
    for match in re.finditer(r'\bactorCreateForKind\s*\(', code):
        depth, start, parts = 1, match.end(), []
        for offset in range(start, len(code)):
            token = code[offset]
            if token == '(':
                depth += 1
            elif token == ')':
                depth -= 1
                if depth == 0:
                    parts.append(code[start:offset].strip())
                    break
            elif token == ',' and depth == 1:
                parts.append(code[start:offset].strip())
                start = offset + 1
        symbol = re.fullmatch(r'&?\s*([A-Za-z_]\w*)', parts[7]) if len(parts) == 9 else None
        if symbol:
            result.append(dict(script_symbol=symbol[1], actor_kind_expression=parts[0],
                               line=code.count('\n', 0, match.start()) + 1))
    return result


def catalog(image, max_instructions=100000):
    """Decode candidates separately; equal virtual addresses do not merge overlays."""
    table_symbol = image.symbol('D_8012B400')
    natives = native_layouts(image, image.table('D_8012B400', 279))
    source = {str(path.relative_to(ROOT)): code_only(path.read_text()) for path in (ROOT / 'src').rglob('*.c')}
    sites = [(file, site) for file, text in source.items() for site in creation_sites(text)]
    companions = companion_sites(image)
    names = opcode_names('vm')
    rows, decodes = [], {}
    for index, section in enumerate(image.elf.sections):
        section_name = image.elf.names[index]
        if section[1] != 1:
            continue
        if section_name not in ('.boot', '.ovl') and not section_name.startswith('.scene_'):
            continue
        image.allowed = {'.boot', '.ovl', section_name}
        if not section[5]:
            continue
        # Authenticate even sections that contain no candidates, so stale bytes
        # cannot silently remove entries from the census.
        raw, _ = image.read_address(section[3], section[5])
        for candidate in candidates(raw, section[3], table_symbol[1]):
            data, location = image.read_address(candidate['program'], candidate['window_bytes'])
            decoded = decode_vm(data, names, natives=natives, limit=max_instructions)
            aliases = sorted({s[0] for s in image.symbols if s[1] == candidate['address'] and
                              (s[5] == index or s[5] == 0xFFF1 and
                               (s[0].endswith(section_name[1:]) if section_name.startswith('.scene_') else '_scene_' not in s[0]))})
            bindings = [dict(file=file, **site) for file, site in sites if site['script_symbol'] in aliases]
            roles = Counter()
            references, globals_used, frames, native_calls = [], set(), [], Counter()
            signature = hashlib.sha256()
            for row in decoded['instructions']:
                offset = row['offset']
                signature.update(struct.pack('>I', offset) + data[offset:offset + row['size']])
                for role, operand in zip(row.get('operand_roles', []), row['operands']):
                    if operand['tag'] == 2:
                        roles[role] += 1
                        references.append(dict(instruction_offset=offset, role=role, value=operand['value']))
                if row['opcode'] in (10, 25, 33, 34) and integer_operand(row['operands'][0]) == -1:
                    global_index = integer_operand(row['operands'][1])
                    if global_index is not None:
                        globals_used.add(global_index)
                if row['opcode'] == 32:
                    frames.append({role: integer_operand(operand) for role, operand in zip(row['operand_roles'], row['operands'])})
                if 'native_selector' in row:
                    native_calls[str(row['native_selector'])] += 1
            identity = f'{section_name[1:]}/{candidate["address"]:08x}'
            rows.append(dict(**candidate, section=section_name, location=location,
                             candidate_basis='Aligned retained-table prefix with local program before header; not allocation/type proof.',
                             symbols=aliases, direct_creation_sites=bindings,
                             companion_scenario_zero_sites=[site for site in companions if site['section'] == section_name and site['address'] == candidate['address']],
                             instructions=len(decoded['instructions']), diagnostics=decoded['diagnostics'],
                             decoded_code_sha256=signature.hexdigest(), code_reference_roles=dict(roles),
                             code_reference_uses=references, global_indices=sorted(globals_used),
                             frame_operands=frames, native_calls=dict(native_calls),
                             script_message_posts=message_posts(decoded['instructions']),
                             listing=identity + '.json'))
            decodes[identity] = decoded
    if not rows:
        raise ValueError('no configuration candidates found; a script census was not established')
    complete = [row for row in rows if not row['diagnostics']]
    roles = Counter()
    for row in complete:
        roles.update(row['code_reference_roles'])
    return dict(schema_version=1, rom_sha1=image.sha1,
                scope='Retained-table configuration candidates with local programs before their headers. No claim of exhaustive resource discovery or live execution.',
                candidate_count=len(rows), diagnostic_free_candidates=len(complete),
                directly_created_candidates=sum(bool(row['direct_creation_sites']) for row in rows),
                companion_scenario_zero_candidates=sum(bool(row['companion_scenario_zero_sites']) for row in rows),
                companion_scenario_zero_references=companions,
                distinct_decoded_code_hashes=len({row['decoded_code_sha256'] for row in rows}),
                code_reference_roles=dict(roles), configurations=rows), decodes


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=ROOT / 'rom/baserom.us.z64')
    parser.add_argument('--elf', type=Path, default=ROOT / 'build/pikachu.elf')
    parser.add_argument('--max-instructions', type=number, default=100000)
    parser.add_argument('--output', type=Path, default=OUTPUT / 'catalog')
    args = parser.parse_args(argv)
    try:
        output = args.output.resolve()
        if not output.is_relative_to(OUTPUT.resolve()):
            raise ValueError('catalog outputs must stay under build/script-decoder/')
        if not 1 <= args.max_instructions <= 100000:
            raise ValueError('instruction budget must be between 1 and 100000')
        image = LocalImage(args.rom, args.elf)
        report, decodes = catalog(image, args.max_instructions)
        report['elf_sha256'] = hashlib.sha256(args.elf.read_bytes()).hexdigest()
        report['decoder_sha256'] = hashlib.sha256(Path(__file__).with_name('formats.py').read_bytes()).hexdigest()
        files = {'catalog.json': report, **{name + '.json': row for name, row in decodes.items()}}
        # Validate every output before writing any content, including existing symlinks.
        for name in files:
            if not (output / name).resolve().is_relative_to(OUTPUT.resolve()):
                raise ValueError('catalog listing escapes build/script-decoder/')
        for name, content in files.items():
            target = output / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(json.dumps(content, indent=2, allow_nan=False) + '\n')
        print(f"{report['diagnostic_free_candidates']}/{report['candidate_count']} candidates decoded; "
              f"{report['directly_created_candidates']} have direct creation bindings. {output}")
        return int(report['diagnostic_free_candidates'] != report['candidate_count'])
    except (OSError, ValueError, KeyError, IndexError, struct.error) as error:
        print(f'error: {error}', file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
