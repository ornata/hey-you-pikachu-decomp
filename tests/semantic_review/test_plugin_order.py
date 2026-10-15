"""RSP initialization must receive the selected graphics/audio callbacks."""

import ctypes as C
import hashlib
from types import SimpleNamespace
from tools.semantic_review.emulator import mupen


def test_graphics_audio_input_attach_before_rsp(tmp_path, monkeypatch):
    rom = tmp_path / "synthetic-API-test.z64"
    rom.write_bytes(b"API-only fixture")
    monkeypatch.setattr(mupen, "ROM_SHA1", hashlib.sha1(rom.read_bytes()).hexdigest())
    attached = []
    detached = []

    def success(*args):
        return 0

    lib = SimpleNamespace(
        _handle=1,
        CoreDoCommand=success,
        CoreAttachPlugin=lambda kind, handle: attached.append(kind) or 0,
        CoreDetachPlugin=lambda kind: detached.append(kind) or 0,
        DebugSetCallbacks=success,
        CoreShutdown=success,
    )
    plugins = {
        kind: SimpleNamespace(
            _handle=kind, PluginStartup=success, PluginShutdown=success
        )
        for kind in (1, 2, 3, 4)
    }
    core = mupen.Core.__new__(mupen.Core)
    core.lib = lib
    core.plugins = plugins
    core._log = mupen.LOG(lambda *_: None)
    core.run(rom, lambda *_: None)
    assert attached == [2, 3, 4, 1]
    assert detached == [1, 4, 3, 2]
