# Shared layout evidence

This audit records the scene, maze, player-control and textured-rectangle recovery
after the C interface sweep. It does not certify the remaining project-wide
layout work as complete. Shared types describe established storage and contracts;
partial views do not establish an allocation size for their containing resource.

| Record | Established extent | Evidence and ownership |
| --- | --- | --- |
| `SceneConfigurationPrefix` | `0x7C` bytes | Current-area consumers establish cell dimensions, callbacks, entrance poses, clipping, fog, asset metadata, backdrop state, path/script tables, card textures and collection titles. Borrowed from the loaded area; do not allocate or copy a configuration through the prefix. |
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

The word at configuration offset `0x1C` is the authored `spawnCount`. All 23
configured values are between one and four and correspond to paired entrance
records. Nine areas provide an independently named following object whose
address bounds the spawn table exactly; their different counts validate the
`0x20`-byte record relationship without deriving the boundary from the count.

| Area | Count at `0x1C` | Spawn-table bytes before following object | Following object |
| --- | ---: | ---: | --- |
| Bedroom | 3 | `0x60` | `D_80134520_scene_bedroom` |
| Front yard | 3 | `0x60` | `D_80134590_scene_front_yard` |
| Springleaf entrance | 3 | `0x60` | `D_80134BB0_scene_springleaf_entrance` |
| Oddish meadow | 3 | `0x60` | `D_80134DC0_scene_oddish_meadow` |
| Springleaf field | 3 | `0x60` | `D_80134BE0_scene_springleaf_field` |
| Olivine lake | 2 | `0x40` | `D_80141578_scene_olivine_lake` |
| Ochre deep | 4 | `0x80` | `D_80134C80_scene_ochre_deep` |
| Playground | 1 | `0x20` | `D_801344B0_scene_playground` |
| Credits | 1 | `0x20` | `D_80143D04_scene_credits` |

The runtime still relies on callers supplying valid entrance indices; recovery
of this metadata does not add a bounds check or change selection behavior.
The configuration pointer table now uses the shared prefix type directly.

Presentation consumers establish `cardTextures` at `0x74` and `collectionTitles`
at `0x78`. The former borrows per-scenario texture lists with three used indices;
the latter borrows per-scenario strings. Tables or individual entries may be
null. Card drawing and collection-title creation use these shared pointer fields
directly, including the existing mutable third card texture. This extends the
established prefix to `0x7C`; the complete allocation remains unproven.

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

## Remaining common-field consumers

Action commands, actor creation and movement, toolbox objects, collection
presentation, title/menu backdrop selection, maze initialization, and the Ochre
entrance and Cobalt Island initializers now use the shared scene members directly.
The recovered scalar types retain their original widths, signedness and offsets;
no control flow or expression ordering changes accompany this adoption. Each of
the ten source-unit mutations passed the preserved object comparison separately.

## Maze replacement and ground-shadow consumers

`mazeReplaceBlock` uses shared maze depth, model references and region coordinates,
and shared object transforms and sibling links. Ghidra at `0x8010DC88` confirms
the node/object access widths and offsets. The sibling chain retains its existing
traversal order; this recovery adds no hierarchy restructuring.

Ground-shadow consumers now obtain their owned `GroundShadowGrid` from the shared
object state member. Transform updates distinguish the shadow, its borrowed owner
and the owner's default model part, using shared scale, local position and world
position fields. Ghidra at `0x80036330` confirms the terrain-conforming vertex
inputs. The shadow owns its rendering allocations, while owner and model references
remain borrowed. Both source units preserve the full object baseline.

## Actor-part and action-command consumers

Model selection now reads the owning actor's shared held-item handle and attachment
mode, then the resolved item's signed kind. Ghidra at `0x800148E0` confirms these
fields and the default display-list fallback. Appearance and animated-texture draw
callbacks use the shared display-list member; framebuffer command storage uses
the checked `ActorPartTextureState.commandBuffers` array. Part shaking obtains
its private state through the common object prefix.

Action commands use shared held-item and stored-target fields, target coordinates,
interaction time and exact-kind search inputs. These retain their scalar widths
and evaluation order. The animation builder's separate retained-value byte cursor
remains the documented IDO matching exception; `ValueTrack.value` is a signed
halfword, not a full-width integer. Each accepted source-unit change passed the
strict object baseline before the clean acceptance build.

## Contact-effect state and actor placement

`ContactEffectState` is a private owned 0x10-byte allocation: a signed trigger
flag followed by a `ValueTrack` borrowing resident animation keys. Allocation,
initialization, contact responses, animation evaluation and texture selection
agree on this layout; compile-time checks cover the extent, track offset 4 and
signed frame offset 0xC. Ghidra at `0x8001D7D8` confirms contact detection, reaction
transition and completion behavior. All four callbacks use the typed state.

Contact-effect object positions, scale, radius, display list and private-state
storage now use `RuntimeObjectPrefix`. Following actors use the established
`ActorScriptState.targetPosition` and object local/world position members. No
allocation size, lifecycle policy or gameplay logic changes accompany recovery.
The source-unit changes each preserve all baseline objects.

## Movement action workspace and shared consumers

