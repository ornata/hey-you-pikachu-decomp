# Interface contracts

The interface sweep covers written game C definitions and declarations, direct
calls to functions with a C definition, old-style callback tables, and shared
header coverage. Every finding in this scope has been resolved or reviewed in
[INTERFACE_CONTRACTS.json](INTERFACE_CONTRACTS.json). Assembly-only functions and
SDK interfaces are not assigned inferred contracts by this audit.

Compared with the starting commit `0f41200`:

| Finding | Before | After |
| --- | ---: | ---: |
| Unspecified parameter declarations | 66 | 5 |
| Conflicting written signatures | 29 | 10 |
| Direct-call arity disagreements | 19 | 11 |
| K&R definitions | 5 | 2 |
| Unspecified callback tables | 1 | 1 |
| Exported functions without a shared header | 178 | 1 |

These are findings, not distinct broken functions. The 30 retained findings
describe matching-only call representations or an unresolved callback ABI.
Each exception records its exact signature or arity, source location, reason,
and evidence needed to replace it. Arity exceptions also identify the caller.
The sole function without a shared header is the inactive `heapDebugNoOp` hook,
whose original callers disagree with its K&R formal argument count. Publishing
a portable prototype for that hook would misrepresent its retained ABI.

## Recovered interfaces

Functions referenced across game C source units now have shared, documented
declarations, except for that diagnostic hook. Headers describe purpose,
parameters, results, and established
ownership and lifetime requirements; their types reuse the existing shared
records. Original identifier and address notes remain available.

The sweep corrected the VM integer reader, ROM byte reader, text-window
background drawing, help-text string length call, chapter-assignment wrapper,
CRC result, effect result, actor action flags, area names, and several pointer
and scalar argument views. The menu chapter wrapper now explicitly forwards
its incoming argument; its registration site remains unproven. The four-bit
outline interface reuses `TextImage` instead of a duplicate bitmap record.

The remaining compatibility views include heap allocation, collection hierarchy
calls, integer versus pointer ROM offsets, the font helper's unused argument,
the progress reader's dead formal argument, pointer-shaped tree keys, device
word-count width, and the orbit query's return width. The interaction action
table also mixes argument views and a void-returning handler. Its unspecified
result is documented rather than replaced with an invented value.

## Verification and future changes

Run `make check-interfaces` for a concise audit or
`python3 tools/interface_contracts.py --json` for declarations and findings with
current locations. `make verify` runs the interface gate alongside the existing
ROM, frozen-name, hygiene, test, recovery-ledger, and strict object checks.
New findings, missing review evidence, changed exception fingerprints, and stale
exceptions fail the gate. Adding an exception is a manual evidence review; the
tool does not accept or refresh exceptions automatically.

This is a lexical audit, not a replacement for C type checking, callback and
ownership analysis, or ROM evidence. Some header includes retain their original
declaration boundary because moving all declarations to the top can change
earlier implicit call handling under IDO. Dependency tracking still rebuilds
every consumer when those headers change.

Acceptance requires a clean build printing `OK: ROM matches`, direct ROM byte
comparison, and strict comparison against the preserved 483-object baseline.
The comparator and its relocation-addend checks are unchanged. New and changed
interface headers are also checked independently with the target compiler.
