"""Read-only ROM interpreter capture through Mupen64Plus's public debugger API.

Guest writes are permitted only in explicitly labelled synthetic experiments.
External graphics, audio and RSP plugins are optional and fingerprinted by the
capture manifest. Core dummy substitutes cannot establish gameplay fidelity.
All state and saves stay in the output directory.
"""

from __future__ import annotations
import ctypes as C
import hashlib
import threading
from pathlib import Path

PINNED_REVISION = "ba95bab92a76744753bfe61470823a4937850ab0"
ROM_SHA1 = "da1a1e47a86720f9d54fb2d2d247480041bda824"
VOID = C.CFUNCTYPE(None)
UPDATE = C.CFUNCTYPE(None, C.c_uint32)
LOG = C.CFUNCTYPE(None, C.c_void_p, C.c_int, C.c_char_p)
STATE = C.CFUNCTYPE(None, C.c_void_p, C.c_int, C.c_int)


class Unsupported(RuntimeError):
    """The selected core cannot provide the requested evidence."""


class Core:
    """Own one interpreter session; callback storage outlives execution."""

    def __init__(
        self, library: Path, work: Path, input_plugin=None, plugins=None, data_path=None
    ):
        self.path = Path(library).resolve()
        self.lib = C.CDLL(str(self.path))
        self.work = Path(work).resolve()
        self.work.mkdir(parents=True, exist_ok=True)
        data_path = Path(data_path).resolve() if data_path else self.work
        self.messages = []
        self.input_plugin = input_plugin
        self.plugins = {
            int(kind): C.CDLL(str(Path(path).resolve()))
            for kind, path in (plugins or {}).items()
        }
        if input_plugin:
            self.plugins[4] = input_plugin
        signatures = {
            "CoreStartup": (
                [C.c_int, C.c_char_p, C.c_char_p, C.c_void_p, LOG, C.c_void_p, STATE],
                C.c_int,
            ),
            "CoreDoCommand": ([C.c_int, C.c_int, C.c_void_p], C.c_int),
            "CoreAttachPlugin": ([C.c_int, C.c_void_p], C.c_int),
            "CoreDetachPlugin": ([C.c_int], C.c_int),
            "ConfigOpenSection": ([C.c_char_p, C.POINTER(C.c_void_p)], C.c_int),
            "ConfigSetParameter": (
                [C.c_void_p, C.c_char_p, C.c_int, C.c_void_p],
                C.c_int,
            ),
            "DebugSetCallbacks": ([VOID, UPDATE, VOID], C.c_int),
            "DebugSetRunState": ([C.c_int], C.c_int),
            "DebugGetCPUDataPtr": ([C.c_int], C.c_void_p),
            "DebugMemRead8": ([C.c_uint32], C.c_uint8),
            "DebugMemWrite8": ([C.c_uint32, C.c_uint8], None),
            "DebugBreakpointCommand": ([C.c_int, C.c_uint32, C.c_void_p], C.c_int),
        }
        for name, (args, result) in signatures.items():
            fn = getattr(self.lib, name)
            fn.argtypes, fn.restype = args, result
        self._log = LOG(
            lambda _, level, msg: self.messages.append(
                {"level": level, "message": msg.decode(errors="replace")}
            )
        )
        self._state = STATE(lambda *_: None)
        self.check(
            self.lib.CoreStartup(
                0x020001,
                str(self.work).encode(),
                str(data_path).encode(),
                None,
                self._log,
                None,
                self._state,
            ),
            "startup",
        )
        section = C.c_void_p()
        self.check(
            self.lib.ConfigOpenSection(b"Core", C.byref(section)), "configuration"
        )
        for name, value, kind in [
            (b"R4300Emulator", 0, 1),
            (b"EnableDebugger", 1, 3),
            (b"OnScreenDisplay", 0, 3),
            (b"RandomizeInterrupt", 0, 3),
        ]:
            val = C.c_int(value)
            self.check(
                self.lib.ConfigSetParameter(section, name, kind, C.byref(val)),
                name.decode(),
            )
        for name in [b"SaveSRAMPath", b"SaveStatePath", b"ScreenshotPath"]:
            self.check(
                self.lib.ConfigSetParameter(
                    section, name, 4, C.c_char_p(str(self.work).encode())
                ),
                name.decode(),
            )

    @staticmethod
    def check(code, operation):
        if code != 0:
            raise Unsupported(f"Mupen64Plus {operation} failed with API error {code}")

    def ptr(self, kind, ctype, count=1):
        ptr = self.lib.DebugGetCPUDataPtr(kind)
        if not ptr:
            raise Unsupported(f"missing CPU state {kind}")
        return C.cast(ptr, C.POINTER(ctype * count)).contents

    def registers(self):
        result = {f"gpr_{i}": int(x) for i, x in enumerate(self.ptr(2, C.c_uint64, 32))}
        result.update(
            hi=int(self.ptr(3, C.c_uint64)[0]), lo=int(self.ptr(4, C.c_uint64)[0])
        )
        result.update(
            {f"fpr_{i}": int(x) for i, x in enumerate(self.ptr(8, C.c_uint64, 32))}
        )
        return result

    def read(self, address, size):
        # DebugMemRead8 performs the core's host-word byte normalization.
        return bytes(self.lib.DebugMemRead8(address + i) for i in range(size))

    def run(self, rom: Path, callback, max_seconds=30):
        data = Path(rom).read_bytes()
        if hashlib.sha1(data).hexdigest() != ROM_SHA1:
            raise ValueError("capture requires the accepted original big-endian US ROM")
        self.rom = data
        self._rom = C.create_string_buffer(data)
        self.check(self.lib.CoreDoCommand(1, len(data), self._rom), "ROM open")
        for kind in (2, 3, 4, 1):
            plugin = self.plugins.get(kind)
            if plugin:
                plugin.PluginStartup.argtypes = [C.c_void_p, C.c_void_p, LOG]
                self.check(
                    plugin.PluginStartup(self.lib._handle, None, self._log),
                    "plugin startup",
                )
            self.check(
                self.lib.CoreAttachPlugin(kind, plugin._handle if plugin else None),
                "plugin attach",
            )
        self.failure = None

        def update(pc):
            try:
                callback(self, pc)
            except BaseException as exc:
                self.failure = exc
                self.stop()

        self._init = VOID(lambda: self.lib.DebugSetRunState(1))
        self._update = UPDATE(update)
        self._vi = VOID(lambda: None)
        self.check(
            self.lib.DebugSetCallbacks(self._init, self._update, self._vi),
            "debug callbacks (run python -m tools.semantic_review.emulator.build_core)",
        )
        timed_out = []

        def timeout():
            timed_out.append(True)
            self.stop()

        timer = threading.Timer(max_seconds, timeout)
        timer.start()
        try:
            self.check(self.lib.CoreDoCommand(5, 0, None), "execution")
        finally:
            timer.cancel()
            for kind in (1, 4, 3, 2):
                self.lib.CoreDetachPlugin(kind)
            for plugin in self.plugins.values():
                plugin.PluginShutdown()
            self.lib.CoreDoCommand(2, 0, None)
            self.lib.CoreShutdown()
        if self.failure:
            raise self.failure
        if timed_out:
            raise Unsupported("capture wall-clock limit exhausted")

    def stop(self):
        self.lib.CoreDoCommand(6, 0, None)
        self.lib.DebugSetRunState(2)


