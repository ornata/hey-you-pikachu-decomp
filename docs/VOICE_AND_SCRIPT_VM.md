# Voice interaction and actor scripts

Voice recognition produces a candidate command. Actor scripts decide what that
command means in the current activity. Between those stages, gameplay can suppress,
redirect, or modify delivery. Recognizing a word therefore does not guarantee a
particular response.

This guide describes the recovered C control flow. Names such as `ScriptVm` and
`voiceCommandTakeMessage` describe that behavior; they are not claims about the
original source names. The VM's debug strings call it "PPU"; here that means the
script interpreter, not an additional hardware processor.

## From speech to behavior

```mermaid
flowchart TD
    Request[Gameplay requests dictionary and listening mode] --> Session[Microphone session]
    Session --> VRU[Upload vocabulary and poll VRU]
    VRU --> Result[Pending recognition result]
    Result --> Command[Command type, value, and quality information]
    Command --> Messenger[Target-directed messenger effect]
    Messenger --> Mailbox[Actor's single pending-message slot]
    Mailbox --> VM[Actor script VM]
    VM --> Action[Native gameplay commands and action handlers]
    Action --> Sequence[Timed action sequencer and motion playback]
    VM --> Request
```

### 1. Select vocabulary and control listening

[Microphone requests](../src/game/microphone_requests.c) queue configuration for
processing by the controller thread. Returning from a request does not establish
that the requested dictionary has finished uploading or that recognition has begun.

The [microphone session](../src/game/microphone_session.c) manages device discovery,
controller-button triggers, amplifier state, dictionary changes, and recognition
requests. Trigger mode can be held or toggled; recognition can follow the trigger
or an explicit override.

A dictionary is a list of global word IDs. Each word has encoded text for the VRU
and associated gameplay metadata. During a dictionary change, the session suppresses
recognition requests, waits for the current read to become inactive, and registers
words in batches of ten. Recognition polling resumes when uploading is complete.
See `micSessionUpdate`, `micSessionUpdateDictionary`, and `voiceDictionaryUpload`.

### 2. Consume a recognition result

[Voice recognition](../src/game/voice_recognition.c) starts and polls VRU reads through
`osVoiceStartReadData` and `osVoiceGetReadData`. A completed read supplies candidate
indices, match distances, warning flags, voice level, and voice duration. Candidate
indices refer to the active dictionary; `voiceDictionaryGetWordId` resolves them to
global word IDs. Match distance is a device metric, not a probability.

`voiceRecognitionTakeResult` consumes the pending-result notification, even if its
output pointer is null. It returns the first answer index or -1 when no result is
pending. Consumers must check the answer count before treating an answer as valid.
The stored data is not a queue of every past utterance.

### 3. Translate and deliver a command

[Voice command processing](../src/game/voice_commands_text_panel.c) resolves word
metadata, rejects unusable word lookups, and adds quality flags. The prepared
six-word message has this meaning:

| Field | Prepared voice-command meaning |
| --- | --- |
| `type` | Word's gameplay command category |
| `arg0` | Word's gameplay command value |
| `arg1` | Device warnings, plus 0x400 for quiet input and 0x4000 for a match distance above the configured threshold |
| `arg2` | Voice level |
| `arg3` | Voice duration |
| `arg4` | Interaction candidate selected by `voiceCommandUpdate` |

The shared [ScriptMessage type](../include/script_message.h) defines this six-word
layout. These meanings apply to voice commands. Other script-message producers can
use arguments differently. The command type and value are metadata, not the raw
recognition index.

`voiceCommandLaunchEffect` checks whether gameplay permits delivery, chooses a
recipient, and creates a [messenger](../src/game/flipbook_messenger.c). Its visual
flight participates in delivery: homing or collision can lead to `messengerDeliver`,
which calls `actorPostScriptMessage`. Loss of a target or an expired lifetime can
prevent delivery. Special recipient/contact modes can modify the prepared message;
the table above describes it before those transformations.

### 4. Inspect and acknowledge the mailbox

The [actor mailbox](../src/ovl/actor_target_interaction.c) holds one pending message.
Posting normally fails when a message is already pending. A forced post replaces
it. A successful post copies the message; the caller need not retain its storage.

[Script message commands](../src/ovl/actor_script_messages_items.c) expose the pending
flag, type, and arguments to the VM. Inspecting fields leaves the flag unchanged.
`actorAcknowledgeScriptMessage` clears the pending flag and returns its previous
value; it does not erase the stored fields. Scripts should use the pending flag to
distinguish a new message from previously stored data.

