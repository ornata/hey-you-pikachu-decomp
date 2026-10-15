"""Predeclared EEPROM, scene and audio observations with explicit CPU-slice limits."""

import struct


def experiments():
    """Exercise state predicates, retry thresholds, wrapping, panning and storage widths."""
    from .cases import RAM, word

    cases = []

    def add(name, symbol, args, patches, observations, result=None, **metadata):
        cases.append(
            dict(
                id="numeric.system." + name,
                symbol=symbol,
                args=args,
                patches=patches,
                observations=observations,
                expected_return=result,
                **metadata,
            )
        )

    # Original800039B4/800039EC: only probe type1 exposes the state; 0/3/4/5 are terminal.
    for device in [0, 1, 2]:
        for state in range(7):
            job = word(state) + b"\xa5" * (0x21C - 4)
            patches = [(0x800AD1E0, job), (0x800AD3FC, word(device))]
            visible = state if device == 1 else 0
            add(
                f"eeprom.state.{device}.{state}",
                "eepromGetTransferState",
                [],
                patches,
                patches,
                visible,
            )
            busy = int(visible not in [0, 3, 4, 5])
            add(
                f"eeprom.busy.{device}.{state}",
                "eepromIsTransferBusy",
                [],
                patches,
                patches,
                busy,
            )

    # Ghidra80003488/80003590: the selected failure branch increments retries and compares with11.
    # Stop before the epilogue; no stack prologue, SDK I/O or physical failure is simulated.
    for symbol, pending, failed, stop in [
        ("eepromReadNextBlock", 1, 3, 0xF8),
        ("eepromWriteNextBlock", 2, 5, 0xC8),
    ]:
        for prior, expected in [(9, pending), (10, failed), (11, failed)]:
            job = bytearray(b"\xa5" * 0x21C)
            job[:8] = word(pending) + word(prior)
            wanted = bytearray(job)
            wanted[4:8] = word(prior + 1)
            add(
                f"eeprom.failure.{pending}.{prior}",
                symbol,
                [],
                [(0x800AD1E0, bytes(job))],
                [(0x800AD1E0, bytes(wanted))],
                expected,
                slice=dict(entry_offset=0x50, stop_offset=stop),
                scope="original failure-branch CPU slice",
                limitations=[
                    "Enters a preselected failure branch; does not establish EEPROM I/O, retry timing, or physical device behavior."
                ],
            )

    # Original8010E598: repeated signed adjustments by23; spawn is retained unchanged.
    for area, expected in [
        (-24, 22),
        (-23, 0),
        (-1, 22),
        (0, 0),
        (22, 22),
        (23, 0),
        (46, 0),
    ]:
        before = b"\xa5" * 4 + word(99) + word(99) + b"\xa5" * 4
        after = b"\xa5" * 4 + word(0) + word(expected) + b"\xa5" * 4
        add(
            f"scene.wrap.{area}",
            "areaSetNextDestination",
            [area, 0],
            [(0x800AE4F8, before)],
            [(0x800AE4F8, after)],
        )

    # Ghidra80005E14: byte mode1 centers; all other bytes use/clamp the channel/sample sum.
    for mode, channel, sample, expected in [
        (1, 0, 0, 64),
        (1, 127, 127, 64),
        (2, 0, 0, 0),
        (2, 127, 127, 127),
        (2, 64, 64, 64),
        (2, 63, 64, 63),
        (2, 127, 0, 63),
        (0, 63, 64, 63),
        (3, 63, 64, 63),
    ]:
        driver = bytearray(b"\xa5" * 0x28)
        driver[0x1C] = mode
        sequence = bytearray(b"\xa5" * 0x200)
        sequence[0x1C:0x1E] = struct.pack(">H", channel << 7)
        voice = bytearray(b"\xa5" * 0x50)
        voice[0x10:0x14] = word(RAM + 0x500)
        voice[0x30] = 0
        sound = bytearray(b"\xa5" * 0x20)
        sound[0xC] = sample
        patches = [
            (RAM, bytes(driver)),
            (RAM + 0x100, bytes(sequence)),
            (RAM + 0x400, bytes(voice)),
            (RAM + 0x500, bytes(sound)),
        ]
        add(
            f"audio.pan.{mode}.{channel}.{sample}",
            "sequenceEngineGetVoicePan",
            [RAM, RAM + 0x100, RAM + 0x400],
            patches,
            patches,
            expected,
        )

    # Original8001FA28: global/FX modes retain a word; the sequence driver retains one byte.
    # Empty players avoid voice-mixing calls. Invalid modes are forwarded, not normalized.
    for mode in [0, 1, 2, 3, 258]:
        player = bytes(0x28)
        wanted_player = bytearray(player)
        wanted_player[0x1C:0x20] = word(mode)
        driver = bytes(0x28)
        wanted_driver = bytearray(driver)
        wanted_driver[0x1C] = mode & 255
        patches = [
            (RAM, player),
            (0x800EC960, word(RAM)),
            (0x800EC9C8, driver),
            (0x80088EE8, word(99)),
        ]
        observations = [
            (RAM, bytes(wanted_player)),
            (0x800EC960, word(RAM)),
            (0x800EC9C8, bytes(wanted_driver)),
            (0x80088EE8, word(mode)),
        ]
        add(
            f"audio.forward.{mode}",
            "audioSetOutputMode",
            [mode],
            patches,
            observations,
            limitations=[
                "Synthetic empty players; no claim about audible output or active voice refresh."
            ],
        )
    return cases
