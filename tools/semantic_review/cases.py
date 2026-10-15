"""Predeclared, assembly-grounded experiments; no retail bytes are stored here.

These cases establish only bounded instruction behavior. They do not establish
hardware behavior, gameplay reachability, or independent R4300 equivalence.
"""

from __future__ import annotations

import hashlib
import struct
from pathlib import Path

from tools.compare_objects import Elf

RAM = 0x80400000
STACK = RAM + 0xE000
STOP = RAM + 0xF000


def word(value):
    return struct.pack(">I", value & 0xFFFFFFFF)


def inventory():
    """Return executable claims and explicit outstanding review boundaries."""
    return [
        {
            "id": "voice.result",
            "claim": "Only pending==1 supplies an unsigned first answer; consumption clears pending even with null output.",
            "evidence": "Ghidra original instructions 80040BA0..80040C6C: bne against1; lhu +0xA; sw zero pending; copy30 bytes.",
            "limitation": "Interrupt-mask calls require full-system execution.",
        },
        {
            "id": "mailbox.post",
            "claim": "Nonzero pending rejects an unforced post; any nonzero force replaces six copied words.",
            "evidence": "Ghidra original instructions 80118E74..80118EE4: state +0x54; pending +0x218; six words +0x21C.",
        },
        {
            "id": "mailbox.ack",
            "claim": "Acknowledgement returns the previous pending word, clears it, and preserves payload.",
            "evidence": "ROM instructions 80119200..8011920C: lw v0; jr ra; sw zero delay slot.",
        },
        {
            "id": "vm.return",
            "claim": "Positive return depth restores the last saved offset; zero or negative depth leaves the VM unchanged.",
            "evidence": "ROM instructions 801153F4..80115418: blez; decrement; indexed load +0x12C.",
        },
        {
            "id": "value.write",
            "claim": "Integer and borrowed string setters write a tag and exactly four payload bytes.",
            "evidence": "ROM instructions 80115544..801155AC: sb tag0/4; bcopy length4.",
        },
        {
            "id": "pool.free",
            "claim": "Pool availability normalizes a free-list pointer to zero or one.",
            "evidence": "ROM instructions 8001C0FC/80041C04: lw free; sltu zero,pointer.",
        },
        *[
            {"id": name, "claim": claim, "status": "inconclusive", "limitation": why}
            for name, claim, why in [
                (
                    "voice.device",
                    "Dictionary batching, device states, recognition delivery and quality meanings.",
                    "Startup and one opening command are exercised separately; physical device fidelity, other dictionaries and quality meanings remain unverified.",
                ),
                (
                    "vm.dispatch",
                    "Opcode/control flow, numeric conversion, native outcomes and action lifetime.",
                    "One opening message branch/action is exercised separately; complete opcode/FPU behavior, native outcomes and action lifetimes remain unverified.",
                ),
                (
                    "actor.ownership",
                    "Hierarchy, handles and destruction preserve documented ownership.",
                    "Requires allocation/free and callback traces across scenes.",
                ),
                (
                    "item.physics",
                    "Item ownership, collision and physics units match documented meanings.",
                    "Requires producer/consumer runtime traces and representative interactions.",
                ),
                (
                    "scene.resources",
                    "Overlay and borrowed-resource lifetimes remain valid across transitions.",
                    "Requires overlay-aware full-system capture.",
                ),
                (
                    "save.persistence",
                    "Save fields, bounds, backup records and asynchronous writes obey contracts.",
                    "Requires EEPROM event traces and readback scenarios.",
                ),
                (
                    "presentation",
                    "Animation, rendering and audio behavior agrees with recovered meanings.",
                    "Requires visual/audio observations beyond instruction equivalence.",
                ),
            ]
        ],
    ]


