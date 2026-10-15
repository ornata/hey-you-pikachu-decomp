#!/bin/sh
# Keep staged C sources and headers clang-formatted.
# format_staged managed hook: installed as the shared pre-commit hook by
# tools/setup_hooks.sh; this marker line identifies it, do not remove.
#   format_staged.sh          rewrite fully staged files and re-stage them
#   format_staged.sh --check  report unformatted staged files, change nothing
# Skipped while a merge, cherry-pick, revert or rebase is in progress: the
# staged diff then carries other authors' lines, which must not be reformatted.
set -u
cd "$(git rev-parse --show-toplevel)" || exit 1
git_dir=$(git rev-parse --git-dir)
for marker in MERGE_HEAD CHERRY_PICK_HEAD REVERT_HEAD rebase-merge rebase-apply; do
    [ -e "$git_dir/$marker" ] && exit 0
done

mode=apply
[ "${1:-}" = "--check" ] && mode=check
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
status=0

for f in $(git diff --cached --name-only --diff-filter=ACM -- '*.c' '*.h'); do
    git show ":$f" >"$tmp/staged"
    clang-format --assume-filename="$f" <"$tmp/staged" >"$tmp/formatted" || exit 1
    cmp -s "$tmp/staged" "$tmp/formatted" && continue
    if [ "$mode" = check ]; then
        echo "not clang-formatted: $f" >&2
        status=1
    elif ! git diff --quiet -- "$f"; then
        echo "not clang-formatted and partially staged: $f" >&2
        echo "  format it by hand (clang-format -i $f) and re-stage" >&2
        status=1
    else
        cp "$tmp/formatted" "$f" && git add -- "$f" || exit 1
        echo "clang-formatted: $f" >&2
    fi
done
exit $status
