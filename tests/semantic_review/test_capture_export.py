"""Live snapshot export refuses invented provenance or altered executable bytes."""

import json
from pathlib import Path
import pytest
from tools.semantic_review.capture import export_replay_slice


class FakeCore:
    rom = bytes.fromhex("24020001") + bytes(4092)

    def __init__(self, tmp_path):
        self.path = tmp_path / "core"
        self.path.write_bytes(b"test-engine")
        self.memory = self.rom

    def read(self, address, size):
        return self.memory[address - 0x80001000 : address - 0x80001000 + size]

    def registers(self):
        return {"gpr_0": 0, "hi": 0, "lo": 0, "fpr_0": 123}


def spec():
    return {
        "start_pc": 0x80001000,
        "stop_pc": 0x80001004,
        "memory": [{"address": 0x80001000, "size": 4096, "permissions": "rwx"}],
        "code": [
            {"address": 0x80001000, "size": 4, "rom_offset": 0, "overlay": "resident"}
        ],
    }


def test_export_preserves_live_bytes_and_only_supported_register_state(tmp_path):
    result = export_replay_slice(FakeCore(tmp_path), spec(), tmp_path)
    assert result["mappings"][0]["data_hex"].startswith("24020001")
    assert "fpr_0" not in result["registers"]
    assert json.loads((tmp_path / "replay.json").read_text()) == result


def test_export_refuses_altered_live_code(tmp_path):
    core = FakeCore(tmp_path)
    core.memory = b"\xff" + core.memory[1:]
    with pytest.raises(ValueError, match="differs"):
        export_replay_slice(core, spec(), tmp_path)
    assert not (tmp_path / "replay.json").exists()


def test_export_refuses_unknown_overlay(tmp_path):
    request = spec()
    request["code"][0]["overlay"] = "unresolved"
    with pytest.raises(ValueError, match="identity"):
        export_replay_slice(FakeCore(tmp_path), request, tmp_path)