The four-byte slot at actor-state offset `0x1F8` is an action-specific
`ActorMovementWorkspace`: approach callbacks apply its `approachScale` to pending
motion, the ground-check action records `groundClearance`, and vertical pulse
actions use `pulseAngle` in degrees. These views share storage and are not
simultaneously valid. Ghidra at `0x8000F934` and `0x80011528` confirms the approach
multiplier and pulse phase. Size and offset checks preserve the layout. The
separate randomized word at `0x1A0` remains unresolved beyond its observed store.

Movement callbacks now use shared orientation, target coordinates, approach
allowance, collision width, airborne status, and object transforms where the
original objects permit. Four callbacks retain their byte views after direct
member trials failed the ROM gate; their names and next evidence are recorded
in the recovery ledger. Each accepted callback mutation was compared against
all 483 baseline objects before proceeding.

## Toolbox object and item consumers

Toolbox display placement, rotation, hierarchy lookup, and item access now use
shared object transforms, child/sibling links, kind, handle and private state.
The player's retained item lookup uses `PlayerControlPrefix.heldItemHandle`.
Storage and removal expose `ItemStatePrefix.kind`, lifecycle state, owner and
flags; item-name and row-eligibility helpers also use the signed kind member.
Ghidra at `0x8002F114` confirms storage's signed kind read and eight-byte slot
stride. The slots retain handles without extending item lifetime.

Transfer confirmation keeps its duplicate-kind byte reads after direct members
changed the ROM; this matching exception and next compiler evidence are recorded
in the recovery ledger. The established layout is not in doubt. Every accepted
source-unit trial preserved all 483 baseline objects.

## Collection-presentation object and actor consumers

Actor placement now uses the active actor's shared local/world position and
state members. The retained ground-clearance offset comes from the actor state’s
borrowed ground-reference part; Ghidra at `0x800285B8` confirms that relationship.
Actor preparation uses shared orientation, target mode/position and model-part
world heights. Its global state load and three global world-coordinate reads
still require integer-address views: each direct-member trial independently
failed the ROM gate. Those four accesses have a focused matching exception.

Collection item placement, item text, collection-value lookup, inventory progress
and response selection use shared object kind/state and signed item-kind members.
Catalog interior aliases retain their symbol identities and strides. No prototype
compatibility exception or relocation comparison has been weakened. Accepted
source-unit trials all preserve the baseline objects.

## Texture motion and model drawing

Private `TextureMotion` storage is a checked 0x10-byte owned record containing
texture-coordinate wrap masks, S/T scroll velocities and an angular rate. Ghidra
at `0x800088F4` confirms initialization from the kind-specific numeric settings
in the object's radius slot. Drawing consumers establish the scrolling and
rotation roles; the packed configuration still requires the original compiler's
bit-field representation. The object prefix documents this radius-slot reuse.

Texture-motion drawing, scene environment drawing, kind-range search and relative
yaw queries use shared object members. Actor downhill movement, double-buffered
model setup and animated-texture return values also use the shared layouts.
The old animated-texture return exception is resolved: offset 0x30 is the display
list, not the child link asserted by the obsolete note. Both source units pass
strict baseline comparison with the canonical display-list read.

## Action-command and item-physics interfaces

Action-command item launch, pickup, target notification and effect emission
consumers use shared object positions, kind, handle and radius. Launch heading
uses `ActorScriptState.orientation`; target emission uses its target-position
array. Command payload records remain separate from object state, and their
packed byte accesses retain the existing command representation.

The costed script-action entry obtains its actor state from the shared object
prefix. Ghidra at `0x80116BC4` confirms the retained VM address, resource check and
action outcome fields. Item friction passes the established X/Z velocity members
by address, terrain effects receive the object's position, and maze events receive
its world position. Existing preload/operand-order exceptions remain unchanged.
Each of the four source units independently passed strict baseline comparison.

## Terrain diagnostics and remaining maze consumers

Terrain diagnostic cells now obtain their root, state, local/world positions,
hierarchy links and selected matrix buffer through `RuntimeObjectPrefix`. The
cell state's corner indices still select matrices within the root's owned buffer.
Ghidra at `0x8002B380` confirms grid creation, corner-index assignment and conversion
of the generated sibling chain into root children. Root private storage remains
a single signed grid-size word; the small player view remains its documented
matching constraint.

Maze terrain sampling uses shared minimum coordinates and extents, while node
lookup/activation/preparation use the child array. Model configuration uses shared
rotations, positions, scales and sibling links; subdivision clears the shared
terrain and event references. Ghidra at `0x8010B528` confirms terrain sampling's
node extents. The visibility callback retains geometry byte reads after its
direct-member trial failed the ROM gate; that focused exception is recorded.
All accepted source-unit mutations preserve the strict baseline objects.

## Player initialization and held-item interfaces

Area initialization now uses shared player position, view angles, eye height,
collision radius, controller masks/port, companion selection, airborne motion,
held-item handle and behavior state. The three retained control-object handles
come from shared object and sibling members. Existing entrance bounds and
initialization order remain unchanged.

Held-item throwing uses shared player position, eye height, view shake, pitch
offset and held-item handle, plus the item's kind, handle, state and position.
Ghidra at `0x8010FC48` confirms those view/launch inputs. Direct yaw and pitch
reads each failed the ROM gate; their checked member-offset reads retain matching
instructions and are recorded explicitly. Facing constraints use the established
actor collision flags with an unsigned view, and position events borrow the
active actor's world-position array. Every accepted mutation passed strict object
comparison before proceeding.

