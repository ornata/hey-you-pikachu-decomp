"""Capture normalized original-ROM interpreter checkpoints for semantic review."""

from __future__ import annotations
import hashlib
import json
from pathlib import Path
from .emulator.mupen import Core, Unsupported


def export_replay_slice(machine: Core, spec: dict, output: Path) -> dict:
    """Export an explicit live checkpoint with authenticated executable ranges.

    Memory pages and executable identities must be supplied by the scenario;
    arbitrary RAM content is never promoted to executable provenance.
    """
    regions = []
    for region in spec["memory"]:
        address, size = region["address"], region["size"]
        if address % 4096 or size <= 0 or size % 4096 or size > 8 * 1024 * 1024:
            raise ValueError(
                "replay memory must use aligned pages within the N64 memory limit"
            )
        regions.append(
            dict(
                address=address,
                size=size,
                permissions=region.get("permissions", "rw"),
                data_hex=machine.read(address, size).hex(),
            )
        )
    authenticated = []
    for segment in spec["code"]:
        address, size, offset = (
            segment["address"],
            segment["size"],
            segment["rom_offset"],
        )
        identity = segment.get("overlay")
        if not identity or identity.lower() in ("unknown", "unresolved"):
            raise ValueError(
                "replay export requires an explicit executable overlay identity"
            )
        if size <= 0 or offset < 0 or address % 4 or size % 4:
            raise ValueError("invalid executable range")
        original = machine.rom[offset : offset + size]
        if len(original) != size or machine.read(address, size) != original:
            raise ValueError("live executable range differs from original ROM")
        containing = [
            r
            for r in regions
            if r["address"] <= address
            and address + size <= r["address"] + r["size"]
            and "x" in r["permissions"]
        ]
        if not containing:
            raise ValueError(
                "authenticated executable must be covered by an executable memory page"
            )
        authenticated.append(dict(segment, sha256=hashlib.sha256(original).hexdigest()))
    start = spec["start_pc"]
    if not any(s["address"] <= start < s["address"] + s["size"] for s in authenticated):
        raise ValueError("start PC is outside authenticated executable ranges")
    registers = {
        k: v for k, v in machine.registers().items() if not k.startswith("fpr_")
    }
    snapshot = dict(
        schema_version=1,
        byte_order="big",
        rom_sha256=hashlib.sha256(machine.rom).hexdigest(),
        start_pc=start,
        stop_pc=spec["stop_pc"],
        registers=registers,
        mappings=regions,
        authenticated_code=authenticated,
        max_instructions=spec.get("max_instructions", 10000),
        external_calls=spec.get("external_calls", []),
        observations=spec.get("observations", []),
        provenance={
            "kind": "original-ROM live interpreter checkpoint",
            "core_sha256": hashlib.sha256(machine.path.read_bytes()).hexdigest(),
        },
        limitations=[
            "FCR31, MMU and delay-slot internal state are unavailable; coprocessor-dependent slices are unsupported."
        ],
    )
    (output / "replay.json").write_text(json.dumps(snapshot, indent=2) + "\n")
    return snapshot


