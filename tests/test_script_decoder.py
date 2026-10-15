"""Synthetic, independently authored format cases; no extracted script bytes."""

import json
import struct
from pathlib import Path

import pytest

from tools.script_decoder import context
from tools.script_decoder.context import LocalImage, annotate, opcode_names
from tools.script_decoder.demo import sample, value
from tools.script_decoder.formats import decode_records, decode_vm, packed
from tools.script_decoder import __main__ as cli


def vm(data, **kwargs):
    return decode_vm(data, opcode_names('vm'), **kwargs)


def test_conditional_and_call_edges_are_distinct_from_execution_order():
    result = vm(sample('vm'), natives={1: dict(address=0x80401000, functions=['inventedHandler'], inline_operands=[])})
    assert not result['diagnostics']
    assert [r['offset'] for r in result['instructions']] == [0, 6, 12, 13, 19, 25]
    branch = result['instructions'][3]
    assert branch['target'] == 25
    assert branch['successors'] == [dict(kind='fallthrough', offset=19), dict(kind='branch', offset=25)]
    result = vm(bytes([26]) + value(7) + bytes([35, 30]))
    assert not result['diagnostics']
    call = result['instructions'][0]
    assert call['successors'] == [dict(kind='return_continuation', offset=6), dict(kind='call', offset=7)]
    assert result['instructions'][-1]['dynamic_return']


def test_cycles_finish_and_unreachable_bytes_do_not_trigger_linear_guessing():
    result = vm(bytes([28]) + value(0) + b'\xff\xff')
    assert len(result['instructions']) == 1
    assert result['unparsed_ranges'] == [[6, 8]]
    assert not result['diagnostics']
    assert not vm(bytes([35, 255]))['diagnostics']
    assert vm(bytes([255]))['diagnostics']
    assert vm(bytes([0, 1, 2, 35]), limit=2)['diagnostics']


def test_lengths_and_operand_roles_cover_each_embedded_operand_family():
    for opcode, count in {10: 2, 24: 1, 25: 2, 26: 1, 27: 1, 28: 1, 29: 1, 32: 3, 33: 2, 34: 2}.items():
        end = 1 + 5 * count
        program = bytes([opcode]) + value(end) * count + bytes([35])
        row = vm(program)['instructions'][0]
        assert row['size'] == end
        assert len(row['operands']) == len(row['operand_roles']) == count
    assert vm(bytes([24, 0, 0]))['diagnostics']
    assert vm(bytes([28]) + value(-1))['diagnostics']
    # Conditional jump enters the payload of the following literal.
    result = vm(bytes([29]) + value(8) + bytes([24]) + value(123) + bytes([35]))
    assert any('enters an operand' in error for error in result['diagnostics'])


def test_unknown_tags_and_nonfinite_payloads_stay_explicit():
    unresolved = packed(value(-17, tag=2))
    assert unresolved['kind'] == 'unresolved_integer_like'
    assert unresolved['tag'] == 2 and unresolved['value'] == -17
    assert packed(value(123, tag=99)) == dict(tag=99, payload_hex='0000007b', kind='unknown')
    assert packed(value(-1, tag=3))['value'] == -1
    assert packed(value(-1, tag=4))['value'] == 0xFFFFFFFF
    nan = packed(b'\x01\x7f\xc0\x00\x01')
    assert nan['value'] is None and nan['payload_hex'] == '7fc00001'
    json.dumps(nan, allow_nan=False)
    assert vm(bytes([28]) + value(5, tag=3))['diagnostics']
    # A tagged float jump is converted like the numeric consumer, not read as raw bits.
    result = vm(bytes([28, 1]) + struct.pack('>f', 6.75) + bytes([35]))
    assert not result['diagnostics'] and result['instructions'][0]['target'] == 6


def test_native_links_require_an_explicit_nonnull_table_entry():
    data = bytes([27]) + value(3) + bytes([35])
    assert 'native' not in vm(data)['instructions'][0]
    natives = {3: dict(address=0x80001000, functions=['inventedHandler'], inline_operands=[])}
    result = vm(data, natives=natives)
    annotate(result, {'inventedHandler': [dict(file='src/example.c', line=12)]})
    assert result['instructions'][0]['native']['source']['inventedHandler'][0]['line'] == 12
    assert vm(data, natives={})['diagnostics']
    assert vm(data, natives={3: dict(address=0, functions=[])})['diagnostics']