## Orbit follower object and lifecycle contract

The follower no longer defines a duplicate `OrbitObject`: updates and creation
use `RuntimeObjectPrefix` for transforms, world position, kind, handle and state.
Its private owned `OrbitState` retains a borrowed owner handle, reset interval,
orbit heading/rate and terrain-query dimensions/policy. Size and offset checks
cover the 0x20-byte state, radius and policy. Ghidra at `0x8002728C` confirms the
creator's owner reference, initial pose and collision-policy selection.

The public follower header documents initialization and update callbacks as well
as creation. A missing owner requests follower destruction; retaining its handle
does not extend the owner's lifetime. The existing terrain-query return-width
compatibility declaration remains its recorded matching exception. Replacing
the duplicate object view and creator offsets preserves all baseline objects.

## Messenger and flipbook remaining consumers

Flipbook geometry and command reset use the established vertex and branch fields;
scroll-board initialization passes its embedded model by member reference.
Messenger contact mode and homing use shared actor/object kind, state, position
and radius. Drawing reads `ValueTrack.value` for opacity and the shared object
display list; creation assigns that display-list member directly.

Rebound reads the copied message's `arg2` through `MessengerFlightState.message`.
Ghidra at `0x80026964` confirms the payload read and comparison of the linked
object's root with the active actor. That comparison retains a checked root-member
offset because a direct member failed the ROM gate; its next compiler evidence
is recorded in the recovery ledger. No mailbox layout or delivery behavior changes.
Every accepted source-unit trial preserved the strict baseline objects.

## World-map actor placement and particle count

World-map actor placement/preparation use shared target coordinates, orientation,
target mode, joint heights and matching direct actor pose members. Confirmation
and update use `ActorScriptState.defaultActionMode`. Ghidra at `0x8002DD70`
confirms the placement target and joint-height relationships. Two placement
coordinates and four preparation accesses require checked object-member offsets;
each direct-member alternative independently failed the ROM gate. These are
known-layout compiler representations, with focused ledger entries.

Converging-particle spacing uses the established private state's signed count
member at 0x28. Neither particle count nor actor behavior changes. Every accepted
source-unit trial preserves the strict baseline objects.

## Collision overlap and motion-playback consumers

Collision overlap now uses shared query/candidate world positions and candidate
radius, flags and kind. Ghidra at `0x8002CCF8` confirms those fields and overlap
response selection. The queried object's radius independently failed a direct
member trial; checked member offsets retain that read and its ledger records
the next compiler evidence. Registry keys and overlap behavior are unchanged.

Motion playback obtains object/part private state and root actor state through
the common object prefix. Pose extraction and blending use shared rotation
components; playback restart uses the established actor movement scale. The
existing private motion-track and pose workspaces remain distinct. Both source
units preserve all 483 baseline objects after their accepted mutations.

## Text-panel storage and eye-catch rectangles

Text-panel line colors use the shared RGBA table. Clearing uses shared count,
capacity, circular index, image table and entry table while retaining its byte
image cursor and existing destruction order. Ghidra at `0x80012EE4` confirms
the table layout and image-release behavior. Voice-command special-item selection
uses shared object state and signed item kind.

Eye-catch authored layouts and animated draw lists now use `TextureRectangle`;
the duplicate `Rec36A60` and `TextureDraw370` views are removed. Initial placement
updates rectangle origins and retains the negative-texture list terminator.
Ghidra at `0x80037034` confirms both lists reach the common rectangle renderer.
The shared size/scale checks cover both uses. Both source units preserve all
strict baseline objects without additional matching exceptions.

## Eye-catch descriptor and animation contracts

`EyeCatchGroup` replaces the address-derived descriptor name and misleading
unused `image` word. The last two bytes of that word are selection weights for
progress values at most twelve and greater than twelve; Ghidra at `0x800379A0`
confirms both weighted-selection passes. The first two bytes remain explicitly
unresolved: no current C consumer establishes a role. Next evidence is tracing
resource producers and any assembly or indirect consumer of those bytes.

The descriptor borrows a terminated rectangle list and six animation-key arrays
per animated layer, carries screen origins and foreground dimensions, selects
the alternate entry/exit sound set, and supplies four backdrop corner colors.
Compile-time extent and member checks preserve its 0x24-byte layout. Each owned
layer workspace is six `ValueTrack` records (0x48 bytes), replacing the float-array
view while retaining drawing's existing integer-address cursor. Initialization
uses typed track addresses and borrowed keys. Every accepted source-unit trial
preserves all 483 baseline objects.

## HUD actor references and title prompt

HUD direction indicators use the active actor's shared radius and world position;
companion indicators use its handle. Ghidra at `0x80021648` confirms direction-query
inputs. The player companion target retains its existing matching byte view, now
tied to a checked `PlayerControlPrefix.companionTarget` member offset.

The title Start prompt replaces a 27-word view with `TitlePromptLayout`, a checked
0x6C-byte drawing snapshot of three shared rectangles. Linked-image inspection
at `gTitleStartPromptSprites` (`0x8009B4CC`) finds texture indices 0, 1 and -1,
confirming two visible entries and a terminator. This establishes the copied
prefix, not the full extent of the symbol's surrounding storage. Ghidra at
`0x800346D8` confirms the snapshot reaches the shared renderer. Both source units
preserve all baseline objects; no new matching exception is needed.

