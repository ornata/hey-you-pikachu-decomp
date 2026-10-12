#include "common.h"
#include "keyed_pool_types.h"
#include "heap_alloc.h"
#include "node_pool.h"
#include "object_types.h"
#include "object_lookup.h"
#include "search_tree_types.h"
#include "midi_sequence_types.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata in source order. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

typedef struct Vec3f {
    f32 x, y, z;
} Vec3f;
void matrix34Identity(Vec3f *);
f32 vector3LengthApprox(Vec3f *);


void matrix34MakeViewFromEulerDegrees();
f32 matrix34GetTransformedPointYawDegrees(f32 *, f32 *);
/// Write cursor for the active graphics command list; also marks the camera-storage end.
/// \note Original identifier: \c D_800ABDD0 (0x800ABDD0).
extern Gfx *gRenderDisplayListCursor;
void objectDrawHierarchyForCurrentGroup(u8 *);
u8 romByteReaderRead();
u8 midiReaderReadByte(u8 *);
s32 midiReaderReadU16(u8 *);
s32 midiReaderReadU32(u8 *);
u8 *keyedPoolGetValue(void *, s32);
void _bzero(void *, u32);
void searchTreeFind(void *, void *);

/// Pending local movement accumulated during hierarchy traversal.
/// \note Original identifier: \c D_800ADA10 (0x800ADA10).
extern Vec3f gObjectPendingLocalMovement;
/// Current affine world transform shared by hierarchy traversal and actor motion consumers.
/// \note Original identifier: \c D_800AD9E0 (0x800AD9E0).
extern Vec3f gObjectTraversalTransform;
void matrix34TranslateLocal(void *, f32, f32, f32);          /* extern */
void matrix33ApplyRotationXYZDegrees(void *, f32, f32, f32); /* extern */
void matrix33ScaleLocal(void *, f32, f32, f32);              /* extern */
void matrix33KeepScaleOnly(f32 *);                    /* extern */
s32 romByteReaderInitialize(u8 *);
s32 animationIntegerTrackEvaluate(f32 *, s32 *, f32);
void objectAllocateHierarchyMatrixBuffers(s32);
/// Segmented shared display list preparing object hierarchy drawing.
/// \note Original identifier: \c D_100C890 (0x100C890).
extern u8 gObjectDrawSetupDisplayList[];
typedef struct Mtx48 {
    s32 w[12];
} Mtx48;
/// Saved affine transforms for nested object hierarchy traversal.
/// \note Original identifier: \c D_800ADA20 (0x800ADA20).
extern Mtx48 gObjectTransformStack[];
/// Number of saved object traversal transforms.
/// \note Original identifier: \c D_800AE380 (0x800AE380).
extern s32 gObjectTransformStackDepth;
void objectResetTraversalTransformAndMovement(void);
void objectTransformStackPush(Mtx48 *);
void objectTransformStackPop(Mtx48 *);
void objectTransformStackPeek(Mtx48 *);
s32 objectAccumulateHierarchyCount(u8 *, s32);
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;
/// Separate root-node symbol within the runtime object search tree.
/// \note Original identifier: \c D_800AE3C4 (0x800AE3C4).
extern s32 gObjectSearchTreeRoot;
/// Object flag group selected by the current drawing traversal.
/// \note Original identifier: \c D_800AE3D4 (0x800AE3D4).
extern s32 gObjectDrawGroup;
s32 searchTreeVisitAscending();
s32 objectDrawSearchTreeEntry();
/// Camera slot selected for the current drawing context.
/// \note Original identifier: \c D_800ABDD8 (0x800ABDD8).
extern s32 gRenderCameraIndex;
f32 vector3GetYawFromPositiveXDegrees(f32 *, u8 *);
void objectRequestHierarchyDestruction(u8 *);
/// Ordered traversal pool of registered active hierarchy roots.
/// \note Original identifier: \c D_800AE3A8 (0x800AE3A8).
extern NodePool gActiveObjectHierarchies;
void objectResponsesDiscard(s32);
void heapFreeLocal();
void searchTreeRemove(void *, u8 *);
void keyedPoolRemoveFirstForKey(void *, s32);
/// Object callback table viewed at destruction callbacks, with twenty-four bytes per kind.
/// \note Original identifier: \c D_80078144 (0x80078144).
extern u8 gObjectKindDestroyEntries[];
/// Owned keyed pool mapping runtime handles to live object addresses.
/// \note Original identifier: \c D_800AE388 (0x800AE388).
extern u8 gObjectHandlePool[];
/// Search tree indexing registered runtime objects by kind and handle order.
/// \note Original identifier: \c D_800AE3C0 (0x800AE3C0).
extern u8 gObjectSearchTree[];
/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern u8 gGameHeap[];
u8 *objectCreateFromTemplate(u8 *);
void objectDestroy(u8 *);
u8 *objectCloneTemplateHierarchy(u8 *, u8 *);
typedef struct CamRec {
    f32 x;
    f32 y;
    f32 z;
    u8 pad[0xD4];
} CamRec;
/// Camera storage used for view transforms, viewports and color overlays.
/// \note Original identifier: \c D_800ABCF0 (0x800ABCF0).
extern u8 gRenderCameras[];
void objectAssignHierarchyRoot(u8 *, s32);
/// Number of hierarchy roots registered in the active traversal pool.
/// \note Original identifier: \c D_800AE3D0 (0x800AE3D0).
extern s32 gActiveObjectHierarchyCount;
u8 *objectRegisterActiveHierarchy(u8 *);
void objectSetHierarchyFlags(u8 *, u16);
void objectClearHierarchyFlags(u8 *, u16);
s32 objectCompareSearchTreeOrder(u8 *, u8 *);
void objectRegisterHierarchyInSearchTree(u8 *);
f32 vector3Normalize(f32 *);
f32 objectOrientAlongWorldSegment(u8 *, u8 *, u8 *);
f32 objectAdjustPitchAlongWorldSegment(u8 *, u8 *, u8 *);
f32 mathUnitDirectionDegrees(f32, f32);
void matrix34MakeRotationZYXDegrees(void *, f32, f32, f32);
void vector3GetEulerDegrees(f32 *, f32 *, f32 *, f32 *);
void matrix34TransformPointVector(void *, f32 *, f32 *);



/* MIDI track reader: start/position/length of the track data, plus the running
 * status byte restored for data bytes that omit a status. */
typedef struct MidiReader {
    u32 start;
    u32 position;
    u32 length;
    u8 runningStatus;
} MidiReader;

/// Buffered cartridge-reader storage with room to align the DMA buffer internally.
typedef struct RomByteReader {
    s32 windowStart;
    u8 storage[0x410];
    s32 alignedBuffer;
    OSMesgQueue queue;
    OSMesg completion;
    u8 unknown434[4];
} RomByteReader;

/// Object-manager allocation containing the hierarchy prefix and its tree entry.
/// The entry pointer initially selects the embedded node at offset 0x5C.
typedef struct ManagedObject {
    RuntimeObjectPrefix base;
    TreeNode *entry;
    TreeNode node;
} ManagedObject;

typedef char RomByteReaderSizeCheck[(sizeof(RomByteReader) == 0x438) ? 1 : -1];
typedef char RomReaderBufferOffsetCheck[((u32)&((RomByteReader *)0)->alignedBuffer == 0x414) ? 1 : -1];
typedef char ManagedObjectSizeCheck[(sizeof(ManagedObject) == 0x68) ? 1 : -1];
typedef char ManagedObjectEntryOffsetCheck[((u32)&((ManagedObject *)0)->entry == 0x58) ? 1 : -1];

/// Resolve an indexed ROM table entry to its data address.
/// \param base Cartridge address of a valid ROM table; its selected entry must identify readable data.
/// \param idx Valid entry index.
/// \returns The data address identified by the selected entry.
/// \note Original decompilation identifier: \c func_80007600 (0x80007600).
u8 *romTableResolveEntry(u8 *base, s32 idx) {
    u8 reader[0x438];
    u8 b3;
    u8 b2;
    u8 b1;

    romByteReaderInitialize(reader);
    romByteReaderRead(reader, base);
    romByteReaderRead(reader, base + 1);
    romByteReaderRead(reader, base + 2);
    romByteReaderRead(reader, base + 3);
    b1 = romByteReaderRead(reader, &((u8(*)[8])base)[idx][6]);
    b2 = romByteReaderRead(reader, &((u8(*)[8])base)[idx][5]);
    b3 = romByteReaderRead(reader, &((u8(*)[8])base)[idx][4]);
    base += romByteReaderRead(reader, &((u8(*)[8])base)[idx][7]) | ((u32)b3 << 24) | (b2 << 16) | (b1 << 8);
    return base;
}

/// Initialize a buffered cartridge byte reader.
/// \param reader Caller-owned, word-aligned storage of at least 0x438 bytes, retained until all reads finish.
/// \returns Zero.
/// \note Portability: Reader addresses are carried as 32-bit integers and the buffered cartridge path uses the N64 DMA interface.
/// \note Original decompilation identifier: \c func_800076E0 (0x800076E0).
s32 romByteReaderInitialize(u8 *reader) {
    ((RomByteReader *)reader)->windowStart = 0x80000000;
    ((RomByteReader *)reader)->alignedBuffer = (s32)(((u32)(reader + 0x13) >> 4) * 0x10);
    osCreateMesgQueue(reader + 0x418, reader + 0x430, 1);
    return 0;
}

/// Read a byte from cartridge ROM.
/// \param r Initialized buffered reader; reads sharing this reader must not overlap.
/// \param addr Readable cartridge byte address; its aligned read window must also be available.
/// \returns The byte at the requested address.
///
/// \note Portability: The unprototyped midiReaderReadByte call passes an extra argument ignored by the original ABI.
/// \note Original decompilation identifier: \c func_80007728 (0x80007728).
u8 romByteReaderRead(r, addr)
u8 *r;
s32 addr;
{
    OSIoMesg io;
    OSMesg msg;

    if (addr < *(s32 *)r || addr >= *(s32 *)r + 0x400) {
        *(s32 *)r = addr & ~0xF;
        osInvalDCache(*(void **)(r + 0x414), 0x400);
        osPiStartDma(&io, OS_MESG_PRI_HIGH, OS_READ, *(s32 *)r, *(void **)(r + 0x414), 0x400,
                     (OSMesgQueue *)(r + 0x418));
        while (osRecvMesg((OSMesgQueue *)(r + 0x418), &msg, OS_MESG_NOBLOCK) != 0) {
        }
    }
    return *(*(u8 **)(r + 0x414) + addr - *(s32 *)r);
}