def experiments():
    """Build synthetic initial states and expected observations before replay."""
    cases = []

    def add(name, symbol, args, patches, observations, expected_return=None):
        cases.append(
            dict(
                id=name,
                symbol=symbol,
                args=args,
                patches=patches,
                observations=observations,
                expected_return=expected_return,
            )
        )

    for symbol, offset in [
        ("nodePoolHasFreeEntry", 0x10),
        ("keyedPoolHasAvailableEntry", 4),
    ]:
        for pointer in (0, RAM + 0x100, 0xFFFFFFFF):
            data = bytearray(32)
            data[offset : offset + 4] = word(pointer)
            add(
                f"{symbol}.{pointer:x}",
                symbol,
                [RAM],
                [(RAM, bytes(data))],
                [(RAM, bytes(data))],
                int(pointer != 0),
            )
    for depth in (0, -1, 1, 3):
        data = bytearray(b"\xa5" * 0x140)
        data[0x14:0x18] = word(0x1234)
        data[0x128:0x12C] = word(depth)
        for i in range(3):
            data[0x12C + i * 4 : 0x130 + i * 4] = word(0x400 + i * 8)
        expected = bytearray(data)
        if depth > 0:
            expected[0x128:0x12C] = word(depth - 1)
            expected[0x14:0x18] = word(0x400 + (depth - 1) * 8)
        add(
            f"vm.return.{depth}",
            "scriptVmRestoreReturnLocation",
            [RAM],
            [(RAM, bytes(data))],
            [(RAM, bytes(expected))],
        )
    # Exercise first and last slots from the documented 64-entry allocation;
    # these slices test access behavior, not allocation provenance.
    for symbol, count_offset, saved_offset, value_offset in [
        ("scriptVmSaveStackDepth", 0x24, 0x28, 0x18),
        ("scriptVmSaveReturnLocation", 0x128, 0x12C, 0x14),
    ]:
        for count in (0, 63):
            data = bytearray(b"\xa5" * 0x230)
            data[count_offset : count_offset + 4] = word(count)
            data[value_offset : value_offset + 4] = word(0x12345678)
            expected = bytearray(data)
            expected[count_offset : count_offset + 4] = word(count + 1)
            expected[saved_offset + count * 4 : saved_offset + count * 4 + 4] = word(
                0x12345678
            )
            add(
                f"{symbol}.{count}",
                symbol,
                [RAM],
                [(RAM, bytes(data))],
                [(RAM, bytes(expected))],
            )
    for count in (1, 64):
        data = bytearray(b"\xa5" * 0x230)
        data[0x24:0x28] = word(count)
        data[0x28 + (count - 1) * 4 : 0x2C + (count - 1) * 4] = word(0x22)
        expected = bytearray(data)
        expected[0x24:0x28] = word(count - 1)
        expected[0x18:0x1C] = word(0x22)
        add(
            f"vm.restore_depth.{count}",
            "scriptVmRestoreStackDepth",
            [RAM],
            [(RAM, bytes(data))],
            [(RAM, bytes(expected))],
        )
    for pending, force in [(0, 0), (1, 0), (7, 0), (1, 1), (7, -1)]:
        actor = bytearray(0x60)
        actor[0x54:0x58] = word(RAM + 0x1000)
        state = bytearray(b"\xa5" * 0x240)
        state[0x218:0x21C] = word(pending)
        message = b"".join(word(x) for x in [0xE, 0xFFFFFFFF, 0x80000000, 3, 4, 5])
        expected = bytearray(state)
        accepted = pending == 0 or force != 0
        if accepted:
            expected[0x218:0x234] = word(1) + message
        add(
            f"mailbox.post.{pending}.{force}",
            "actorPostScriptMessage",
            [RAM, 0xE, 0xFFFFFFFF, 0x80000000],
            [
                (RAM, bytes(actor)),
                (RAM + 0x1000, bytes(state)),
                (STACK + 16, message[12:] + word(force)),
            ],
            [(RAM + 0x1000, bytes(expected))],
            int(accepted),
        )
    for pending in (0, 1, 7):
        actor = bytearray(0x60)
        actor[0x54:0x58] = word(RAM + 0x1000)
        state = bytearray(b"\xa5" * 0x240)
        state[0x218:0x21C] = word(pending)
        expected = bytearray(state)
        expected[0x218:0x21C] = word(0)
        add(
            f"mailbox.ack.{pending}",
            "actorAcknowledgeScriptMessage",
            [RAM],
            [(RAM, bytes(actor)), (RAM + 0x1000, bytes(state))],
            [(RAM + 0x1000, bytes(expected))],
            pending,
        )
    for symbol, tag in [
        ("scriptValueSetInteger", 0),
        ("scriptValueSetStringPointer", 4),
    ]:
        for number in (0, 0xFFFFFFFF, 0x80000123):
            add(
                f"{symbol}.{number:x}",
                symbol,
                [RAM + 1, number],
                [(RAM, b"\xa5" * 8)],
                [(RAM, b"\xa5" + bytes([tag]) + word(number) + b"\xa5" * 2)],
            )
    for operation, depth in [("push", 0), ("push", 3), ("pop", 1), ("pop", 4)]:
        vm = bytearray(b"\xa5" * 0x40)
        vm[0x18:0x1C] = word(depth)
        vm[0x20:0x24] = word(RAM + 0x2000)
        values = bytearray(b"\xa5" * 32)
        box = b"\x04\xfe\xdc\xba\x98"
        dest = b"\xa5" * 7
        expected_vm = bytearray(vm)
        expected_values = bytearray(values)
        if operation == "push":
            expected_vm[0x18:0x1C] = word(depth + 1)
            expected_values[depth * 5 : depth * 5 + 5] = box
            source = b"\xa5" + box + b"\xa5"
            expected_dest = source
        else:
            expected_vm[0x18:0x1C] = word(depth - 1)
            values[(depth - 1) * 5 : (depth - 1) * 5 + 5] = box
            expected_values = bytearray(values)
            source = dest
            expected_dest = b"\xa5" + box + b"\xa5"
        add(
            f"vm.value.{operation}.{depth}",
            "scriptVmPushValue" if operation == "push" else "scriptVmPopValue",
            [RAM, RAM + 0x3001],
            [(RAM, bytes(vm)), (RAM + 0x2000, bytes(values)), (RAM + 0x3000, source)],
            [
                (RAM, bytes(expected_vm)),
                (RAM + 0x2000, bytes(expected_values)),
                (RAM + 0x3000, expected_dest),
            ],
        )
    for selector in (0, 1, 0xFFFFFFFF):
        actor = bytearray(0x60)
        actor[0x54:0x58] = word(RAM + 0x1000)
        state = bytearray(0x200)
        state[0x1B8:0x1BC] = word(RAM + 0x2000)
        vm = bytes(b"\xa5" * 0x300)
        expected = bytearray(vm)
        expected[0x2C0:0x2C8] = word(selector) + word(1)
        add(
            f"action.begin.{selector:x}",
            "actorBeginScriptActionHandler",
            [RAM, selector],
            [(RAM, bytes(actor)), (RAM + 0x1000, bytes(state)), (RAM + 0x2000, vm)],
            [(RAM + 0x2000, bytes(expected))],
        )
    for program in (b"\x23", b"\x00\x01\x02\x23", b"\xff", b"\x24"):
        actor = bytearray(0x60)
        actor[0x54:0x58] = word(RAM + 0x1000)
        state = bytearray(0x200)
        state[0x1B8:0x1BC] = word(RAM + 0x2000)
        vm = bytearray(b"\xa5" * 0x300)
        vm[4:8] = word(RAM + 0x3000)
        vm[0x14:0x18] = word(0)
        expected = bytearray(vm)
        finished = program[-1] == 35
        if finished:
            for offset in (0x14, 0x18, 0x24, 0x128):
                expected[offset : offset + 4] = word(0)
        else:
            expected[0x14:0x18] = word(1)
        add(
            "vm.opcodes." + program.hex(),
            "actorRunScriptVm",
            [RAM],
            [
                (RAM, bytes(actor)),
                (RAM + 0x1000, bytes(state)),
                (RAM + 0x2000, bytes(vm)),
                (RAM + 0x3000, program),
            ],
            [(RAM + 0x2000, bytes(expected))],
            4 if finished else 5,
        )
    for program in (b"\x18\x00\x12\x34\x56\x78\xff", b"\x1f\xff"):
        actor = bytearray(0x60)
        actor[0x54:0x58] = word(RAM + 0x1000)
        state = bytearray(0x200)
        state[0x1B8:0x1BC] = word(RAM + 0x2000)
        vm = bytearray(b"\xa5" * 0x300)
        vm[4:8] = word(RAM + 0x3000)
        vm[0x14:0x18] = word(0)
        vm[0x18:0x1C] = word(0 if program[0] == 24 else 1)
        vm[0x20:0x24] = word(RAM + 0x4000)
        values = b"\xa5" * 16
        expected = bytearray(vm)
        expected[0x14:0x18] = word(len(program))
        expected[0x18:0x1C] = word(1 if program[0] == 24 else 0)
        expected_values = program[1:6] + values[5:] if program[0] == 24 else values
        add(
            "vm.opcodes." + program.hex(),
            "actorRunScriptVm",
            [RAM],
            [
                (RAM, bytes(actor)),
                (RAM + 0x1000, bytes(state)),
                (RAM + 0x2000, bytes(vm)),
                (RAM + 0x3000, program),
                (RAM + 0x4000, values),
            ],
            [(RAM + 0x2000, bytes(expected)), (RAM + 0x4000, expected_values)],
            5,
        )
    for pending, output in [
        (0, RAM + 0x100),
        (1, RAM + 0x100),
        (1, 0),
        (2, RAM + 0x100),
    ]:
        result = bytearray(range(30))
        result[10:12] = b"\xfe\xdc"
        add(
            f"voice.consume.{pending}.{int(output != 0)}",
            "voiceRecognitionTakeResult",
            [output],
            [(0x800F8AD4, word(pending) + bytes(result)), (RAM + 0x100, b"\xa5" * 32)],
            [
                (0x800F8AD4, word(0)),
                (
                    RAM + 0x100,
                    (
                        (bytes(result) + b"\xa5" * 2)
                        if pending == 1 and output
                        else b"\xa5" * 32
                    ),
                ),
            ],
            0xFEDC if pending == 1 else -1,
        )
    from .numeric_domains import experiments as numeric_experiments

    from .participation_domains import experiments as participation_experiments

    from .vm_control_domains import experiments as control_experiments
    from .item_numeric_domains import experiments as item_experiments
    from .system_numeric_domains import experiments as system_experiments
    from .progress_numeric_domains import experiments as progress_experiments
    from .action_cue_numeric_domains import experiments as cue_experiments

    return (
        cases
        + numeric_experiments()
        + participation_experiments()
        + control_experiments()
        + item_experiments()
        + system_experiments()
        + progress_experiments()
        + cue_experiments()
    )


