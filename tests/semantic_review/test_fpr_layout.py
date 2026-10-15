"""Guard the distinction between paired/wide FPR evidence and integer cross-checks."""

from pathlib import Path

from tools.semantic_review import isa
from tools.semantic_review.emulator import mupen


def test_layout_probe_requires_both_modes_and_restored_scratch(monkeypatch, tmp_path):
    calls = []

    def execute(snapshot, *args):
        mode = snapshot["floating_register_mode"]
        calls.append(mode)
        return dict(status="completed", registers=dict(gpr_2=0x3F800000 if mode == "paired32" else 0,
                                                       gpr_26=0),
                    memory=[dict(address=0x807FFFF0, data_hex="a5" * 8)])

    monkeypatch.setattr(mupen, "replay_slice", execute)
    result = isa.run_interpreter_fpr_layout(Path("core"), Path("rom"), tmp_path)
    assert result["status"] == "supported"
    assert calls == ["paired32"] * 3 + ["wide64"] * 3
    monkeypatch.setattr(mupen, "replay_slice", lambda *args: dict(
        status="completed", registers=dict(gpr_2=0x3F800000, gpr_26=0), memory=[]))
    assert isa.run_interpreter_fpr_layout(Path("core"), Path("rom"), tmp_path)["status"] == "contradicted"


def test_cross_probes_do_not_hide_layout_failure(monkeypatch, tmp_path):
    monkeypatch.setattr(isa, "probe_cases", lambda: [])
    monkeypatch.setattr(isa, "run_interpreter_fpr_layout", lambda *args: dict(status="contradicted"))
    result = isa.run_cross_probes(Path("core"), Path("rom"), tmp_path)
    assert result["status"] == "contradicted"