/// Open a single-track format-zero MIDI sequence using tick-based timing.
/// \param reader Caller-owned, word-aligned storage of at least 0x448 bytes, retained while the sequence is read.
/// \param address Complete sequence in cartridge ROM or readable memory; its track extent must fit in signed 32 bits.
/// \returns The ticks-per-quarter-note division, or zero when the header is unsupported or invalid.
/// \note Original decompilation identifier: \c func_80007808 (0x80007808).
s32 midiReaderOpen(u8 *reader, s32 address) {
    s32 new_var;
    s32 sp20;
    s32 division;
    *((s32 *)(((s8 *)reader) + 0)) = address;
    *((s32 *)(((s8 *)reader) + 8)) = 0x16;
    *((s8 *)(((s8 *)reader) + 0xC)) = 0;
    *((s32 *)(((s8 *)reader) + 4)) = address;
    romByteReaderInitialize(reader + 0x10);
    if (midiReaderReadU32(reader) != 0x4D546864) {
        return 0;
    }
    if (midiReaderReadU32(reader) != 6) {
        return 0;
    }
    if (midiReaderReadU16(reader) != 0) {
        return 0;
    }
    if (midiReaderReadU16(reader) != 1) {
        return 0;
    }
    division = midiReaderReadU16(reader);
    sp20 = division;
    if (division & 0x8000) {
        return 0;
    }
    if (midiReaderReadU32(reader) != 0x4D54726B) {
        return 0;
    }
    new_var = midiReaderReadU32(reader);
    *((s32 *)(((s8 *)reader) + 8)) = (s32)((*((s32 *)(((s8 *)reader) + 8))) + new_var);
    return sp20;
}

/// Restore a saved MIDI stream position and running status.
/// \param reader Opened reader to reposition within the same sequence as the saved mark.
/// \param mark Previously saved, word-aligned mark containing at least five readable bytes.
/// \returns 1.
/// \note Original decompilation identifier: \c func_8000790C (0x8000790C).
s32 midiReaderRestoreMark(u8 *reader, u8 *mark) {
    ((MidiReader *)reader)->position = (s32)((SequenceMark *)mark)->position;
    ((MidiReader *)reader)->runningStatus = (u8)((SequenceMark *)mark)->runningStatus;
    return 1;
}

/// Save a MIDI stream position and running status.
/// \param reader Reader whose current state is saved.
/// \param mark Word-aligned destination with room for at least five bytes; must not overlap the reader.
/// \returns 1.
/// \note Original decompilation identifier: \c func_80007924 (0x80007924).
s32 midiReaderSaveMark(u8 *reader, u8 *mark) {
    ((SequenceMark *)mark)->position = (s32)((MidiReader *)reader)->position;
    ((SequenceMark *)mark)->runningStatus = (u8)((MidiReader *)reader)->runningStatus;
    return 1;
}

s32 midiReaderReadVariableLength(u8 *);





/// Consume and decode the next supported MIDI event.
/// Requires complete channel-voice, SysEx, or meta events, valid running status, and meta lengths below 128.
/// The sequence storage must remain readable through the event, including any discarded payload.
/// \param reader Opened MIDI reader with its original backing storage and no overlapping reads.
/// \param event Output buffer, separate from reader and size, with room for at least capacity + 1 bytes.
/// \param size Receives the full event length, which may exceed the retained data; unchanged at end of track.
/// \param capacity Payload limit for SysEx events and total byte limit for meta events; must be at least three.
/// \returns The event delay in MIDI ticks, or -1 when already at the end of track.
/// \note Original decompilation identifier: \c func_8000793C (0x8000793C).
s32 midiReaderReadEvent(MidiReader *reader, u8 *event, s32 *size, s32 capacity) {
    s32 delta;
    s32 length;
    s32 i;
    s32 j;
    if (reader->position >= reader->start + reader->length)
        return -1;
    delta = midiReaderReadVariableLength((u8 *)reader);
    event[0] = midiReaderReadByte((u8 *)reader);
    switch (event[0]) {
    case 0xF0:
    case 0xF7:
        length = midiReaderReadVariableLength((u8 *)reader);
        for (i = 0; i < length && i < capacity; i++)
            event[i + 1] = midiReaderReadByte((u8 *)reader);
        for (; i < length; i++)
            midiReaderReadByte((u8 *)reader);
        *size = (s32)((u32)length + 1U);
        break;
    case 0xFF:
        event[1] = midiReaderReadByte((u8 *)reader);
        event[2] = midiReaderReadByte((u8 *)reader);
        reader->runningStatus = 0;
        for (j = 3; j < event[2] + 3 && j < capacity; j++)
            event[j] = midiReaderReadByte((u8 *)reader);
        for (; j < event[2] + 3; j++)
            midiReaderReadByte((u8 *)reader);
        *size = j;
        break;
    default:
        if (event[0] & 0x80) {
            event[1] = midiReaderReadByte((u8 *)reader);
            reader->runningStatus = event[0];
        } else {
            event[1] = event[0];
            event[0] = reader->runningStatus;
        }
        if ((event[0] & 0xF0) == 0xC0 || (event[0] & 0xF0) == 0xD0)
            *size = 2;
        else
            *size = 3;
        if (*size == 3)
            event[2] = midiReaderReadByte((u8 *)reader);
        break;
    }
    return delta;
}



/// Read and consume the next MIDI stream byte.
/// \param reader Initialized reader with a readable byte at its current position.
/// \returns The next stream byte.
/// \note Portability: Address bit 31 selects directly readable memory; addresses and stream positions use the original 32-bit N64 address model.
/// \note Original decompilation identifier: \c func_80007B9C (0x80007B9C).
u8 midiReaderReadByte(u8 *reader) {
    u8 *p = *(u8 **)(reader + 4);
    u8 r;

    if ((u32)p & 0x80000000) {
        r = *p;
        *(u8 **)(reader + 4) = p + 1;
        return r;
    }
    return romByteReaderRead(reader + 0x10, (*(u8 **)(reader + 4) = p + 1) - 1, reader);
}

/// Read and consume a big-endian unsigned 16-bit MIDI stream value.
/// \param reader Reader positioned at two available bytes.
/// \returns The decoded unsigned value.
/// \note Original decompilation identifier: \c func_80007BE4 (0x80007BE4).
s32 midiReaderReadU16(u8 *reader) {
    s32 v;

    v = midiReaderReadByte(reader) << 8;
    v |= midiReaderReadByte(reader);
    return v & 0xFFFF;
}

/// Read and consume a big-endian 32-bit MIDI stream value.
/// \param reader Reader positioned at four available bytes.
/// \returns The decoded bit pattern represented as s32.
/// \note Original decompilation identifier: \c func_80007C1C (0x80007C1C).
s32 midiReaderReadU32(u8 *reader) {
    u32 v;

    v = (u32)midiReaderReadByte(reader) << 0x18;
    v |= midiReaderReadByte(reader) << 0x10;
    v |= midiReaderReadByte(reader) << 8;
    v |= midiReaderReadByte(reader);
    return (s32)v;
}

/// Read and consume a MIDI variable-length quantity.
/// \param reader Reader positioned at a complete MIDI variable-length quantity of at most four bytes.
/// \returns The decoded quantity.
/// \note Original decompilation identifier: \c func_80007C74 (0x80007C74).
s32 midiReaderReadVariableLength(u8 *reader) {
    s32 firstByte;
    s32 nextByte;
    u32 var_s0;

    firstByte = midiReaderReadByte(reader);
    var_s0 = firstByte;
    if (firstByte & 0x80) {
        var_s0 = firstByte & 0x7F;
        do {
            nextByte = midiReaderReadByte(reader);
            var_s0 = (var_s0 << 7) + (nextByte & 0x7F);
        } while (nextByte & 0x80);
    }
    return (s32)var_s0;
}



/// Provide the inactive per-object diagnostic report hook.
/// \param objectAddress Address of the object to report; unused in this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80007CD0 (0x80007CD0).
void objectDebugReportNoOp(s32 objectAddress) {
    TRACE("nodeObj:%p  objType:%d objID:%d level:%d\n");
}

/// Provide the inactive object diagnostic row report hook.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80007CD8 (0x80007CD8).
void objectDebugReportRowNoOp(void) {
    TRACE("%4d ");
    TRACE("%4d ");
    TRACE("%s\n");
    TRACE("\n");
}

/// Provide an inactive object-manager hook.
/// \param unused Unused argument retained by the hook's interface.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80007CE0 (0x80007CE0).
void objectManagerNoOp7CE0(s32 unused) {
}


/// Provide the inactive diagnostic visitor for an object search-tree entry.
/// \param entryAddress Address of the entry to report; unused in this build.
/// \param depth Traversal depth to report; unused in this build.
/// \returns Zero to continue traversal.
/// \note Original decompilation identifier: \c func_80007CE8 (0x80007CE8).
s32 objectDebugVisitSearchTreeEntryNoOp(s32 entryAddress, s32 depth) {
    TRACE("node:%p level:%d\n", entryAddress, depth);
    return 0;
}

/// Visit object search-tree entries for diagnostic reporting, which is inactive in this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80007CF8 (0x80007CF8).
void objectDebugVisitSearchTree(void) {
    searchTreeVisitAscending(gObjectSearchTreeRoot, objectDebugVisitSearchTreeEntryNoOp);
}

/// Provide the inactive object-manager summary report hook.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80007D24 (0x80007D24).
void objectManagerDebugReportNoOp(void) {
    TRACE("========= Objmanager Info =========\n");
    TRACE("hierarchy total = %d\n");
}

/// Provide an inactive object-manager hook with a zero result.
/// \param unusedFirst Unused first argument retained by the hook's interface.
/// \param unusedSecond Unused second argument retained by the hook's interface.
/// \returns Always zero.
/// \note Original decompilation identifier: \c func_80007D2C (0x80007D2C).
s32 objectManagerReturnZero7D2C(s32 unusedFirst, s32 unusedSecond) {
    return 0;
}

/// Provide the inactive object hash diagnostic report hook.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80007D3C (0x80007D3C).
void objectHashDebugReportNoOp(void) {
    TRACE("-------- objHash Info --------\n");
    TRACE("\n");
    TRACE("\n");
}

