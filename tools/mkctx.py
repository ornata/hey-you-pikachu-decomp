#!/usr/bin/env python3
"""Build build/ctx.c — the preprocessed context m2c needs.

m2c sees one flat translation unit. Feed it every type and extern the
game headers declare, pre-expanded, so its drafts come out with real
types instead of guessed s32 soup.
"""
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent

SOURCE = """\
#include <ultra64.h>
#include "common.h"
#include "script_message.h"
#include "actor_script.h"
#include "script_vm.h"
#include "voice_commands.h"
#include "node_pool.h"
#include "keyed_pool.h"
#include "rom_dma.h"
#include "rom_cache.h"
#include "object_types.h"
#include "item_types.h"
#include "player_types.h"
#include "scene_types.h"
#include "heap_alloc.h"
"""

CMD = [
    "cc", "-E", "-P", "-x", "c", "-",
    "-D_LANGUAGE_C", "-DTARGET_N64", "-DF3DEX_GBI_2", "-D_MIPS_SZLONG=32",
    "-U__GNUC__", "-nostdinc",
    "-Iinclude", "-Iinclude/PR", "-Iinclude/libc", "-Isrc", "-I.",
]


def main() -> None:
    out = ROOT / "build" / "ctx.c"
    out.parent.mkdir(exist_ok=True)
    r = subprocess.run(CMD, input=SOURCE, capture_output=True, text=True, cwd=ROOT)
    if r.returncode != 0:
        sys.exit(f"cpp failed:\n{r.stderr}")
    out.write_text(r.stdout)
    print(f"ok: {out} ({len(r.stdout.splitlines())} lines)")


if __name__ == "__main__":
    main()