def bounded_slice(address, size, offsets):
    """Validate an optional CPU-slice boundary inside one original function."""
    if set(offsets) != {"entry_offset", "stop_offset"}:
        raise ValueError("slice requires explicit entry and stop offsets")
    entry, stop = offsets["entry_offset"], offsets["stop_offset"]
    if (
        type(entry) is not int
        or type(stop) is not int
        or entry % 4
        or stop % 4
        or not 0 <= entry < stop < size
    ):
        raise ValueError("slice boundaries must be aligned and inside the function")
    return address + entry, address + stop


def capture_case(case, elf_path, rom_path):
    """Load locally authenticated retail sections and synthetic writable state."""
    elf = Elf(Path(elf_path))
    rom = Path(rom_path).read_bytes()
    symbols = {
        name: value
        for i, s in enumerate(elf.sections)
        if s[1] == 2
        for name, value, *_ in elf.symbols(i)
    }
    start_pc, stop_pc = symbols[case["symbol"]], STOP
    if "slice" in case:
        sizes = {
            name: size
            for i, section in enumerate(elf.sections)
            if section[1] == 2
            for name, _, size, *_ in elf.symbols(i)
        }
        start_pc, stop_pc = bounded_slice(
            start_pc, sizes[case["symbol"]], case["slice"]
        )
    mappings = []
    authenticated = []
    # Retail resident and shared-overlay sections have disjoint virtual ranges.
    for name in (".boot", ".ovl"):
        i = elf.names.index(name)
        section = elf.sections[i]
        address = section[3]
        raw = elf.data(i)
        offset = symbols[name[1:] + "_ROM_START"]
        if rom[offset : offset + len(raw)] != raw:
            raise ValueError(f"{name}: linked section differs from original ROM")
        start = address & ~4095
        size = ((address - start + len(raw) + 4095) // 4096) * 4096
        mappings.append(
            dict(
                address=start,
                size=size,
                permissions="rwx",
                data_hex=(b"\0" * (address - start) + raw).hex(),
            )
        )
        authenticated.append(
            dict(
                address=address,
                size=len(raw),
                rom_offset=offset,
                sha256=hashlib.sha256(raw).hexdigest(),
                overlay=name,
            )
        )
    ram = bytearray(0x10000)
    for address, data in case["patches"]:
        if RAM <= address and address + len(data) <= RAM + len(ram):
            ram[address - RAM : address - RAM + len(data)] = data
            continue
        for region in mappings:
            if (
                region["address"] <= address
                and address + len(data) <= region["address"] + region["size"]
            ):
                blob = bytearray.fromhex(region["data_hex"])
                offset = address - region["address"]
                if len(blob) < offset + len(data):
                    blob.extend(b"\0" * (offset + len(data) - len(blob)))
                blob[offset : offset + len(data)] = data
                region["data_hex"] = blob.hex()
                break
        else:
            start = address & ~4095
            blob = bytearray(4096)
            blob[address - start : address - start + len(data)] = data
            mappings.append(
                dict(address=start, size=4096, permissions="rw", data_hex=blob.hex())
            )
    mappings.append(
        dict(address=RAM, size=len(ram), permissions="rwx", data_hex=ram.hex())
    )
    regs = {"gpr_29": STACK | 0xFFFFFFFF00000000, "gpr_31": STOP | 0xFFFFFFFF00000000}
    regs.update(
        {
            f"gpr_{4 + i}": (value & 0xFFFFFFFF)
            | (0xFFFFFFFF00000000 if value & 0x80000000 else 0)
            for i, value in enumerate(case["args"])
        }
    )
    return dict(
        schema_version=1,
        byte_order="big",
        rom_sha256=hashlib.sha256(rom).hexdigest(),
        start_pc=start_pc,
        stop_pc=stop_pc,
        registers=regs,
        mappings=mappings,
        authenticated_code=authenticated,
        max_instructions=10000,
        observations=[dict(address=a, size=len(b)) for a, b in case["observations"]],
    )


def run_cases(rom_path, elf_path):
    """Classify bounded observations; completion alone is never a semantic pass."""
    from tools.semantic_review.replay import replay

    results = []
    for case in experiments():
        observed = replay(capture_case(case, elf_path, rom_path), Path(rom_path))
        status = "unsupported"
        reason = observed["reason"]
        if observed["status"] == "completed":
            memory = {
                x["address"]: bytes.fromhex(x["data_hex"])
                for x in observed.get("memory", [])
            }
            correct = all(memory.get(a) == b for a, b in case["observations"])
            if case["expected_return"] is not None:
                correct &= (
                    observed["registers"]["gpr_2"] & 0xFFFFFFFF
                    == case["expected_return"] & 0xFFFFFFFF
                )
            status = "supported" if correct else "contradicted"
            reason = (
                "Predeclared state/return observations agree."
                if correct
                else "Predeclared state/return observation differs."
            )
        results.append(
            dict(
                id=case["id"],
                subsystem=("pools" if "Pool" in case["symbol"] else "voice-vm"),
                status=status,
                claim=case["symbol"] + " bounded return and write footprint",
                evidence="ROM-authenticated instructions; expected values predeclared from disassembly.",
                reason=reason,
                instructions=observed["instructions"],
                scope=case.get("scope", "function entry"),
                limitations=case.get("limitations", [])
                + observed.get("limitations", [])
                + [
                    "Synthetic initial state; gameplay reachability and R4300 cross-validation unverified."
                ],
                observations=[
                    {"address": a, "expected_hex": b.hex()}
                    for a, b in case["observations"]
                ],
            )
        )
    results.extend(
        dict(
            item,
            subsystem="voice-vm",
            status=item.get("status", "inconclusive"),
            observations=[],
            limitations=[
                item.get("limitation", "Not yet exercised as an integration contract.")
            ],
        )
        for item in inventory()
    )
    return results