/// Provide an inactive object-manager hook.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80007D44 (0x80007D44).
void objectManagerNoOp7D44(void) {
}

/// Provide an inactive object-manager hook.
/// \param unused Unused argument retained by the hook's interface.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80007D4C (0x80007D4C).
void objectManagerNoOp7D4C(s32 unused) {
}

void objectResponsesDebugReportNoOp(); /* extern */
void collisionGridDebugReportNoOp(); /* extern */

/// Request object-response and collision diagnostics, which are inactive in this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80007D54 (0x80007D54).
void objectDebugReportResponsesAndCollisions(void) {
    objectResponsesDebugReportNoOp();
    collisionGridDebugReportNoOp();
}

/// Prepare the object search tree around a requested object key.
/// \param kind Object kind to locate.
/// \param handle Object handle to locate within the kind.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80007D7C (0x80007D7C).
void objectPrepareSearchTreeAtKey(u16 kind, s32 handle) {
    u8 msg[0x5C];

    _bzero(msg, 0x5C);
    *(u16 *)(msg + 0x34) = kind;
    *(s32 *)(msg + 0x38) = handle;
    searchTreeFind(gObjectSearchTree, msg);
}

s32 objectGetNextHandle(void);

/// Prepare the object search tree with distributed handle searches for a requested kind.
/// \param kind Object kind for the searches.
/// \param searchCount Requested number of distributed searches; values below two perform only the initial search. The object search tree must be nonempty.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80007DC8 (0x80007DC8).
void objectPrepareDistributedKindSearches(u16 kind, s32 searchCount) {
    u32 now = objectGetNextHandle();
    u32 start;
    s32 i;

    objectPrepareSearchTreeAtKey(kind, 0);
    start = *(s32 *)(*(u8 **)(gObjectSearchTreeRoot + 8) + 0x38);
    for (i = 1; i < searchCount; i++) {
        objectPrepareSearchTreeAtKey(kind, (now - start) * i / searchCount + start);
    }
}

/// Separate symbol for the handle pool next automatically assigned identifier.
/// \note Original identifier: \c D_800AE3A0 (0x800AE3A0).
extern s32 gObjectNextHandle;

/// Get the handle that will be assigned to the next successfully created object.
/// \returns Next object handle.
/// \note Original decompilation identifier: \c func_80007E80 (0x80007E80).
s32 objectGetNextHandle(void) {
    return gObjectNextHandle;
}

/// Find an object that is still available for use.
/// \param handle Handle identifying the requested object.
/// \returns Borrowed object with the requested handle, valid until its destruction, or null if absent or pending destruction.
/// \note Original decompilation identifier: \c func_80007E8C (0x80007E8C).
u8 *objectFindLiveByHandle(s32 handle) {
    u8 *r = keyedPoolGetValue(gObjectHandlePool, handle);

    if (r == NULL || (((RuntimeObjectPrefix *)r)->flags & 0x30)) {
        return NULL;
    }
    return r;
}

/// Count objects in a hierarchy and add them to an existing count.
/// \param object First object in a live, acyclic sibling and descendant hierarchy, or null.
/// \param count Initial count; the resulting total must fit in signed 32 bits.
/// \returns Initial count plus the number of objects visited.
/// \note Original decompilation identifier: \c func_80007ED8 (0x80007ED8).
s32 objectAccumulateHierarchyCount(u8 *object, s32 count) {
    u8 *p = object;
    while (p != NULL) {
        count += 1;
        if (*(u8 **)(p + 0x2C) != NULL) {
            count = objectAccumulateHierarchyCount(*(u8 **)(p + 0x2C), count);
        }
        p = *(u8 **)(p + 0x28);
    }
    return count;
}

/// Save a transform on the object traversal stack.
/// \param transform Transform to save; the stack must have available capacity.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80007F34 (0x80007F34).
void objectTransformStackPush(Mtx48 *transform) {
    gObjectTransformStack[gObjectTransformStackDepth] = *transform;
    gObjectTransformStackDepth += 1;
}

/// Restore and remove the most recently saved object transform.
/// \param transform Output receiving the saved transform; the stack must be nonempty.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80007F90 (0x80007F90).
void objectTransformStackPop(Mtx48 *transform) {
    gObjectTransformStackDepth -= 1;
    *transform = gObjectTransformStack[gObjectTransformStackDepth];
}

/// Read the most recently saved object transform without removing it.
/// \param transform Output receiving the saved transform; the stack must be nonempty.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80007FF0 (0x80007FF0).
void objectTransformStackPeek(Mtx48 *transform) {
    *transform = gObjectTransformStack[gObjectTransformStackDepth - 1];
}

/// Remove the most recently saved object transform from a nonempty traversal stack.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80008048 (0x80008048).
void objectTransformStackDiscardTop(void) {
    gObjectTransformStackDepth -= 1;
}

/// Get the movement accumulated for the current object update.
/// \param movement Receives the pending three-component movement in object-local coordinates.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80008060 (0x80008060).
void objectGetPendingLocalMovement(u8 *movement) {
    ((Vec3f *)movement)->x = (f32)gObjectPendingLocalMovement.x;
    ((Vec3f *)movement)->y = (f32)gObjectPendingLocalMovement.y;
    ((Vec3f *)movement)->z = (f32)gObjectPendingLocalMovement.z;
}

/// Replace the movement pending for the current object update.
/// \param movement Three-component movement in object-local coordinates.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80008084 (0x80008084).
void objectSetPendingLocalMovement(u8 *movement) {
    gObjectPendingLocalMovement.x = (f32)((Vec3f *)movement)->x;
    gObjectPendingLocalMovement.y = (f32)((Vec3f *)movement)->y;
    gObjectPendingLocalMovement.z = (f32)((Vec3f *)movement)->z;
}

/// Add movement to the current object update.
/// \param movement Three-component movement to add in object-local coordinates.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800080A8 (0x800080A8).
void objectAddPendingLocalMovement(Vec3f *movement) {
    gObjectPendingLocalMovement.x += movement->x;
    gObjectPendingLocalMovement.y += movement->y;
    gObjectPendingLocalMovement.z += movement->z;
}

/// Find the final object in a sibling chain.
/// \param object Non-null object starting the sibling-chain search.
/// \returns Last sibling, including the supplied object when it has no following sibling.
/// \note Original decompilation identifier: \c func_800080E4 (0x800080E4).
u8 *objectGetLastSibling(u8 *object) {
    while (*(u8 **)(object + 0x28) != NULL) {
        object = *(u8 **)(object + 0x28);
    }
    return object;
}

/// Insert an object after another object in a sibling chain.
/// \param previous Object after which the new object is inserted.
/// \param object Live object to insert; must not already occur in the resulting chain.
/// \returns Inserted object.
/// \note Original decompilation identifier: \c func_80008108 (0x80008108).
u8 *objectInsertSiblingAfter(u8 *previous, u8 *object) {
    ((RuntimeObjectPrefix *)object)->sibling.bytes = (u8 *)((RuntimeObjectPrefix *)previous)->sibling.bytes;
    ((RuntimeObjectPrefix *)previous)->sibling.bytes = object;
    return object;
}

/// Detach an object from a sibling chain.
/// \param previous Object immediately preceding object in this chain, or null when the caller maintains its head link.
/// \param object Object whose sibling link is detached.
/// \returns Former next sibling address, or zero when absent.
/// \note Original decompilation identifier: \c func_8000811C (0x8000811C).
s32 objectDetachSiblingLink(u8 *previous, u8 *object) {
    s32 nextSiblingAddress;

    nextSiblingAddress = ((RuntimeObjectPrefix *)object)->sibling.address;
    if (previous != NULL) {
        ((RuntimeObjectPrefix *)previous)->sibling.address = nextSiblingAddress;
    }
    ((RuntimeObjectPrefix *)object)->sibling.address = 0;
    return nextSiblingAddress;
}

/// Insert an object after another object along the child links.
/// \param previous Object after which the new object is inserted.
/// \param object Live object to insert; must not already occur in the resulting chain.
/// \returns Inserted object.
/// \note Original decompilation identifier: \c func_80008134 (0x80008134).
u8 *objectInsertChildLinkAfter(u8 *previous, u8 *object) {
    ((RuntimeObjectPrefix *)object)->child.bytes = (u8 *)((RuntimeObjectPrefix *)previous)->child.bytes;
    ((RuntimeObjectPrefix *)previous)->child.bytes = object;
    return object;
}

/// Detach an object from a chain of child links.
/// \param previous Object immediately preceding object in this chain, or null when the caller maintains its head link.
/// \param object Object whose child link is detached.
/// \returns Former next child address, or zero when absent.
/// \note Original decompilation identifier: \c func_80008148 (0x80008148).
s32 objectDetachChildLink(u8 *previous, u8 *object) {
    s32 firstChildAddress;

    firstChildAddress = ((RuntimeObjectPrefix *)object)->child.address;
    if (previous != NULL) {
        ((RuntimeObjectPrefix *)previous)->child.address = firstChildAddress;
    }
    ((RuntimeObjectPrefix *)object)->child.address = 0;
    return firstChildAddress;
}

typedef struct ObjHead38 {
    u32 w[13];
    u16 kind;
    u16 flags;
} ObjHead38;
/* Per-kind object hooks. */
typedef struct ObjKind18 {
    /* 0x00 */ void (*init)(u8 *);
    /* 0x04 */ s32 pad4;
    /* 0x08 */ void (*update)(u8 *);
    /* 0x0C */ Gfx *(*draw)(u8 *);
    /* 0x10 */ void (*drawChild)(u8 *, u8 *);
    /* 0x14 */ s32 pad14;
} ObjKind18;
/// Per-kind object initialization, destruction, update and drawing callbacks.
/// \note Original identifier: \c D_80078140 (0x80078140).
extern ObjKind18 gObjectKindCallbacks[];
s32 keyedPoolInsertWithNextId(u8 *, u8 *);