### 5. Let the actor's program choose a response

The [actor VM](../src/ovl/script_vm.c) evaluates expressions, branches, accesses local
and global variables, calls script subroutines, and invokes native gameplay commands.
An actor retains its instruction position and execution context across updates.

Local frames belong to each VM. Frame -1 instead selects the shared bank of 500
packed `ScriptValue` records declared in `script_vm_globals.h`. Profile
initialization and `scriptVmResetGlobals` both clear the same 2,500-byte bank;
returning from a completed visit also requests that reset through bedroom
scenario selection. The bank is separate from each context's owned value stack.
Its indices and any borrowed string lifetimes remain caller responsibilities.

`actorRunScriptVm` runs until execution yields, transfers to an action handler,
finishes, or encounters an invalid instruction or missing context. Ordinary
instructions do not automatically yield once per frame. A program must reach a
yielding operation for other frame work to proceed.

Native commands are selected through the context's command table by
`actorScriptDispatchRegisteredCommand` in
[actor_script_action_commands.c](../src/ovl/actor_script_action_commands.c).
Commands consume arguments from the VM stack and can push a result. Their status
controls scheduling:

| Native-command result | Interpreter behavior |
| --- | --- |
| 0 | Continue interpreting immediately |
| 1 | Yield; resume bytecode on a later actor update |
| 2 | Transfer to the actor action handler |
| Other nonzero value | The interpreter treats it as a yield |

The actor update stores an execution state separately:

| VM state | Actor update behavior |
| --- | --- |
| 0 or 1 | Run bytecode |
| 2 | Advance the selected action handler |
| 4 | Finished; reset the execution state to 0 on the next update |
| 5 | Missing context or invalid opcode reported by the interpreter; a later update with a context resets the state to 0 |

An action handler can remain in state 2 across updates, then return 1 so bytecode
can resume on a later update. See `actorUpdateScriptVm`, `actorScriptYield`,
`actorScriptStartActionAndWait`, and `actorScriptWaitForActionResult`.

The lower-level [action sequencer](../src/game/action_script.c) is a separate system.
It schedules timed action requests, iteration waits, and sounds. Its callback return
values are not the actor VM's scheduling statuses. Motion playback is handled in
[motion_playback.c](../src/game/motion_playback.c).

## Recovered shared interfaces

The first type-recovery pass introduces these headers:

| Header | Recovered concepts |
| --- | --- |
| [`voice_recognition.h`](../include/voice_recognition.h) | Selected vocabulary, global word records, command metadata, and result-consumption contracts |
| [`voice_commands.h`](../include/voice_commands.h) | Command production/delivery contracts and gameplay quality flags |
| [`actor_script.h`](../include/actor_script.h) | Actor-state prefix, pending mailbox, and shared post/acknowledge contracts |
| [`script_vm.h`](../include/script_vm.h) | Five-byte values, known value tags, stack/return fields, execution states, and native-command callbacks |

`RuntimeObjectPrefix.state.actor` is the link at object offset `0x54`.
The recovered actor-state view names `vmAddress` at `0x1B8`,
`messengerContactMode` at `0x214`, `messagePending` at `0x218`, and
`message` at `0x21C`. Initialization establishes a `0x408`-byte actor-state
allocation. The separate runtime object prefix does not establish the actor
object's full allocation or transfer ownership of referenced resources.
Posting and acknowledgement at `0x80118E74` and `0x80119200`, together with the
script getters beginning at `0x8011C884`, establish the shared layout.

`ScriptValue` has byte alignment and a five-byte stride. Its named `data` view
contains a tag and four payload bytes; its `bytes` view supports whole-value
copies. Integer, float, string-index, and string-pointer tags are recovered.
Tag 2 is handled as integer-like by the interpreter but remains unnamed because
its distinct meaning has not been established. Do not replace the payload with
an aligned C word or numeric union: that changes the value stride.

`ScriptVm` retains the original `0x2C8` extent. The value-stack depth and pointer
are at `0x18` and `0x20`; saved depths, return offsets, and local-frame bases
start at `0x28`, `0x12C`, and `0x22C`. The command table at `0x2AC` contains
callbacks invoked with the actor; each command accesses its retained VM through
actor state. The retained table at `0x8012B400` has 278 handlers and reserved null
selector zero. All handlers share the actor-only callback signature.
`python3 tools/native_commands.py --json` lists selectors, function names,
addresses and source locations. Unknown regions remain explicitly unknown.

