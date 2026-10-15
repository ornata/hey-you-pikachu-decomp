"""Build a pinned HLE RSP plugin locally for deterministic startup experiments."""

from pathlib import Path
from .build_common import run_make
import argparse
import subprocess
import platform

REVISION = "8a7a472a7172eb2c8725b305eae26818ed7b51a2"


def build(directory: Path, core_source: Path) -> Path:
    directory = directory.resolve()
    directory.parent.mkdir(parents=True, exist_ok=True)
    if not directory.exists():
        subprocess.run(
            [
                "git",
                "clone",
                "https://github.com/mupen64plus/mupen64plus-rsp-hle",
                str(directory),
            ],
            check=True,
        )
    subprocess.run(
        ["git", "-C", str(directory), "checkout", "--detach", REVISION], check=True
    )
    options = ["APIDIR=" + str((core_source / "src/api").resolve()), "NO_ASM=1"]
    if platform.machine() == "arm64":
        options.append("HOST_CPU=aarch64")
    run_make(directory / "projects/unix", options)
    return next(
        path
        for path in (directory / "projects/unix").glob("mupen64plus-rsp-hle.*")
        if path.suffix in (".dylib", ".so")
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--directory", type=Path, default=Path("build/semantic-review/upstream/rsp")
    )
    parser.add_argument(
        "--source", type=Path, default=Path("build/semantic-review/upstream/core")
    )
    args = parser.parse_args()
    print(build(args.directory, args.source))
