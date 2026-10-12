import struct
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from header_deps import dependencies, escape, write_depfile
from compare_objects import compare, normalized


def elf_fixture(path, text=b'\x00' * 4, bss=8, debug=b'old', external='callee', address=None,
                relocation=2):
    """Make small MIPS REL/EXEC fixtures independently of the cross toolchain."""
    names = ['', '.text', '.bss', '.strtab', '.symtab', '.rel.text', '.shstrtab', '.debug']
    shstrings = b'\0'
    name_offsets = []
    for name in names:
        name_offsets.append(len(shstrings))
        shstrings += name.encode() + b'\0'
    strings = b'\0function\0' + external.encode() + b'\0'
    symbols = bytes(16) + struct.pack('>IIIBBH', 1, 0, 4, 0x12, 0, 1)
    symbols += struct.pack('>IIIBBH', 10, address or 0, 0, 0x10, 0, 1 if address is not None else 0)
    payloads = [b'', text, b'', strings, symbols, struct.pack('>II', 0, (2 << 8) | relocation), shstrings, debug]
    types = [0, 1, 8, 3, 2, 9, 3, 1]
    blob = bytearray(52)
    sections = []
    for i, payload in enumerate(payloads):
        offset = len(blob)
        blob.extend(payload)
        sections.append((name_offsets[i], types[i], 2 if i in (1, 2) else 0, 0, offset,
                         bss if i == 2 else len(payload), 3 if i == 4 else 4 if i == 5 else 0,
                         1 if i == 5 else 0, 4 if i in (1, 2) else 1, 16 if i == 4 else 8 if i == 5 else 0))
    shoff = len(blob)
    blob.extend(b''.join(struct.pack('>10I', *s) for s in sections))
    struct.pack_into('>16sHHIIIIIHHHHHH', blob, 0, b'\x7fELF\x01\x02\x01' + bytes(9),
                     2 if address is not None else 1, 8, 1, 0, 0, shoff, 0, 52, 0, 0, 40, len(sections), 6)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(blob)


class DependencyTests(unittest.TestCase):
    def test_nested_conditional_cycle_and_assembly_inputs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'unit.c').write_text('#include "first.h"\n#pragma GLOBAL_ASM("fallback.s")\n')
            (root / 'first.h').write_text('#if 0\n#include "second.h"\n#endif\n/* #include "fake.h" */\n')
            (root / 'second.h').write_text('#include "first.h"\n')
            (root / 'fallback.s').write_text('.include "macros.inc"\n.incbin "data.bin"\n')
            (root / 'macros.inc').write_text('')
            (root / 'data.bin').write_bytes(b'\xff')
            self.assertEqual(set(dependencies(root / 'unit.c', [root])),
                             {p.resolve() for p in root.iterdir()})

    def test_optional_missing_header_watches_directory(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / 'unit.c'
            source.write_text('#if 0\n#include "optional.h"\n#endif\n')
            self.assertIn(root.resolve(), dependencies(source, [root]))
            (root / 'optional.h').write_text('')
            self.assertIn((root / 'optional.h').resolve(), dependencies(source, [root]))

    def test_macro_include_fails_explicitly(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / 'unit.c'
            source.write_text('#include HEADER\n')
            with self.assertRaisesRegex(ValueError, 'unsupported include'):
                dependencies(source, [])

    def test_depfile_escaping_and_phony_prerequisites(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / 'space #$.c'
            source.write_text('')
            depfile = source.with_suffix('.d')
            write_depfile(source, 'build/unit.o', depfile, [])
            self.assertIn(escape(source) + ':\n', depfile.read_text())
            self.assertIn('space\\ \\#$$.c', depfile.read_text())


class ObjectTests(unittest.TestCase):
    def test_debug_and_resolved_symbol_spelling_ignored(self):
        with tempfile.TemporaryDirectory() as directory:
            a, b = Path(directory) / 'a.o', Path(directory) / 'b.o'
            elf_fixture(a)
            elf_fixture(b, debug=b'new debug info', external='renamed')
            self.assertEqual(normalized(a, {'callee': 0x80001000}),
                             normalized(b, {'renamed': 0x80001000}))
            self.assertNotEqual(normalized(a, {'callee': 1}), normalized(b, {'renamed': 2}))

    def test_bytes_bss_and_relocation_changes_detected(self):
        with tempfile.TemporaryDirectory() as directory:
            a, b = Path(directory) / 'a.o', Path(directory) / 'b.o'
            elf_fixture(a)
            baseline = normalized(a, {})
            for options in ({'text': b'\x01' * 4}, {'bss': 16}, {'relocation': 4}):
                elf_fixture(b, **options)
                self.assertNotEqual(baseline, normalized(b, {}))

    def test_inventory_and_empty_builds(self):
        with tempfile.TemporaryDirectory() as directory:
            old, new = Path(directory) / 'old', Path(directory) / 'new'
            with self.assertRaisesRegex(ValueError, 'contain objects'):
                compare(old, new)
            for root in (old, new):
                elf_fixture(root / 'unit.o')
                elf_fixture(root / 'pikachu.elf', address=0x80001000)
            self.assertEqual(compare(old, new), (1, []))
            elf_fixture(new / 'extra.o')
            self.assertIn('extra.o: extra object', compare(old, new)[1])

    def test_malformed_elf_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'bad.o'
            path.write_bytes(bytes(52))
            with self.assertRaisesRegex(ValueError, 'expected ELF32'):
                normalized(path, {})
