"""Build a pinned Rice graphics plugin for full-system voice experiments."""

from pathlib import Path
from .build_common import run_make
import argparse
import platform
import subprocess

REVISION = "f0a7b9f391b0e9bc14962b114f7da1ba553060be"


def build(directory: Path, core_source: Path) -> Path:
    """Build the pinned upstream plugin using the review shell's native tools."""
    directory = directory.resolve()
    directory.parent.mkdir(parents=True, exist_ok=True)
    if not directory.exists():
        subprocess.run(
            [
                "git",
                "clone",
                "https://github.com/mupen64plus/mupen64plus-video-rice",
                str(directory),
            ],
            check=True,
        )
    subprocess.run(
        ["git", "-C", str(directory), "checkout", "--detach", REVISION], check=True
    )
    options = [
        "APIDIR=" + str((core_source / "src/api").resolve()),
        "CC=clang",
        "CXX=clang++",
    ]
    if platform.machine() == "arm64":
        options.append("HOST_CPU=aarch64")
    run_make(directory / "projects/unix", options)
    return next(
        p
        for p in (directory / "projects/unix").glob("mupen64plus-video-rice.*")
        if p.suffix in (".dylib", ".so")
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--directory", type=Path, default=Path("build/semantic-review/upstream/rice")
    )
    parser.add_argument(
        "--source", type=Path, default=Path("build/semantic-review/upstream/core")
    )
    args = parser.parse_args()
    print(build(args.directory, args.source))
