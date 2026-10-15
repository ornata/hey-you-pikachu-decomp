"""Compile and configure project-local deterministic Mupen input plugin."""

import ctypes as C
import subprocess
import sys
from pathlib import Path


def build(source: Path, output: Path) -> Path:
    output = Path(output).resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    flags = ["-dynamiclib"] if sys.platform == "darwin" else ["-shared", "-fPIC"]
    subprocess.run(
        [
            "cc",
            *flags,
            "-I",
            str(Path(source) / "src/api"),
            str(Path(__file__).with_name("input.c")),
            "-o",
            str(output),
        ],
        check=True,
    )
    return output


def configure(path: Path, scenario: dict, log: Path):
    """Configure poll-indexed keys and result-read-indexed VRU responses."""
    lib = C.CDLL(str(Path(path).resolve()))
    polls = scenario.get("controller", [])
    if any(not isinstance(v, int) or not 0 <= v <= 0xFFFFFFFF for v in polls):
        raise ValueError("controller values must be unsigned 32-bit BUTTONS values")
    words = []
    for result in scenario.get("vru", {}).get("results", []):
        matches = result.get("matches", [])
        if len(matches) > 10:
            raise ValueError(
                "VRU supports at most ten match words (five index/score pairs)"
            )
        row = (
            [
                result.get(k, 0)
                for k in (
                    "error_flags",
                    "num_results",
                    "mic_level",
                    "voice_level",
                    "voice_length",
                )
            ]
            + matches
            + [0] * (10 - len(matches))
        )
        if (
            any(not isinstance(v, int) or not 0 <= v <= 65535 for v in row)
            or row[1] > 5
        ):
            raise ValueError("VRU result fields must fit uint16 and num_results <= 5")
        words.extend(row)
    lib.ConfigureInput.argtypes = [
        C.POINTER(C.c_uint32),
        C.c_uint,
        C.POINTER(C.c_uint16),
        C.c_uint,
        C.c_int,
        C.c_char_p,
    ]
    lib.ConfigureInput.restype = C.c_int
    rc = lib.ConfigureInput(
        (C.c_uint32 * len(polls))(*polls),
        len(polls),
        (C.c_uint16 * len(words))(*words),
        len(words) // 15,
        scenario.get("vru", {}).get("present", True),
        str(Path(log).resolve()).encode(),
    )
    if rc:
        raise RuntimeError(f"input plugin configuration failed ({rc})")
    speech_polls = scenario.get("vru", {}).get("talk_polls", 0)
    if not isinstance(speech_polls, int) or not 0 <= speech_polls <= 65536:
        raise ValueError("talk_polls must be in 0..65536")
    lib.ConfigureVRUTalking.argtypes = [C.c_uint]
    lib.ConfigureVRUTalking(speech_polls)
    return lib