## Canonical scrolling-backdrop camera view

Scrolling-backdrop drawing now uses `MatrixCamera` directly, removing the
address-derived `ScrollView333` record. The retained `gRenderCameraBackdropImages`
symbol aliases the primary camera's backdrop-image member. Its containing-camera
view uses that established member offset, preserving the original relocation
identity while exposing image, dimensions and offsets through shared fields.
Ghidra at `0x800333D4` confirms the five adjacent backdrop fields and draw roles.

A checked private slice first established correspondence with all five shared
members; the subsequent canonical view preserved every baseline object and made
the slice unnecessary. This is a borrowed view of persistent camera storage,
not an allocation or ownership transfer. The separate offset-updater compiler
constraint remains unchanged.

## Menu backdrop camera contract

The menu no longer defines a private `MenuBackdropView`. Its retained scroll
symbol is declared consistently as a floating-point scalar and yields the
containing `MatrixCamera` through the checked shared scroll offset. Menu opening,
closing and backdrop selection use the canonical `useBackdrop` member, preserving
the original interior-symbol relocation identity. Ghidra at `0x8011A32C` confirms
the opening transition's flag write next to the primary scroll value.

The private camera alias borrows persistent rendering storage and changes no
lifecycle policy. Canonical conversion passed the strict object baseline without
new compatibility exceptions.

## Paired-texture descriptor contract

The private `TexturePair` has ten established words: two image/palette/format/size
sets followed by shared dimensions. Images and palettes remain borrowed 32-bit
rendering addresses. Ghidra at `0x8004592C` confirms the initializer's ten stores;
renderer consumers establish their upload roles. Size and offset checks cover
the 0x28-byte record. Initialization now accepts `TexturePair *` directly and
documents replacement without resource release. No other C unit shares this
record, so it remains private. The typed interface preserves all baseline objects.

## Actor storage requiring further evidence

These fields remain unresolved rather than inferred from their sizes or defaults:

| Storage | Current evidence | Next evidence |
| --- | --- | --- |
| Actor `0x5C` and `0xDC` words | No named-field consumer in current C; raw and indirect consumers have not been excluded. | Trace allocation clearing, assembly references and resource/script access before classifying as padding. |
| Actor `0x102`, `0x122`, `0x142` two-byte gaps | Lie between established fields; alignment alone does not prove their purpose. | Exclude raw/indirect reads and writes before identifying padding. |
| Actor `0x208` float / kind configuration `0x34` | Creation copies the configuration value multiplied by object scale. No other named-field consumer establishes a physical dimension. | Trace raw/indirect consumers and compare authored kind records. |
| Actor `0x1A0` word | Approach setup stores a randomized floating value; its later role is unproven. | Trace action continuations and indirect consumers before assigning an angle or timing meaning. |

This table records evidence limits; it does not certify these bytes are unused.

## Actor `0x208` fixed-offset access audit

The linked executable's disassembly has one positive 520-byte (`0x208`)
load/store displacement: `swc1 $f4,520(s2)` at `0x8001E400` in
`actorCreateForKind` (`0x8001E1C4`). The source and resident Ghidra decompilation
agree that creation stores kind-configuration offset `0x34` multiplied by scale.
The negative 520-byte candidate at `0x8001392C` is a word store in
`actorPartInitializeAnimatedTextures`' command-buffer initialization after its
cursor advances by 0x280; it does not establish an actor-state reader.

Reproduce the displacement inventory from the accepted linked image with:

```sh
mips-linux-gnu-objdump -d build/pikachu.elf > /tmp/pikachu-layout-disassembly.txt
rg '\b(lwc1|swc1|lw|sw|lh|lhu|sh|lb|lbu|sb)\b.*520\(' /tmp/pikachu-layout-disassembly.txt
```

This scan does not prove the value is unused: biased bases, computed pointers,
indexed accesses, block copies and script consumers still need data-flow tracing.
Its size and scale operation do not establish a physical dimension. The source
comments therefore describe an unresolved scaled configuration value, retaining
`unknown208` and `unknown34` until stronger evidence exists.

## Typed VM creation interface

`scriptVmCreateWithCapacity` and `scriptVmCreate` now return owned `ScriptVm *`
contexts. The explicit-capacity creator uses a typed local for its configuration
fields and owned value stack; Ghidra at `0x80115114` confirms construction and
reset. The shared lifecycle header and definitions agree. Reset and release now also
accept typed contexts, preserving the target calling convention.

Actor creation's reused byte-pointer local requires an explicit conversion from
the typed result. A separate typed context local failed the ROM gate; the boundary
cast preserves matching and has a focused ledger entry. Ownership and release
requirements remain unchanged. Every accepted source-unit/header mutation passed
strict comparison against all 483 baseline objects.

## Typed VM reset and destruction

The lifecycle interface consistently accepts or returns `ScriptVm *`. Reset
accesses retained execution, stack and timing fields directly; destruction releases
the owned value stack and then the context, retaining borrowed resources. Ghidra
at `0x801151E4` confirms the two owned releases and null-context behavior. The
actor model destructor converts its retained N64 context address to the canonical
context pointer at the call boundary.