/// Create and initialize an object from its template.
/// \param definition Readable object template with a valid runtime kind and initialized resources required by that kind.
/// \returns Newly owned object; release it with objectDestroy when it is no longer in an active hierarchy.
/// \note Original decompilation identifier: \c func_80008160 (0x80008160).
u8 *objectCreateFromTemplate(u8 *definition) {
    ObjHead38 *tmpl = (ObjHead38 *)definition;
    u8 *o = heapAllocLocal(gGameHeap, 0x68, 0x275);
    void (*init)(u8 *);

    _bzero(o, 0x5C);
    *(ObjHead38 *)o = *tmpl;
    *(s16 *)(o + 0x36) = 1;
    *(u8 **)(o + 0x58) = o + 0x5C;
    *(s32 *)(o + 0x38) = keyedPoolInsertWithNextId(gObjectHandlePool, o);
    init = gObjectKindCallbacks[tmpl->kind].init;
    if (init != NULL) {
        init(o);
    }
    return o;
}

typedef struct ObjTemplate38 {
    s32 w[14];
} ObjTemplate38;
/// Template copied when creating a default runtime object.
/// \note Original identifier: \c D_80063660 (0x80063660).
extern ObjTemplate38 gDefaultObjectTemplate;

/// Create an object from the default object template.
/// \returns Newly owned object; release it with objectDestroy when it is no longer in an active hierarchy.
/// \note Original decompilation identifier: \c func_8000823C (0x8000823C).
u8 *objectCreateDefault(void) {
    ObjTemplate38 tmpl = gDefaultObjectTemplate;

    return objectCreateFromTemplate((u8 *)&tmpl);
}

/// Assign each hierarchy object its transform-buffer slots.
/// \param object First object in a live hierarchy with assigned roots and sufficiently large root transform buffers, or null.
/// \param index Nonnegative starting slot index; slot zero retains the root buffers. The final index must fit in the allocated buffers.
/// \returns Next unused slot index.
/// \note Original decompilation identifier: \c func_800082A0 (0x800082A0).
s32 objectAssignHierarchyMatrixSlots(u8 *object, s32 index) {
    s32 i;
    u8 *p;

    p = object;
    if (p != NULL) {
        do {
            if (index > 0) {
                for (i = 0; i < 2; i++) {
                    ((s32 *)(p + 0x3C))[i] = ((s32 *)(*(u8 **)(p + 0x50) + 0x3C))[i] + (index << 6);
                }
            }
            index++;
            if (*(u8 **)(p + 0x2C) != NULL) {
                index = objectAssignHierarchyMatrixSlots(*(u8 **)(p + 0x2C), index);
            }
            p = *(u8 **)(p + 0x28);
        } while (p != NULL);
    }
    return index;
}


/// Allocate transform buffers for an object hierarchy.
/// \param root Live, acyclic hierarchy with assigned roots and no unreleased transform buffers; its required allocation size must fit in signed 32 bits.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80008338 (0x80008338).
void objectAllocateHierarchyMatrixBuffers(s32 root) {
    s32 i;
    s32 n;

    n = objectAccumulateHierarchyCount((u8 *)root, 0);
    *(u8 **)(root + 0x3C) = heapAllocLocal(gGameHeap, n << 7, 0x2F4);
    for (i = 1; i < 2; i++) {
        ((s32 *)(root + 0x3C))[i] = *(s32 *)(root + 0x3C) + ((n * i) << 6);
    }
    objectAssignHierarchyMatrixSlots((u8 *)root, 0);
}



/// Create objects from a template hierarchy and associate them with a common root.
/// \param src Non-null, acyclic template hierarchy with valid runtime kinds and resources.
/// \param root Existing hierarchy root, or null to make the first new object the root.
/// \returns Owned first object in the cloned hierarchy; release the new objects with objectDestroyHierarchy.
/// \note Original decompilation identifier: \c func_800083C0 (0x800083C0).
u8 *objectCloneTemplateHierarchy(u8 *src, u8 *root) {
    u8 *head;
    u8 **link;
    u8 *n;

    link = &head;
    do {
        n = objectCreateFromTemplate(src);
        *link = n;
        if (root == NULL) {
            root = n;
            *(u16 *)(root + 0x36) |= 4;
        }
        *(u8 **)(*link + 0x50) = root;
        if (*(u8 **)(src + 0x2C) != NULL) {
            *(u8 **)(*link + 0x2C) = objectCloneTemplateHierarchy(*(u8 **)(src + 0x2C), root);
        }
        src = *(u8 **)(src + 0x28);
        link = (u8 **)(*link + 0x28);
    } while (src != NULL);
    return head;
}


/// Create an initialized object hierarchy with transform buffers from a template.
/// \param definition Non-null, acyclic object hierarchy template with valid runtime kinds and resources.
/// \returns Owned hierarchy root, released with objectDestroyHierarchy after removal from active processing.
/// \note Original decompilation identifier: \c func_80008450 (0x80008450).
u8 *objectCreateHierarchyFromTemplate(void *definition) {
    u8 *root = objectCloneTemplateHierarchy(definition, NULL);
    objectAllocateHierarchyMatrixBuffers((s32)root);
    return root;
}

/// Associate a hierarchy with a root and clear its existing root designations.
/// \param object First object whose siblings and descendants receive the root, or null.
/// \param root Root object address to associate with the hierarchy.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80008480 (0x80008480).
void objectAssignHierarchyRoot(u8 *object, s32 root) {
    u8 *firstChild;
    u8 *var_s0;

    var_s0 = object;
    if (object != NULL) {
        do {
            firstChild = ((RuntimeObjectPrefix *)var_s0)->child.bytes;
            ((RuntimeObjectPrefix *)var_s0)->root.address = root;
            ((RuntimeObjectPrefix *)var_s0)->flags = (u16)(((RuntimeObjectPrefix *)var_s0)->flags & ~4);
            if (firstChild != NULL) {
                objectAssignHierarchyRoot(firstChild, root);
            }
            var_s0 = ((RuntimeObjectPrefix *)var_s0)->sibling.bytes;
        } while (var_s0 != NULL);
    }
}

/// Designate an object as the root of its hierarchy.
/// \param object Non-null object that becomes the hierarchy root.
/// \returns Supplied object.
///
/// \note Portability: Some translation units declare this pointer-returning helper as void and discard the original ABI result.
/// \note Original decompilation identifier: \c func_800084E8 (0x800084E8).
u8 *objectMakeHierarchyRoot(u8 *object) {
    objectAssignHierarchyRoot(object, (s32)object);
    *(u8 **)(object + 0x50) = object;
    *(u16 *)(object + 0x36) |= 4;
    return object;
}

/// Adjust object bounds throughout a hierarchy to account for inherited scale.
/// \param object First object whose siblings and descendants are adjusted, or null.
/// \param parentScale Scale inherited from the parent hierarchy.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80008520 (0x80008520).
void objectScaleHierarchyBounds(u8 *object, f32 parentScale) {
    f32 combinedScale;
    u8 *firstChild;
    if (object != NULL) {
        do {
            firstChild = *(u8 **)(object + 0x2C);
            combinedScale = *(f32 *)(object + 0) * parentScale;
            *(f32 *)(object + 0x24) *= combinedScale * 0.5f;
            if (firstChild != NULL) {
                objectScaleHierarchyBounds(firstChild, combinedScale);
            }
            object = *(u8 **)(object + 0x28);
        } while (object != NULL);
    }
}

/// Compare an object with a search-tree entry by object kind and handle.
/// \param entry Search-tree entry containing the comparison object.
/// \param object Candidate object whose search order is requested.
/// \returns Minus one, zero, or one when the candidate sorts before, equal to, or after the entry object.
/// \note Original decompilation identifier: \c func_800085A4 (0x800085A4).
s32 objectCompareSearchTreeOrder(u8 *entry, u8 *object) {
    u8 *v0;
    u32 d;
    v0 = *(u8 **)(entry + 8);
    if (*(u16 *)(object + 0x34) < *(u16 *)(v0 + 0x34)) {
        return -1;
    }
    if (*(u16 *)(v0 + 0x34) < *(u16 *)(object + 0x34)) {
        return 1;
    }
    entry = (u8 *)*(u32 *)(object + 0x38);
    d = *(u32 *)(v0 + 0x38);
    if ((u32)entry < d) {
        return -1;
    }
    if (d < (u32)entry) {
        return 1;
    }
    return 0;
}

TreeNode *searchTreeInsert(u8 *, TreeNode *, s32); /* extern */

/// Register objects throughout a hierarchy in the runtime object search tree.
/// \param object First object whose siblings and descendants are registered, or null.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80008610 (0x80008610).
void objectRegisterHierarchyInSearchTree(u8 *object) {
    u8 *firstChild;
    u8 *var_s0;

    var_s0 = object;
    if (object != NULL) {
        do {
            firstChild = ((RuntimeObjectPrefix *)var_s0)->child.bytes;
            if (firstChild != NULL) {
                objectRegisterHierarchyInSearchTree(firstChild);
            }
            searchTreeInsert(gObjectSearchTree, ((ManagedObject *)var_s0)->entry, (s32)var_s0);
            var_s0 = ((RuntimeObjectPrefix *)var_s0)->sibling.bytes;
        } while (var_s0 != NULL);
    }
}

/// Register a hierarchy for active runtime object processing.
/// \param object Live hierarchy with initialized runtime kinds and roots, not already registered for active processing.
/// \returns Supplied hierarchy root.
/// \note Original decompilation identifier: \c func_80008674 (0x80008674).
u8 *objectRegisterActiveHierarchy(u8 *object) {
    u8 *var_v0;

    objectRegisterHierarchyInSearchTree(object);
    nodePoolAppend(&gActiveObjectHierarchies, (s32)object);
    gActiveObjectHierarchyCount += 1;
    var_v0 = object;
    if (object != NULL) {
        do {
            *(u16 *)(var_v0 + 0x36) = *(u16 *)(var_v0 + 0x36) | 8;
            var_v0 = *(u8 **)(var_v0 + 0x28);
        } while (var_v0 != NULL);
    }
    return object;
}

/// Release the optional state allocation owned by an object.
/// \param object Live object whose optional state allocation is owned by the local heap.
/// Any released state pointer becomes invalid; this call does not clear it.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800086E0 (0x800086E0).
void objectReleaseStateAllocation(u8 *object) {
    s32 objectState;

    objectState = ((RuntimeObjectPrefix *)object)->state.address;
    if (objectState != 0) {
        heapFreeLocal(&gGameHeap, objectState);
    }
}

