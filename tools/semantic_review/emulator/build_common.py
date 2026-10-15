"""Keep native compiler intermediates separate from matching game objects."""

from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]


def run_make(project: Path, options: list[str], objects_root: Path | None = None):
    """Build a native helper and discard its temporary object tree on exit.

    Libraries remain in the upstream build directory. Game verification continues
    to compare every retained object under build/ without native ELF exceptions.
    """
    objects_root = objects_root or ROOT / "build/semantic-review/native-objects"
    objects_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="objects-", dir=objects_root) as objects:
        subprocess.run(
            ["make", "-C", str(project), "all", "-j8", "OBJDIR=" + objects, *options],
            check=True,
        )