def test_cue_stop_is_not_collection_end_and_launch_payload_is_typed():
    result = decode_records(sample('cue'), opcode_names('cue'), 'cue')
    assert [r['opcode'] for r in result['instructions']] == [18, 25, 48, 49]
    assert not result['instructions'][0]['time_applies']
    assert result['instructions'][1]['launch'] == dict(kind=5, name='target_position', speed=1.25)
    assert not result['diagnostics']
    raw = struct.pack('>ifIfI', 46, 0, 0x80400014, 1, 0) + struct.pack('>ifIII', 49, 0, 0, 0, 0)
    result = decode_records(raw, opcode_names('cue'), 'cue', 0x80400000)
    assert result['instructions'][0]['jump']['offset'] == 20
    bad = bytearray(raw)
    struct.pack_into('>I', bad, 8, 0x80400015)
    assert decode_records(bad, opcode_names('cue'), 'cue', 0x80400000)['diagnostics']
    assert decode_records(raw[:-1], opcode_names('cue'), 'cue')['diagnostics']


def test_sequencer_uses_sixteen_bytes_and_preserves_unused_tail():
    raw = sample('sequence') + b'\xff' * 16
    result = decode_records(raw, opcode_names('sequence'), 'sequence')
    assert [r['offset'] for r in result['instructions']] == [0, 16]
    assert result['instructions'][0]['arguments'] == [123, 18]
    assert result['unparsed_ranges'] == [[32, 48]]
    assert not result['diagnostics']
    assert decode_records(b'\0' * 15, opcode_names('sequence'), 'sequence')['diagnostics']


def fake_elf():
    class Elf:
        names = ['', '.boot', '.scene_first', '.scene_second']
        sections = [(0,) * 10, (0, 1, 0, 0x80000000, 0, 16),
                    (0, 1, 0, 0x80100000, 0, 16), (0, 1, 0, 0x80100000, 0, 16)]

        def data(self, index):
            return bytes([index]) * 16
    return Elf()


def image_fixture():
    image = LocalImage.__new__(LocalImage)
    image.elf = fake_elf()
    image.allowed = {'.boot'}
    image.rom = b'\x01' * 16 + b'\x02' * 16 + b'\x03' * 16
    image.symbols = [('boot_ROM_START', 0, 0, 0, 0, 0xFFF1),
                     ('scene_first_ROM_START', 16, 0, 0, 0, 0xFFF1),
                     ('scene_second_ROM_START', 32, 0, 0, 0, 0xFFF1)]
    return image


def test_overlay_identity_and_rom_authentication_are_required():
    image = image_fixture()
    assert image.read_address(0x80000000, 4)[0] == b'\x01' * 4
    with pytest.raises(ValueError, match='unique ROM-backed'):
        image.locate(0x80100000, 4)
    image.allowed.add('.scene_first')
    assert image.read_address(0x80100000, 4)[0] == b'\x02' * 4
    image.allowed.add('.scene_second')
    with pytest.raises(ValueError, match='unique ROM-backed'):
        image.locate(0x80100000, 4)
    image.allowed.remove('.scene_second')
    image.rom = b'\0' * 48
    with pytest.raises(ValueError, match='differ'):
        image.read_address(0x80100000, 4)


def test_cli_keeps_outputs_local_and_demo_requires_no_rom(tmp_path, monkeypatch):
    monkeypatch.setattr(cli, 'OUTPUT', tmp_path / 'build/script-decoder')
    assert cli.main(['vm', '--demo']) == 0
    report = json.loads((cli.OUTPUT / 'demo-vm/listing.json').read_text())
    assert report['provenance']['synthetic']
    assert cli.main(['cue', '--demo', '--output', str(tmp_path / 'published')]) == 2
    assert not (tmp_path / 'published').exists()
    assert cli.main(['vm', '--rom-offset', '0', '--bytes', '-1']) == 2
    assert cli.main(['vm', '--demo', '--elf', 'missing.elf']) == 2
    directory = cli.OUTPUT / 'escape'
    directory.mkdir()
    (directory / 'listing.json').symlink_to(tmp_path / 'outside.json')
    assert cli.main(['vm', '--demo', '--output', str(directory)]) == 2
    assert not (tmp_path / 'outside.json').exists()


def test_wrong_rom_is_rejected_before_any_output(tmp_path):
    rom = tmp_path / 'invented.z64'
    rom.write_bytes(b'\x80\x37\x12\x40' + bytes(64))
    with pytest.raises(ValueError, match='normalized USA ROM'):
        LocalImage(rom)