/// Destroy an object and release its owned resources and runtime registrations.
/// \param object Live object with owned resources and a valid runtime kind; its storage becomes invalid.
/// The caller must remove active hierarchy and sibling links that would otherwise retain the object.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80008710 (0x80008710).
void objectDestroy(u8 *object) {
    void (*destroyCallback)(u8 *);
    u8 *matrixStorage;

    objectResponsesDiscard((s32)object);
    destroyCallback = *(void (**)(u8 *))(gObjectKindDestroyEntries + *(u16 *)(object + 0x34) * 0x18);
    if (destroyCallback != NULL) {
        destroyCallback(object);
    }
    searchTreeRemove(gObjectSearchTree, object);
    if (*(u16 *)(object + 0x36) & 4) {
        matrixStorage = *(u8 **)(object + 0x3C);
        if (matrixStorage != NULL) {
            heapFreeLocal(gGameHeap, matrixStorage);
        }
    }
    keyedPoolRemoveFirstForKey(gObjectHandlePool, *(s32 *)(object + 0x38));
    heapFreeLocal(gGameHeap, object);
}

/// Mark an object hierarchy for deferred destruction and stop its active processing.
/// \param object First object whose siblings and descendants are marked, or null.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800087BC (0x800087BC).
void objectRequestHierarchyDestruction(u8 *object) {
    objectSetHierarchyFlags(object, 0x62);
}

/// Destroy an object hierarchy and release its objects and owned resources.
/// \param object First object in a live, acyclic hierarchy to destroy, or null.
/// Its objects and owned resources become invalid; the caller must remove any active hierarchy registration.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800087DC (0x800087DC).
void objectDestroyHierarchy(u8 *object) {
    u8 *firstChild;
    u8 *nextSibling;
    u8 *var_s1;

    var_s1 = object;
    if (object != NULL) {
        do {
            firstChild = ((RuntimeObjectPrefix *)var_s1)->child.bytes;
            if (firstChild != NULL) {
                objectDestroyHierarchy(firstChild);
            }
            nextSibling = ((RuntimeObjectPrefix *)var_s1)->sibling.bytes;
            objectDestroy(var_s1);
            var_s1 = nextSibling;
        } while (nextSibling != NULL);
    }
}

/// Orient an object horizontally toward the active camera for drawing.
/// \param object Object whose orientation is updated.
/// \returns Nothing.
/// \note Portability: The local yaw-helper declaration passes an extra argument ignored by the original ABI.
/// \note Original decompilation identifier: \c func_80008830 (0x80008830).
void objectFaceCameraHorizontally(u8 *object) {
    f32 d[3];

    d[0] = ((CamRec *)gRenderCameras)[gRenderCameraIndex].x - *(f32 *)(object + 0x44);
    d[1] = 0.0f;
    d[2] = ((CamRec *)gRenderCameras)[gRenderCameraIndex].z - *(f32 *)(object + 0x4C);
    *(f32 *)(object + 0x10) = vector3GetYawFromPositiveXDegrees(d, object);
    if (!(*(u16 *)(object + 0x36) & 0x1008)) {
        *(u16 *)(object + 0x36) = *(u16 *)(object + 0x36) | 0x1000;
    }
}

void objectOrientTowardCamera(u8 *); /* extern */

/// Orient an object toward the active camera for drawing.
/// \param object Object whose orientation is updated.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800088BC (0x800088BC).
void objectUpdateCameraFacingForDraw(u8 *object) {
    u16 objectFlags;

    objectOrientTowardCamera(object);
    objectFlags = ((RuntimeObjectPrefix *)object)->flags;
    if (!(objectFlags & 0x1008)) {
        ((RuntimeObjectPrefix *)object)->flags = (u16)(objectFlags | 0x1000);
    }
}

typedef union PackedTextureMotion {
    u32 word;
    struct {
        unsigned unused : 8;
        unsigned widthShift : 3;
        unsigned radius : 5;
        unsigned heightShift : 3;
        unsigned angle : 5;
        signed elevation : 7;
        unsigned flag : 1;
    } fields;
} PackedTextureMotion;
typedef struct TextureMotion {
    s16 sMask;
    s16 tMask;
    f32 x, y, z;
} TextureMotion;
f32 mathSinDegrees(f32);
f32 mathCosDegrees(f32);
/// Initialize object texture scrolling and rotation from its configured motion settings.
/// \param object Live object with valid texture-motion settings and no unreleased state allocation.
/// \returns Nothing.
/// \note Portability: Packed motion fields rely on the original compiler bit-field layout; their numeric configuration must be representable as u32.
/// \note Original decompilation identifier: \c func_800088F4 (0x800088F4).
void objectInitializeTextureMotion(u8 *object) {
    s32 elevationStep;
    PackedTextureMotion packed;
    f32 radius;
    f32 angle;
    f32 elevation;
    TextureMotion *extent;
    extent = heapAllocLocal(gGameHeap, 16, 0x491);
    _bzero(extent, 16);
    packed.word = (u32) * (f32 *)(object + 0x24);
    extent->sMask = (1 << (packed.fields.widthShift + 4)) * 2 - 1;
    extent->tMask = (1 << (packed.fields.heightShift + 4)) * 2 - 1;
    radius = packed.fields.radius * 2.0f;
    angle = packed.fields.angle * 11.25f;
    elevationStep = packed.fields.elevation;
    elevation = (elevationStep >= 0 ? elevationStep : -elevationStep) * 5.0f;
    if (elevationStep < 0)
        elevation = -elevation;
    extent->x = mathCosDegrees(angle) * radius * 32.0f;
    extent->y = mathSinDegrees(angle) * radius * 32.0f;
    extent->z = elevation * 32.0f;
    *(TextureMotion **)(object + 0x54) = extent;
}

u64 runtimeGetElapsedMilliseconds(void);
s32 objectApplySceneEnvironmentColorForDraw(u8 *);




/// Apply animated texture scrolling and rotation for object drawing.
/// \param obj Live object with initialized texture motion state; requires a display-list cursor with sufficient capacity.
/// \returns Object drawing continuation link.
/// \note Original decompilation identifier: \c func_80008AFC (0x80008AFC).
s32 objectApplyTextureMotionForDraw(u8 *obj) {
    TextureMotion *motion;
    Gfx *gdl;
    s32 u, v;
    f32 time, angle;
    s32 sine, cosine;

    motion = *(TextureMotion **)(obj + 0x54);
    time = (f32)(runtimeGetElapsedMilliseconds() & 0x7FFFF) * 0.001f;
    if (motion->x != 0.0f) {
        u = (s32)(motion->x * time) & motion->sMask;
    } else {
        u = 0;
    }
    if (motion->y != 0.0f) {
        v = (s32)(motion->y * time) & motion->tMask;
    } else {
        v = 0;
    }
    gdl = gRenderDisplayListCursor++;
    gMoveWd(gdl, G_MW_FOG, 0x0C, _SHIFTL(u, 16, 16) | _SHIFTL(v, 0, 16));
    if (motion->z != 0.0f) {
        angle = (f32)((s32)(motion->z * time) % 11520) * 0.03125f;
        sine = (s32)(mathCosDegrees(angle) * 32767.0f);
        cosine = (s32)(mathSinDegrees(angle) * 32767.0f);
        gSPTexture(gRenderDisplayListCursor++, sine, cosine, 0, 0, 1);
        gMoveWd(gRenderDisplayListCursor++, G_MW_FOG, 0x10, _SHIFTL(-cosine, 16, 16) | _SHIFTL(sine, 0, 16));
    }
    if (*(u16 *)(obj + 0x34) == 0x97) {
        return objectApplySceneEnvironmentColorForDraw(obj);
    }
    return *(s32 *)(obj + 0x30);
}

/// Apply the scene environment color for object drawing.
/// \param object Live object whose drawing continuation is returned; requires initialized scene data and sufficient display-list capacity.
/// \returns Object drawing continuation link.
/// \note Original decompilation identifier: \c func_80008CE0 (0x80008CE0).
s32 objectApplySceneEnvironmentColorForDraw(u8 *object) {
    gDPPipeSync(gRenderDisplayListCursor++);
    gDPSetEnvColor(gRenderDisplayListCursor++, gCurrentAreaConfiguration[0x38], gCurrentAreaConfiguration[0x39],
                   gCurrentAreaConfiguration[0x3A], 0);
    return *(s32 *)(object + 0x30);
}

s32 interactionConsiderObject(u8 *);

/// Update hierarchy visibility according to the object's interaction eligibility.
/// \param object Object whose eligibility determines visibility for its hierarchy.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80008D48 (0x80008D48).
void objectUpdateHierarchyInteractionVisibility(u8 *object) {
    if (interactionConsiderObject(object) != 0) {
        if (((RuntimeObjectPrefix *)object)->flags & 2) {
            objectClearHierarchyFlags(object, 2);
        }
    } else if (!(((RuntimeObjectPrefix *)object)->flags & 2)) {
        objectSetHierarchyFlags(object, 2);
    }
}

f32 terrainGetWorldHeightForQuery(s32, f32, f32, f32, u8 **); /* extern */

/// Measure the object world height relative to the ground beneath it.
/// \param object Object whose world position is queried.
/// \returns Signed height above the ground; uses ground height zero when no surface is found.
/// \note Portability: The terrain call passes X and Z float bit patterns through integer parameters, relying on the original MIPS ABI.
/// \note Original decompilation identifier: \c func_80008DA8 (0x80008DA8).
f32 objectGetHeightAboveGround(u8 *object) {
    return ((RuntimeObjectPrefix *)object)->worldPosition[1] - terrainGetWorldHeightForQuery(3, ((RuntimeObjectPrefix *)object)->worldPosition[0], ((RuntimeObjectPrefix *)object)->worldPosition[1],
                                                        ((RuntimeObjectPrefix *)object)->worldPosition[2], 0);
}

/// Enable object flags throughout an object hierarchy.
/// \param object First object whose sibling and descendant hierarchy is updated, or null.
/// \param flags Object flag bits to change.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80008DE8 (0x80008DE8).
void objectSetHierarchyFlags(u8 *object, u16 flags) {
    u8 *firstChild;
    if (object != 0) {
        do {
            firstChild = *(u8 **)(object + 0x2C);
            *(u16 *)(object + 0x36) = *(u16 *)(object + 0x36) | flags;
            if (firstChild != 0) {
                objectSetHierarchyFlags(firstChild, flags);
            }
            object = *(u8 **)(object + 0x28);
        } while (object != 0);
    }
}

