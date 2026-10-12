#!/usr/bin/env bash
# One-time toolchain setup for the Hey You, Pikachu! decomp.
#
#   .--------------------------------------------------------------.
#   | The game was compiled in 1998-2000 with SGI's IRIX compiler  |
#   | (IDO 7.1). To match the ROM byte-for-byte we must use that   |
#   | exact compiler — a modern compiler will never reproduce its  |
#   | register allocation or scheduling. decompals/ido-static-     |
#   | recomp translates the original IRIX binaries into native     |
#   | macOS/Linux executables (incl. Apple Silicon), so the 1998   |
#   | codegen runs at native speed with no emulator.               |
#   '--------------------------------------------------------------'
#
# What this installs (all gitignored; compilers are checksum-pinned, repos are commit-pinned):
#   mips-linux-gnu binutils   assembler/linker/objcopy (brew/apt)
#   tools/ido-7.1/            the original compiler, recompiled
#   .venv/                    python env via uv (splat + tool deps)
#   tools/m2c/                asm -> C draft decompiler
#   tools/asm-differ/         per-function diff loop
#   tools/decomp-permuter/    brute-force matcher for stubborn funcs
#   rom/baserom.us.z64        your dump, normalized + verified
set -euo pipefail
cd "$(dirname "$0")/.."

IDO_RELEASE="https://github.com/decompals/ido-static-recomp/releases/download/v1.2"

# Pinned tool revisions. Bumping one is a deliberate, gated commit.
M2C_SHA=708d2d2cb2698f091a92492b328f73b24209f72d
ASM_DIFFER_SHA=0dd09af8f8008f1f880327cf0aca3b26d2562ea2
PERMUTER_SHA=059609d4aec73eb0650726772954e1ad575825f8

# SHA-256 of every release asset we accept; a download that does not match is rejected.
ido_sha256() {
    case "$1" in
        ido-7.1-recomp-macos.tar.gz)     echo 7f75570ed1ca14e6161b84b10743d440c685f27291bfbfaf87c6a82236553bc6 ;;
        ido-7.1-recomp-linux.tar.gz)     echo 0d411696e178fcca34c31c3bf02011b928d7fd9c1fa7f8bf45070e0781b58e15 ;;
        ido-7.1-recomp-linux-arm.tar.gz) echo d74443a65b1e7c92c7853c60a80de97b40c7b374eb0565d67b754d86bbf5f123 ;;
        ido-5.3-recomp-macos.tar.gz)     echo 5b1ca006ee4b158ffba0422fc3f00b9b330f9036f279bae2ea1b4703317ad9c0 ;;
        ido-5.3-recomp-linux.tar.gz)     echo ab5c741561f80913d58c8b074771f23941a3edd312505a8ebed6d1dfeb65e506 ;;
        *) echo "" ;;
    esac
}
sha256_of() { if command -v sha256sum >/dev/null 2>&1; then sha256sum "$1" | cut -d' ' -f1; else shasum -a 256 "$1" | cut -d' ' -f1; fi; }

case "$(uname -s)" in
    Darwin) IDO_TARBALL="ido-7.1-recomp-macos.tar.gz" ;;
    Linux)
        case "$(uname -m)" in
            aarch64) IDO_TARBALL="ido-7.1-recomp-linux-arm.tar.gz" ;;
            *)       IDO_TARBALL="ido-7.1-recomp-linux.tar.gz" ;;
        esac ;;
    *) echo "error: unsupported OS $(uname -s)" >&2; exit 1 ;;
esac

# --- MIPS binutils (assembler / linker / objcopy) ---------------------
if ! command -v mips-linux-gnu-as >/dev/null 2>&1; then
    if command -v brew >/dev/null 2>&1; then
        echo "installing mips-linux-gnu-binutils via Homebrew..."
        brew install mips-linux-gnu-binutils
    else
        echo "error: mips-linux-gnu-as not found." >&2
        echo "  macOS:  brew install mips-linux-gnu-binutils" >&2
        echo "  Debian: apt install binutils-mips-linux-gnu" >&2
        exit 1
    fi
fi

# --- uv + python environment ------------------------------------------
if ! command -v uv >/dev/null 2>&1; then
    if command -v brew >/dev/null 2>&1; then
        brew install uv
    else
        echo "error: uv not found; see https://docs.astral.sh/uv/" >&2
        exit 1
    fi
fi
uv sync

# --- IDO (the original compilers, statically recompiled) --------------
# 7.1 built the game code; 5.3 built the libultra objects Nintendo
# shipped (proven per-function in wave 1). Both are required.
for V in 7.1 5.3; do
    if [ ! -x "tools/ido-$V/cc" ]; then
        T="${IDO_TARBALL/7.1/$V}"
        want="$(ido_sha256 "$T")"
        if [ -z "$want" ]; then
            echo "error: no pinned checksum for $T (no such release asset for this platform)" >&2
            exit 1
        fi
        echo "downloading IDO $V ($T)..."
        tmp="$(mktemp)"
        curl -sfL -o "$tmp" "${IDO_RELEASE}/${T}"
        got="$(sha256_of "$tmp")"
        if [ "$got" != "$want" ]; then
            rm -f "$tmp"
            echo "error: $T checksum mismatch (got $got, want $want)" >&2
            exit 1
        fi
        mkdir -p "tools/ido-$V"
        tar xz -C "tools/ido-$V" -f "$tmp"
        rm -f "$tmp"
    fi
done

# --- matching tools, pinned -------------------------------------------
fetch_pinned() { # dir url sha
    if [ ! -d "tools/$1" ]; then
        git clone -q "https://github.com/$2.git" "tools/$1"
    fi
    git -C "tools/$1" fetch -q --depth 1 origin "$3" 2>/dev/null || git -C "tools/$1" fetch -q origin
    git -C "tools/$1" checkout -q "$3"
}
fetch_pinned m2c              matt-kempster/m2c           "$M2C_SHA"
fetch_pinned asm-differ       simonlindholm/asm-differ    "$ASM_DIFFER_SHA"
fetch_pinned decomp-permuter  simonlindholm/decomp-permuter "$PERMUTER_SHA"

# --- base ROM ---------------------------------------------------------
if [ ! -f rom/baserom.us.z64 ]; then
    found=""
    for f in rom/*.n64 rom/*.v64 rom/*.z64; do
        [ -e "$f" ] || continue
        found="$f"
        break
    done
    if [ -z "$found" ]; then
        echo "error: place your Hey You, Pikachu! (USA) dump in rom/" >&2
        exit 1
    fi
    python3 tools/rom.py "$found" rom/baserom.us.z64
fi

echo "setup complete. run: make -j"
