#!/bin/sh
# Install tools/format_staged.sh as the shared pre-commit hook.
# The hooks directory lives in the common git dir, so every worktree of this
# clone is covered regardless of which branch it has checked out. Idempotent.
cd "$(git rev-parse --show-toplevel 2>/dev/null)" || exit 0
common=$(git rev-parse --git-common-dir) || exit 0
hook="$common/hooks/pre-commit"
marker='format_staged managed hook'

# core.hooksPath replaces the hooks directory, so a stale in-tree path would
# silently disable the shared hook on branches that lack it.
path=$(git config --local --get core.hooksPath 2>/dev/null)
if [ "$path" = .githooks ]; then
    git config --local --unset core.hooksPath
elif [ -n "$path" ]; then
    echo "setup-hooks: core.hooksPath=$path overrides $common/hooks; pre-commit formatting is not enforced" >&2
    exit 0
fi

if [ -e "$hook" ] && ! grep -q "$marker" "$hook"; then
    echo "setup-hooks: $hook exists and is not ours; leaving it alone" >&2
    exit 0
fi
cmp -s tools/format_staged.sh "$hook" 2>/dev/null && exit 0
mkdir -p "$common/hooks" && cp tools/format_staged.sh "$hook" && chmod +x "$hook"
echo "setup-hooks: installed pre-commit hook in $common/hooks" >&2