/// Disable object flags throughout an object hierarchy.
/// \param object First object whose sibling and descendant hierarchy is updated, or null.
/// \param flags Object flag bits to change.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80008E50 (0x80008E50).
void objectClearHierarchyFlags(u8 *object, u16 flags) {
    u8 *firstChild;
    if (object != 0) {
        do {
            firstChild = *(u8 **)(object + 0x2C);
            *(u16 *)(object + 0x36) = *(u16 *)(object + 0x36) & ~flags;
            if (firstChild != 0) {
                objectClearHierarchyFlags(firstChild, flags);
            }
            object = *(u8 **)(object + 0x28);
        } while (object != 0);
    }
}

/// Update world positions throughout the active object hierarchy from the current reference transform.
/// Requires a live, acyclic hierarchy and enough traversal-stack capacity for its depth.
/// \param object First object whose sibling and descendant hierarchy is updated, or null.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80008EB8 (0x80008EB8).
void objectUpdateHierarchyWorldPositions(u8 *object) {
    u8 *firstChild;
    if (object != 0) {
        do {
            if (!((*((u16 *)(((s8 *)object) + 0x36))) & 0x1C0)) {
                objectTransformStackPush(&gObjectTraversalTransform);
                matrix34TranslateLocal(&gObjectTraversalTransform, *((f32 *)(((s8 *)object) + 0x18)),
                                       *((f32 *)(((s8 *)object) + 0x1C)), *((f32 *)(((s8 *)object) + 0x20)));
                if ((*((u16 *)(((s8 *)object) + 0x36))) & 0x1000) {
                    matrix33KeepScaleOnly(&gObjectTraversalTransform);
                }
                if ((*((u16 *)(((s8 *)object) + 0x34))) != 0x64) {
                    matrix33ApplyRotationXYZDegrees(&gObjectTraversalTransform, *((f32 *)(((s8 *)object) + 0xC)),
                                                    *((f32 *)(((s8 *)object) + 0x10)),
                                                    *((f32 *)(((s8 *)object) + 0x14)));
                    matrix33ScaleLocal(&gObjectTraversalTransform, *((f32 *)(((s8 *)object) + 0)),
                                       *((f32 *)(((s8 *)object) + 4)), *((f32 *)(((s8 *)object) + 8)));
                }
                firstChild = *((u8 **)(((s8 *)object) + 0x2C));
                {
                    *((f32 *)(((s8 *)object) + 0x44)) = (f32)(*((f32 *)(((s8 *)(&gObjectTraversalTransform)) + 0x24)));
                }
                *((f32 *)(((s8 *)object) + 0x48)) = (f32)(*((f32 *)(((s8 *)(&gObjectTraversalTransform)) + 0x28)));
                *((f32 *)(((s8 *)object) + 0x4C)) = (f32)(*((f32 *)(((s8 *)(&gObjectTraversalTransform)) + 0x2C)));
                if (firstChild != 0) {
                    objectUpdateHierarchyWorldPositions(firstChild);
                }
                objectTransformStackPop(&gObjectTraversalTransform);
            }
            object = *((u8 **)(((s8 *)object) + 0x28));
        } while (object != 0);
    }
}

/// Refresh hierarchy world positions while preserving the surrounding traversal state.
/// \param object First object whose siblings and descendants are refreshed, or zero.
/// \returns Supplied object address.
/// \note Original decompilation identifier: \c func_80008FAC (0x80008FAC).
s32 objectRefreshHierarchyWorldPositions(s32 object) {
    f32 sp24;
    f32 sp20;
    f32 sp1C;

    objectTransformStackPush(&gObjectTraversalTransform);
    sp1C = gObjectPendingLocalMovement.x;
    sp20 = gObjectPendingLocalMovement.y;
    sp24 = gObjectPendingLocalMovement.z;
    objectResetTraversalTransformAndMovement();
    objectUpdateHierarchyWorldPositions((u8 *)object);
    gObjectPendingLocalMovement.x = sp1C;
    gObjectPendingLocalMovement.y = sp20;
    gObjectPendingLocalMovement.z = sp24;
    objectTransformStackPop(&gObjectTraversalTransform);
    return object;
}

void objectResponsesDispatchAndClear(s32);
void matrix33TransformVector(f32 *, f32 *, f32 *);
void actorConstrainMovementToTerrain(u8 *, Vec3f *);
void matrix34ToFixedMtx(void *, void *);

/// Update hierarchy behavior, movement, and drawing transforms from the current reference transform.
/// Requires initialized kind state, a live acyclic hierarchy, and enough traversal-stack capacity for its depth.
/// Drawing transforms must fit signed 16.16 values and any matrix buffers must remain allocated.
/// \param object First object whose siblings and descendants are updated, or null.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80009030 (0x80009030).
void objectUpdateHierarchyFromCurrentTransform(u8 *object) {
    s32 kind;
    f32 offset[3];
    while (object != NULL) {
        if (!(*(u16 *)(object + 0x36) & 0xC0)) {
            objectTransformStackPush((Mtx48 *)&gObjectTraversalTransform);
            if (*(u16 *)(object + 0x36) & 0x800) {
                objectResponsesDispatchAndClear((s32)object);
            }
            if (gObjectKindCallbacks[*(u16 *)(object + 0x34)].update != NULL) {
                gObjectKindCallbacks[*(u16 *)(object + 0x34)].update(object);
            }
            if (!(*(u16 *)(object + 0x36) & 0x100)) {
                matrix34TranslateLocal(&gObjectTraversalTransform, *(f32 *)(object + 0x18), *(f32 *)(object + 0x1C),
                                       *(f32 *)(object + 0x20));
                if (*(u16 *)(object + 0x36) & 0x1000) {
                    matrix33KeepScaleOnly(&gObjectTraversalTransform);
                }
                if (*(u16 *)(object + 0x34) != 0x64) {
                    matrix33ApplyRotationXYZDegrees(&gObjectTraversalTransform, *(f32 *)(object + 0xC),
                                                    *(f32 *)(object + 0x10), *(f32 *)(object + 0x14));
                    matrix33ScaleLocal(&gObjectTraversalTransform, *(f32 *)object, *(f32 *)(object + 4),
                                       *(f32 *)(object + 8));
                    if (gObjectPendingLocalMovement.x != 0.0f || gObjectPendingLocalMovement.y != 0.0f ||
                        gObjectPendingLocalMovement.z != 0.0f) {
                        matrix33TransformVector((f32 *)&gObjectTraversalTransform, (f32 *)&gObjectPendingLocalMovement,
                                                (f32 *)&offset);
                        gObjectPendingLocalMovement.x = 0.0f;
                        gObjectPendingLocalMovement.y = 0.0f;
                        gObjectPendingLocalMovement.z = 0.0f;
                    } else {
                        offset[0] = 0.0f;
                        offset[1] = 0.0f;
                        offset[2] = 0.0f;
                    }
                    kind = *(u16 *)(object + 0x34);
                    if (kind > 0 && kind < 0x26) {
                        actorConstrainMovementToTerrain(object, (Vec3f *)offset);
                    }
                    ((f32 *)&gObjectTraversalTransform)[9] += offset[0];
                    ((f32 *)&gObjectTraversalTransform)[10] += offset[1];
                    ((f32 *)&gObjectTraversalTransform)[11] += offset[2];
                    *(f32 *)(object + 0x18) += offset[0];
                    *(f32 *)(object + 0x1C) += offset[1];
                    *(f32 *)(object + 0x20) += offset[2];
                }
                if (*(void **)(object + 0x3C) != NULL) {
                    matrix34ToFixedMtx(&gObjectTraversalTransform,
                                       ((void **)(object + 0x3C))[((*(u16 *)(object + 0x36) & 0x8000) ? 1 : 0) ^ 1]);
                    *(u16 *)(object + 0x36) ^= 0x8000;
                }
                *(f32 *)(object + 0x44) = ((f32 *)&gObjectTraversalTransform)[9];
                *(f32 *)(object + 0x48) = ((f32 *)&gObjectTraversalTransform)[10];
                *(f32 *)(object + 0x4C) = ((f32 *)&gObjectTraversalTransform)[11];
                if (*(u16 *)(object + 0x36) & 0x600) {
                    if (*(u16 *)(object + 0x36) & 0x400) {
                        *(u16 *)(object + 0x36) &= ~0x600;
                        *(u16 *)(object + 0x36) |= 0x200;
                    } else {
                        *(u16 *)(object + 0x36) &= ~0x600;
                        *(u16 *)(object + 0x36) |= 0x100;
                    }
                }
            }
            if (*(u8 **)(object + 0x2C) != NULL) {
                objectUpdateHierarchyFromCurrentTransform(*(u8 **)(object + 0x2C));
            }
            objectTransformStackPop((Mtx48 *)&gObjectTraversalTransform);
        }
        object = *(u8 **)(object + 0x28);
    }
}

/// Update an object hierarchy and complete its initial-update state.
/// \param object Non-null hierarchy root to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80009308 (0x80009308).
void objectUpdateHierarchy(u8 *object) {
    objectUpdateHierarchyFromCurrentTransform(object);
    if (((RuntimeObjectPrefix *)object)->flags & 1) {
        objectClearHierarchyFlags(object, 1);
    }
}

void collisionGridRandomizeAlignment(void);
void cameraBuildViewProjection(s32, u8 *, u8 *);
/// Viewport storage prepared for object hierarchy updates.
/// \note Original identifier: \c D_800AE430 (0x800AE430).
extern u8 gObjectUpdateViewport[];
/// Projection and view matrix storage prepared for object hierarchy updates.
/// \note Original identifier: \c D_800AE440 (0x800AE440).
extern u8 gObjectUpdateCameraMatrices[];

/// Update active object hierarchies belonging to the selected runtime group.
/// \param group Object group selector: zero or 0x4000.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80009344 (0x80009344).
void objectUpdateGroup(s32 group) {
    u8 *sp34;
    s32 pad;

    sp34 = NULL;
    objectResetTraversalTransformAndMovement();
    collisionGridRandomizeAlignment();
    cameraBuildViewProjection(0, gObjectUpdateViewport, gObjectUpdateCameraMatrices);
    nodePoolIterateFirst(&gActiveObjectHierarchies, (s32 *)&sp34);
    if (sp34 != NULL) {
        do {
            if ((group == (*(u16 *)(sp34 + 0x36) & 0x4000)) && !(*(u16 *)(sp34 + 0x36) & 0x30)) {
                objectUpdateHierarchy(sp34);
            }
            nodePoolIterateNext(&gActiveObjectHierarchies, (s32 *)&sp34);
        } while (sp34 != NULL);
    }
}


