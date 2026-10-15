"""Fail-closed checks for the bounded CPU replay backend."""

import hashlib
from pathlib import Path
import pytest

pytest.importorskip("unicorn")
from tools.semantic_review.replay import replay
from tools.semantic_review.isa import run_probes


def snapshot(words, limit=20):
    return {
        "schema_version": 1,
        "byte_order": "big",
        "start_pc": 0x1000,
        "stop_pc": 0x1000 + len(words) * 4,
        "max_instructions": limit,
        "mappings": [
            {
                "address": 0x1000,
                "size": 4096,
                "permissions": "rwx",
                "data_hex": b"".join(w.to_bytes(4, "big") for w in words).hex(),
            }
        ],
    }


def test_native_isa_probes():
    results = run_probes()
    assert results["status"] == "supported", results
    assert results["cross_validated"] is False


def test_wrong_endian_is_rejected():
    data = snapshot([0])
    data["byte_order"] = "little"
    assert replay(data, synthetic=True)["status"] == "unsupported"


def test_budget_exhaustion_is_not_success():
    data = snapshot([0x1000FFFF, 0], 8)
    result = replay(data, synthetic=True)
    assert result["status"] == "unsupported"
    assert "budget" in result["reason"]


def test_unmapped_access_is_not_success():
    data = snapshot([0x8C020000])
    assert replay(data, synthetic=True)["status"] == "unsupported"


def test_write_footprint():
    data = snapshot([0x24021080, 0x24030007, 0xAC430000])
    result = replay(data, synthetic=True)
    assert result["status"] == "completed"
    assert result["writes"] == [{"address": 0x1080, "size": 4, "value": 7}]


def test_requires_original_rom(tmp_path):
    data = snapshot([0x24020007])
    assert replay(data)["status"] == "unsupported"
    raw = bytes.fromhex(data["mappings"][0]["data_hex"])
    rom = tmp_path / "original.z64"
    rom.write_bytes(raw)
    data["rom_sha256"] = hashlib.sha256(raw).hexdigest()
    data["authenticated_code"] = [
        {
            "address": 0x1000,
            "rom_offset": 0,
            "size": 4,
            "sha256": hashlib.sha256(raw).hexdigest(),
            "overlay": "probe",
        }
    ]
    assert replay(data, rom)["status"] == "completed"
    data["mappings"][0]["data_hex"] = "24020009"
    assert replay(data, rom)["status"] == "unsupported"


def test_external_call_is_not_silently_stubbed():
    data = snapshot([0x24020007])
    data["external_calls"] = [{"address": 0x1000, "return_value": 42}]
    assert "external call" in replay(data, synthetic=True)["reason"]


def test_unsupported_register_state():
    data = snapshot([0])
    data["registers"] = {"fpr_0": 0}
    assert replay(data, synthetic=True)["status"] == "unsupported"


def test_coprocessor_is_explicitly_unsupported():
    result = replay(snapshot([0x44020000]), synthetic=True)
    assert result["status"] == "unsupported"
    assert "coprocessor" in result["reason"]


def test_cached_uncached_aliases_rejected():
    data = snapshot([0])
    data["mappings"] = [
        {"address": 0x80001000, "size": 4096},
        {"address": 0xA0001000, "size": 4096},
    ]
    assert "aliases" in replay(data, synthetic=True)["reason"]


def test_observed_memory_and_capture_immutable():
    data = snapshot([0x24021080, 0x24030007, 0xAC430000])
    data["observations"] = [{"address": 0x1080, "size": 4}]
    result = replay(data, synthetic=True)
    assert result["memory"] == [{"address": 0x1080, "data_hex": "00000007"}]


def test_wrong_expected_value_is_detected():
    from tools.semantic_review.isa import probe

    assert probe("poison", [0x24020007], {"gpr_2": 9})["status"] == "unsupported"


def test_sign_extended_and_uncached_kseg_share_memory():
    # lui creates sign-extended pointer; write cached, read uncached.
    data = snapshot([0x3C028040, 0x24030007, 0xAC430000, 0x3C04A040, 0x8C850000])
    data["mappings"].append({"address": 0x80400000, "size": 4096, "permissions": "rw"})
    data["observations"] = [{"address": 0x80400000, "size": 4}]
    result = replay(data, synthetic=True)
    assert result["status"] == "completed", result
    assert result["registers"]["gpr_5"] == 7
    assert result["memory"][0]["data_hex"] == "00000007"


def test_uncached_alias_cannot_modify_authenticated_code(tmp_path):
    # lui v0,a000; ori v0,1000; sw zero,0(v0), using the uncached alias.
    data = snapshot([0x3C02A000, 0x34421000, 0xAC400000])
    raw = bytes.fromhex(data["mappings"][0]["data_hex"])
    data["start_pc"] = data["mappings"][0]["address"] = 0x80001000
    data["stop_pc"] = 0x8000100C
    data["authenticated_code"] = [
        {
            "address": 0x80001000,
            "size": len(raw),
            "rom_offset": 0,
            "overlay": "synthetic-test",
            "sha256": hashlib.sha256(raw).hexdigest(),
        }
    ]
    data["rom_sha256"] = hashlib.sha256(raw).hexdigest()
    rom = tmp_path / "synthetic-rom"
    rom.write_bytes(raw)
    result = replay(data, rom)
    assert result["status"] == "unsupported"
    assert "write to authenticated" in result["reason"]


def test_mmio_cannot_be_supplied_as_ordinary_ram():
    data = snapshot([0])
    data["mappings"].append(
        {"address": 0xA4600000, "size": 4096, "permissions": "rw", "data_hex": ""}
    )
    result = replay(data, synthetic=True)
    assert result["status"] == "unsupported"
    assert "MMIO" in result["reason"]


def test_malformed_capture_is_classified_instead_of_crashing():
    assert replay([], synthetic=True)["status"] == "unsupported"