Typed reset, destruction, internal creation/reset calls and the external actor
destruction caller each preserve the full object baseline. The actor creator's
separate local-allocation constraint remains documented; no new lifecycle
exception is introduced.

## Typed borrowed-string stack interface

`scriptVmPushStringPointer` now accepts `ScriptVm *` and a borrowed `char *`
instead of integer addresses. Its shared header documents that string storage
must outlive use of the stacked value. Ghidra at `0x80116084` confirms construction
of a string value and insertion into the context's stack without copying text.

All seven collection/profile native-command callers explicitly convert their
retained N64 context addresses at this interface. Player, item, actor, dictionary
and area names remain borrowed; the bedroom catalog keeps its original signed
address load before conversion, preserving symbol and load representation. The
producer/header and caller source-unit changes each pass the strict baseline
objects without an added compatibility exception.

## Typed string pop and scheduled VM update

`scriptVmPopString` now accepts `ScriptVm *`, and all four text-command callers
use that shared declaration. Ghidra at `0x80116104` confirms direct borrowed
string pointers, borrowed string-table entries and the shared numeric conversion
buffer. The returned numeric text remains valid only until the next conversion;
the existing 32-byte formatting precondition is unchanged. The value-pop helper now also accepts the shared typed context.

`actorUpdateScriptVm` retains a typed context while advancing timers, servicing
waiting actions and retaining the next scheduling state. No timing, return-value
or state-transition behavior changes. Producer/header, callers and update-local
changes each preserve the strict object baseline without added exceptions.

## Typed numeric stack interfaces

Integer and floating-point push/pop functions now take `ScriptVm *` in their
definitions and shared stack header. Numeric payload and result types remain
unchanged. Ghidra at `0x801160B4` confirms integer pop removes a value and applies
the established numeric conversion. Callers retain their stack-capacity/type
preconditions; this recovery adds no bounds checks or conversion behavior.

All seven native-command source units and the interpreter use typed context
arguments at these interfaces, including nested calls. Retained integer context
addresses are converted explicitly at the call boundary. Every source unit
independently passed strict comparison against all 483 baseline objects after
its mutation; no new matching exception was needed.

## Typed value-stack, frame and expression helpers

Value push/pop, saved-depth and return-location helpers, operand and variable
readers, expression operators and decimal parsing now accept `ScriptVm *`.
Interpreter calls use that type directly; redundant byte/integer context casts
are removed where strict matching permits. The public value/operand declarations
retain original address notes and their existing capacity/type preconditions.

Ghidra at `0x801156A0` confirms variable-reference resolution through saved local
frame bases or global storage. Invalid frame/index behavior remains the documented
original constraint. All five external operand-reader calls use the shared typed
context boundary. Producer/header and caller units each preserve the strict
baseline; five-byte value copies and original expression evaluation order remain
unchanged. No matching exception was introduced.

## Native-command context and canonical actor object

`ScriptNativeCommand` describes dispatch with a typed `ScriptVm` context. Ghidra
at `0x8011BD50` confirms selection from the context's command table and passage
of the actor and retained context to the chosen resident target. Dispatch uses
that type directly without changing selectors or scheduling results.

All remaining `ActorScriptObject.scriptState` consumers now use
`RuntimeObjectPrefix.state.actor`, including message/item commands and target
interaction. The redundant opaque 0x54-byte object view and its duplicate checks
are removed; the common object prefix retains its size/state-offset checks.
The voice/VM walkthrough names the canonical member. Producer/header and each
consumer mutation preserve all strict baseline objects without added exceptions.

## Packed integer-value interface

`scriptValueGetInteger` now takes `ScriptValue *` consistently with the other
numeric value interfaces. It reads the byte tag and copies the four-byte payload
without an unaligned word access. Ghidra at `0x80115424` confirms integer-like
tags and floating conversion. All interpreter callers use the typed value or
explicitly view a complete five-byte bytecode/global operand as a value.

Compile-time checks establish tag offset zero and payload offset one alongside
the five-byte extent. The established integer tag uses its existing enum; tag
two remains an unresolved integer-like form and is not renamed from guesswork.
Typed producer/header, caller and packed-offset changes each preserve the strict
baseline without adding a matching exception or changing conversion behavior.

## Shared MIDI and cartridge-reader storage

`midi_reader_types.h` defines the active 0x434-byte cartridge-reader prefix and
0x444-byte MIDI cursor/cache view. The cached window, aligned buffer, completion
queue and completion slot are shared with the cartridge reader implementation.
Ghidra at `0x800076E0` confirms buffer setup and queue initialization; the existing
read path establishes the completion slot at prefix offset 0x430. Size and
member checks cover those boundaries. Initialized storage must remain stable.

`SequenceState` embeds `MidiReader`, replacing an opaque 0x444-byte array. Its
timing division remains at 0x444, now checked explicitly. The old private reader's
trailing four bytes were part of the standalone 0x438-byte stack buffer, beyond
the active fields; in sequence storage, that location holds division and its
neighboring unresolved bytes. The active prefix avoids claiming those bytes as
reader state. Existing standalone 0x438/0x448 storage contracts and stack-buffer
sizes are unchanged; the prefix is not a new allocation-size contract.