/// Advance pending hierarchy destruction and release hierarchies ready for removal.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800093FC (0x800093FC).
void objectSweepDeferredHierarchyDestruction(void) {
    u8 *sp2C;

    sp2C = NULL;
    nodePoolIterateFirst(&gActiveObjectHierarchies, (s32 *)&sp2C);
    if (sp2C != NULL) {
        do {
            if (*(u16 *)(sp2C + 0x36) & 0x10) {
                objectDestroyHierarchy(sp2C);
                nodePoolRemoveCurrent(&gActiveObjectHierarchies);
                gActiveObjectHierarchyCount -= 1;
            } else if (*(u16 *)(sp2C + 0x36) & 0x20) {
                *(u16 *)(sp2C + 0x36) = *(u16 *)(sp2C + 0x36) | 0x10;
            }
            nodePoolIterateNext(&gActiveObjectHierarchies, (s32 *)&sp2C);
        } while (sp2C != NULL);
    }
}

/// Request deferred destruction of active hierarchies in the selected object group.
/// \param group Object group selector: zero for unflagged hierarchies or 0x4000 for flagged hierarchies.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800094B4 (0x800094B4).
void objectRequestGroupDestruction(s32 group) {
    u8 *sp2C;

    sp2C = NULL;
    nodePoolIterateFirst(&gActiveObjectHierarchies, (s32 *)&sp2C);
    if (sp2C != NULL) {
        do {
            if ((group == (*(u16 *)(sp2C + 0x36) & 0x4000)) && !(*(u16 *)(sp2C + 0x36) & 0x30)) {
                objectRequestHierarchyDestruction(sp2C);
            }
            nodePoolIterateNext(&gActiveObjectHierarchies, (s32 *)&sp2C);
        } while (sp2C != NULL);
    }
}

/* Draw a tree payload with its kind hooks and active matrix buffer. */
typedef struct DrawObject {
    u8 header[0x30];
    Gfx *displayList;
    u16 kind;
    u16 flags;
    u32 unk38;
    Mtx *matrices[2];
    u8 unk44[0xC];
    struct DrawObject *parent;
} DrawObject;
extern u8 D_100C858[];

