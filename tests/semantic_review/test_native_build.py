"""Native review helpers must not pollute the strict game object inventory."""

from pathlib import Path
import subprocess
import pytest
from tools.semantic_review.emulator.build_common import run_make


@pytest.mark.parametrize("fails", [False, True])
def test_intermediates_are_removed_even_when_make_fails(tmp_path, monkeypatch, fails):
    paths = []

    def make(command, *, check):
        assert check
        objects = Path(
            next(v.split("=", 1)[1] for v in command if v.startswith("OBJDIR="))
        )
        paths.append(objects)
        (objects / "native.o").write_bytes(b"native host object")
        if fails:
            raise subprocess.CalledProcessError(2, command)

    monkeypatch.setattr(subprocess, "run", make)
    if fails:
        with pytest.raises(subprocess.CalledProcessError):
            run_make(tmp_path, [], tmp_path / "objects")
    else:
        run_make(tmp_path, [], tmp_path / "objects")
    assert paths and all(not path.exists() for path in paths)
    assert not list(tmp_path.rglob("*.o"))