The stream prefix's bytes 0xD through 0xF remain unresolved rather than inferred
as padding. The shared type owns no sequence data; source storage stays borrowed.
All source/header mutations preserve the strict baseline objects.

## Typed MIDI opening and embedded-reader interfaces

MIDI opening now accepts `MidiReader *` in its shared declaration and definition.
Cartridge-reader initialization takes the shared active prefix; stream-byte reading
passes the embedded cartridge member directly. Ghidra at `0x80007808` confirms
cursor initialization, buffered-reader setup and format-zero/tick-timing checks.
Existing backing-storage and minimum-allocation contracts remain unchanged.

Sequence start, event decoding and loop mark save/restore pass `&state->reader`,
replacing enclosing-state casts. The standalone ROM-table path retains its
original stack buffer and explicitly views its active reader prefix. Both source
units preserve every baseline object after mutation, with no new matching
exception or change to timing, I/O or event parsing.

## Sequence playback fields and loop markers

The shared sequence header documents tempo units, bounded held-note tables,
per-channel sustain levels, saved loop state and repetition count. The event
reader retains up to 32 channel/key pairs and emits pending note/sustain releases.
Ghidra at `0x8000530C` confirms these consumers and the custom loop controllers.

`SEQUENCE_MARK_LOOP_START` and `SEQUENCE_MARK_LOOP_END` name controllers 0x66
and 0x67. They save/restore reader marks and control iteration/channel selection;
the existing numeric values and event flow are unchanged. Additional offset
checks cover note count, paired note arrays, sustain, loop mark and repetitions.
Opaque slots at 0x446, 0x44C and 0x5B4 retain their offsets and unresolved status;
no physical or padding role is inferred. Header/source changes preserve the
strict object baseline without new matching exceptions.

## Sequence event limit versus destination storage

The registered `eventLimit` replaces the misleading `eventCapacity` name. It is
the selected reader's limit, not a general allocation-size guarantee. MIDI SysEx
copying retains a leading status byte and can write `min(payload length, limit)
+ 1` bytes; Ghidra at `0x8000793C` confirms the loop. Meta/channel events and the
eight-byte generated volume event have different storage requirements.

Sequence control, callback and registration comments now require destination
storage meeting the selected reader's contract. The queued-control reader also
requires its existing minimum MIDI data limit. Volume quantization documents
that scaled inputs must be finite and fit signed 32-bit conversion before
clamping. No bounds check, allocation, I/O, timing or event encoding changes.
A new offset check preserves the one-byte event limit at 0x1F1.

A linear scan of the retail ROM found 45 header/track-compatible MIDI candidates;
all parsed successfully and contained zero SysEx events. This supports those raw
tracks only: it does not prove the complete registered/constructed input set,
and does not relax the reader's storage precondition. The current selected bank and direct registration path are verified below;
alternate or constructed input still requires the documented precondition. Both source-unit changes preserve the strict baseline objects.

## Selected cue bank and registered storage verification

Runtime initialization selects the bank at ROM `0xB140C0`. Its revision-one table
contains 45 offset/length pairs, and those pairs map exactly to the 45 MIDI tracks
previously scanned. The C playback path resolves bank entries, starts a sequence
and registers the four sequence slots with limit 20 and 20-byte event storage.
Generated reset, note/sustain release and volume controls use one, three and
eight bytes.

`make check-sequence-bank` uses `tools/verify_sequence_bank.py` to bound every
entry and track, walk supported events and check actual writes against storage.
For the selected retail bank it verifies 45 cues, zero SysEx events and a maximum
write of 18/20 bytes. The same check now runs in `make verify`; it does not alter
the ROM comparator or object baseline. Eight synthetic tests cover running
status, end-of-track handling, exact SysEx status-byte boundaries, truncated
resources, unsupported meta lengths and generated-control storage.

This proves the selected bank's storage requirement under the registered limit,
not arbitrary alternate banks or caller-constructed streams. Public playback
contracts now state the 20-byte limit and SysEx payload restriction. Allocation
sizes, parser behavior and playback code remain unchanged.

## Distinct cue-bank ROM window contract

The private cue resolver uses `CueBankRomWindow` with descriptive transfer-storage,
window-start, aligned-buffer and completion-queue/message fields. Initialization
and byte-read parameters name the reader and address. Additional checks cover
window start at 0x400 and aligned-buffer pointer at 0x404, alongside the existing
0x424-byte extent and queue/message checks. Ghidra at `0x80023388` confirms setup.
The linked retained symbol is at `0x800F0290`, satisfying its 16-byte alignment
precondition; its ELF symbol has no size, so alignment alone is not an allocation
size proof. Initialized storage must remain stable and reads must not overlap.

This window is distinct from `RomByteReaderPrefix`: its transfer storage starts
at offset zero with 0x400 bytes, while the word-aligned MIDI cache retains 0x410
bytes after its initial window word. Their buffer and queue offsets differ.
The cue record remains private because no other C unit shares it; the existing
reader implementations and allocations are preserved. Renamed fields and
parameters pass the full strict object baseline without a matching exception.

## Player interaction callback contract

All seven populated entries in `gPlayerInteractionActionHandlers` now implement
`PlayerInteractionAction`: borrowed player storage in, integer handled status out.
Slots 0, 3 and 4 remain null. The area callback interprets the player's first
three floats as its world position; the save-prompt callback ignores its input.
Other callbacks consume the player's selected target or held-item handle.
The controller is used by dispatch to test activation input and is not a callback
parameter. The callback type remains private to its sole source unit.