def capture(scenario: dict, output: Path, core: Path, rom: Path) -> dict:
    """Capture explicit CPU checkpoints; return unsupported for missing facilities.

    Scenario keys: checkpoint_pcs, max_dispatches, memory [{address,size}],
    max_seconds, data_path, plugins (Mupen plugin type to path), input_plugin,
    controller and vru. Missing graphics/audio/RSP plugins use core dummies;
    captures with those substitutions cannot establish presentation fidelity.
    """
    if (scenario.get("controller") or scenario.get("vru")) and not scenario.get(
        "input_plugin"
    ):
        raise Unsupported(
            "supply input_plugin built with emulator.input.build for deterministic device inputs"
        )
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    if scenario.get("replay_slice"):
        (output / "replay.json").unlink(missing_ok=True)
    plugin = None
    if scenario.get("input_plugin"):
        from .emulator.input import configure

        plugin = configure(
            Path(scenario["input_plugin"]), scenario, output / "device-events.jsonl"
        )
    machine = Core(
        core,
        output / "state",
        plugin,
        scenario.get("plugins"),
        scenario.get("data_path"),
    )
    remaining = set(scenario.get("checkpoint_pcs", []))
    replay_spec = scenario.get("replay_slice")
    replay_exported = False
    if replay_spec:
        remaining.add(replay_spec["start_pc"])
    checkpoints = []
    count = 0
    traces = []
    trace_pcs = set(scenario.get("trace_pcs", []))

    def step(m, pc):
        nonlocal count, replay_exported
        count += 1
        if replay_spec and not replay_exported and pc == replay_spec["start_pc"]:
            export_replay_slice(m, replay_spec, output)
            replay_exported = True
        if pc in trace_pcs:
            if len(traces) >= scenario.get("max_trace_hits", 256):
                raise Unsupported("trace hit limit exhausted")
            event_path = output / "device-events.jsonl"
            logged = (
                [json.loads(line) for line in event_path.read_text().splitlines()]
                if event_path.exists()
                else []
            )
            traces.append(
                {
                    "pc": pc,
                    "dispatch": count,
                    "registers": m.registers(),
                    "memory": [
                        dict(r, data_hex=m.read(r["address"], r["size"]).hex())
                        for r in scenario.get("trace_memory", [])
                    ],
                    "dictionary_words": sum(
                        e["event"] == "dictionary_word" for e in logged
                    ),
                }
            )
        if pc in remaining or (not remaining and not checkpoints):
            memories = []
            for region in scenario.get("memory", []):
                address, size = region["address"], region["size"]
                if size < 0 or size > 8 * 1024 * 1024:
                    raise ValueError("capture region must fit N64 RAM")
                raw = m.read(address, size)
                memories.append(
                    dict(
                        region,
                        data_hex=raw.hex(),
                        sha256=hashlib.sha256(raw).hexdigest(),
                    )
                )
            code = m.read(pc, 64)
            offset = m.rom.find(code)
            unique = offset >= 0 and m.rom.find(code, offset + 1) < 0
            checkpoints.append(
                {
                    "pc": pc,
                    "dispatch": count,
                    "registers": m.registers(),
                    "cop0": list(m.ptr(5, __import__("ctypes").c_uint32, 32)),
                    "memory": memories,
                    "code": {
                        "address": pc,
                        "size": len(code),
                        "sha256": hashlib.sha256(code).hexdigest(),
                        "rom_offset": offset if unique else None,
                        "overlay": "unresolved",
                    },
                }
            )
            remaining.discard(pc)
        if (checkpoints and not remaining) or count >= scenario.get(
            "max_dispatches", 100000
        ):
            m.stop()

    machine.run(rom, step, scenario.get("max_seconds", 30))
    result = {
        "schema_version": 1,
        "byte_order": "big",
        "rom_sha256": hashlib.sha256(Path(rom).read_bytes()).hexdigest(),
        "core_sha256": hashlib.sha256(Path(core).read_bytes()).hexdigest(),
        "engine": "mupen64plus-pure-interpreter",
        "status": "completed" if not remaining else "inconclusive",
        "replay_exported": replay_exported,
        "checkpoints": checkpoints,
        "traces": traces,
        "missing_checkpoint_pcs": sorted(remaining),
        "dispatches": count,
        "messages": machine.messages,
        "limitations": [
            "Unspecified plugin types use core dummy implementations; substituted graphics/audio/RSP behavior cannot establish presentation or gameplay fidelity.",
            "FCR31 and delay-slot branch state are not exposed by the debugger API.",
            "Unresolved overlay identities cannot authenticate replay slices.",
        ],
    }
    build_info = Path(core).parent / "semantic-build.json"
    if build_info.exists():
        provenance = json.loads(build_info.read_text())
        if provenance.get("core_sha256") == result["core_sha256"]:
            result["core_build"] = provenance
    events = output / "device-events.jsonl"
    result["device_events"] = (
        [json.loads(line) for line in events.read_text().splitlines()]
        if events.exists()
        else []
    )
    result["plugin_sha256"] = {
        str(k): hashlib.sha256(Path(v).read_bytes()).hexdigest()
        for k, v in scenario.get("plugins", {}).items()
    }
    if scenario.get("input_plugin"):
        result["plugin_sha256"]["4"] = hashlib.sha256(
            Path(scenario["input_plugin"]).read_bytes()
        ).hexdigest()
    (output / "capture.json").write_text(json.dumps(result, indent=2) + "\n")
    return result
