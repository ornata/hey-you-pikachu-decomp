"""Build the pinned debugger core locally without installing host packages."""

from pathlib import Path
import argparse
import hashlib
import json
import subprocess
from .mupen import PINNED_REVISION
from .build_common import run_make


def build(directory: Path) -> Path:
    """Fetch the reviewed revision and build its interpreter/debugger API."""
    directory = directory.resolve()
    directory.parent.mkdir(parents=True, exist_ok=True)
    if not directory.exists():
        subprocess.run(
            [
                "git",
                "clone",
                "https://github.com/mupen64plus/mupen64plus-core",
                str(directory),
            ],
            check=True,
        )
    subprocess.run(
        ["git", "-C", str(directory), "checkout", "--detach", PINNED_REVISION],
        check=True,
    )
    run_make(
        directory / "projects/unix",
        [
            "DEBUGGER=1",
            "DEBUGGER_NO_DISASM=1",
            "OSD=0",
            "NO_ASM=1",
            "NETPLAY=0",
        ],
    )
    candidates = list((directory / "projects/unix").glob("libmupen64plus.*"))
    library = next(
        p for p in candidates if p.suffix in (".dylib", ".so") or ".so." in p.name
    )
    (library.parent / "semantic-build.json").write_text(
        json.dumps(
            {
                "revision": PINNED_REVISION,
                "core_sha256": hashlib.sha256(library.read_bytes()).hexdigest(),
                "flags": [
                    "DEBUGGER=1",
                    "DEBUGGER_NO_DISASM=1",
                    "OSD=0",
                    "NO_ASM=1",
                    "NETPLAY=0",
                ],
            },
            indent=2,
        )
        + "\n"
    )
    return library


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--directory", type=Path, default=Path("build/semantic-review/upstream/core")
    )
    args = parser.parse_args()
    print(build(args.directory))