Each `VoiceWord` at `0x8010452C` occupies sixteen bytes: display name, device
text, command category, and command value. The former table symbols at
`0x80104534` and `0x80104538` are interior addresses of those last two fields,
not independent tables. Accessors at `0x80040CF8` and `0x80040D60` confirm the
stride and field addresses. Using a common record changes two object relocation
expressions from interior symbol addresses to record address plus field offset;
the linked instructions and complete ROM remain identical.

The shared interfaces preserve existing target calling conventions. In
particular, mailbox posting still accepts a 32-bit actor address. Unprototyped
VM helpers that receive surplus arguments have not been globally rewritten;
changing those calls would change the original instruction stream. These
remaining ABI questions and unknown fields are further recovery work, not
claims that all actor or VM types are understood.

## Conceptual interaction

This is explanatory pseudocode, not a decoded retail script or its actual syntax:

```text
request the dictionary for this activity

repeat:
    if no message is pending:
        yield
        continue

    read the message type, value, and quality flags
    acknowledge the message

    choose a response for the message and current gameplay state
    start that response and wait for its action to finish
```

Dictionary readiness, successful delivery, and an acceptable recognition result
are separate conditions. An actual program can choose how to handle each one.

## Bytecode reference and reading order

Script values occupy five bytes: a tag followed by a four-byte payload. Numeric
accessors treat integer tag 0 and code-reference tag 2 as integers; setters emit
0 for integers and 1 for floats. String access distinguishes table indices (tag 3) from borrowed pointers
(tag 4). The recovered code also contains conversions; consult the value accessors
before assigning meaning to another tag.

Instructions use a one-byte opcode with operands determined by the operation.
Selected structural opcodes in `actorRunScriptVm` are:

| Opcode | Operation |
| --- | --- |
| 10 | Store a variable |
| 24 / 25 | Push a literal / variable |
| 26 / 30 | Script call / return |
| 27 | Dispatch a native command |
| 28 / 29 | Unconditional jump / jump when false |
| 31 | Discard a value |
| 32 | Set up a local frame |
| 33 / 34 | Read / write an indexed value |
| 35 | End execution |

These numbers come from the interpreter switch. Native command selectors are a
separate namespace interpreted through the configured command table. This guide
does not identify every retail script, dictionary, or command-table entry.

Start with `micSessionUpdate`, then follow `voiceCommandUpdate`, `messengerDeliver`,
`actorPostScriptMessage`, and `actorRunScriptVm`. For a script command, follow its
native dispatch into the `actor_script_*.c` files and then into the gameplay helper
it calls. Existing function comments retain original decompilation identifiers and
addresses for comparison with disassembly.

The interpreter names its recovered byte operations with the private `ScriptOpcode`
enum in `src/ovl/script_vm.c`. Binary operands use the second-from-top stack value
as the left operand: opcode 13 is greater-than, while 14 is less-than. Floating
equality has separate named relative and absolute tolerances. Packed values use
`SCRIPT_VALUE_BYTES` and `SCRIPT_VALUE_PAYLOAD_BYTES` from `script_vm.h`; their
storage remains byte-aligned. Bytes zero through two each consume one opcode byte
without changing the value stack; a following instruction still executes normally.
`SCRIPT_VALUE_CODE_REFERENCE` (tag two) carries bytecode offsets and native-command
selectors in the retained configuration corpus. It follows the signed integer
reader and is rejected by the decimal-string parser, just like integer tag zero.
The name describes those observed roles; the original compiler terminology is
unknown. The optional [semantic review](SEMANTIC_REVIEW.md) covers those
contrasts alongside asymmetric integer and floating-equality scenarios.

Dictionary selection uses `VOICE_DICTIONARY_NONE`, and consuming a result without
a pending notification returns `VOICE_RECOGNITION_NO_RESULT`. Amplifier operation
arguments use `VoiceAmplifierSetting`; the retained amplifier state remains a
Boolean. The session limits uploads to `MIC_DICTIONARY_UPLOAD_BATCH_WORDS` per
update. Startup and injected-command/no-speech scenarios observe these paths
without establishing physical VRU fidelity. Device command-status codes whose
individual meanings remain unresolved retain their numeric forms.

