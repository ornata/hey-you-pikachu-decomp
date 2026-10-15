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
 actorCreateForKind(9, 0, 0, 0, 0, 0, 1, &otherConfig, 0);
}'''
    assert module.creation_sites(code) == [dict(script_symbol='inventedConfig', actor_kind_expression='7', line=2),
                                          dict(script_symbol='otherConfig', actor_kind_expression='9', line=4)]


def test_conditional_bindings_stay_within_function_and_reject_changed_pointer_values():
    code = '''void factory(void) {
 u8 *record;
 if (flag) { record = &firstConfig; } else { record = secondConfig; }
 actorCreateForKind(5, 0, 0, 0, 0, 0, 1, record, 0);
}
void unrelated(void) {
 u8 *record;
 record = &thirdConfig;
}'''
    sites = module.assigned_creation_sites(code)
    assert {site['script_symbol'] for site in sites} == {'firstConfig', 'secondConfig'}
    assert all(site['function'] == 'factory' and site['line'] == 4 for site in sites)
    for changed in (code.replace('record = secondConfig;', 'record = secondConfig + 4;'),
                    code.replace('if (flag)', 'escape(&record); if (flag)'),
                    code.replace('actorCreateForKind(5', '++record; actorCreateForKind(5'),
                    code.replace('if (flag)', '{ u8 *record; } if (flag)'),
                    code.replace('actorCreateForKind(5', 'record += 4; actorCreateForKind(5')):
        assert not module.assigned_creation_sites(changed)


def test_companion_bindings_follow_explicit_overlay_and_reject_stale_scene_catalog(tmp_path, monkeypatch):
    (tmp_path / 'src/ovl').mkdir(parents=True)
    (tmp_path / 'include').mkdir()
    (tmp_path / 'include/area_catalog.h').write_text('enum { SCENE_AREA_COUNT = 23 };')
    (tmp_path / 'src/ovl/area_terrain_maze.c').write_text(
        'SceneConfigurationPrefix *gAreaConfigurations[SCENE_AREA_COUNT] = {' +
        ','.join(f'(SceneConfigurationPrefix *)(scene_test{i}_DATA_START + 0x0)' for i in range(23)) + '};')
    monkeypatch.setattr(module, 'ROOT', tmp_path)
    monkeypatch.setattr(module, 'scenario_domains', lambda code, areas: {i: dict(indices=[0], selector=None) for i in range(23)})
    (tmp_path / 'src/game').mkdir()
    (tmp_path / 'src/game/collection_presentation.c').write_text('/* Scenario domains supplied by this fixture. */')

    class Image:
        stale = False

        def symbol(self, name):
            return name, 0x80400000 if name == 'gAreaConfigurations' else 0x80100000

        def read_address(self, address, size):
            if address == 0x80400000:
                return struct.pack('>I', 0x80100004 if self.stale else 0x80100000) + struct.pack('>I', 0x80100000) * 22, {}
            # Every overlay intentionally shares the configuration/table addresses.
            index = int(next(section for section in self.allowed if section.startswith('.scene_test')).removeprefix('.scene_test'))
            if size == 0x7C:
                return b'\0' * 0x6C + struct.pack('>I', 0x80100100) + b'\0' * 12, {}
            assert address == 0x80100100 and size == 4
            return struct.pack('>I', 0x80100200 + index * 20), {}

    image = Image()
    sites = module.companion_sites(image)
    assert len(sites) == 23
    assert sites[8]['section'] == '.scene_test8'
    assert sites[8]['address'] == 0x801002A0
    assert sites[8]['actor_kind'] == 0x25 and sites[8]['scenario'] == 0
    image.stale = True
    import pytest
    with pytest.raises(ValueError, match='disagrees'):
        module.companion_sites(image)


def test_catalog_preserves_candidate_scope_and_direct_creation_evidence(tmp_path, monkeypatch):
    (tmp_path / 'src').mkdir()
    (tmp_path / 'src/example.c').write_text('void f(void) { actorCreateForKind(7, 0, 0, 0, 0, 0, 1, inventedConfig, 0); }')
    monkeypatch.setattr(module, 'ROOT', tmp_path)
    monkeypatch.setattr(module, 'companion_sites', lambda image: [])
    monkeypatch.setattr(module, 'actor_kind_records', lambda image: [])
    monkeypatch.setattr(module, 'vocabulary_catalog', lambda image: {'scope': 'Invented vocabulary fixture.'})
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
    assert report['voice_vocabulary'] == {'scope': 'Invented vocabulary fixture.'}
    row = report['configurations'][0]
    assert row['direct_creation_sites'][0]['script_symbol'] == 'inventedConfig'
    assert row['frame_operands'] == [dict(argument_count=0, local_count=0, frame_index=0)]
    assert row['candidate_basis'].endswith('not allocation/type proof.')
    assert not next(iter(listings.values()))['diagnostics']
