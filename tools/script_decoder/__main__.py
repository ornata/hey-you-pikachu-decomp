"""Decode local USA ROM scripts into bounded listings under build/script-decoder/."""

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import sys

from .context import ROOT, ENUM_SOURCES, LocalImage, annotate, native_layouts, opcode_names, source_index
from .demo import sample
from .formats import decode_records, decode_vm

OUTPUT = ROOT / 'build/script-decoder'


def number(value):
    return int(value, 0)


def render(report):
    """Make branch labels and source navigation readable without hiding raw encodings."""
    lines = [f"{report['format']} — {report['scope']}",
             f"Input: {report['provenance']}", '']
    for row in report['instructions']:
        operands = row.get('operands', row.get('arguments', []))
        lines.append(f"L_{row['offset']:04X}: [{row['opcode']}] {row['name']} {json.dumps(operands, allow_nan=False)}")
        for field in ('launch', 'clock', 'jump', 'native', 'handler_targets'):
            if field in row:
                lines.append(f"    {field}: {json.dumps(row[field], allow_nan=False)}")
        if row.get('operand_roles'):
            lines.append('    operand roles: ' + ', '.join(row['operand_roles']))
        if 'time_bits' in row:
            lines.append(f"    time={row['time']} (bits {row['time_bits']}) payload={row['payload_hex']}")
        for edge in row.get('successors', []):
            lines.append(f"    {edge['kind']} -> L_{edge['offset']:04X}")
        if row.get('dynamic_return'):
            lines.append('    return -> saved runtime location (not statically inferred)')
        lines.append(f"    consumer: {row['handler']} " + ', '.join(f"{s['file']}:{s['line']}" for s in row['source']))
    lines += ['', 'Unparsed ranges: ' + str(report['unparsed_ranges'])]
    lines += ['Diagnostic: ' + error for error in report['diagnostics']]
    return '\n'.join(lines) + '\n'


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('format', choices=('vm', 'cue', 'sequence'))
    origin = parser.add_mutually_exclusive_group(required=True)
    origin.add_argument('--demo', action='store_true', help='decode newly authored synthetic examples, without a ROM')
    origin.add_argument('--input', type=Path, help='local extracted/decompressed script; its ROM origin is not inferred')
    origin.add_argument('--rom-offset', type=number, help='explicit offset in your normalized local ROM')
    origin.add_argument('--symbol', help='program address or symbol; requires --elf')
    origin.add_argument('--config', help='VM configuration address or symbol; requires --elf and vm format')
    parser.add_argument('--rom', type=Path, default=ROOT / 'rom/baserom.us.z64')
    parser.add_argument('--elf', type=Path, help='matching local ELF for addresses, tables and handler names')
    parser.add_argument('--section', help='exact ELF overlay section, e.g. .scene_bedroom')
    parser.add_argument('--bytes', type=number, help='explicit window bound; no allocation size is inferred')
    parser.add_argument('--address', type=number, help='known virtual base of a direct ROM-offset or raw-input window')
    parser.add_argument('--input-offset', type=number, default=0, help='offset within --input (default zero)')
    parser.add_argument('--entry', type=number, action='append', help='VM entry offset within the program window (repeatable)')
    parser.add_argument('--max-instructions', type=number, default=10000, help='VM decode budget, at most 100000 (default 10000)')
    parser.add_argument('--native-table', help='explicit native table address/symbol; requires --elf and --native-count')
    parser.add_argument('--native-count', type=number, help='explicit native table bound; includes null slot0 when present')
    parser.add_argument('--output', type=Path, help='output directory under build/script-decoder/')
    args = parser.parse_args(argv)
    try:
        output = (args.output or OUTPUT / ('demo-' + args.format if args.demo else args.format)).resolve()
        if not output.is_relative_to(OUTPUT.resolve()):
            raise ValueError('decoded outputs must stay under build/script-decoder/')
        if args.format != 'vm' and (args.config or args.entry or args.native_table or args.native_count):
            raise ValueError('configuration, entry and native-table options apply only to vm')
        if args.address is not None and args.rom_offset is None and args.input is None:
            raise ValueError('--address applies only to --rom-offset or --input')
        if args.address is not None and not 0 <= args.address <= 0xFFFFFFFF:
            raise ValueError('--address must be an unsigned 32-bit target address')
        if args.input_offset and args.input is None:
            raise ValueError('--input-offset requires --input')
        if not 1 <= args.max_instructions <= 100000:
            raise ValueError('--max-instructions must be between 1 and 100000')
        native_table, natives, cue_handlers, config = args.native_table, None, None, None
        image = None
        address = args.address
        if args.demo:
            if args.elf or args.section or args.bytes is not None or native_table or args.native_count:
                raise ValueError('--demo uses only synthetic bytes and no ELF, window or table options')
            data = sample(args.format)
            if args.format == 'vm':
                natives = {1: dict(address=0x80401000, functions=['syntheticExampleCommand'], inline_operands=[])}
            provenance = dict(synthetic=True, description='Newly authored example; no extracted game script data.')
        elif args.input is not None:
            if args.bytes is None or not 0 < args.bytes <= 0x100000 or args.input_offset < 0:
                raise ValueError('supply a positive --bytes bound up to 0x100000 and nonnegative --input-offset')
            with args.input.open('rb') as stream:
                stream.seek(args.input_offset)
                data = stream.read(args.bytes)
            if len(data) != args.bytes:
                raise ValueError('requested window is outside the local input')
            if args.elf:
                image = LocalImage(args.rom, args.elf, args.section)
            elif native_table is not None or args.native_count is not None or args.section:
                raise ValueError('table resolution requires --elf and the local ROM, even for raw inputs')
            provenance = dict(synthetic=False, origin='local_input_without_ROM_authentication',
                              input=str(args.input), input_offset=args.input_offset, window_bytes=len(data),
                              address=address)
        else:
            if args.bytes is None or not 0 < args.bytes <= 0x100000:
                raise ValueError('supply --bytes between 1 and 0x100000; this is a bound, not an inferred resource size')
            image = LocalImage(args.rom, args.elf, args.section)
            if args.config:
                raw, config_origin = image.read_address(args.config, 20)
                program, strings, commands, unknown, capacity = struct.unpack('>4Ii', raw)
                config = dict(**config_origin, program=program, string_table=strings,
                              commands=commands, unresolved_word=unknown, stack_capacity=capacity)
                if native_table is not None:
                    raise ValueError('--config supplies its own table; do not override it with --native-table')
                native_table = commands
                data, location = image.read_address(program, args.bytes)
                address = location['address']
            elif args.symbol:
                data, location = image.read_address(args.symbol, args.bytes)
                address = location['address']
            else:
                data = image.read_rom(args.rom_offset, args.bytes)
                location = dict(rom_offset=args.rom_offset, address=address)
                if address is not None and args.elf:
                    _, mapped_offset, section = image.locate(address, args.bytes)
                    if mapped_offset != args.rom_offset:
                        raise ValueError('--address disagrees with the ELF ROM mapping')
                    location['section'] = section
            provenance = dict(synthetic=False, rom_sha1=image.sha1, **location,
                              elf_sha256=hashlib.sha256(Path(args.elf).read_bytes()).hexdigest() if args.elf else None,
                              window_bytes=len(data), configuration=config,
                              native_count=args.native_count)
        if image is not None:
            if args.native_count is not None:
                if native_table is None:
                    raise ValueError('--native-count needs --native-table or --config')
                natives = native_layouts(image, image.table(native_table, args.native_count))
            elif args.native_table:
                raise ValueError('--native-table requires an explicit --native-count')
            if args.format == 'cue' and args.elf:
                # Retained table has27 handlers, codes19..45. Authenticate its actual words.
                cue_handlers = image.table('gTimedActionCommandHandlers', 27)
            if args.input:
                provenance['table_context'] = dict(rom_sha1=image.sha1,
                    elf_sha256=hashlib.sha256(Path(args.elf).read_bytes()).hexdigest(),
                    scope='Lookup tables only; the input script is not authenticated against ROM.')
        names = opcode_names(args.format)
        if args.format == 'vm':
            result = decode_vm(data, names, tuple(args.entry or (0,)), natives, args.max_instructions)
        else:
            result = decode_records(data, names, args.format, address)
        result.update(schema_version=2, provenance=provenance,
                      window_sha256=hashlib.sha256(data).hexdigest(),
                      schema_source_sha256={path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest()
                                            for path in (ENUM_SOURCES[args.format][0],
                                                         'src/ovl/actor_script_action_commands.c',
                                                         'tools/script_decoder/formats.py')},
                      limitations=['Static syntax and candidate control flow only; no execution or gameplay reachability proof.',
                                   'No strings, assets, pointer targets or compressed resources are automatically extracted.',
                                   'Only reviewed native inline layouts are used; stack effects and runtime return targets are not inferred.'])
        if config and natives is None:
            result['limitations'].append('Configured native table is unresolved; supply --native-count for bounded handler resolution.')
        annotate(result, source_index(), cue_handlers)
        result['counts'] = dict(opcodes=dict(Counter(str(row['opcode']) for row in result['instructions'])),
                                value_tags=dict(Counter(str(value['tag']) for row in result['instructions']
                                                        for value in row.get('operands', []))))
        result['code_reference_uses'] = [
            dict(offset=row['offset'] + 1 + 5 * i, instruction_offset=row['offset'],
                 role=row['operand_roles'][i], value=value['value'])
            for row in result['instructions'] for i, value in enumerate(row.get('operands', []))
            if value['tag'] == 2
        ]
        output.mkdir(parents=True, exist_ok=True)
        for name in ('listing.json', 'listing.txt'):
            if not (output / name).resolve().is_relative_to(OUTPUT.resolve()):
                raise ValueError('listing paths must stay under build/script-decoder/')
        (output / 'listing.json').write_text(json.dumps(result, indent=2, allow_nan=False) + '\n')
        (output / 'listing.txt').write_text(render(result))
        print(f"Decoded {len(result['instructions'])} {args.format} instructions/records; {len(result['diagnostics'])} diagnostics. {output}")
        return 1 if result['diagnostics'] else 0
    except (OSError, ValueError, KeyError, IndexError, struct.error) as error:
        print(f'error: {error}', file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
