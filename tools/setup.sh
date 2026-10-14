#!/usr/bin/env bash
# One-time toolchain setup for the Hey You, Pikachu! decomp.
# Installs only repository-local compiler/tool checkouts and the Python environment.
# Host prerequisites come from `nix develop` or an explicitly prepared environment.
# Game C uses IDO 5.3; the Makefile owns compiler and per-unit ISA settings.
set -euo pipefail
cd "$(dirname "$0")/.."

case "${1:-}" in
    "") check_only=0 ;;
    --check) check_only=1 ;;
    --help|-h)
        echo "Usage: tools/setup.sh [--check]"
        echo "Check host prerequisites, then prepare repository-local tools and Python dependencies."
        echo "--check performs prerequisite checks only; nothing is downloaded or installed."
        exit 0 ;;
    *) echo "error: unknown argument: $1" >&2; exit 1 ;;
esac
if [ "$#" -gt 1 ]; then
    echo "error: expected at most one argument" >&2
    exit 1
fi

IDO_RELEASE="https://github.com/decompals/ido-static-recomp/releases/download/v1.2"

# Pinned tool revisions. Bumping one is a deliberate, gated commit.
M2C_SHA=708d2d2cb2698f091a92492b328f73b24209f72d
ASM_DIFFER_SHA=0dd09af8f8008f1f880327cf0aca3b26d2562ea2
PERMUTER_SHA=059609d4aec73eb0650726772954e1ad575825f8

# SHA-256 of every release asset we accept; a download that does not match is rejected.
ido_sha256() {
    case "$1" in
        ido-5.3-recomp-macos.tar.gz)     echo 5b1ca006ee4b158ffba0422fc3f00b9b330f9036f279bae2ea1b4703317ad9c0 ;;
        ido-5.3-recomp-linux.tar.gz)     echo ab5c741561f80913d58c8b074771f23941a3edd312505a8ebed6d1dfeb65e506 ;;
        *) echo "" ;;
    esac
}
sha256_of() { if command -v sha256sum >/dev/null 2>&1; then sha256sum "$1" | cut -d' ' -f1; else shasum -a 256 "$1" | cut -d' ' -f1; fi; }

case "$(uname -s):$(uname -m)" in
    Darwin:arm64|Darwin:x86_64) IDO_TARBALL="ido-5.3-recomp-macos.tar.gz" ;;
    Linux:x86_64) IDO_TARBALL="ido-5.3-recomp-linux.tar.gz" ;;
    *) echo "error: no pinned IDO 5.3 release for $(uname -s)/$(uname -m)." >&2; exit 1 ;;
esac

# Check all prerequisites before downloading tools or changing local environments.
missing=()
for tool in mips-linux-gnu-as mips-linux-gnu-ld mips-linux-gnu-objcopy uv python3 git curl tar make shasum; do
    command -v "$tool" >/dev/null 2>&1 || missing+=("$tool")
done
if [ "${#missing[@]}" -ne 0 ]; then
    printf 'error: missing prerequisite: %s\n' "${missing[@]}" >&2
    echo "Enter the pinned environment with: nix develop --command bash" >&2
    echo "Or supply these tools yourself; setup never installs system packages." >&2
    exit 1
fi
if ! python3 -c 'import sys; sys.exit(sys.version_info < (3, 12))'; then
    echo "error: Python 3.12 or newer is required; use nix develop --command bash." >&2
    exit 1
fi
if [ "$check_only" -eq 1 ]; then
    echo "OK: setup prerequisites are available; no files changed."
    exit 0
fi
uv sync --locked

# --- IDO (the original compilers, statically recompiled) --------------
# The matching game build requires 5.3. SDK assembly is generated from the ROM.
for V in 5.3; do
    if [ ! -x "tools/ido-$V/cc" ]; then
        T="$IDO_TARBALL"
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

echo "setup complete. Supply local SDK headers (docs/LOCAL_SDK.md), then run: uv run make -j"
