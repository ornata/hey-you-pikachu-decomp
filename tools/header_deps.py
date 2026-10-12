#!/usr/bin/env python3
"""Write conservative Make dependencies without preprocessing with a host compiler.

Both branches of conditional includes are followed. Macro includes are rejected
rather than silently producing an incomplete dependency graph.
"""

import argparse
from pathlib import Path
import re
import sys


INCLUDE = re.compile(r'^\s*#\s*include\s+([^\n]+)', re.MULTILINE)
ASM_INPUT = re.compile(r'(?:^\s*\.(?:include|incbin)\s+|#\s*pragma\s+GLOBAL_ASM\s*\(\s*)"([^"\n]+)"', re.MULTILINE)


def dependencies(source, include_dirs):
    """Return existing transitive inputs, including inactive conditional includes."""
    found = set()

    def visit(path):
        path = path.resolve()
        if path in found:
            return
        found.add(path)
        if path.suffix not in {'.c', '.h', '.s', '.inc'}:
            return
        # Preserve newlines while removing comments; include-like text in comments
        # must not create dependencies. Strings containing comment markers survive.
        text = re.sub(r'"(?:\\.|[^"\\])*"|/\*.*?\*/|//[^\n]*',
                      lambda m: m[0] if m[0].startswith('"') else '\n' * m[0].count('\n'),
                      path.read_text(), flags=re.DOTALL)
        for match in INCLUDE.finditer(text):
            operand = match[1].strip()
            literal = re.fullmatch(r'([<"])([^>"]+)[>"]\s*', operand)
            if not literal:
                raise ValueError(f'{path}: unsupported include operand: {operand}')
            dirs = ([path.parent] if literal[1] == '"' else []) + include_dirs
            candidates = [directory / literal[2] for directory in dirs]
            target = next((p for p in candidates if p.is_file()), None)
            if target is None:
                # SDK headers mention optional headers in inactive branches.
                # Watch their nearest existing parent so adding one rescans.
                # An active missing include still fails in the real compiler.
                for candidate in candidates:
                    parent = candidate.parent
                    while not parent.is_dir():
                        parent = parent.parent
                    found.add(parent.resolve())
            else:
                visit(target)
        for match in ASM_INPUT.finditer(text):
            # GAS and GLOBAL_ASM paths are relative to the build working directory.
            candidates = [Path(match[1]), path.parent / match[1]] + [d / match[1] for d in include_dirs]
            target = next((p for p in candidates if p.is_file()), None)
            if target is None:
                raise ValueError(f'{path}: cannot find assembly input {match[1]}')
            visit(target)

    visit(source)
    return sorted(found)


def escape(path):
    return str(path).replace('$', '$$').replace('\\', '\\\\').replace('#', '\\#').replace(' ', '\\ ').replace(':', '\\:')


def write_depfile(source, target, output, include_dirs):
    root = Path.cwd()
    paths = dependencies(source, include_dirs)
    names = [escape(p.relative_to(root) if p.is_relative_to(root) else p) for p in paths]
    content = escape(target) + ': ' + ' \\\n  '.join(names) + '\n'
    content += ''.join(name + ':\n' for name in names)
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_suffix(output.suffix + '.tmp')
    temporary.write_text(content)
    temporary.replace(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--target', required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('-I', dest='include_dirs', type=Path, action='append', default=[])
    parser.add_argument('source', type=Path)
    args = parser.parse_args()
    try:
        write_depfile(args.source, args.target, args.output, args.include_dirs)
    except (OSError, ValueError) as error:
        print(f'header dependencies: {error}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