Ghidra ROM disassembly at `0x801100A8` shows the pickup wrapper calling
`0x8011271C` and returning without modifying `v0`. The callee returns zero on
failure and one on success. At `0x80110414` the dispatcher calls the selected
entry and likewise forwards `v0` unchanged. Thus an explicit pickup result
recovers existing behavior rather than supplying a new default result.
The seven written handlers, table entries, dispatch, and result tests now agree.
A two-parameter representation failed the ROM gate and was reverted; the
one-parameter representation passes all 483 original objects, including strict
relocation identities. The old callback exception is removed.

## Opaque record inventory

`python3 tools/layout_contracts.py` (or `--json`) reports offset-named `unknown`
scalar, pointer and array members in game struct/union definitions. The current
inventory is 53 declarations across 29 files. Comments, literals, ordinary
locals and locally supplied SDK files are excluded. This is a lexical inventory:
inactive preprocessor branches are included, and function-pointer declarations,
other opaque spellings, raw offsets and ownership evidence require separate
review. An inventory entry does not establish padding, unused storage or a
semantic role. Existing evidence and matching constraints remain in this audit
and the recovery ledger; these counts do not certify completed shared layouts.

## Typed tree visitor and comparator lifetimes

`TreeVisitor` now receives `TreeNode *` throughout all three traversal modes,
the object diagnostic/drawing/nearest-target visitors, and the item nearest
candidate visitor. The previously unused traversal-continuation hook uses the
same node representation. Callbacks read the entry's value directly rather than
recovering it from a byte pointer. The item callback's second parameter is named
`depth`, matching dispatch, rather than suggesting an arbitrary context pointer.

Ghidra at `0x800413B4` confirms child links at offsets zero and four and the
callback receiving the current node in `a0` and depth in `a1`. Checks now cover
`TreeNode.links` at zero and `TreeNode.value` at eight as well as its 12-byte size.
Comparator contracts explicitly describe key-versus-entry ordering, stable
ordering while linked, and the retained callback lifetime. The actual object and
item comparators return negative for a candidate preceding the entry and positive
for a candidate following it. Visitors borrow nodes and must not invalidate
values or mutate tree links; nested traversal remains unsupported.

The tree's words at zero and eight are cleared by initialization, but their
later roles remain unproven. They retain offset names rather than being called
unused or assigned padding semantics. The integer-versus-pointer key matching
exceptions in lookup consumers remain separate from this visitor contract.
Each changed C unit passed the full strict baseline before the next was edited.

## Managed-object allocation and presentation snapshot

`managed_object_types.h` now shares the 0x68-byte allocation previously private
to the object manager. It contains the 0x58-byte runtime prefix, a search-entry
pointer at 0x58 and an embedded 0xC-byte tree node at 0x5C. Ghidra at
`0x80008160` confirms allocation of 0x68 bytes, clearing the first 0x5C bytes,
and storing the allocation address plus 0x5C at offset 0x58. Registration supplies
the node key and links. The shared header documents ownership, detachment before
destruction, and the limits of shallow copies. Size and offset checks move with
the layout rather than being duplicated in consumers.

The presentation snapshot's formerly unidentified four bytes at 0x58 are that
search-entry pointer. Its field is now typed `TreeNode *`; checks relate its
offset and 0x5C-byte extent to `ManagedObject`. The snapshot borrows the original
entry and does not contain the embedded node. Saving still copies the same bytes;
restoration still restores selected transform and actor fields rather than
registering the snapshot as another object. Both C units pass all 483 objects.

The private interaction-proxy template also has a recognizable `ObjectTemplate`
prefix, but substituting that shared type changed defined symbol attributes in
`player_companion.o` and failed strict comparison despite identical ROM bytes.
That trial was reverted. Its original resource extent and representation remain;
a future conversion must preserve compiler-emitted symbol attributes as well as
the bytes and relocations. No comparator rule was relaxed.

The opaque-member inventory is now 52 declarations across 28 files; the decrease
comes from the presentation entry pointer's supported recovery. Other opaque
fields retain their documented uncertainty.

## Reviewed unresolved-layout ledger

`OPAQUE_LAYOUTS.json` records all 52 declarations in the lexical opaque-member
inventory, with source locations, declaration fingerprints, known layout facts,
reasons for retaining the representation, next evidence needed and matching
constraints. Record-specific investigations include kind 0xAF allocation and
initialization, computed actor accesses, glyph resource consumers, audio event
and controller paths, and resource-template symbol attributes. Defaults and
alignment are explicitly insufficient evidence for semantic names.

`make check-layouts` rejects new or changed declarations without reviews, stale
entries, duplicates and absent evidence fields. It is part of `make verify`.
Source-line movement is permitted; changing type, width, record or member
identity requires review. Initialized anonymous resources use their variable
names as stable owners. Tests cover these review boundaries in addition to
comment/string masking, nested records, SDK exclusion and local variables.

This ledger closes the missing per-declaration accounting for the inventory's
scope. It does not prove the declarations semantically resolved, audit every
possible opaque spelling or raw offset, or prove every callback lifetime.
Further layout recovery must follow the recorded evidence rather than treat
the presence of a review as proof of a complete type.

