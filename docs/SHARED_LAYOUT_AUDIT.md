# Shared layout evidence

This audit records the scene, maze, player-control and textured-rectangle recovery
after the C interface sweep. It does not certify the remaining project-wide
layout work as complete. Shared types describe established storage and contracts;
partial views do not establish an allocation size for their containing resource.

| Record | Established extent | Evidence and ownership |
| --- | --- | --- |
| `SceneConfigurationPrefix` | `0x74` bytes | Current-area consumers establish cell dimensions, callbacks, entrance poses, clipping, fog, asset metadata, backdrop state and path/script tables. Borrowed from the loaded area; do not allocate or copy a configuration through the prefix. |
| `SceneEventLink` | `0x1C` bytes | `mazeConfigureEventLinks` advances by `0x1C`, reads source XYZ cell indices, installs the callback at `0xC`, and optionally resolves target indices at `0x10`. A null handler terminates the borrowed list. |
| `MazeNode` | `0x50` bytes | Allocation and initialization establish the extent. Root geometry now uses the shared node; event installation and dispatch share its handler at `0x48` and linked region at `0x4C`. The maze owns nodes; linked-region references are borrowed. |
| `SceneSpawnPoint` | `0x20` bytes | Player initialization selects entrances with a `0x20` stride and reads XYZ/yaw at `0..0xC`; companion spawning reads the following XYZ/yaw at `0x10..0x1C`. Borrowed entrance records remain in the loaded configuration. |
| `PlayerControlPrefix` | `0x100` bytes | Existing record-stride evidence and member checks establish the player allocation view. Control-hierarchy positioning now uses its position, eye height and terrain-clearance fields, replacing the private three-float position view. |
| `TextureRectangle` | `0x24` bytes | `textureDrawRectangles` consumes nine-word records. A negative texture index terminates the borrowed list; negative dimensions select source dimensions. This semantic name replaces `TextureDrawC354` throughout current code. |
| `PresentationDrawTemplate` | `0x90` bytes | Four shared rectangles hold up to three presentation panels and a terminator. Templates and stack copies retain their original extents. |

## Scene metadata

The scene prefix now exposes fog RGBA at `0x38`, projection extent at `0x2C`, the
existing `AssetDescriptor` at `0x48`, a vertical backdrop offset at `0x68`, and
event links at `0x18`. Size and offset assertions cover the new fields and the
entrance-pose components. `MazePositionEvent` and `SceneCellSampler` document
callback inputs and results without changing their calling conventions.

Ghidra at `0x800006E4` confirms the asset-loader inputs and their common bias.
The linked image's descriptors for all 23 configured areas have cartridge ranges
containing the encoded bytes plus eight bytes per correction, with eight or
sixteen bytes remaining at the recorded ROM end. This supports reuse of the
existing seven-word asset descriptor rather than a coincidentally sized record.
Ghidra at `0x8010DEF8` independently confirms the event-link stride and member
roles. Player initialization at `0x80111F40` confirms the paired entrance stride.

The word at configuration offset `0x1C` remains explicitly opaque. Its role is
not established; it is not declared padding or assigned an invented scalar
meaning. Next evidence: trace configuration producers and indirect/script
consumers before naming or retyping it. Presentation-specific configuration
fields beyond this prefix also remain outside this completed entry.

## Retained matching representations

- Renderer asset metadata keeps unsigned reads through checked shared-member
  offsets. Direct descriptor reads fail the ROM gate. Fog endpoints also retain
  checked offsets, while fog color and projection extent use direct fields.
- Scene dimensions use the existing `Vec3f` component view with checks against
  the shared configuration array. Direct array accesses change root/control
  hierarchy instructions.
- Player initialization now uses `SceneSpawnPoint` directly. The private scalar
  spawn view was removed after typing the configuration global made the canonical
  entrance-record accesses preserve all object bytes.
- `SceneOccupancyView` retains the sampler's matching scalar representation.
  Size and component/callback offsets are checked against the shared prefix;
  it cannot be used to allocate or copy configurations.
- Event-list traversal retains the original target's null-plus-zero byte-cursor
  behavior for an absent list; this portability constraint is documented beside
  the function.

These exceptions have known layouts. Remaining work is compiler representation
recovery, not permission to weaken object comparison or change the baseline.
Investigate argument/load ordering and alias effects with the target compiler
before removing a matching view. ROM identity and strict relocation-addend checks
remain required for every replacement.

## Typed scene storage and initialization phase

`scene_configuration.h` owns the common declaration of
`gCurrentAreaConfiguration` as a pointer to the established scene prefix. Its
producer and all consumers use that declaration. Byte arithmetic and
area-specific extensions use explicit byte views; established common fields use
direct members. Five private `SceneScale` records and their incompatible global
pointer declarations were removed. The typed global and every consumer pass the
original object comparison.

The callback at configuration offset `0x40` is `initializeScene`, not a maze
update operation. The runtime invokes it at the end of common scene gameplay
initialization. Resolving the callback in every one of the 23 configured overlays
identifies area initializers (including the intentional no-op scene). The public
wrapper is `areaRunSceneInitializer`; its original address remains `0x8010E53C`.
This corrects the lifecycle contract without changing the callback ABI or bytes.

## Player chase state and material tint

The player byte at `0xF7` is `chaseStage`, sharing storage with the retained
`gPlayerChaseStage` linker view at `0x800EF797`. The setter, getter, reset and Ghidra
at `0x801127DC` agree on its byte width and role. Saved furthest-stage progress is
separate and uses the setter's full input value. The profile bytes at `0xF4` and
`0xF5` remain unresolved: initialization to six and eight does not establish their
meaning. The `0x40..0x4B` region also remains opaque. Next evidence is producer and
indirect/script consumer tracing; no birthday, timer or vector interpretation is
assigned from defaults or size alone.

Material drawing obtains tint storage from `ItemStatePrefix.tint`, checked at
`0x4C`. A private, size-checked `ItemMaterialTintView` preserves the original
component loads because both direct-byte and bounded-array alternatives fail
the ROM gate. The view borrows four RGBA bytes and owns no item resources.
Encounter setup in the shop and deep forest now uses `ActorScriptState` target
position/handle fields, shared object world positions and `ACTOR_TARGET_FOLLOW`.