def replay_slice(snapshot: dict, rom: Path, core: Path, output: Path) -> dict:
    """Execute an explicitly synthetic CPU experiment in the N64 interpreter."""
    mode = snapshot.get("floating_register_mode", "paired32")
    if mode not in ("paired32", "wide64"):
        raise Unsupported("unknown floating-register mode")
    machine = Core(core, output)
    seen = []
    setup_pc = 0x807FFFF0
    setup_bytes = None
    result = {
        "status": "unsupported",
        "experiment": "synthetic guest-memory injection",
        "engine": "mupen64plus-pure-interpreter",
        "floating_register_mode": mode,
        "cpu_setup": "Synthetic MTC0 Status before target entry; temporary instruction bytes restored.",
    }

    def step(m, pc):
        nonlocal setup_bytes
        if not seen:
            if setup_bytes is None:
                if any(setup_pc <= snapshot[key] < setup_pc + 8 for key in ("start_pc", "stop_pc")):
                    raise Unsupported("target entry/stop overlaps CPU setup instructions")
                for mapping in snapshot["mappings"]:
                    raw = bytes.fromhex(mapping["data_hex"])
                    for i, b in enumerate(raw):
                        m.lib.DebugMemWrite8(int(mapping["address"]) + i, b)
                setup_bytes = m.read(setup_pc, 8)
                # A guest MTC0 must update the core's FPR pointer layout. A raw
                # CP0 debugger write changes Status without updating those pointers.
                # COP1 enabled, interrupts disabled; paired32 is the game's MIPS2 ABI.
                status = 0x20000000 if mode == "paired32" else 0x24000000
                m.ptr(2, C.c_uint64, 32)[26] = status
                for i, b in enumerate(bytes.fromhex("409a600000000000")):
                    m.lib.DebugMemWrite8(setup_pc + i, b)
                m.ptr(1, C.c_uint32)[0] = setup_pc
                return
            if pc != setup_pc + 4:
                raise Unsupported("CPU setup did not reach the expected MTC0 boundary")
            expected_status = 0x20000000 if mode == "paired32" else 0x24000000
            if m.ptr(5, C.c_uint32, 32)[12] != expected_status:
                raise Unsupported("CPU setup did not establish the requested Status")
            for i, b in enumerate(setup_bytes):
                m.lib.DebugMemWrite8(setup_pc + i, b)
            regs = m.ptr(2, C.c_uint64, 32)
            for i in range(32):
                regs[i] = snapshot["registers"].get(f"gpr_{i}", 0)
            m.ptr(3, C.c_uint64)[0] = snapshot["registers"].get("hi", 0)
            m.ptr(4, C.c_uint64)[0] = snapshot["registers"].get("lo", 0)
            for i in range(32):
                m.ptr(8, C.c_uint64, 32)[i] = snapshot["registers"].get(f"fpr_{i}", 0)
            m.ptr(1, C.c_uint32)[0] = snapshot["start_pc"]
            seen.append(snapshot["start_pc"])
            return
        seen.append(pc)
        if pc == snapshot["stop_pc"]:
            result.update(
                status="completed",
                registers=m.registers(),
                stop_pc=pc,
                mappings=[
                    dict(x, data_hex=m.read(x["address"], x["size"]).hex())
                    for x in snapshot["mappings"]
                ],
                memory=[
                    dict(x, data_hex=m.read(x["address"], x["size"]).hex())
                    for x in snapshot.get("observations", [])
                ],
            )
            m.stop()
        elif len(seen) > snapshot.get("max_instructions", 1000):
            result["reason"] = "interpreter checkpoint budget exhausted"
            m.stop()

    machine.run(rom, step)
    result["checkpoint_pcs"] = seen
    result["core_sha256"] = hashlib.sha256(Path(core).read_bytes()).hexdigest()
    result["limitations"] = [
        "Instruction callbacks omit recursively executed branch delay slots; budget counts interpreter dispatches.",
        "FCR31 is not exposed by the public debugger API; floating rounding-state comparisons are unsupported.",
    ]
    return result