## Kind 0xAF scene-region waypoint state

The apparent path-object prefix is the complete `SceneRegionState`, rather than
an unrelated record with coincident offsets. The linked object-kind table entry
for 0xAF at `0x800791A8` has no initializer, the generic state destructor, and
`sceneRegionUpdateHighlight` (`0x8001F318`) as its update hook. The shared
region template at `0x80088E60` stores kind 0xAF at offset 0x34.
`sceneRegionCreate` (`0x8001F1F0`) performs the custom initialization and allocates
0x10 bytes: half-extents at zero/four, highlight at eight, borrowed waypoint
address at 0xC. Ghidra confirms these stores and the highlight consumer; its
inferred float type for the fourth argument is contradicted by the actual
pointer consumers and authored resources.

The four creation calls in `ochre_deep.c` pass resources at 0x8014A0EC,
0x8014A110, 0x8014A134 and 0x8014A164. Their linked coordinate triples contain
respectively two, two, three and two waypoints followed by (-1, -1, -1).
These agree with the kind-checked first-point and following-point queries.
`SceneRegionState.points` and the creation parameter are now floating-point
pointers, the scene resource declarations use floating-point arrays, and both
queries reuse the complete shared state. The duplicate opaque 12-byte prefix
is removed. Checks cover highlight at eight, points at 0xC and total size 0x10.

The object owns its state but borrows waypoint storage for its lifetime. Query
contracts retain nonnegative axis/index requirements and a complete negative-X
marker. The three bytes after the highlight flag remain unresolved; nothing in
this recovery establishes that they are unused or padding. The inventory is now
51 opaque declarations across 27 files. Every affected C unit passed the full
strict baseline before the next was changed.

## Sequence voice calculation and event-reader boundaries

Volume and pitch helpers now accept the established driver, sequence and voice
record types, with consistent declarations and all callers passing those types.
Their retained byte expressions preserve the previously identified scheduling
constraints; interface recovery is separate from replacing every internal load.
A typed voice-stop trial still failed the ROM gate, both with direct native-voice
access and a checked member-offset view, and was reverted.

The registration callback contract no longer calls its third argument a buffer
capacity: it is the reader-specific event limit. Callback and event storage must
be valid during registration, which requests an event immediately, and while
the sequence remains registered. Zero-delay runs must be finite. Completion is
represented by negative delay; the pending byte-length field is not a negative
end marker. Nonzero lengths must fit one byte, and lengths above 127 rely on
the target signed-byte conversion. The dispatcher reads that stored length as
an unsigned byte. The existing bounded retail-bank verifier covers the actual
20-byte destination and selected reader, rather than assuming the limit equals
the allocation size.

Ghidra at `0x80006068` confirms callback/context at eight/four, limit at 0x1F1,
destination pointer at 0x1F4, delay at 0x1F8 and result stored at 0x1F0. New
checks cover callback, context, destination and delay offsets; the driver master
volume offset 0x24 is also checked. No parser, event loop or allocation changes.

## Typed direction-animation API and mode contract

All eight direction-animation routines and the retained mode-update callback
now receive `DirectionAnimation *`. The public reset, update, next-pose and
start-motion declarations agree with the actor, item and action-command callers,
which pass embedded animation members directly. Internal state reads and writes
use the shared record; the private word-aligned `DirectionPoseCopy` remains
because its packed-pose copy representation is a documented matching constraint.

The callback table at 0x8009A6D8 contains null, 0x80032148 and 0x8003225C.
Ghidra at 0x80032448 confirms dispatch by mode at 0x70 and pose selection at
0x72. The first active handler performs requested motion, then sets mode two
and rates toward the default directions for half a second; the second advances
that return phase and restores both default poses. The new enum names stopped,
timed and returning modes, preserving the stored 16-bit width and original
values. The earlier vague description of mode two as retargeting is corrected
to return-to-default playback.

The enclosing actor/item owns pose and motion storage; update callbacks borrow
it for the call. Contracts require a valid mode and pose index zero or one.
Checks additionally cover the 0x10-byte angular-motion stride, pose offset zero
and duration at 0x74, alongside the existing total extent and mode/index checks.
Every changed source unit passed all 483 objects before the next semantic edit.

## Item bounce allowance and held-callback state

`ItemKind.initialBounceCount` replaces the offset-only `unk8` name. Item creation
copies catalog offset eight into the item state's bounceCount; terrain response
consumes and decrements that counter, clamping it at zero. Ghidra at 0x80113944
confirms the catalog word copied into state offset 0x34. The new offset check
preserves the catalog's original 0x74-byte stride. The held-kind callback helper
now accepts `ItemStatePrefix *`, consistently at its two callers; it selects the
callback using the established item kind without an internal state cast.
The collision callback contract now describes zero as suppressing the optional
automatic terrain response, rather than leaving result semantics unspecified.

Opaque inventory now also recognizes the shorter `unk` offset spelling. This
adds the catalog's unresolved halfword at 0x3E, word at 0x6C and byte at 0x70,
with individual next-evidence records. The inventory consequently increases to
54 declarations across 28 files; the increase reflects wider coverage, not new
unknown fields. Known bounce-count recovery does not assign meanings to those
three adjacent or trailing fields. Tests cover both placeholder spellings.