def test_local_extracted_input_is_bounded_and_does_not_claim_rom_provenance(tmp_path, monkeypatch):
    monkeypatch.setattr(cli, 'OUTPUT', tmp_path / 'build/script-decoder')
    path = tmp_path / 'invented.bin'
    program = bytes([24]) + value(33) + bytes([35])
    path.write_bytes(b'prefix' + program)
    args = ['vm', '--input', str(path), '--input-offset', '6', '--bytes', str(len(program))]
    assert cli.main(args) == 0
    report = json.loads((cli.OUTPUT / 'vm/listing.json').read_text())
    assert report['provenance']['origin'] == 'local_input_without_ROM_authentication'
    assert 'rom_sha1' not in report['provenance']
    assert cli.main(['vm', '--input', str(path), '--bytes', '1000']) == 2
    assert cli.main(args + ['--native-table', '0x80001000', '--native-count', '2']) == 2


def test_configuration_selects_program_and_its_own_bounded_command_table(tmp_path, monkeypatch):
    monkeypatch.setattr(cli, 'OUTPUT', tmp_path / 'build/script-decoder')
    class Image:
        sha1 = 'synthetic-ROM-fixture'

        def __init__(self, *args):
            pass

        def read_address(self, address, size):
            if address == 'inventedConfiguration':
                assert size == 20
                return struct.pack('>4Ii', 0x80400000, 0x80401000, 0x80402000, 0, 16), dict(address=0x80403000)
            assert address == 0x80400000 and size == len(sample('vm'))
            return sample('vm'), dict(address=address)

        def table(self, address, count):
            assert (address, count) in ((0x80402000, 2), ('D_8012B400', 279))
            return {1: dict(address=0x80001000, functions=['inventedHandler'])}

        def symbol(self, name):
            assert name == 'D_8012B400'
            return (name, 0x80402000, 279 * 4)

    monkeypatch.setattr(cli, 'LocalImage', Image)
    assert cli.main(['vm', '--config', 'inventedConfiguration', '--bytes', str(len(sample('vm'))), '--native-count', '2']) == 0
    report = json.loads((cli.OUTPUT / 'vm/listing.json').read_text())
    assert report['provenance']['configuration']['program'] == 0x80400000
    assert report['instructions'][4]['native']['functions'] == ['inventedHandler']
    assert cli.main(['vm', '--config', 'inventedConfiguration', '--bytes', '26', '--native-table', 'other']) == 2


def test_inline_native_operands_preserve_the_following_instruction_boundary():
    program = bytes([27]) + value(3) + value(2) + bytes([35])
    native = {3: dict(address=0x80001000, functions=['inventedTextCommand'], inline_operands=['string_count'])}
    result = vm(program, natives=native)
    assert not result['diagnostics']
    assert [row['offset'] for row in result['instructions']] == [0, 11]
    assert result['instructions'][0]['operand_roles'] == ['native_selector', 'string_count']
    assert vm(program, natives={3: dict(address=0x80001000, functions=['unreviewedCommand'])})['diagnostics']
    assert vm(program[:9], natives=native)['diagnostics']


def test_retained_native_inline_reader_contracts_have_not_silently_changed():
    import re
    from tools.magic_numbers import regions
    readers = {}
    for path in (context.ROOT / 'src').rglob('*.c'):
        code = context.code_only(path.read_text())
        for start, end, name in regions(code)[0]:
            body = code[code.index('{', start):end]
            count = len(re.findall(r'\bscriptVmReadIntegerOperand\s*\(', body))
            if count and name not in ('actorScriptDispatchRegisteredCommand', 'actorRunScriptVm', 'scriptVmReadVariableReference'):
                readers[name] = count
    assert readers == {name: 1 for name in context.INLINE_NATIVE_OPERANDS}


def test_decoded_boundaries_include_the_predeclared_runtime_control_endpoints():
    from tools.semantic_review.cases import RAM
    from tools.semantic_review.vm_control_domains import experiments
    native = {0: dict(address=0x80001000, functions=['syntheticControl'], inline_operands=[])}
    for case in experiments():
        program = next(data for address, data in case['patches'] if address == RAM + 0x3000)
        result = vm(program, natives=native)
        final_vm = case['observations'][2][1]
        cursor = int.from_bytes(final_vm[0x14:0x18], 'big')
        boundaries = {row['offset'] for row in result['instructions']}
        boundaries.update(row['offset'] + row['size'] for row in result['instructions'])
        assert cursor in boundaries, case['id']
