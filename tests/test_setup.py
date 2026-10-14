"""Check that prerequisite failures and check-only setup cannot install packages."""
import os
from pathlib import Path
import shutil
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
BASH = shutil.which('bash')


class SetupTests(unittest.TestCase):
    def run_setup(self, complete):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            tools = root / 'tools'
            tools.mkdir()
            shutil.copy(ROOT / 'tools/setup.sh', tools / 'setup.sh')
            bin_dir = root / 'bin'
            bin_dir.mkdir()
            calls = root / 'unexpected-calls'
            for name in ['brew', 'apt', 'sudo', 'uv', 'git', 'curl', 'tar']:
                tool = bin_dir / name
                tool.write_text('#!' + BASH + '\nprintf "%s\\n" "$0" >> "$SETUP_TEST_CALLS"\nexit 91\n')
                tool.chmod(0o755)
            for name, body in [('dirname', 'echo ' + shlex.quote(str(tools))),
                               ('uname', 'case "$1" in -s) echo Darwin;; -m) echo arm64;; esac'),
                               ('python3', 'exit 0')]:
                tool = bin_dir / name
                tool.write_text('#!' + BASH + '\n' + body + '\n')
                tool.chmod(0o755)
            if complete:
                for name in ['mips-linux-gnu-as', 'mips-linux-gnu-ld', 'mips-linux-gnu-objcopy', 'make', 'shasum']:
                    tool = bin_dir / name
                    tool.write_text('#!' + BASH + '\nexit 0\n')
                    tool.chmod(0o755)
            env = dict(os.environ, PATH=str(bin_dir), SETUP_TEST_CALLS=str(calls))
            result = subprocess.run([BASH, str(tools / 'setup.sh')] + (['--check'] if complete else []),
                                    env=env, capture_output=True, text=True)
            self.assertFalse(calls.exists(), 'setup executed an install/download/environment command')
            self.assertFalse((root / '.venv').exists())
            return result

    def test_missing_dependencies_fail_without_installing(self):
        result = self.run_setup(False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('missing prerequisite: mips-linux-gnu-as', result.stderr)
        self.assertIn('nix develop', result.stderr)

    def test_check_only_does_not_sync_or_download(self):
        result = self.run_setup(True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('no files changed', result.stdout)
