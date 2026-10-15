"""Trace retail voice-to-script flow using controller and VRU inputs only."""

from __future__ import annotations
import argparse
import ctypes as C
import hashlib
import json
import sys
from pathlib import Path
from .mupen import Core, Unsupported
from .input import build, configure

WATCH = {
    0x800409CC: "poll",
    0x80040BA0: "recognition_take",
    0x80012508: "prepare",
    0x800121B0: "launch",
    0x800260F8: "messenger",
    0x80118E74: "deliver",
    0x80119200: "acknowledged",
    0x80012034: "action",
    0x800406B4: "dictionary",
    0x80054BB0: "device_start",
    0x80054C70: "device_read",
    0x8011C8B0: "read_type",
    0x8011C8DC: "read_arg0",
    0x801164A0: "branch_before",
    0x801164CC: "branch_after",
}
VOICE_FIELDS = {
    "device_state": 0x8009D530,
    "dictionary": 0x8009D534,
    "command_status": 0x800F8AC4,
    "read_requested": 0x800F8AC8,
    "device_error": 0x800F8ACC,
    "result_pending": 0x800F8AD4,
    "read_active": 0x800F8B10,
    "override": 0x800AD420,
    "follows_trigger": 0x800AD424,
    "trigger": 0x800AD428,
    "enabled": 0x800AD450,
    "microphone_available": 0x800722D0,
    "commands_allowed": 0x800722D4,
    "program": 0x800AE4F0,
    "game_mode": 0x800AE504,
    "area": 0x800AE4F8,
    "pikachu": 0x800760A8,
    "title": 0x8009B3E4,
}


class Breakpoint(C.Structure):
    _fields_ = [("address", C.c_uint32), ("endaddr", C.c_uint32), ("flags", C.c_uint)]


def controller_value(frame: int, title: int, game_mode: int) -> int:
    """Navigate the blank-save title and text dialogs without reopening Start menus."""
    value = 0
    if frame % 30 == 0:
        if game_mode == 18 and title == 12:
            value = 0x10  # Start on the standard controller.
        elif (game_mode == 18 and title in (10, 13, 14, 15)) or game_mode == 3:
            value = 0x80  # A confirms menus and advances text.
    if game_mode in (0, 8) and frame % 120 < 40:
        value |= 0x20  # Standard-controller Z permits listening.
    return value


def action_context(registers, vm_address, read_word):
    """Record the caller and retained VM action state at an action start."""
    result = {
        "action_id": int(registers[5]) & 0xFFFFFFFF,
        "caller_ra": int(registers[31]) & 0xFFFFFFFF,
    }
    if 0x80000000 <= vm_address < 0x80800000:
        result.update(
            vm_action_selector=read_word(vm_address + 0x2C0),
            action_initializing=read_word(vm_address + 0x2C4),
        )
    return result


def remove_owned_breakpoints(core, addresses):
    """Remove only this driver's execution breakpoints after execution ends.

    The pinned core retains its breakpoint table across CoreShutdown. Its
    execution-only removal path does not access the released guest memory.
    """
    for address in sorted(addresses):
        core.check(
            core.lib.DebugBreakpointCommand(4, address, None),
            "owned execution breakpoint removal",
        )


