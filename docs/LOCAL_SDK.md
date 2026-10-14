# Local SDK inputs

This repository does not distribute SDK implementations or SDK-related headers.
The matching build needs two kinds of local input:

- Your own USA ROM dump, normalized to `rom/baserom.us.z64` with SHA-1
  `da1a1e47a86720f9d54fb2d2d247480041bda824`.
- The 81 headers listed by path and SHA-256 in `LOCAL_SDK_FILES.json`.
  These include SDK interfaces, internal audio/controller layouts, and support
  macros. Obtain and use these only where you have the necessary rights.

Supply a local directory with the manifest's `include/...` layout, then run:

```sh
python3 tools/materialize_ultralib.py --from /path/to/local-sdk-inputs
python3 tools/materialize_ultralib.py --check
make clean
make -j8
```

The importer validates all source files before copying. It never downloads or
clones an SDK repository. Its historical filename is retained for existing users.
Existing local headers from an earlier checkout can be verified in place with
`--check`. All manifest paths are ignored by Git; retain any copyright notices
in your local copies. Hashes identify accepted build inputs, not permission to
redistribute them. A raw SDK installation may need different directory layout
or may differ from these exact build inputs; the importer does not convert it.

`config/pikachu.yaml` describes SDK units as assembly and their data as separate ROM
sections. Splat generates them from the supplied ROM into ignored `build/asm/`.
Neither those files nor ROMs, extracted assets, objects, or compilers belong in
an exported repository. The ROM gate verifies the complete result, including
these generated sections. Game object comparisons verify that this change did
not alter the retained game code.

## Publication checks

Run `make verify-clean` before publishing. Ignored ROMs, generated SDK assembly,
assets, local headers, compilers and build artifacts must remain local. The check
covers selected hygiene issues; see the [project notice](../README.md#notice).
