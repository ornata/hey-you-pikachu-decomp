"""Invented configuration prefixes and source calls; no game resource bytes."""

import struct

from tools.script_decoder import catalog as module
from tools.script_decoder.demo import value

BASE = 0x80400000
COMMANDS = 0x80405000


def resource(program=BASE, strings=0, commands=COMMANDS):
    code = bytes([32]) + value(0) * 3 + bytes([35]) + b'\0' * 3
    return code + struct.pack('>4Ii', program, strings, commands, 0, 200)


def test_prefix_requires_the_retained_table_and_a_preceding_local_program():
    rows = module.candidates(resource(), BASE, COMMANDS)
    assert len(rows) == 1
    assert rows[0]['address'] == BASE + 20
    assert rows[0]['window_bytes'] == 20
    assert rows[0]['stack_capacity'] == 200
    assert not module.candidates(resource(program=BASE + 40), BASE, COMMANDS)
    assert not module.candidates(resource(strings=BASE + 100), BASE, COMMANDS)
    assert not module.candidates(resource(commands=COMMANDS + 4), BASE, COMMANDS)
    assert not module.candidates(resource()[:-1], BASE, COMMANDS)


def test_creation_bindings_do_not_treat_symbol_arithmetic_as_a_configuration():
    code = '''void f(void) {
 actorCreateForKind(7, helper(1, 2), 0, 0, 0, 0, 1, inventedConfig, 0);
 actorCreateForKind(8, 0, 0, 0, 0, 0, 1, inventedConfig + 4, 0);
}'''
    assert module.creation_sites(code) == [dict(script_symbol='inventedConfig', actor_kind_expression='7', line=2)]


def test_catalog_preserves_candidate_scope_and_direct_creation_evidence(tmp_path, monkeypatch):
    (tmp_path / 'src').mkdir()
    (tmp_path / 'src/example.c').write_text('void f(void) { actorCreateForKind(7, 0, 0, 0, 0, 0, 1, inventedConfig, 0); }')
    monkeypatch.setattr(module, 'ROOT', tmp_path)
    raw = resource()

    class Elf:
        names = ['', '.boot', '.scene_invented_bss']
        sections = [(0,) * 10, (0, 1, 0, BASE, 0, len(raw)), (0, 8, 0, BASE + 100, 0, 40)]

        def data(self, index):
            assert index == 1
            return raw

    class Image:
        elf = Elf()
        sha1 = 'synthetic'
        symbols = [('inventedConfig', BASE + 20, 0, 17, 0, 1)]

        def symbol(self, name):
            return (name, COMMANDS, 279 * 4)

        def table(self, address, count):
            return {0: dict(address=0, functions=[])}

        def read_address(self, address, size):
            start = address - BASE
            return raw[start:start + size], dict(address=address, section='.boot')

    report, listings = module.catalog(Image())
    assert report['candidate_count'] == report['diagnostic_free_candidates'] == 1
    assert report['directly_created_candidates'] == 1
    row = report['configurations'][0]
    assert row['direct_creation_sites'][0]['script_symbol'] == 'inventedConfig'
    assert row['frame_operands'] == [dict(argument_count=0, local_count=0, frame_index=0)]
    assert row['candidate_basis'].endswith('not allocation/type proof.')
    assert not next(iter(listings.values()))['diagnostics']
