"""Invented configurations guard actor/prop domains, offsets and same-name identities."""

import struct

import pytest

from tools.script_decoder.actors import actor_kind_records


class Image:
    def symbol(self, name):
        assert name == 'gActorKindConfigurationsFromOne'
        return name, 0x80401000

    def read_address(self, address, size):
        if address in (0x80401000, 0x80401054):
            data = bytearray(0x54)
            struct.pack_into('>I', data, 8, 0x80402000)
            struct.pack_into('>I', data, 0x18, 0x80403000 if address == 0x80401000 else 0x80403100)
            struct.pack_into('>i', data, 0x50, 1 if address == 0x80401000 else 2)
        elif 0x80402000 <= address < 0x80402000 + 9:
            data = b'Authored\0'[address - 0x80402000:]
        elif address in (0x80403034, 0x80403134):
            data = struct.pack('>H', 1 if address == 0x80403034 else 2)
        else:
            raise ValueError('outside synthetic input')
        return bytes(data[:size]), dict(address=address, section='.boot')


def test_same_inspection_names_do_not_merge_distinct_actor_configurations():
    rows = actor_kind_records(Image(), {1: 'ACTOR_KIND_INVENTED_PROP', 2: 'ACTOR_KIND_INVENTED_ALTERNATE'})
    assert len(rows) == 2
    assert rows[0]['default_name'] == rows[1]['default_name'] == 'Authored'
    assert rows[0]['configuration'] == 0x80401000
    assert rows[1]['configuration'] == 0x80401054
    assert [row['resource_index'] for row in rows] == [1, 2]
    assert rows[0]['alternate_name'] is None
    with pytest.raises(ValueError, match='not creation kinds'):
        actor_kind_records(Image(), {38: 'ACTOR_SEARCH_RANGE_ENDPOINT'})