## Held-item launch selectors

Cue operation `ACTION_CUE_LAUNCH_HELD_ITEM` (25) carries a signed launch-kind word
and a floating speed argument. Its private `ActionLaunchKind` enum describes
trajectories, separately from the cue operation, actor target mode and item mode.
For positive command speeds:

| Kind | Meaning |
| --- | --- |
| 0 | Aim horizontally at the player when following a live player target; otherwise use the actor's heading. |
| 1 | Use the actor's heading with an upward component. |
| 2 | Use the actor's heading plus 240 degrees with an upward component. |
| 3 | Use the actor's heading with a steep upward component. |
| 4 | Use the actor's heading with a downward component. |
| 5 | Aim from the item's world position toward the stored three-dimensional target. |
| 6 | Randomize speed and heading around actor heading plus 200 degrees. |
| 7 | Randomize heading over a full circle and emit linked and falling-particle effects. |

Kind 5 produces zero velocity when the target is too close to normalize. Kind 7
chooses horizontal speed independently of the command speed; the command still
scales its vertical component. Kinds outside zero through seven are unsupported.
The original branches and three launch tables establish these descriptions;
they do not establish animation intent, gameplay reachability or collision behavior.

## Script decoder

The independent Python reader supports three separate formats:

| Format | Representation | Consumer |
| --- | --- | --- |
| `vm` | One-byte opcodes with packed five-byte operands | `actorRunScriptVm` |
| `cue` | Twenty-byte timed cues and selection tags | `actorUpdateActionScript` |
| `sequence` | Sixteen-byte action-sequencer records | `actionScriptUpdate` |

Start with a newly authored example, without a ROM:

```sh
python3 -m tools.script_decoder vm --demo
python3 -m tools.script_decoder cue --demo
python3 -m tools.script_decoder sequence --demo
```

Each command writes `listing.txt` and `listing.json` under
`build/script-decoder/demo-FORMAT/`. The text shows labels, operand roles, candidate
branches/calls and source locations. JSON also records raw tag/payload encodings,
code-reference uses, opcode/tag counts, diagnostics and unparsed byte ranges.
No extracted retail scripts or strings are bundled with the tool or examples.

For your normalized USA ROM, supply an explicit window. For example, after a
matching build, the first retained action-sequencer program can be read with:

```sh
python3 -m tools.script_decoder sequence --symbol 0x8006EE60 \
  --elf build/pikachu.elf --bytes 0x20
```

Use `--rom-offset OFFSET --bytes LENGTH` for a known ROM range, or
`--symbol SYMBOL_OR_ADDRESS --elf build/pikachu.elf --bytes LENGTH` for a known
mapped program. Scene addresses require the exact `--section .scene_NAME`.
The tool authenticates mapped ELF section bytes against the supplied ROM;
it does not guess which overlay was active during a runtime capture.

For a VM configuration, use `vm --config SYMBOL_OR_ADDRESS --elf ... --bytes ...`.
This reads the established configuration prefix and follows its program pointer
within the explicit mappings. Add `--native-count COUNT` to bound its command
table. For a standalone VM program, native resolution instead takes
`--native-table SYMBOL_OR_ADDRESS --native-count COUNT`. Counts include any
reserved null slot. The retained `D_8012B400` table has 279 slots, including slot 0.
Alternative tables must be identified explicitly; their slots need not agree.

Native-handler links come from the authenticated table, not merely matching
selector numbers. Four retained text handlers also consume an inline packed
string-count operand. Their reviewed layouts preserve the next instruction
boundary. Unknown handler layouts stop that path; argument stack effects and
dynamic return locations remain unresolved. VM calls expose both their target
and return continuation, while conditional branches expose both possible paths.
Use repeated `--entry OFFSET` options to inspect additional known VM entry points.
Larger programs can raise the explicit budget with `--max-instructions 100000`.
VM targets are offsets from the program start: keep that start at window offset
zero rather than rebasing a fragment taken from the middle of a program.

Decompressed or captured script windows can be read with
`--input /path/to/local/script.bin --bytes LENGTH`, optionally adding
`--input-offset OFFSET`. Such inputs are explicitly marked as lacking ROM
authentication. Decompression, heap relocation, string extraction and automatic
resource discovery are outside this decoder. `--bytes` bounds a read; it does not
assert that the complete allocation or program size is known.
Raw VM inputs may still use `--elf`, `--native-table` and `--native-count` to resolve
handlers from your ROM. The report authenticates those tables separately and
continues to mark the input script as unauthenticated.

