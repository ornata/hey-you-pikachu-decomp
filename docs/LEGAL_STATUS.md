# Rights and distribution status

This project reconstructs a copyrighted game. Nintendo and other rights holders
retain their rights to the original game, SDK, assets, and other material.
This repository does not grant a license to those works. Removing SDK files does
not establish that the remaining decompilation may be distributed under an
open-source license.

## SDK exclusion

SDK C implementations and associated assembly have been removed from tracked
source. They are regenerated locally from a user-supplied ROM. SDK-related
headers, including reconstructed interfaces and internal layouts, are local-only
inputs listed by hash in `LOCAL_SDK_FILES.json`. No SDK source is downloaded by
the header importer. See `LOCAL_SDK.md` for the build and export procedure.

This is a distribution boundary, not a claim of clean-room authorship or legal
clearance. Decompiled C, modified source, and disassembly can still embody
protected expression. Removing notices or changing names does not resolve that
question. Public availability of upstream material does not establish permission
to redistribute it.

The U.S. Copyright Office describes copyright protection for expression in
[computer programs](https://www.copyright.gov/register/tx-programs.html).
Its guidance on [derivative works](https://www.copyright.gov/circs/circ14.pdf)
explains the distinction between new contributions and preexisting material.
Applicable law and permission depend on jurisdiction and circumstances; obtain
qualified review before public distribution or selecting a project-wide license.

## Local files

Ignored local inputs are not part of the tracked repository. Do not publish
ROMs, generated SDK assembly, assets, local headers, compiler binaries, or build
artifacts. Review tracked files before publishing.

`make verify-clean` detects selected hygiene problems. It cannot prove that
tracked source contains no protected expression or certify legal safety.
