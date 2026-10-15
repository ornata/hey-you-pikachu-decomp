"""Repeat the retail startup dictionary scenario and compare normalized evidence."""

from pathlib import Path
import argparse
import hashlib
import json
from ..capture import capture
from .input import build


def run(rom: Path, core: Path, source: Path, rsp: Path, output: Path) -> dict:
    """Verify three startup captures reach the same voice/device checkpoint."""
    scenario = json.loads(
        (
            Path(__file__).resolve().parents[3]
            / "tests/semantic_review/voice-startup.json"
        ).read_text()
    )
    output = Path(output)
    plugin = build(
        source,
        output
        / ("input.dylib" if __import__("sys").platform == "darwin" else "input.so"),
    )
    scenario.update(
        data_path=str(Path(source) / "data"),
        input_plugin=str(plugin),
        plugins={"1": str(rsp)},
    )
    captures = [capture(scenario, output / f"run-{i}", core, rom) for i in range(3)]
    result = assess(captures, scenario["expected"])
    result["limitations"] = scenario["limitations"] + result["limitations"]
    (output / "integration.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


def assess(captures: list[dict], expected: dict) -> dict:
    """Classify observations independently of emulator capture success."""
    normalized = [
        dict(
            checkpoints=c["checkpoints"],
            device_events=c["device_events"],
            traces=c["traces"],
            dispatches=c["dispatches"],
        )
        for c in captures
    ]
    identical = all(n == normalized[0] for n in normalized[1:])
    events = captures[0]["device_events"]
    clear = [e for e in events if e["event"] == "dictionary_clear"]
    words = [e for e in events if e["event"] == "dictionary_word"]
    agreed = (
        len(clear) == expected["dictionary_clear_count"]
        and len(words) == expected["dictionary_words"]
        and clear[0]["a"] == len(words)
    )
    batches = [t["registers"]["gpr_5"] for t in captures[0]["traces"]]
    cursors = [
        int.from_bytes(bytes.fromhex(t["memory"][0]["data_hex"])[:4], "big")
        for t in captures[0]["traces"]
    ]
    event_counts = [t["dictionary_words"] for t in captures[0]["traces"]]
    batching = (
        bool(batches)
        and all(b == expected["batch_size"] for b in batches)
        and event_counts == cursors
        and all(
            0 <= b - a <= expected["batch_size"] for a, b in zip(cursors, cursors[1:])
        )
    )
    final = (
        bytes.fromhex(captures[0]["checkpoints"][0]["memory"][0]["data_hex"])
        if captures[0]["checkpoints"]
        else bytes(12)
    )
    final_cursor = int.from_bytes(final[:4], "big")
    between_uploads = int.from_bytes(final[4:8], "big")
    batching &= (
        bool(cursors)
        and final_cursor == expected["final_cursor"]
        and between_uploads == expected["between_uploads"]
        and 0 <= final_cursor - cursors[-1] <= expected["batch_size"]
    )
    supported = (
        identical
        and agreed
        and batching
        and all(c["status"] == "completed" for c in captures)
    )
    result = {
        "status": "supported"
        if supported
        else "contradicted"
        if all(c["status"] == "completed" for c in captures)
        else "inconclusive",
        "claim": "Original-ROM startup uploads the declared dictionary before the first recognition poll, with repeatable CPU/device observations.",
        "repeats": 3,
        "normalized_identical": identical,
        "dictionary_clear_count": len(clear),
        "dictionary_words": len(words),
        "dispatches": captures[0]["dispatches"],
        "batch_sizes": batches,
        "entry_cursors": cursors,
        "entry_word_counts": event_counts,
        "final_cursor": final_cursor,
        "between_uploads": between_uploads,
        "normalized_sha256": hashlib.sha256(
            json.dumps(normalized[0], sort_keys=True).encode()
        ).hexdigest(),
        "core_sha256": captures[0]["core_sha256"],
        "plugin_sha256": captures[0]["plugin_sha256"],
        "limitations": captures[0]["limitations"],
    }
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for arg in ("rom", "core", "source", "rsp"):
        parser.add_argument("--" + arg, type=Path, required=True)
    parser.add_argument(
        "--output", type=Path, default=Path("build/semantic-review/voice-startup")
    )
    args = parser.parse_args()
    result = run(args.rom, args.core, args.source, args.rsp, args.output)
    print(json.dumps(result, indent=2))
    raise SystemExit(
        0
        if result["status"] == "supported"
        else 1
        if result["status"] == "contradicted"
        else 2
    )
