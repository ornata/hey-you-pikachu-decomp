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
from .scenarios import scenario_domains, function_body
from .actions import action_calls, action_resources
from .actors import actor_kind_records, actor_transformation_requests
from .functions import action_producer_functions
from .vocabulary import vocabulary_catalog
from .dictionaries import dictionary_requests, source_dictionary_requests, attach_dictionary_usage
from .name_catalog import pokemon_name_catalog
from .native_frames import native_frame_reviews, review_manifest_fingerprint
from .control_schemes import control_scheme_catalog, control_scheme_requests
from .windows import text_window_requests
from .caller_frames import analyze_caller_frames
from .progress_fields import progress_field_catalog, progress_field_requests
from .items import item_kind_catalog, item_creation_requests


def companion_sites(image):
    """Follow explicit selector return indices through authenticated scene configurations."""
    locations = configuration_locations(code_only((ROOT / 'src/ovl/area_terrain_maze.c').read_text()),
                                        code_only((ROOT / 'include/area_catalog.h').read_text()))
    table = image.symbol('gAreaConfigurations')
    image.allowed = {'.boot', '.ovl'}
    pointers, _ = image.read_address(table[1], 23 * 4)
    area_ids = {name: int(value) for name, value in re.findall(r'\b(AREA_\w+)\s*=\s*(\d+)\b', code_only((ROOT / 'include/area_catalog.h').read_text()))}
    domains = scenario_domains(code_only((ROOT / 'src/game/collection_presentation.c').read_text()), area_ids)
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
            for scenario in domains[area_index]['indices']:
                raw, _ = image.read_address(scripts + scenario * 4, 4)
                script = struct.unpack('>I', raw)[0]
                result.append(dict(section=section, address=script, area_index=area_index,
                                   scene_configuration=configuration, scenario=scenario, actor_kind=0x25,
                                   selector=domains[area_index]['selector'],
                                   selection_basis='Literal selector return index; branch feasibility not established.',
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


def assigned_creation_sites(code):
    """List conditional local assignments before a creation call, without asserting path feasibility."""
    result = []
    definitions = re.finditer(r'^\s*(?:void|s32)\s+(\w+)\([^;{}]*\)\s*\{', code, re.M)
    for definition in definitions:
        function = definition[1]
        body = function_body(code, function)
        for site in creation_sites(body):
            variable = site['script_symbol']
            if len(re.findall(r'\bu8\s*\*\s*' + re.escape(variable) + r'\s*;', body)) != 1:
                continue
            if re.search(r'&\s*' + re.escape(variable) + r'\b|\b' + re.escape(variable) + r'\s*(?:\+=|-=|\+\+|--)|(?:\+\+|--)\s*' + re.escape(variable) + r'\b', body):
                continue
            assignments = list(re.finditer(r'\b' + re.escape(variable) + r'\s*=\s*([^;]+);', body))
            symbols = [re.fullmatch(r'&?\s*([A-Za-z_]\w*)', assignment[1].strip()) for assignment in assignments]
            # Do not report a later assignment as a source for an earlier call.
            if not assignments or not all(symbols) or any(body.count('\n', 0, assignment.start()) + 1 >= site['line'] for assignment in assignments):
                continue
            for symbol in sorted({match[1] for match in symbols}):
                result.append(dict(script_symbol=symbol, actor_kind_expression=site['actor_kind_expression'],
                                   line=code.count('\n', 0, definition.end()) + site['line'],
                                   function=function, local_variable=variable,
                                   basis='Possible local assignment before creation; branch feasibility not established.'))
    return result


def catalog(image, max_instructions=100000):
    """Decode candidates separately; equal virtual addresses do not merge overlays."""
    table_symbol = image.symbol('D_8012B400')
    natives = native_layouts(image, image.table('D_8012B400', 279))
    source = {str(path.relative_to(ROOT)): code_only(path.read_text()) for path in (ROOT / 'src').rglob('*.c')}
    sites = [(file, site) for file, text in source.items() for site in creation_sites(text)]
    assigned = [(file, site) for file, text in source.items() for site in assigned_creation_sites(text)]
    companions = companion_sites(image)
    actors = actor_kind_records(image)
    progress_fields = progress_field_catalog(image)
    item_kinds = item_kind_catalog(image)
    vocabulary = vocabulary_catalog(image)
    name_catalog = pokemon_name_catalog(image, vocabulary)
    frame_reviews = native_frame_reviews()
    controls = control_scheme_catalog(image)
    actor_by_name = {row['enum_name']: row for row in actors}

    def bind(file, site):
        row = dict(file=file, **site)
        identity = actor_by_name.get(site['actor_kind_expression'])
        if identity:
            row.update(actor_kind=identity['actor_kind'], actor_identity=identity['default_name'])
        return row
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
            bindings = [bind(file, site) for file, site in sites if site['script_symbol'] in aliases]
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
            actions = action_calls(decoded['instructions'])
            rows.append(dict(**candidate, section=section_name, location=location,
                             candidate_basis='Aligned retained-table prefix with local program before header; not allocation/type proof.',
                             symbols=aliases, direct_creation_sites=bindings,
                             assigned_creation_sites=[bind(file, site) for file, site in assigned if site['script_symbol'] in aliases],
                             companion_scenario_sites=[site for site in companions if site['section'] == section_name and site['address'] == candidate['address']],
                             instructions=len(decoded['instructions']), diagnostics=decoded['diagnostics'],
                             decoded_code_sha256=signature.hexdigest(), code_reference_roles=dict(roles),
                             code_reference_uses=references, global_indices=sorted(globals_used),
                             frame_operands=frames, native_calls=dict(native_calls),
                             script_message_posts=message_posts(decoded['instructions']),
                             native_action_calls=actions,
                             action_producer_functions=action_producer_functions(decoded['instructions'], actions),
                             player_control_scheme_requests=control_scheme_requests(decoded['instructions'], controls['schemes']),
                             text_window_requests=text_window_requests(decoded['instructions']),
                             actor_transformation_requests=actor_transformation_requests(decoded['instructions'], actors),
                             progress_field_requests=progress_field_requests(decoded['instructions'], progress_fields),
                             item_creation_requests=item_creation_requests(decoded['instructions'], item_kinds),
                             caller_frame_analysis=(analyze_caller_frames(decoded['instructions'], frame_reviews['active'], frame_reviews['active_deferred'])
                                                    if not decoded['diagnostics'] else dict(scope='Not analyzed: incomplete decoded CFG.')),
                             voice_dictionary_requests=dictionary_requests(decoded['instructions'], vocabulary.get('dictionaries', []), name_catalog,
                                                                           frame_reviews['active'], frame_reviews['active_deferred']),
                             listing=identity + '.json'))
            decodes[identity] = decoded
    if not rows:
        raise ValueError('no configuration candidates found; a script census was not established')
    complete = [row for row in rows if not row['diagnostics']]
    roles = Counter()
    for row in complete:
        roles.update(row['code_reference_roles'])
    resource_ids = {call['constant_script_id'] for row in rows for call in row['native_action_calls'] if 'constant_script_id' in call}
    resource_ids.update(value for row in rows for function in row['action_producer_functions']
                        if function['static_return_domain_complete'] for value in function['integer_return_domain'])
    source_requests = source_dictionary_requests(source, vocabulary.get('dictionaries', []))
    attach_dictionary_usage(vocabulary, rows, source_requests)
    return dict(schema_version=2, rom_sha1=image.sha1,
                scope='Retained-table configuration candidates with local programs before their headers. No claim of exhaustive resource discovery or live execution.',
                candidate_count=len(rows), diagnostic_free_candidates=len(complete),
                directly_created_candidates=sum(bool(row['direct_creation_sites']) for row in rows),
                conditionally_assigned_candidates=sum(bool(row['assigned_creation_sites']) for row in rows),
                companion_scenario_candidates=sum(bool(row['companion_scenario_sites']) for row in rows),
                companion_scenario_references=companions,
                unbound_configurations=[dict(section=row['section'], address=row['address'], program=row['program'],
                                             next_evidence='Find an owning producer or live context; absence of recovered bindings does not prove disuse.')
                                        for row in rows if not (row['direct_creation_sites'] or row['assigned_creation_sites'] or row['companion_scenario_sites'])],
                distinct_decoded_code_hashes=len({row['decoded_code_sha256'] for row in rows}),
                action_script_resources=action_resources(image, resource_ids),
                actor_kinds=actors,
                progress_fields=progress_fields,
                item_kinds=item_kinds,
                voice_vocabulary=vocabulary,
                bedroom_name_catalog=name_catalog,
                native_frame_effect_reviews=frame_reviews,
                player_control_schemes=controls,
                voice_dictionary_source_requests=source_requests,
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
        report['analysis_sha256'] = {name: hashlib.sha256(Path(__file__).with_name(name + '.py').read_bytes()).hexdigest()
                                     for name in ('catalog', 'stack', 'messages', 'actions', 'scenarios', 'actors', 'functions', 'table_returns', 'vocabulary', 'dictionaries', 'name_catalog', 'saved_dictionary', 'native_frames', 'control_schemes', 'windows', 'caller_frames', 'progress_fields', 'items')}
        report['native_frame_review_sha256'] = review_manifest_fingerprint()
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