/// Draw an eligible object from a search-tree entry for the current draw group.
/// \param node Search-tree entry containing the object to draw.
/// \param unused Unused callback argument.
/// \returns Always zero, allowing traversal to continue.
/// \note Original decompilation identifier: \c func_80009548 (0x80009548).
s32 objectDrawSearchTreeEntry(u8 *node, s32 unused) {
    DrawObject *object;
    Gfx *displayList;
    DrawObject *parent;

    object = *(DrawObject **)(node + 8);
    if (gObjectDrawGroup != (object->flags & 0x2000) || (object->flags & 0xB3)) {
        return 0;
    }
    displayList = object->displayList;
    if (gObjectKindCallbacks[object->kind].draw != NULL) {
        displayList = gObjectKindCallbacks[object->kind].draw((u8 *)object);
    }
    if (object->matrices[0] != NULL && displayList != NULL) {
        parent = object->parent;
        if (gObjectKindCallbacks[parent->kind].drawChild != NULL) {
            gObjectKindCallbacks[parent->kind].drawChild((u8 *)parent, (u8 *)object);
        } else if (object->kind != 0x98 && object->kind != 0x95 && object->kind != 0x97) {
            gSPDisplayList(gRenderDisplayListCursor++, D_100C858);
        }
        gSPMatrix(gRenderDisplayListCursor++, (u32)object->matrices[(object->flags & 0x8000) ? 1 : 0] + 0x80000000,
                  G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPDisplayList(gRenderDisplayListCursor++, displayList);
    }
    return 0;
}

/// Draw registered objects belonging to the selected draw group.
/// \param group Draw group selector: zero or 0x2000.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800096D4 (0x800096D4).
void objectDrawGroup(s32 group) {
    gObjectDrawGroup = group;
    gSPDisplayList(gRenderDisplayListCursor++, gObjectDrawSetupDisplayList);
    searchTreeVisitAscending(gObjectSearchTreeRoot, objectDrawSearchTreeEntry);
}

/// Draw eligible objects in a hierarchy using the current draw group.
/// \param object First object whose siblings and descendants are considered, or null.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80009730 (0x80009730).
void objectDrawHierarchyForCurrentGroup(u8 *object) {
    u8 *firstChild;
    u8 *var_s0;

    var_s0 = object;
    if (object != NULL) {
        do {
            objectDrawSearchTreeEntry((s32)((ManagedObject *)var_s0)->entry, 0);
            firstChild = ((RuntimeObjectPrefix *)var_s0)->child.bytes;
            if (firstChild != NULL) {
                objectDrawHierarchyForCurrentGroup(firstChild);
            }
            var_s0 = ((RuntimeObjectPrefix *)var_s0)->sibling.bytes;
        } while (var_s0 != NULL);
    }
}

/// Draw the selected group of objects within a hierarchy.
/// \param object First object whose siblings and descendants are considered, or null.
/// \param group Draw group selector: zero or 0x2000.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80009784 (0x80009784).
void objectDrawHierarchyGroup(u8 *object, s32 group) {
    gObjectDrawGroup = group;
    gSPDisplayList(gRenderDisplayListCursor++, gObjectDrawSetupDisplayList);
    objectDrawHierarchyForCurrentGroup(object);
}

/// Reset the object traversal transform and pending local movement.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800097D0 (0x800097D0).
void objectResetTraversalTransformAndMovement(void) {
    matrix34Identity(&gObjectTraversalTransform);
    gObjectPendingLocalMovement.x = 0.0f;
    gObjectPendingLocalMovement.y = 0.0f;
    gObjectPendingLocalMovement.z = 0.0f;
}

void actorInitializePartRegistry();                            /* extern */
void objectResponsesInitialize(s32, s32);            /* extern */
s32 interactionMarkerCreate();                            /* extern */
void collisionGridInitialize();                            /* extern */
void searchTreeInitialize(u8 *, s32); /* extern */
void keyedPoolInitialize(KeyedPool *, u32); /* extern */

/// Initialize the runtime object system and its supporting registries.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80009808 (0x80009808).
void objectSystemInitialize(void) {
    gObjectTransformStackDepth = 0;
    gActiveObjectHierarchyCount = 0;
    objectResetTraversalTransformAndMovement();
    actorInitializePartRegistry();
    objectResponsesInitialize(0xC7, 0x2000);
    collisionGridInitialize();
    keyedPoolInitialize((KeyedPool *)gObjectHandlePool, 0x8F9);
    nodePoolInitialize(&gActiveObjectHierarchies, 0x761);
    searchTreeInitialize(&gObjectSearchTree, &objectCompareSearchTreeOrder);
    interactionMarkerCreate();
}

void actorDestroyPartIdMap();                  /* extern */
void objectResponsesDestroy();                  /* extern */
void collisionGridDestroy();                  /* extern */
void keyedPoolDestroy(KeyedPool *);           /* extern */

/// Destroy registered object hierarchies and release the runtime object system.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000988C (0x8000988C).
void objectSystemDestroy(void) {
    s32 sp2C;

    sp2C = 0;
    nodePoolIterateFirst(&gActiveObjectHierarchies, (s32 *)&sp2C);
    if (sp2C != 0) {
        do {
            objectDestroyHierarchy(sp2C);
            nodePoolRemoveCurrent(&gActiveObjectHierarchies);
            gActiveObjectHierarchyCount -= 1;
            nodePoolIterateNext(&gActiveObjectHierarchies, (s32 *)&sp2C);
        } while (sp2C != 0);
    }
    nodePoolDestroy(&gActiveObjectHierarchies);
    keyedPoolDestroy((KeyedPool *)gObjectHandlePool);
    collisionGridDestroy();
    objectResponsesDestroy();
    actorDestroyPartIdMap();
}

/// Orient an object along a world-space segment.
/// \param object Object whose orientation is updated when the segment has nonzero length.
/// \param start Three-component world-space segment start.
/// \param end Three-component world-space segment end.
/// \returns Segment length, or zero for coincident endpoints.
/// \note Original decompilation identifier: \c func_80009948 (0x80009948).
f32 objectOrientAlongWorldSegment(u8 *object, u8 *start, u8 *end) {
    f32 sp34;
    f32 sp30;
    f32 sp2C;
    f32 len;
    f32 v[3];

    v[0] = *(f32 *)(end + 0) - *(f32 *)(start + 0);
    v[1] = *(f32 *)(end + 4) - *(f32 *)(start + 4);
    v[2] = *(f32 *)(end + 8) - *(f32 *)(start + 8);
    len = vector3Normalize(v);
    if (len == 0.0f) {
        return 0.0f;
    }
    vector3GetEulerDegrees(v, &sp34, &sp30, &sp2C);
    *(f32 *)(object + 0xC) = sp34;
    *(f32 *)(object + 0x10) = sp30;
    *(f32 *)(object + 0x14) = sp2C;
    return len;
}

/// Adjust object pitch to follow a world-space segment while retaining its other orientation angles.
/// \param object Object whose pitch is updated when the segment has nonzero length.
/// \param start Three-component world-space segment start.
/// \param end Three-component world-space segment end.
/// \returns Segment length, or zero for coincident endpoints.
/// \note Original decompilation identifier: \c func_800099F4 (0x800099F4).
f32 objectAdjustPitchAlongWorldSegment(u8 *object, u8 *start, u8 *end) {
    f32 len;
    Mtx48 m;
    f32 v[3];

    v[0] = *(f32 *)(end + 0) - *(f32 *)(start + 0);
    v[1] = *(f32 *)(end + 4) - *(f32 *)(start + 4);
    v[2] = *(f32 *)(end + 8) - *(f32 *)(start + 8);
    len = vector3Normalize(v);
    if (len == 0.0f) {
        return 0.0f;
    }
    matrix34MakeRotationZYXDegrees(&m, 0.0f, -*(f32 *)(object + 0x10), -*(f32 *)(object + 0x14));
    matrix34TransformPointVector(&m, v, v);
    *(f32 *)(object + 0xC) = mathUnitDirectionDegrees(v[1], v[2]);
    return len;
}

/// Align an object's orientation with the active camera's viewing direction.
/// \param object Object whose orientation is updated.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80009AB8 (0x80009AB8).
void objectOrientAlongCameraView(u8 *object) {
    u8 *cameraView;
    cameraView = (gRenderCameraIndex * 0xE0) + gRenderCameras;
    objectOrientAlongWorldSegment(object, cameraView + 0xC, cameraView);
}

/// Align object pitch with the active camera's viewing direction while retaining its other orientation angles.
/// \param object Object whose pitch is updated.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80009AF8 (0x80009AF8).
void objectAdjustPitchAlongCameraView(u8 *object) {
    u8 *cameraView;
    cameraView = (gRenderCameraIndex * 0xE0) + gRenderCameras;
    objectAdjustPitchAlongWorldSegment(object, cameraView + 0xC, cameraView);
}

/// Orient an object toward the active camera position.
/// \param object Object whose orientation is updated.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80009B38 (0x80009B38).
void objectOrientTowardCamera(u8 *object) {
    objectOrientAlongWorldSegment(object, object + 0x44, (gRenderCameraIndex * 0xE0) + gRenderCameras);
}

/// Adjust object pitch toward the active camera while retaining its other orientation angles.
/// \param object Object whose orientation is updated.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80009B78 (0x80009B78).
void objectAdjustPitchTowardCamera(u8 *object) {
    objectAdjustPitchAlongWorldSegment(object, object + 0x44, (gRenderCameraIndex * 0xE0) + gRenderCameras);
}

typedef struct CamSlot {
    Mtx *mtx;
    u8 pad[0xDC];
} CamSlot;
/// Camera storage viewed at retained matrix pointers, with 224 bytes per camera.
/// \note Original identifier: \c D_800ABD14 (0x800ABD14).
extern CamSlot gRenderCameraMatrixEntries[];
s32 audioPlaySoundEffect(s32);
void audioSetSoundEffectPan(s32, s32);
void audioSetSoundEffectVolume(s32, s32);
void audioGetPositionVolumeAndPan(u8 *, Mtx, s32 *, s32 *, f32, f32);


/// Play a sound effect at an object position with spatial volume and panning.
/// \param obj Live object emitting the sound; with an active camera matrix, its camera-space position must have nonzero length.
/// \param kind Valid sound effect identifier in the loaded bank.
/// \param near Distance at which volume attenuation begins; must be less than far.
/// \param far Distance at which the sound reaches minimum volume; must be greater than near.
/// \returns Playback handle returned by the sound-effect player.
/// \note Original decompilation identifier: \c func_80009BB8 (0x80009BB8).
s32 objectPlaySpatialSound(u8 *obj, s32 kind, f32 near, f32 far) {
    s32 h;
    s32 vol;
    s32 pan;

    h = audioPlaySoundEffect(kind);
    if (gRenderCameraMatrixEntries[gRenderCameraIndex].mtx == NULL) {
        return h;
    }
    audioGetPositionVolumeAndPan(obj + 0x44, *gRenderCameraMatrixEntries[gRenderCameraIndex].mtx, &vol, &pan, near,
                                 far);
    audioSetSoundEffectPan(h, pan);
    audioSetSoundEffectVolume(h, vol);
    return h;
}


/// Measure approximate world-space distance from an object to a point.
/// \param object Object providing the first position.
/// \param point World-space point to measure against.
/// \returns Approximate distance between the object and point.
/// \note Original decompilation identifier: \c func_80009CA4 (0x80009CA4).
f32 objectGetApproxDistanceToPoint(u8 *object, Vec3f *point) {
    Vec3f d;

    d.x = *(f32 *)(object + 0x44) - point->x;
    d.y = *(f32 *)(object + 0x48) - point->y;
    d.z = *(f32 *)(object + 0x4C) - point->z;
    return vector3LengthApprox(&d);
}


/// Measure approximate world-space distance between two objects.
/// \param first Object providing the first position.
/// \param second Object providing the second position.
/// \returns Approximate distance between the objects.
/// \note Original decompilation identifier: \c func_80009CF4 (0x80009CF4).
f32 objectGetApproxDistance(u8 *first, u8 *second) {
    Vec3f d;

    d.x = *(f32 *)(first + 0x44) - *(f32 *)(second + 0x44);
    d.y = *(f32 *)(first + 0x48) - *(f32 *)(second + 0x48);
    d.z = *(f32 *)(first + 0x4C) - *(f32 *)(second + 0x4C);
    return vector3LengthApprox(&d);
}

/// Evaluate a point's horizontal direction relative to an object's world pose without exposing a result.
/// \param object Object providing the reference position and orientation.
/// \param pointAddress Address of the three-component world-space point, represented as an integer.
/// \returns Nothing.
///
/// \note Portability: The unprototyped view-matrix call passes an extra argument ignored by the original ABI; pointAddress requires 32-bit pointers.
/// \note Original decompilation identifier: \c func_80009D44 (0x80009D44).
void objectEvaluatePointRelativeYaw(u8 *object, s32 pointAddress) {
    u8 v[0x30];

    matrix34MakeViewFromEulerDegrees(object + 0x44, object + 0xC, (Vec3f *)v, object);

    matrix34GetTransformedPointYawDegrees((f32 *)v, (f32 *)pointAddress);
}

/// Prepare the object search tree for traversal beginning at a requested kind.
/// \param kind Object kind at which the search begins.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80009D80 (0x80009D80).
void objectPrepareKindSearch(s32 kind) {
    u8 msg[0x5C];

    _bzero(msg, 0x5C);
    *(s16 *)(msg + 0x34) = kind;
    *(s32 *)(msg + 0x38) = -1;
    searchTreeFind(gObjectSearchTree, msg);
    *(s32 *)(msg + 0x38) = 0;
    searchTreeFind(gObjectSearchTree, msg);
}

/* Target picker: the candidate in reach, its distance, the search origin and facing. */
typedef struct TargetPick {
    /* 0x00 */ u8 *self;
    /* 0x04 */ u8 *best;
    /* 0x08 */ Vec3f origin;
    /* 0x14 */ f32 dist;
    /* 0x18 */ Vec3f facing;
    /* 0x24 */ u8 pad24[0x24];
    /* 0x48 */ f32 cone;
    /* 0x4C */ s32 minKind;
    /* 0x50 */ s32 maxKind;
} TargetPick;
/// Shared nearest-target search context, including origin, facing, kind range and best candidate.
/// \note Original identifier: \c D_800AE3D8 (0x800AE3D8).
extern TargetPick gObjectNearestTargetSearch;
f32 mathWrapSignedDegrees(f32);
extern float fabsf(float);
#pragma intrinsic(fabsf)


/// Consider a search-tree object for the current nearest-target request.
/// \param node Search-tree entry containing the candidate object.
/// \param depth Traversal depth; unused.
/// \returns One to stop traversal beyond the requested kind range, or zero to continue.
/// \note Original decompilation identifier: \c func_80009DDC (0x80009DDC).
s32 objectConsiderNearestTargetEntry(u8 *node, s32 depth) {
    u8 *obj;
    f32 d;

    obj = *(u8 **)(node + 8);
    if (gObjectNearestTargetSearch.maxKind < *(u16 *)(obj + 0x34)) {
        return 1;
    }
    if (obj == gObjectNearestTargetSearch.self) {
        return 0;
    }
    if (gObjectNearestTargetSearch.cone < 360.0f &&
        gObjectNearestTargetSearch.cone < fabsf(mathWrapSignedDegrees(matrix34GetTransformedPointYawDegrees(
                                              (f32 *)&gObjectNearestTargetSearch.facing, (f32 *)(obj + 0x44))))) {
        return 0;
    }
    d = objectGetApproxDistanceToPoint(obj, &gObjectNearestTargetSearch.origin);
    if (d < gObjectNearestTargetSearch.dist) {
        gObjectNearestTargetSearch.best = obj;
        gObjectNearestTargetSearch.dist = d;
    }
    return 0;
}


/// Find the nearest object within a distance, kind range, and optional horizontal viewing angle.
/// \param self Object excluded from consideration, or null.
/// \param origin World position from which distance and direction are measured.
/// \param dist Exclusive upper limit on approximate distance.
/// \param rot Readable, finite viewing Euler angles in degrees; unused when angular filtering is disabled.
/// \param cone Maximum absolute horizontal angle in degrees; 360 or greater disables angular filtering.
/// \param minKind Inclusive lower bound on object kind.
/// \param maxKind Inclusive upper bound on object kind.
/// \returns Borrowed nearest qualifying object, valid until destruction, or null if none qualifies.
/// Uses shared query state and must not overlap another nearest-target search.
/// \note Original decompilation identifier: \c func_80009ED4 (0x80009ED4).
u8 *objectFindNearestTargetInView(u8 *self, Vec3f *origin, f32 dist, u8 *rot, f32 cone, s32 minKind, s32 maxKind) {
    u8 *first;

    gObjectNearestTargetSearch.self = self;
    gObjectNearestTargetSearch.best = NULL;
    gObjectNearestTargetSearch.origin.x = origin->x;
    gObjectNearestTargetSearch.origin.y = origin->y;
    gObjectNearestTargetSearch.origin.z = origin->z;
    gObjectNearestTargetSearch.dist = dist;
    gObjectNearestTargetSearch.minKind = minKind;
    gObjectNearestTargetSearch.maxKind = maxKind;
    if (cone < 360.0f) {
        matrix34MakeViewFromEulerDegrees(origin, rot, &gObjectNearestTargetSearch.facing);
        gObjectNearestTargetSearch.cone = cone;
    } else {
        matrix34Identity(&gObjectNearestTargetSearch.facing);
        gObjectNearestTargetSearch.cone = 360.0f;
    }
    objectPrepareKindSearch(minKind);
    first = *(u8 **)(gObjectSearchTreeRoot + 8);
    if (first == NULL) {
        return NULL;
    }
    if (maxKind < *(u16 *)(first + 0x34)) {
        return NULL;
    }
    if (*(u16 *)(first + 0x34) >= minKind) {
        objectConsiderNearestTargetEntry(*(u8 **)(first + 0x58), 0);
    }
    searchTreeVisitAscending(*(s32 *)(*(u8 **)(first + 0x58) + 4), objectConsiderNearestTargetEntry);
    return gObjectNearestTargetSearch.best;
}