def run(
    rom: Path,
    core: Path,
    source: Path,
    rsp: Path,
    output: Path,
    seconds=90,
    mode="adaptive",
    response=None,
    gfx=None,
    poll_bound=3300,
):
    """Run a blank-save scenario to a deterministic voice-poll boundary.

    Wall time is a failure backstop. Native instruction breakpoints observe the
    original program; all injected values enter through the input plugin ABI.
    """
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    state = output / "state"
    if state.exists() and any(state.iterdir()):
        raise ValueError(
            "use a fresh output directory; this experiment requires blank saves"
        )
    plugin = build(
        source, output / ("input.dylib" if sys.platform == "darwin" else "input.so")
    )
    controls = (
        [0] * 200 + ([0x90] * 2 + [0] * 28) * 600 if mode == "pulse" else [0] * 65536
    )
    scenario = {
        "controller": controls,
        "vru": {
            "present": True,
            "talk_polls": 60 if response else 0,
            "results": [response] if response else [],
        },
    }
    lib = configure(plugin, scenario, output / "device-events.jsonl")
    plugins = {"1": str(rsp)}
    if gfx:
        plugins["2"] = str(gfx)
    machine = Core(core, state, lib, plugins, Path(source) / "data")
    lib.SetControllerValue.argtypes = [C.c_uint]
    events, hits, pending_calls, branches = [], {}, {}, {}
    installed = set()
    armed, last_state, completed, active_actor = False, None, False, 0
    log = (output / "runtime-events.jsonl").open("w")

    def read32(address):
        return int.from_bytes(machine.read(address, 4), "big")

    def message(address):
        return [read32(address + i * 4) for i in range(6)]

    def valid(address):
        return 0x80000000 <= address < 0x80800000

    def actor_info(actor):
        info = {"actor": actor}
        if valid(actor):
            actor_state = read32(actor + 0x54)
            if valid(actor_state):
                vm = read32(actor_state + 0x1B8)
                info.update(
                    actor_state=actor_state,
                    pending=read32(actor_state + 0x218),
                    message=message(actor_state + 0x21C),
                    vm=vm,
                )
                if valid(vm):
                    info.update(vm_offset=read32(vm + 0x14), program=read32(vm + 4))
        return info

    def record(stage, **fields):
        event = dict(
            stage=stage, dispatch=sum(hits.values()), poll=hits.get("poll", 0), **fields
        )
        events.append(event)
        log.write(json.dumps(event) + "\n")
        log.flush()

    def install(address):
        if address not in installed:
            bp = Breakpoint(address, address, 9)
            if machine.lib.DebugBreakpointCommand(2, 0, C.byref(bp)) < 0:
                raise Unsupported("could not install execution watchpoint")
            installed.add(address)

    def arm_return(kind, regs, **fields):
        sp, ra = int(regs[29]) & 0xFFFFFFFF, int(regs[31]) & 0xFFFFFFFF
        install(ra)
        pending_calls.setdefault((sp, ra), []).append(dict(kind=kind, **fields))

    def step(m, pc):
        nonlocal armed, last_state, completed, active_actor
        if not armed:
            for address in WATCH:
                install(address)
            armed = True
            m.lib.DebugSetRunState(2)
            return
        regs = m.ptr(2, C.c_uint64, 32)
        args = [int(regs[i]) & 0xFFFFFFFF for i in range(4, 8)]
        sp = int(regs[29]) & 0xFFFFFFFF
        kind = WATCH.get(pc, "return")
        hits[kind] = hits.get(kind, 0) + 1
        waiting = pending_calls.get((sp, pc), [])
        if waiting:
            call = waiting.pop()
            retval = int(regs[2]) & 0xFFFFFFFF
            if call["kind"] == "prepare" and retval == 1:
                record(
                    "prepared",
                    pc=pc,
                    message=message(call["out"]),
                    actor=read32(0x800760A8),
                    return_value=retval,
                )
            elif call["kind"] == "deliver":
                info = actor_info(call["actor"])
                record("posted", pc=pc, return_value=retval, **info)
                if retval == 1:
                    active_actor = call["actor"]
            elif call["kind"] in ("read_type", "read_arg0"):
                info = actor_info(call["actor"])
                vm = info.get("vm", 0)
                if valid(vm):
                    depth = read32(vm + 0x18)
                    values = read32(vm + 0x20)
                    if 0 < depth < 1024 and valid(values):
                        top = m.read(values + (depth - 1) * 5, 5)
                        info.update(
                            value_tag=top[0], value=int.from_bytes(top[1:], "big")
                        )
                record(
                    "message_read",
                    pc=pc,
                    field="type" if call["kind"] == "read_type" else "arg0",
                    return_value=retval,
                    **info,
                )
        if kind == "poll":
            current = {name: read32(address) for name, address in VOICE_FIELDS.items()}
            if mode == "adaptive":
                lib.SetControllerValue(
                    controller_value(
                        hits["poll"], current["title"], current["game_mode"]
                    )
                )
            if current != last_state:
                record("voice_state", pc=pc, **current)
                last_state = current
            if hits["poll"] >= poll_bound:
                completed = True
                m.stop()
                return
        elif kind == "prepare":
            arm_return(kind, regs, out=args[0])
        elif kind == "recognition_take" and read32(0x800F8AD4) == 1:
            raw = m.read(0x800F8AD8, 30)
            record(
                "recognition",
                pc=pc,
                dictionary=read32(0x8009D534),
                result_hex=raw.hex(),
            )
        elif kind == "launch":
            payload = args + [read32(sp + 16), read32(sp + 20)]
            record("launch", pc=pc, message=payload, actor=read32(0x800760A8))
        elif kind == "messenger":
            messenger_state = read32(args[0] + 0x54)
            record(
                "messenger",
                pc=pc,
                actor=args[2],
                messenger=args[0],
                message=message(messenger_state + 0x28),
                mode=args[3],
            )
        elif kind == "deliver":
            arm_return(kind, regs, actor=args[0])
            record(
                "deliver",
                pc=pc,
                actor=args[0],
                message=args[1:] + [read32(sp + 16 + i * 4) for i in range(3)],
            )
        elif kind in ("read_type", "read_arg0") and args[0] == active_actor:
            arm_return(kind, regs, actor=args[0])
        elif kind in ("branch_before", "branch_after") and active_actor:
            vm = int(regs[16]) & 0xFFFFFFFF
            actor = read32(sp + 0xA8)
            if actor == active_actor and valid(vm):
                offset = read32(vm + 0x14)
                if kind == "branch_before":
                    program = read32(vm + 4)
                    branches[(sp, vm)] = dict(
                        actor=actor,
                        vm=vm,
                        condition=int(regs[2]) & 0xFFFFFFFF,
                        instruction_offset=offset - 1,
                        operand_offset=offset,
                        fallthrough=offset + 5,
                        target=read32(program + offset + 1),
                        program=program,
                    )
                elif (sp, vm) in branches:
                    branch = branches.pop((sp, vm))
                    record("branch", pc=pc, next_offset=offset, **branch)
        elif kind in ("action", "acknowledged"):
            info = actor_info(args[0])
            if kind == "action":
                info.update(action_context(regs, info.get("vm", 0), read32))
            record(kind, pc=pc, **info)
        elif kind in ("dictionary", "device_start", "device_read"):
            record(kind, pc=pc, args=args)
        m.lib.DebugSetRunState(2)

    reason = "deterministic voice-poll boundary reached"
    try:
        machine.run(rom, step, seconds)
    except Unsupported as exc:
        completed = False
        reason = str(exc)
    finally:
        remove_owned_breakpoints(machine, installed)
        log.close()
    result = dict(
        status="completed" if completed else "inconclusive",
        reason=reason,
        mode=mode,
        owned_breakpoints_removed=len(installed),
        poll_bound=poll_bound,
        hits=hits,
        events=events,
        device_events=[
            json.loads(line)
            for line in (output / "device-events.jsonl").read_text().splitlines()
        ],
        messages=machine.messages,
        core_sha256=hashlib.sha256(Path(core).read_bytes()).hexdigest(),
        plugin_sha256={
            k: hashlib.sha256(Path(v).read_bytes()).hexdigest()
            for k, v in dict(plugins, **{"4": str(plugin)}).items()
        },
        rom_sha256=hashlib.sha256(Path(rom).read_bytes()).hexdigest(),
        limitations=[
            "Dispatch counts are debugger breakpoint hits, not instruction counts.",
            "Controller and VRU device inputs only; no guest memory/register/code mutation.",
            "Dummy audio output; physical speech accuracy and audible fidelity unverified.",
        ],
    )
    (output / "runtime.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    for name in ("rom", "core", "source", "rsp", "output"):
        p.add_argument("--" + name, type=Path, required=True)
    p.add_argument("--gfx", type=Path)
    p.add_argument("--seconds", type=int, default=90)
    p.add_argument("--poll-bound", type=int, default=3300)
    p.add_argument("--mode", choices=["pulse", "none", "adaptive"], default="adaptive")
    p.add_argument("--control", action="store_true")
    a = p.parse_args()
    response = (
        None
        if a.control
        else {
            "num_results": 1,
            "mic_level": 8000,
            "voice_level": 8000,
            "voice_length": 200,
            "matches": [0, 0],
        }
    )
    r = run(
        a.rom,
        a.core,
        a.source,
        a.rsp,
        a.output,
        a.seconds,
        a.mode,
        response,
        a.gfx,
        a.poll_bound,
    )
    print(
        json.dumps(
            {"status": r["status"], "hits": r["hits"], "reason": r["reason"]}, indent=2
        )
    )