All listings stay under ignored `build/script-decoder/`; `--output` may select a
subdirectory there. Keep extracted content and generated listings local. The CLI
returns 0 for a decode without diagnostics, 1 for a partial decode with diagnostics,
and 2 for an input or mapping error. None of these results establishes execution,
gameplay reachability, resource ownership or a recovered source-language meaning.

To catalog the retained layout across the resident and scene sections:

```sh
python3 -m tools.script_decoder.catalog --elf build/pikachu.elf
```

The catalog authenticates every scanned section and keeps its overlay identity.
It finds aligned configuration candidates that reference the retained native table
and a local program preceding the header. A program-to-header window bounds the
decode; it is not an inferred allocation size. Each candidate retains diagnostics,
code fingerprints, frame operands, global-reference base indices, native calls and
code-reference roles. Direct `actorCreateForKind` calls are linked only when the
script argument is an exact configuration symbol or its address; 70 candidates
have such bindings. Nine more configurations have conditional local assignments
before actor creation; the catalog records these separately as possible sources,
without asserting branch feasibility. It also follows 47 explicit scenario
selector return indices through the 23 area configurations to Pikachu's scripts,
retaining the overlay at every pointer read. It validates the source area's
selection against the linked, ROM-authenticated catalog. These are configured
references, not a claim that
every scene or script branch has executed. Other candidates remain explicitly
unproven as live configurations. The results stay under
`build/script-decoder/catalog/`; this scan does not discover every possible script
layout or compressed resource. Two of the 128 candidates currently have no
recovered binding: resident configuration 0x8012B870 and front-yard configuration
0x80158B7C. This is not evidence that they are unused.

Decoder report schema 2 calls tag 2 `code_reference` and records its uses in
`code_reference_uses`. These references select bytecode offsets or configured
native-command slots; their payloads retain signed integer conversion behavior.
The catalog preserves separate entries for overlays even when virtual addresses
coincide, and fingerprints decoded code to expose repeated programs.

## Area scenario selection

`presentationInitializeForCurrentArea` selects `gAreaScenarioIndex` from story
progress. That index selects companion behavior, card textures and collection
titles together; it also affects scene population. The selector functions now
use area names and the `Scenario` suffix to describe this shared role.

The recovered literal-return domains are:

| Area or group | Scenario indices |
| --- | --- |
| Bedroom | 0–9 |
| Entryway | 0 |
| Front yard | 0–6 |
| Viridian entrance | 0–2 |
| Viridian forest | 0 |
| Viridian deep | 0, 1 |
| Springleaf entrance, Oddish meadow, Springleaf field | 0, 1 |
| Olivine lakeshore/lake, Ochre entrance/deep, Cobalt island/coast | 0 |
| Abra shop | 0, 1 |
| Wilderness camp and the three fishing areas | 0 |
| Viridian edge | 0, 1 |
| Playground | 0 |
| Credits | 0, 1 |

These domains bound the entries inspected by catalog schema 2; they do not prove
every return path is reachable or infer the complete table allocation. The
inventory rejects unresolved return expressions and unfamiliar area assignments.
`companion_scenario_sites` records each reference's selector and index.

For the forest edge, chapter 1 selects scenario 0 and chapter 7 selects scenario 1.
Both request presentation action 0xF4 and audio cue 0x15. Credits selects scenario 1
at chapter 0, setting audio to zero while preserving the caller's action; chapter
9 selects scenario 0 with action 0x32 and audio 0x15. Unsupported chapters return
zero while leaving those outputs unchanged. Six original-instruction experiments
check these outputs, return indices and fallback preservation in three interpreter
repetitions each. Unicorn remains unsupported for these cases. Other selector
domains currently rely on matching source and authenticated scene-table evidence.

## Actor mailbox categories

`ActorScriptMessageKind` names categories with established gameplay producers.
Scheduler notifications use the separate `GameMessageKind` domain.

| Category | Producer | Payload |
| --- | --- | --- |
| 14: item impact | `actorPostItemImpactMessage` | Item handle in arguments 0/4; speed magnitude in argument 2. |
| 15: cooperation request | `playerRequestTargetCooperation` | Selected interaction target handle in argument 4. |
| 16: action notification | `actionCommandNotifyStoredTarget` | Cue values in arguments 0/2/3; sender actor handle in argument 4. |
| 17: proxy used | `playerConsumeInteractionProxy` | Proxy metadata in argument 0, remaining uses in 2 and proxy handle in 4. |

