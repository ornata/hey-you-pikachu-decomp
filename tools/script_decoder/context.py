"""Source navigation and explicit ROM/ELF address mapping for local decoding."""

import hashlib
import re
from pathlib import Path

from tools.compare_objects import Elf
from tools.source_quality import code_only

ROOT = Path(__file__).resolve().parents[2]
ROM_SHA1 = 'da1a1e47a86720f9d54fb2d2d247480041bda824'
ENUM_SOURCES = {
    'vm': ('src/ovl/script_vm.c', 'ScriptOpcode'),
    'cue': ('include/actor_action_cue.h', 'ActorActionCueCode'),
    'sequence': ('include/action_script_types.h', 'ActionScriptOpcode'),
}

# These four retained handlers read one packed string-count operand in addition
# to the dispatch selector. Other retained handlers consume their arguments from
# VM state/stack. A source-consumer regression test guards this reviewed list.
INLINE_NATIVE_OPERANDS = dict.fromkeys((
    'actorScriptBuildSharedText', 'actorScriptBuildSharedTextAlternate',
    'actorScriptEnqueueMessage', 'actorScriptAppendWindowText',
), ['string_count'])


def opcode_names(kind):
    """Use this checkout's recovered enum names, retaining numeric encodings in reports."""
    path, enum = ENUM_SOURCES[kind]
    code = code_only((ROOT / path).read_text())
    match = re.search(r'\benum\s+' + enum + r'\s*\{([^}]+)\}', code)
    if not match:
        raise ValueError(f'missing recovered enum {enum}')
    result = {}
    for name, raw in re.findall(r'\b(\w+)\s*=\s*(0[xX][\da-fA-F]+|\d+)\b', match[1]):
        result[int(raw, 0)] = name
    return result


class LocalImage:
    """Read a supplied normalized USA ROM; ELF mappings must agree with its bytes."""

    def __init__(self, rom, elf=None, section=None):
        self.path = Path(rom)
        self.rom = self.path.read_bytes()
        self.sha1 = hashlib.sha1(self.rom).hexdigest()
        if self.sha1 != ROM_SHA1:
            raise ValueError('expected the normalized USA ROM; run tools/rom.py on your own dump')
        self.section = section
        self.elf = Elf(Path(elf)) if elf else None
        self.symbols = list(self.elf.symbols(self.elf.names.index('.symtab'))) if self.elf else []
        self.allowed = {'.boot', '.ovl'}
        if section is not None:
            if self.elf is None or section not in self.elf.names or section not in {'.boot', '.ovl'} and not section.startswith('.scene_'):
                raise ValueError('section must name an explicit boot/resident/scene ELF section')
            self.allowed.add(section)

    def symbol(self, name):
        """Reject ambiguous overlay symbols rather than silently choosing an address."""
        if not self.elf:
            raise ValueError('symbol/address resolution requires --elf')
        found = [s for s in self.symbols if s[0] == name and (s[5] == 0xFFF1 or s[5] < len(self.elf.sections)
                 and self.elf.names[s[5]] in self.allowed)]
        if len(found) != 1:
            raise ValueError(f'{name}: expected one symbol in the selected sections')
        return found[0]

    def locate(self, value, size):
        """Resolve an absolute target address or a symbol to a ROM-backed byte span."""
        if not self.elf:
            raise ValueError('address resolution requires --elf')
        try:
            address = int(value, 0) if isinstance(value, str) else value
        except ValueError:
            address = self.symbol(value)[1]
        candidates = []
        for i, section in enumerate(self.elf.sections):
            name = self.elf.names[i]
            if name not in self.allowed or section[1] == 8:
                continue
            if section[3] <= address and address + size <= section[3] + section[5]:
                candidates.append((i, name, section))
        if len(candidates) != 1:
            raise ValueError(f'{address:#x}: no unique ROM-backed mapping; select the exact scene with --section')
        i, name, section = candidates[0]
        start = self.symbol(name[1:] + '_ROM_START')[1]
        raw = self.elf.data(i)
        if self.rom[start:start + len(raw)] != raw:
            raise ValueError(f'{name}: ELF bytes differ from the supplied ROM')
        offset = start + address - section[3]
        return address, offset, name

    def read_rom(self, offset, size):
        if offset < 0 or size <= 0 or offset + size > len(self.rom):
            raise ValueError('requested window is outside ROM')
        return self.rom[offset:offset + size]

    def read_address(self, value, size):
        address, offset, section = self.locate(value, size)
        return self.read_rom(offset, size), dict(address=address, rom_offset=offset, section=section)

    def functions(self, address):
        """Return all function aliases in the explicit loaded-section set."""
        if not self.elf:
            return []
        return sorted({s[0] for s in self.symbols if s[1] == address and s[3] & 15 == 2
                       and s[5] < len(self.elf.sections) and self.elf.names[s[5]] in self.allowed})

    def table(self, value, count):
        if not 0 < count <= 4096:
            raise ValueError('table count must be between 1 and 4096')
        raw, _ = self.read_address(value, count * 4)
        return {i: dict(address=int.from_bytes(raw[i * 4:i * 4 + 4], 'big'),
                        functions=self.functions(int.from_bytes(raw[i * 4:i * 4 + 4], 'big')))
                for i in range(count)}


def source_index():
    """Locate written game functions; an address alias alone is not a semantic name."""
    result = {}
    for path in sorted((ROOT / 'src').rglob('*.c')):
        code = code_only(path.read_text())
        for match in re.finditer(r'^\w[^;{}\n]*\b(\w+)\([^;{}]*\)\s*\{', code, re.M):
            name = match[1]
            result.setdefault(name, []).append(dict(file=str(path.relative_to(ROOT)),
                                                   line=code.count('\n', 0, match.start()) + 1))
    return result


def native_layouts(image, entries):
    """Apply reviewed layouts only to functions in the authenticated retail catalog."""
    # This is the existing bounded retained table, including its reserved null slot.
    if image.symbol('D_8012B400')[2] != 279 * 4:
        raise ValueError('retained native catalog changed; review its bound and inline layouts')
    catalog = image.table('D_8012B400', 279)
    known = {name for row in catalog.values() for name in row['functions']}
    for row in entries.values():
        names = row['functions']
        if names and all(name in known for name in names):
            row['inline_operands'] = next((INLINE_NATIVE_OPERANDS[n] for n in names if n in INLINE_NATIVE_OPERANDS), [])
    return entries


def annotate(report, index, cue_handlers=None):
    """Attach navigation, preserving unresolved table targets and alias lists."""
    for row in report['instructions']:
        if report['format'] == 'cue' and 19 <= row['opcode'] <= 45 and cue_handlers is not None:
            row['handler_targets'] = cue_handlers[row['opcode'] - 19]
        row['source'] = index.get(row['handler'], [])
        for field in ('native', 'handler_targets'):
            if field in row:
                row[field]['source'] = {name: index.get(name, []) for name in row[field]['functions']}
    return report
