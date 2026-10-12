# asm-differ hook. Usage, from repo root after `make expected`:
#
#   uv run tools/asm-differ/diff.py -mwo3 func_80012345
#
#   baseimg = the last known-matching build (frozen by `make expected`)
#   myimg   = your current build
#
# -m rebuilds via make, -w watches sources, -o3 shows three columns.


def apply(config, args):
    config["baseimg"] = "expected/build/pikachu.z64"
    config["myimg"] = "build/pikachu.z64"
    config["mapfile"] = "build/pikachu.map"
    config["source_directories"] = ["src", "include"]
    config["arch"] = "mips"
    config["objdump_executable"] = "mips-linux-gnu-objdump"
    # asm-differ appends the selected object or ROM target. Object comparisons
    # must not additionally link the ROM (a draft may have unresolved symbols).
    config["makeflags"] = ["-j8"]
    config["show_line_numbers_default"] = True