The opening voice scenario executes program 0x8013C0E4. A read-only live capture
at poll 2086 matches all 29,924 bytes from that address to configuration
0x801435C8 in `.scene_viridian_edge`. The credits candidate at the same program
address differs at byte 10. Independently, all 19,105 conditional branches in the
retained observation window agree with the forest-edge decode. The program tests
categories 13, 15 and 14 before copying other categories into local slots. Matching
this byte window establishes the resource correspondence; it does not establish
its complete allocation, actor ownership, or the meaning of every variable.

The catalog's `script_message_posts` entries recover arguments for all 150
decoded calls to `actorScriptPostMessage` in the configuration census.
These counts include repeated configurations (120 distinct code/offset call
sites). Recovery follows at most 32 contiguous producer instructions with no
competing entry into their interior. Supported producers are integer literals,
variable reads, four reviewed handle getters and representable constant integer
addition. Other stack effects remain unresolved if encountered. The argument order is recipient handle,
type, argument 0, argument 2, argument 3 and argument 4. The native handler sets
argument 1 to zero, rejects missing recipients, requests unforced delivery and
pushes success. Variable operands retain their frame/slot coordinates; they are
not inferred actor identities or persistent variable names.

Of those recovered calls, 138 construct category 13 across 15 scene sections.
Their payloads vary, so category 13 and its subcommands still need recipient-side
analysis before receiving gameplay names. These category names establish
producer contracts; they do not identify every recipient
reaction or override arbitrary messages constructed by script code. Impact cases
check the exact mailbox payload and pending-message rejection with synthetic
velocities, without claiming collision or item-lifetime coverage.

### Resident attention exchange

The field's kind-3 resident uses
`gResidentAttentionScript_scene_springleaf_field` (configuration 0x801571E4).
Its creation is conditional on progress field 0x12. At script offset 466 it sends
Pikachu category 13, argument 0 equal to 1, and its own handle in argument 2.
The entrance's corresponding resource at 0x801573B8 sends the same payload at
offset 369. This supports naming these resources for their attention-request
role without assigning a creature or animation identity from numeric IDs.

For field scenario zero, the area table selects Pikachu configuration 0x80156884.
Its message intake copies categories 13/14/15 into frame-zero slots 9–13 in
type/argument-0/argument-2/argument-3/argument-4 order. On the category-13,
argument-0-equals-1 path at offsets 30487–30587, it reads the sender handle from
slot 11, tracks that object and calls the response routine at offset 29859.
That routine starts action 17 and sends the sender category 13 with argument 0
equal to 3 before selecting further reactions. Another path uses the same
incoming handle to establish tracking and selects local behavior state 7.
These are static script paths; runtime acceptance and rendered reactions have
not yet been observed for this exchange.

The field resident's seven frame-zero slots have these scoped roles:

| Slots | Role |
| --- | --- |
| 0–2 | Initial world position, retained for return positioning. |
| 3 | Whether the current chapter is 8, affecting the exit path. |
| 4 | Response count and exit threshold value; 8 ends the repetition. |
| 5 | Initial total-elapsed timestamp for the delayed start. |
| 6 | Per-attempt response stage, distinguishing the first reply and later sound. |

These roles describe this resource only. Equal slot numbers in other scripts
do not establish shared variables. The growth-resident reaction resource at
0x80156F04 in the entrance uses a different contract: category 1 starts an escape
reaction, while category 13 starts a Pikachu-facing action sequence and later
sends category 13 with argument 0 equal to 2. Its creation argument selects the
growth progress entry. Category 13 therefore cannot be assigned one universal
payload layout or reaction name from the attention exchange alone.

To compare a local live capture with the ROM-backed candidates:

```sh
python3 -m tools.script_decoder.live --runtime build/semantic-review/decoded-live-program/runtime.json
```

The voice runtime driver accepts `--capture-program-bytes 29924` to save one
read-only window per observed program address after poll 2000. Captures remain
under the local runtime output directory. The comparison authenticates the ROM,
ELF mappings, runtime ROM digest and captured-file digest, and reports every
same-address candidate separately. A shorter shared prefix can remain ambiguous.
