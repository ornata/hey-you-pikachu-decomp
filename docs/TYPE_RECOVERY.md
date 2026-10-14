# Type and name recovery

Recover types and names from allocation, initialization, readers, writers and
ROM assembly. Preserve access width, signedness, alignment, ownership and the
matching calling convention. Partial record views do not establish allocation
sizes; unresolved fields must retain explicit evidence limits.

## Inventory

The before column records the original recovery baseline; use the reporter for
current counts.

Counts exclude comments, strings and character literals and cover game C under
`src/`. Occurrences and affected files are separate measures.

| Category | Before occurrences / files | After occurrences / files |
| --- | --- | --- |
| `M2C_FIELD` | 2,575 / 75 | 0 / 0 |
| `M2C_UNK*` | 1,012 / 71 | 0 / 0 |
| Address-named globals | 6,650 / 102 | 7 / 3 |
| `temp_*` | 2,480 / 58 | 0 / 0 |
| Raw offset candidates | 4,040 / 86 | 865 / 64 |

Supplemental cast-base offset candidates: **41 occurrences / 6 files**.
This category was not tracked in the original baseline. The historical raw-offset
metric recognizes identifier bases; explicit byte casts can move expressions out
of that metric without recovering their layout. Both inventories are review
candidates, not defect or completion counts.

Address-global counts are references, not declarations: three address-named
globals remain in three files, with seven references. Their unresolved roles
are recorded in the recovery ledger. Counts reflect the current source;
regenerate the report after further recovery. Temporary-name uniqueness is
spelling uniqueness, not distinct local variables. The raw-offset heuristic
includes scalar arithmetic; it is a review list, not a count of incorrect
pointer accesses.

Run `make source-quality`, `make source-quality JSON=1`, or
`python3 tools/source_quality.py --json` for current counts and every occurrence's
location, function and source context.

## Evidence and unresolved cases

- `TYPE_RECOVERY.json` records current placeholder and raw-offset locations.
- `TYPE_RECOVERY_NOTES.json` records matching constraints and next evidence needed.
- `GLOBAL_RECOVERY.json` records accepted global names, original addresses and evidence.
- `GLOBAL_RECOVERY_REFERENCE_AUDIT.json` records the bounded relocation investigation
  of the three unresolved address-named globals. Its method and exclusions are
  explicit; absence of a named relocation does not prove absence of all consumers.
- [Shared layout evidence](SHARED_LAYOUT_AUDIT.md) records storage and ownership facts.
- [Interface contracts](INTERFACE_CONTRACTS.md) describes the reviewed ABI exceptions.

Original names remain in declaration comments. Do not invent semantic names to
lower counts, merge interior symbols that change relocation addends, or replace
unknown types mechanically with `void *`.

## Continuing recovery

`TYPE_RECOVERY.json` inventories every residual occurrence by file and token, with
`line:column` locations. Category policies give the reason for retaining each class
and the next evidence needed; file-specific exceptions take precedence. Included
project headers identify available shared views. This is an inventory survey of all
game C, not a claim that every residual expression has received a complete semantic
analysis. Raw offsets remain candidates and may be intentional byte arithmetic.

Before editing, save a matching build outside `build/`. Keep one source-unit
mutation active, use Ghidra to resolve ambiguity, and preserve access width,
alignment, signedness, ownership and expression order. Do not assign speculative
field meanings or replace unknown types mechanically with `void *`.

After source locations or names change, update any relevant exception notes and
regenerate the ledger:

```sh
python3 tools/source_quality.py --ledger > docs/TYPE_RECOVERY.json
python3 tools/source_quality.py --check-ledger
```

The stale-ledger check is part of `make verify`. Acceptance also requires a clean
build ending in `OK: ROM matches`, full object comparison to the saved baseline,
direct ROM comparison, name checks without errors, tests and publication hygiene.
Regenerate m2c context when headers change. Do not replace the baseline to hide a
failed comparison. See `AGENT_WORKFLOW.md` for the exact verification commands.
