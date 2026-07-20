#include "global.h"

#include "PRinternal/viint.h"
#include "pad.h"

#include "fault.h"
#include "segment_symbols.h"
#include "stack.h"
#include "stackcheck.h"
#include "ys64thread.h"

// Extracts bits [27:24] from an address. Segmented addresses pack a segment index (0-15) here
// with the offset in bits [23:0]. Resolved KSEG0 pointers (0x800XXXXX) always have 0 in these bits.
// IDO emits sll 4 + srl 28 rather than srl 24 + andi 0xF.
#define SEGMENT_INDEX(p) (((u32)(p) << 4) >> 28)
// Sentinel value in heap block headers (HeapBlock.magic) to identify valid blocks.
// Free and validate functions check this before operating on a pointer.
// 2323 = 0x913. Assumed goroawase for "nii-san nii-san" (兄さん兄さん, "big brother").
#define HEAP_BLOCK_MAGIC 2323
#define DEG_TO_RAD 0.01745329238f

// Shared argument block built by the SoundEventParams_Set* ctors and passed, together
// with a numeric event ID, to the NA sound library's Na_Event dispatcher
// (e.g. Sound_PostEvent(0x26A, SoundEventParams_SetPos(x, y))). CONFIRMED sound:
// the dispatcher func_80011D1C prints "NA : pan = %d volume = %f" and its
// unit holds 49 "NA : Na_Event <NAME>" strings; posX/posY feed the panning.
typedef struct SoundEventParams {
    /* 0x00 */ s16 posX; // screen coord clamped [-320,640], scaled x2/5
    /* 0x02 */ s16 posY;
    /* 0x04 */ UNK_TYPE unk4;
    /* 0x08 */ UNK_TYPE unk8; // consumer func_80011D1C switches on 0..12
    /* 0x0C */ UNK_TYPE unkC;
    /* 0x10 */ UNK_TYPE unk10;
    /* 0x14 */ UNK_TYPE unk14;
    /* 0x18 */ UNK_TYPE unk18;
    /* 0x1C */ char pad1C[0x04];
} SoundEventParams; // size = 0x20

extern SoundEventParams gSoundEventParams;

typedef struct Struct800A65C0 {
    /* 0x00 */ UNK_TYPE unk0;
    /* 0x04 */ UNK_TYPE unk4;
} Struct800A65C0; // size = 0x08

extern Struct800A65C0 D_800A65C0;
extern u16 gAtanTable[];

// Physical segment-base table; full table is 16 entries (tail split across D_800FB00C/10/14),
// consumed by 76310.s @80075BF8 to emit gSPSegment commands for segments 0-15
extern u32 gSegments[2];
extern u8 D_802078B0[];
extern s32 gBootCodeOk;
extern Input gNeutralInput;
extern UNK_TYPE gFrameImageAlloc;
extern s32 D_800A6E94;

typedef struct MesgQueueRing {
    /* 0x00 */ u8 index;
    /* 0x01 */ u8 pendingCount[10];
    /* 0x0B */ char pad0B[0x1];
    /* 0x0C */ OSMesgQueue queues[10];
    /* 0xFC */ OSMesg msgBuf[10][10];
} MesgQueueRing; // size = 0x28C

// Central game manager (single instance gGameMgr): input snapshot pointers,
// per-frame callback dispatch, frame mesg-queue ring, slot sub-array pointers,
// and the embedded EEPROM save image (gSaveData aliases gGameMgr+0xFB8).
typedef struct GameMgr {
    /* 0x000 */ u8 unk0;
    /* 0x001 */ u8 mode; // tentative: requested mode/scene ID, written by the 6DF40.s registrar (func_8006DFC4); keys
                         // mode-callback selection (5/7/8 special)
    /* 0x002 */ u8 activeMode; // tentative: latched from mode by Main_InitSubsystems; checked post-startup (7/8)
    /* 0x003 */ u8 unk3;
    /* 0x004 */ u8 unk4;
    /* 0x005 */ u8 unk5;
    /* 0x006 */ u8 unk6;
    /* 0x007 */ u8 unk7;
    /* 0x008 */ OSContPad* input;
    /* 0x00C */ char padC[0x4];
    /* 0x010 */ void (*updateInputFunc)(void*); // only ever Input_Update, invoked with &input
    /* 0x014 */ char pad14[0x4];                // +0x14 is also the separate extern D_800F7EB4 (demo-playback flag)
    /* 0x018 */ void (*unk18)(void);
    /* 0x01C */ UNK_PTR tickFunc; // per-frame top-level handler: Main_ModeChangeTick / Main_GameTick / Main_SoftReset
    /* 0x020 */ u8 unk20;
    /* 0x021 */ char pad21[0x3];
    /* 0x024 */ void (*callback)(UNK_TYPE);
    /* 0x028 */ s16 callbackCount;
    /* 0x02A */ char pad2A[0x2];
    /* 0x02C */ struct {
        UNK_TYPE data;
        void (*func)(UNK_TYPE);
    } callbacks[20];
    // per-mode lifecycle callbacks, registered together by Main_SetModeCallbacks (selected by mode in 6DF40.s)
    /* 0x0CC */ UNK_TYPE modeResetFunc;   // invoked by Main_ResetMode
    /* 0x0D0 */ UNK_TYPE modeInitFunc;    // invoked at mode start by Main_ModeStartupPhase0 (raw offset 0xD0)
    /* 0x0D4 */ UNK_TYPE modeStartedFunc; // invoked once when startup phases complete (raw offset 0xD4 in
                                          // Main_RunModeStartupPhases)
    /* 0x0D8 */ s32 frameCount;           // tentative: ++ per Main_Loop iteration, zeroed in Main_Init; no named reader
    /* 0x0DC */ char padDC[0x14];
    /* 0x0F0 */ UNK_TYPE unkF0; // latched copy of frameImage, refreshed by Main_FrameCallback each frame
    /* 0x0F4 */ char padF4[0x64];
    /* 0x158 */ UNK_TYPE frameImage; // 64-byte-aligned 0x25800 buffer, see Main_AllocFrameImage;
                                     // extern D_800F7FF8 in asm
    /* 0x15C */ MesgQueueRing mesgRing;
    /* 0x3E8 */ char pad3E8[0x4];
    /* 0x3EC */ s32 slot; // active slot index (Main_SelectSlot); also used as byte offset into per-slot byte arrays
    // cleared by func_800685F0 when both status halfwords are idle; fed into Main_SelectSlot by Main_ModeStartupPhase0
    // at state start - looks like a pending/requested slot index, but not 100% proven yet
    /* 0x3F0 */ s32 pendingSlot;
    /* 0x3F4 */ UNK_TYPE unk3F4;
    /* 0x3F8 */ char pad3F8[0x80];
    /* 0x478 */ UNK_TYPE unk478;
    /* 0x47C */ char pad47C[0x100];
    /* 0x57C */ UNK_TYPE unk57C;
    /* 0x580 */ char pad580[0x1C];
    /* 0x59C */ void* unk59C;
    /* 0x5A0 */ char pad5A0[0x94];
    /* 0x634 */ UNK_TYPE unk634;
    /* 0x638 */ char pad638[0x298];
    /* 0x8D0 */ s32 unk8D0;
    /* 0x8D4 */ char pad8D4[0x84];
    /* 0x958 */ UNK_TYPE unk958;
    /* 0x95C */ char pad95C[0x36];
    /* 0x992 */ s16 unk992;
    /* 0x994 */ char pad994[0x12];
    /* 0x9A6 */ s16 unk9A6;
    /* 0x9A8 */ char pad9A8[0xF8];
    /* 0xAA0 */ UNK_TYPE unkAA0;
    /* 0xAA4 */ char padAA4[0x514];
    /* 0xFB8 */ u8 unkFB8; // start of the 0x800-byte save image: gSaveData = gGameMgr+0xFB8
    /* 0xFB9 */ char padFB9[0x3];
    /* 0xFBC */ s32 unkFBC;
} GameMgr; // size >= 0xFC0 (accessed up to 0x14F0 via byte arithmetic)

extern GameMgr gGameMgr;
extern u8 D_800F7EB4;
extern UNK_PTR D_800F87F8;
extern u8 D_800F828A;
extern UNK_TYPE D_800F938C;
extern Input gInputs[];
extern UNK_TYPE gModeChangeState;

extern s32 D_800A6EB4;
extern s32 D_800A6EB8;
extern s32 D_800F843C;
extern u32 D_A0000164;
extern u32 gRomSegments[];
extern UNK_TYPE D_800A6F10[];
// Heap block header for a circular doubly-linked allocator.
// Evidence: Heap_SplitBlock writes all 5 fields when splitting a block,
// Heap_AllocFromBlockFront sets inUse=1 and returns the user data at +0x10,
// Heap_GetTotalAllocated sums size for inUse blocks (totalAlloc),
// Heap_CountFreeBlocks counts inUse==0 blocks (blockCnt).
typedef struct HeapBlock {
    /* 0x00 */ u16 magic;
    /* 0x02 */ u16 inUse;
    /* 0x04 */ struct HeapBlock* prev;
    /* 0x08 */ struct HeapBlock* next;
    /* 0x0C */ s32 size;
} HeapBlock; // size = 0x10

extern HeapBlock gHeapSentinel;

void* Heap_AllocFromBlockEnd(HeapBlock* block, s32 size);

void Main_ModeChangeTick(GameMgr* arg0);
s32 Main_ModeStartupStep(void);
void func_800448BC(void);
void func_80044CA0(void);

void Main_GameTick(GameMgr* /*arg0*/);
void func_8007C5EC(void);

// Copies the two pad Inputs at D_8010DEE0 into gInputs and stores a pointer to
// each in the outInputs array; then, in demo playback (D_800F7EB4) or when the
// boot-code check failed (gBootCodeOk == 0), overwrites the first entry with the
// all-zero gNeutralInput, deadening the controls.
// The for(;;)+break form escapes the unroller; the whole-Input direct-indexed copy
// expands via ugen's 2-temp (t7/t6) block move with loads batched per struct.
void Input_Update(void* outInputs) {
    Input** ptrs;
    Input* dst;
    s32 i;

    func_8007C5EC();

    ptrs = (Input**)outInputs;
    i = 0;

    for (;;) {
        gInputs[i] = ((Input*)&D_8010DEE0)[i];
        *ptrs++ = &gInputs[i];
        i++;
        if (i == 2) {
            break;
        }
    }

    if (D_800F7EB4 == 1) {
        dst = gInputs;
        *dst = gNeutralInput;
        *(Input**)outInputs = dst;
    }

    if (gBootCodeOk == 0) {
        dst = gInputs;
        *dst = gNeutralInput;
        *(Input**)outInputs = dst;
    }
}

// Invokes the installed input-update handler (only ever Input_Update) with the
// manager's Input-pointer slots at +0x8.
void Input_CallUpdateFunc(GameMgr* mgr) {
    mgr->updateInputFunc(&mgr->input);
}

// Computes the boot-code (anti-piracy) check flag (func_8006F500 re-reads the
// cart IPL3's lui/ori immediates at ROM 0x458/0x45C into D_80108620), then zeroes
// the Input-pointer slots (the MallocRecord* cast is an artifact of the generic
// fill Mem_FillHalfwords), starts the pad manager, and installs the update handler.
void Input_Init(GameMgr* mgr) {
    gBootCodeOk = (D_80108620 == 0x260BCD5);
    Mem_FillHalfwords((MallocRecord*)&mgr->input, 0x10, 0);
    func_8007C540();
    mgr->updateInputFunc = Input_Update;
}

// Zeroes mallocRecord (sizeof(MallocRecord) = 20). Unused: no callers repo-wide.
void Heap_ClearMallocRecord(void) {
    Mem_FillHalfwords(&mallocRecord, 20, 0);
}

s32 Heap_GetTotalFree(void);

s32 Heap_CountFreeBlocks(void);

// Snapshots heap stats into mallocRecord
void Heap_UpdateMallocRecord(u32 allocP, UNK_TYPE requestSize) {
    mallocRecord.allocP = allocP;
    mallocRecord.requestSize = requestSize;
    mallocRecord.totalFree = Heap_GetTotalFree();
    mallocRecord.totalAlloc = Heap_GetTotalAllocated();
    mallocRecord.blockCnt = Heap_CountFreeBlocks();
}

// Initializes the heap: sets heap bounds (gHeapStart/gHeapEnd) and builds the
// circular free list: sentinel gHeapSentinel <-> one free block spanning D_802078B0..0x80400000
void Heap_Init(void) {
    HeapBlock* sentinel = &gHeapSentinel;
    u16 magic = HEAP_BLOCK_MAGIC;
    HeapBlock* block;
    s32 start;

    gHeapStart = (s32)D_802078B0;
    gHeapEnd = (s32)0x80400000;

    sentinel->magic = magic;
    sentinel->inUse = 1;
    block = (HeapBlock*)gHeapStart;
    sentinel->prev = block;
    sentinel->next = block;
    sentinel->size = sizeof(HeapBlock);

    start = gHeapStart;
    block->size = gHeapEnd - start;
    block->magic = magic;
    block->inUse = 0;
    block->next = sentinel;
    block->prev = sentinel;
}

// Splits a heap block: carves off a new free block at block+size if remainder >= 0x21
HeapBlock* Heap_SplitBlock(HeapBlock* block, s32 size) {
    if (block->size - size >= 0x21) {
        HeapBlock* newBlock = (HeapBlock*)((u8*)block + size);

        newBlock->magic = HEAP_BLOCK_MAGIC;
        newBlock->inUse = 0;
        newBlock->prev = block;
        newBlock->next = block->next;
        newBlock->size = block->size - size;
        newBlock->next->prev = newBlock;
        block->next = newBlock;
        block->size = size;
    }

    if (block == NULL) {
        Heap_UpdateMallocRecord(0, size);
    }

    return block;
}

// Marks the block in-use and returns its user data. The s16 indexing is load-bearing:
// result[1] writes inUse (offset 0x02), result + 8 is the user data at +0x10.
s16* Heap_AllocFromBlockFront(HeapBlock* block, s32 size) {
    s16* result = (s16*)Heap_SplitBlock(block, size);
    result[1] = 1;
    return result + 8;
}

// Heap alloc (end-of-block): aligns size, splits from end if room, marks in-use
void* Heap_AllocFromBlockEnd(HeapBlock* block, s32 size) {
    HeapBlock* cur = block;
    // ALIGN16(size + sizeof(HeapBlock)) - shifts needed, & ~0xF emits AND not SRL/SLL
    s32 remaining = block->size - (((u32)(size + 0x1F) >> 4) << 4);

    if (remaining >= 0x20) {
        HeapBlock* result = Heap_SplitBlock(block, remaining);
        if (result == NULL) {
            return NULL;
        } else {
            cur = result->next;
        }
    } else {
        cur = block;
    }
    cur->inUse = 1;
    return (void*)((u8*)cur + sizeof(HeapBlock));
}

// Heap alloc (best-fit): aligns size+header to 16, walks free list for smallest fit
Y65430UnkStruct* Heap_Alloc(u32 reqSize) {
    HeapBlock* head;
    HeapBlock* cur;
    HeapBlock* best;
    s32 size;

    // Align (request + header) up to 16 bytes
    if ((reqSize + sizeof(HeapBlock)) & 0xF) {
        size = ((reqSize + sizeof(HeapBlock)) & ~0xF) + sizeof(HeapBlock);
    } else {
        size = reqSize + sizeof(HeapBlock);
    }

    head = &gHeapSentinel;
    best = NULL;
    cur = head->next;

    while (cur != head) {
        if (cur->inUse == 0 && cur->size >= size) {
            if (best == NULL) {
                best = cur;
            } else if (cur->size < best->size) {
                best = cur;
            }
        }
        cur = cur->next;
    }

    if (best != NULL) {
        return (Y65430UnkStruct*)Heap_AllocFromBlockFront(best, size);
    }

    return NULL;
}

// Heap alloc (best-fit via prev list): aligns size+header to 16, walks free list for smallest fit
void* Heap_AllocR(u32 reqSize) {
    HeapBlock* head;
    HeapBlock* cur;
    HeapBlock* best;
    s32 size;

    // Align (request + header) up to 16 bytes
    if ((reqSize + sizeof(HeapBlock)) & 0xF) {
        size = ((reqSize + sizeof(HeapBlock)) & ~0xF) + sizeof(HeapBlock);
    } else {
        size = reqSize + sizeof(HeapBlock);
    }

    head = &gHeapSentinel;
    best = NULL;
    cur = head->prev;

    while (cur != head) {
        if (cur->inUse == 0 && cur->size >= size) {
            if (best == NULL) {
                best = cur;
            } else if (cur->size < best->size) {
                best = cur;
            }
        }
        cur = cur->prev;
    }

    if (best != NULL) {
        return Heap_AllocFromBlockEnd(best, size);
    }

    return NULL;
}

// Heap free: validates pointer, unlinks block, merges adjacent free blocks
void Heap_Free(Y65430UnkStruct* ptr) {
    HeapBlock* block;
    HeapBlock* next;

    if (ptr == NULL) {
        return;
    }
    if (SEGMENT_INDEX(ptr)) {
        return;
    }
    block = (HeapBlock*)((u32)ptr - sizeof(HeapBlock));
    if ((u32)block == 0) {
        return;
    }
    if (block->inUse == 0) {
        return;
    }
    if (block->magic != HEAP_BLOCK_MAGIC) {
        return;
    }

    if (block->prev->inUse == 0) {
        block->next->prev = block->prev;
        block->prev->next = block->next;
        block->prev->size += block->size;
        block = block->prev;
    }
    next = block->next;

    if (next->inUse == 0) {
        next->next->prev = block;
        next = block->next; // matching: redundant re-read is load-bearing
        block->size += next->size;
        block->next = next->next;
    }
    block->inUse = 0;
}

// Heap realloc: allocates new buffer, copies min(newSize, old block size) bytes, frees old
Y65430UnkStruct* Heap_Realloc(Y65430UnkStruct* ptr, u32 newSize) {
    HeapBlock* header = (HeapBlock*)((u8*)ptr - sizeof(HeapBlock));
    u32 copySize = header->size;
    Y65430UnkStruct* newBuf;

    if (newSize < copySize) {
        copySize = newSize;
    }
    newBuf = Heap_Alloc(newSize);
    Mem_CopyHalfwords(newBuf, ptr, copySize);
    Heap_Free(ptr);
    return newBuf;
}

// Validates a heap pointer: non-null, in valid RAM, within heap bounds, and has valid block magic.
s32 Heap_IsValidPtr(u8* ptr) {
    if (ptr == NULL) {
        goto ret0;
    }
    if (SEGMENT_INDEX(ptr)) {
        goto ret0;
    }
    if ((u32)ptr < (u32)gHeapStart) {
        goto ret0;
    }
    if ((u32)ptr >= (u32)(gHeapEnd - 1)) {
        goto ret0;
    }
    return ((HeapBlock*)(ptr - sizeof(HeapBlock)))->magic == HEAP_BLOCK_MAGIC;
ret0:
    return 0;
}

s32 Heap_GetTotalAllocated(void) {
    s32 sum;
    HeapBlock* node;
    HeapBlock* head;

    sum = 0;
    head = &gHeapSentinel;
    node = head->next;
    if (node != head) {
        do {
            if (node->inUse != 0) {
                sum += node->size;
            }
            node = node->next;
        } while (node != head);
    }
    return sum;
}

s32 Heap_GetTotalFree(void) {
    return (gHeapEnd - gHeapStart) - Heap_GetTotalAllocated();
}

s32 Heap_CountFreeBlocks(void) {
    s32 count;
    HeapBlock* node;
    HeapBlock* head;

    count = 0;
    head = &gHeapSentinel;
    node = head->next;
    if (node != head) {
        do {
            if (node->inUse == 0) {
                count++;
            }
            node = node->next;
        } while (node != head);
    }
    return count;
}

// Stores an arbitrary pair into D_800A65C0; the pair is never read. No callers.
void func_8006509C(UNK_TYPE arg0, UNK_TYPE arg1) {
    Struct800A65C0* p = &D_800A65C0;

    p->unk0 = arg0;
    p->unk4 = arg1;
}

// Halfword fill with s16-typed value; pure wrapper of Mem_FillHalfwords. No callers.
void Mem_FillHalfwordsS16(MallocRecord* dest, s32 size, s16 value) {
    Mem_FillHalfwords(dest, size, value);
}

// Verifies each of the size/2 halfwords at dest matches value, faults on mismatch. No callers.
void Mem_VerifyHalfwords(s16* dest, s32 size, s16 value) {
    s32 count = (u32)size >> 1;
    s16* ptr = dest;
    s32 i = 0;

    if (count > 0) {
        do {
            if (value != *ptr) {
                Fault_HungUp("./utility.c", 53);
            }
            i++;
            ptr++;
        } while (i != count);
    }
}

// Increments *counter; returns 1 on reaching limit, then wraps to 0 (hold == 0) or stays at limit - 1
s32 Timer_CountUp(s16* counter, s16 limit, s32 hold) {
    (*counter)++;
    if (*counter >= limit) {
        if (hold == 0) {
            *counter = 0;
        } else {
            *counter = limit - 1;
        }
        return 1;
    }
    return 0;
}

// Decrements *counter; returns 1 on passing 0, then wraps to limit - 1 (hold == 0) or stays at 0
s32 Timer_CountDown(s16* counter, s16 limit, s32 hold) {
    (*counter)--;
    if (*counter < 0) {
        if (hold == 0) {
            *counter = limit - 1;
        } else {
            *counter = 0;
        }
        return 1;
    }
    return 0;
}

// Insertion sort on a u16 array (ascending). No callers.
void SortU16(u16* arr, s32 count) {
    s32 j;
    s32 i;
    u16 key;
    u16* p;

    key = arr[0];
    p = arr + 1;
    if (count < 2) {
        return;
    }

    i = 1;
    do {
        // clang-format off
        // `key ^ 0` and the statement layout of this line are load-bearing: they steer
        // uopt's web coloring (ptr=t1/value=t2) and as1's delay-slot fills to the original's.
        key = *p; j = i - 1; if (j >= 0 && (key ^ 0) < arr[j]) { s32 v; do {
                v = arr[j];
                j--;
                arr[j + 2] = v;
            } while (j >= 0 && key < (v = arr[j]));
        }
        // clang-format on
        i++;
        p++;
        arr[j + 1] = key;
    } while (i != count);
}

// No-op default callback: fills slots in function-pointer tables (D_800998F0, ...) and callback args
UNK_RET Stub_Return0(void) {
    return 0;
}

// Self-reloading countdown: returns 1 when *counter hits 0, reloads to reloadValue on the next call. No callers.
s32 Timer_CountDownLoop(s32* counter, s32 reloadValue) {
    if (*counter == 0) {
        *counter = reloadValue;
        return 0;
    }
    *counter -= 1;
    return *counter < 1;
}

// Byte-wise equality comparison: 1 if the first size bytes match (inverted bcmp). No callers.
s32 Mem_IsEqual(u8* bufA, u8* bufB, s32 size) {
    u8* p;
    u8* q;
    u32 count;

    p = bufA;
    q = bufB;
    count = size;
    size--;
    if (count != 0) {
        while (1) {
            count = *q ^ *p;
            p++;
            q++;
            count = count != 0;
            if (count) {
                return 0;
            }
            count = 0;
            count = size | count;
            size--;
            if (!count) {
                break;
            }
        }
    }
    return 1;
}

// Repeated squaring: computes value ^ (2 ^ (count - 1))
f32 Math_RepeatedSquare(f32 value, s32 count) {
    s32 i;

    for (i = 1; i < count; i++) {
        value *= value;
    }
    return value;
}

// Fixed-point conversion: float * 1000 → integer
s32 Math_ToThousandths(f32 value) {
    return (s32)(value * 1000.0f);
}

// Adds two floats in 0.001-quantized fixed-point space and converts back
f32 Math_AddViaThousandths(f32 a, f32 b) {
    return (f32)(Math_ToThousandths(a) + Math_ToThousandths(b)) / 1000.0f;
}

extern u32 gRandSeed; // RNG seed

// Sets the RNG seed
void Rand_SetSeed(u32 seed) {
    gRandSeed = seed;
}

// Linear congruential generator (multiplier=0x41C64E6D, increment=12345)
s32 Rand_Next(void) {
    gRandSeed = gRandSeed * 0x41C64E6D + 12345;
    return (gRandSeed >> 16) & 0x7FFF;
}

extern u32 gRandSeedF; // RNG seed (float variant)

// Sets the RNG seed (float variant)
void Rand_SetSeedF(u32 seed) {
    gRandSeedF = seed;
}

// LCG returning f64 in [0.0, 1.0) (multiplier=0x5D588B65, increment=1)
f64 Rand_NextF(void) {
    gRandSeedF = gRandSeedF * 0x5D588B65 + 1;
    return gRandSeedF * (1.0 / 4294967296.0);
}

// Copies 2 consecutive floats from src to dst. No callers.
void Vec2f_Copy(f32* dst, f32* src) {
    *dst++ = *src++; // dst[0] = src[0]
    *dst = *src;     // dst[1] = src[1]
}

// Copies 3 consecutive floats from src to dst
void Vec3f_Copy(f32* dst, f32* src) {
    *dst++ = *src++; // dst[0] = src[0]
    *dst++ = *src++; // dst[1] = src[1]
    *dst = *src;     // dst[2] = src[2]
}

// 1 if (dest, src, size) is a valid copy request: pointers non-NULL, size a positive
// multiple of elemSize, src not a segmented address. Callers fault when this returns 0.
s32 Mem_ValidateCopy(u32 dest, u32 src, u32 size, u32 elemSize) {
    if (dest == 0) {
        return 0;
    }
    if (src == 0) {
        return 0;
    }
    if (size < elemSize || (size / elemSize) * elemSize != size) {
        return 0;
    }
    if (SEGMENT_INDEX(src)) {
        return 0;
    }
    return 1;
}

// Validated halfword copy: copies size bytes (size/2 halfwords) from src to dest.
void Mem_CopyHalfwords(void* dest, void* src, s32 size) {
    s32 i;
    u16* destP;
    u16* srcP;

    destP = (u16*)dest;
    srcP = (u16*)src;

    if (Mem_ValidateCopy((u32)dest, (u32)src, size, 2) == 0) {
        Fault_HungUp("./utility.c", 456);
    }

    size = (u32)size >> 1; // s32 / 2 generates sign-correction code; cast keeps srl
    for (i = 0; i != size; i++) {
        *destP++ = *srcP++;
    }
}

// Validated byte copy: copies size bytes from src to dest.
void Mem_CopyBytes(void* dest, void* src, s32 size) {
    s32 i;
    u8* destP;
    u8* srcP;

    destP = (u8*)dest;
    srcP = (u8*)src;

    if (Mem_ValidateCopy((u32)dest, (u32)src, size, 1) == 0) {
        Fault_HungUp("./utility.c", 484);
    }

    for (i = 0; i != size; i++) {
        *destP++ = *srcP++;
    }
}

// 1 if (dest, size) is a valid fill request: dest non-NULL and not a segmented address,
// size a positive multiple of elemSize. Callers fault when this returns 0.
s32 Mem_ValidateFill(u32 dest, u32 size, u32 elemSize) {
    if (size < elemSize) {
        return 0;
    }
    if ((size / elemSize) * elemSize != size) {
        return 0;
    }
    if (dest == 0) {
        return 0;
    }
    if (SEGMENT_INDEX(dest)) {
        return 0;
    }
    return 1;
}

// Validated halfword memset: fills size bytes (size/2 halfwords) at dest with value.
// Hand-unrolled like Mem_FillBytes (while(1)+break escapes the auto-unroller).
void Mem_FillHalfwords(MallocRecord* dest, s32 size, s32 value) {
    s32 i;
    s16* ptr;

    ptr = (s16*)dest;
    {
        // block-scoped result keeps the (s16) reload in the call-return block (lh in the bnez delay slot)
        s32 ok = Mem_ValidateFill((u32)dest, size, 2);
        dest = (MallocRecord*)(s16)value; // dest reused as the fill value (dead after the call)
        if (ok == 0) {
            Fault_HungUp("./utility.c", 583);
            return;
        }
    }

    i = 0;
    size = (u32)size >> 1;
    if (size != 0) {
        s32 val;
        s32 hold = (s32)dest;
        if ((size & 3) != 0) {
            s32 bound = (size & 3) | i; // | i (i == 0) keeps the bound in its own register (or a1,a2,zero)
            val = hold | i;
            hold = val; // reassignment extends val's web upward so it outranks bound for v0
            while (1) {
                i++;
                *ptr++ = hold;
                if (bound == i) {
                    break;
                }
            }
            if (i == size) {
                return;
            }
        }
        val = (s32)dest | ((u32)ptr - (u32)ptr); // late-folding zero anchors this copy below the loop (or v0,t0,zero)
        while (1) {
            ptr[1] = val;
            ptr[2] = val;
            ptr[3] = val;
            i += 4;
            ptr += 4;
            ptr[-4] = val;
            if (i == size) {
                break;
            }
        }
    }
}

// Validated byte memset: fills size bytes at dest with value.
// Hand-unrolled: while(1)+break loops escape uopt's auto-unroller, which can't
// produce the original's rotated store order (1,2,3,0 with the group's first
// store in the branch delay slot via sb -4).
void Mem_FillBytes(u32 dest, s32 size, u8 value) {
    s32 i;
    u8* ptr;

    ptr = (u8*)dest;
    if (Mem_ValidateFill(dest, size, 1) == 0) {
        Fault_HungUp("./utility.c", 612);
        return;
    }

    i = 0;
    if (size != 0) {
        if ((size & 3) != 0) {
            dest = (size & 3) | i; // dest reused as bound; | i (i == 0) keeps it in its own register (or a0,a2,zero)
            while (1) {
                i++;
                *ptr++ = value;
                if ((s32)dest == i) {
                    break;
                }
            }
            if (i == size) {
                return;
            }
        }
        while (1) {
            ptr[1] = value;
            ptr[2] = value;
            ptr[3] = value;
            i += 4;
            ptr += 4;
            ptr[-4] = value;
            if (i == size) {
                break;
            }
        }
    }
}

// Float round-to-nearest-integer (rounds half away from zero)
s32 Math_Round(f32 value) {
    if (value > 0.0f) {
        return (s32)(value + 0.5f);
    } else if (value < 0.0f) {
        return (s32)(value - 0.5f);
    }
    return 0;
}

// Float round-away-from-zero (ceiling for positive, floor for negative)
s32 Math_RoundAwayFromZero(f32 value) {
    if (value > 0.0f) {
        return (s32)(value + 0.9999989867f);
    } else if (value < 0.0f) {
        return (s32)(value - 0.9999989867f);
    }
    return 0;
}

// Integer absolute value
s32 Math_Abs(s32 value) {
    s32 result;

    if (value < 0) {
        result = -value;
    } else {
        result = value;
    }
    return result;
}

// Float absolute value
f32 Math_AbsF(f32 value) {
    f32 result;

    if (value < 0.0f) {
        result = -value;
    } else {
        result = value;
    }
    return result;
}

f32 Math_SinDeg(f32 angle);
f32 Math_CosDeg(f32 angle);

// Point on the ellipse around (centerX, centerY): x = cos(angle) * radiusX, y = sin(angle) * radiusY (degrees)
void Math_GetEllipsePoint(f32* outX, f32* outY, f32 centerX, f32 centerY, f32 radiusX, f32 radiusY, f32 angle) {
    *outX = Math_CosDeg(angle) * radiusX + centerX;
    *outY = Math_SinDeg(angle) * radiusY + centerY;
}

// Normalizes angle to [-180, 180) degrees
f32 Math_NormalizeAngle(f32 angle) {
    while (angle < 180.0f) {
        angle += 360.0f;
    }
    while (angle >= 180.0f) {
        angle -= 360.0f;
    }
    return angle;
}

// Point on the circle of given radius around (centerX, centerY), angle in degrees
void Math_GetCirclePoint(f32* outX, f32* outY, f32 centerX, f32 centerY, f32 radius, f32 angle) {
    *outX = Math_CosDeg(angle) * radius + centerX;
    *outY = Math_SinDeg(angle) * radius + centerY;
}

// Clamped addition: returns min(value + amount, max)
f32 Math_AddClampMax(f32 value, f32 amount, f32 max) {
    f32 sum = value + amount;

    return (sum < max) ? sum : max;
}

// 2D overlap test: true iff |x1-x2| <= rangeX and |y1-y2| <= rangeY (note rangeY before rangeX)
int Math_Overlap2D(s32 x1, s32 y1, s32 x2, s32 y2, s32 rangeY, s32 rangeX) {
    int zero = 0;

    return ((rangeX - Math_Abs(x1 - x2)) >= zero) && ((rangeY - Math_Abs(y1 - y2)) >= zero);
}

// Math_Overlap2D variant that also writes the per-axis overlap margins
int Math_Overlap2DOut(s32 x1, s32 y1, s32 x2, s32 y2, s32 rangeY, s32 rangeX, s32* outX, s32* outY) {
    *outX = rangeX - Math_Abs(x1 - x2);
    *outY = rangeY - Math_Abs(y1 - y2);

    return (*outX >= 0) && (*outY >= 0);
}

// 2D box overlap: thresholds are the summed half-extents of both boxes (note halfH before halfW)
int Math_BoxOverlap2D(s32 x1, s32 y1, s32 halfH1, s32 halfW1, s32 x2, s32 y2, s32 halfH2, s32 halfW2) {
    int zero = 0;

    return ((halfW1 + halfW2 - Math_Abs(x1 - x2)) >= zero) && ((halfH1 + halfH2 - Math_Abs(y1 - y2)) >= zero);
}

// Math_BoxOverlap2D variant that also writes the per-axis overlap margins
int Math_BoxOverlap2DOut(s32 x1, s32 y1, s32 halfH1, s32 halfW1, s32 x2, s32 y2, s32 halfH2, s32 halfW2, s32* outX,
                         s32* outY) {
    *outX = halfW1 + halfW2 - Math_Abs(x1 - x2);
    *outY = halfH1 + halfH2 - Math_Abs(y1 - y2);

    return (*outX >= 0) && (*outY >= 0);
}

// 3D box overlap writing per-axis overlap margins; half-extent order per box is (halfH, halfW, halfD).
// The flag+goto chain is a load-bearing hand-written short-circuit - keep the shape.
s32 Math_BoxOverlap3DOut(f32 x1, f32 y1, f32 z1, f32 halfH1, f32 halfW1, f32 halfD1, f32 x2, f32 y2, f32 z2, f32 halfH2,
                         f32 halfW2, f32 halfD2, f32* outX, f32* outY, f32* outZ) {
    s32 result;

    *outX = halfW1 + halfW2 - Math_AbsF(x1 - x2);
    *outY = halfH1 + halfH2 - Math_AbsF(y1 - y2);
    *outZ = halfD1 + halfD2 - Math_AbsF(z1 - z2);

    result = 0;
    if (0.0f <= *outX) {
        result = 1;
    }
    if (!result) {
        goto end;
    }
    result = 0;
    if (0.0f <= *outY) {
        result = 1;
    }
    if (!result) {
        goto end;
    }
    result = 0;
    if (0.0f <= *outZ) {
        result = 1;
    }
end:
    return result;
}

// Per-axis sign flags of point 2 relative to point 1: 1 = x2<x1, 2 = x2>x1, 8 = y2<y1, 4 = y2>y1; equal points -> 0xF
u8 Math_DirCode2D(s32 x1, s32 y1, s32 x2, s32 y2) {
    u8 result = 0;

    if (x2 < x1) {
        result = 1;
    }
    if (x1 < x2) {
        result |= 2;
    }
    if (y2 < y1) {
        result |= 8;
    }
    if (y1 < y2) {
        result |= 4;
    }
    if (x2 == x1 && y2 == y1) {
        result = 0xF;
    }
    return result;
}

// 3D version of Math_DirCode2D (adds 16 = z2<z1, 32 = z2>z1); equal points -> 63
u8 Math_DirCode3D(f32 x, f32 y, f32 z, f32 x2, f32 y2, f32 z2) {
    u8 result = 0;

    if (x2 < x) {
        result |= 1;
    }
    if (x < x2) {
        result |= 2;
    }
    if (y2 < y) {
        result |= 8;
    }
    if (y < y2) {
        result |= 4;
    }
    if (z2 < z) {
        result |= 16;
    }
    if (z < z2) {
        result |= 32;
    }
    if (x2 == x && y2 == y && z2 == z) {
        result = 63;
    }

    return result;
}

void func_8007FF58(Mtx*, f32);
void func_8007FF0C(Mtx*, f32);
void func_8007FEC8(Mtx*, f32);
void func_8007FE74(Mtx*, f32, f32);
void func_8007FFF0(Mtx*, f32, f32, f32);

// Rotate X, Y, Z (degrees; func_8007FF58/FF0C/FEC8 = rotX/rotY/rotZ) then translate (x, y, -z)
void Mtx_RotateXYZTranslate(Mtx* mtx, f32 rotX, f32 rotY, f32 rotZ, f32 x, f32 y, f32 z) {
    func_8007FF58(mtx, rotX * DEG_TO_RAD);
    func_8007FF0C(mtx, rotY * DEG_TO_RAD);
    func_8007FEC8(mtx, rotZ * DEG_TO_RAD);
    func_8007FFF0(mtx, x, y, -z);
}

// Translate (x, y, -z) + conditional X/Y mirror + scale (func_8007FE74 = 2D scale)
void Mtx_TranslateMirrorScale(Mtx* mtx, f32 x, f32 y, f32 z, s32 mirrorX, s32 mirrorY, f32 scaleX, f32 scaleY) {
    func_8007FFF0(mtx, x, y, -z);

    func_8007FE74(mtx, (mirrorX != 0) ? -1.0f : 1.0f, (mirrorY != 0) ? -1.0f : 1.0f);
    func_8007FE74(mtx, scaleX, scaleY);
}

// Translate (x, y, -z) + conditional mirror + rotate XYZ (degrees) + scale
void Mtx_TranslateMirrorRotateScale(Mtx* mtx, f32 x, f32 y, f32 z, s32 mirrorX, s32 mirrorY, f32 rotX, f32 rotY,
                                    f32 rotZ, f32 scaleX, f32 scaleY) {
    func_8007FFF0(mtx, x, y, -z);

    func_8007FE74(mtx, (mirrorX != 0) ? -1.0f : 1.0f, (mirrorY != 0) ? -1.0f : 1.0f);
    func_8007FF58(mtx, rotX * DEG_TO_RAD);
    func_8007FF0C(mtx, rotY * DEG_TO_RAD);
    func_8007FEC8(mtx, rotZ * DEG_TO_RAD);
    func_8007FE74(mtx, scaleX, scaleY);
}

// Mtx_TranslateMirrorRotateScale + final plain translate by (offsetX, offsetY, offsetZ) (local offset, z not negated)
void Mtx_TranslateMirrorRotateScaleTranslate(Mtx* mtx, f32 x, f32 y, f32 z, f32 offsetX, f32 offsetY, f32 offsetZ,
                                             s32 mirrorX, s32 mirrorY, f32 rotX, f32 rotY, f32 rotZ, f32 scaleX,
                                             f32 scaleY) {
    func_8007FFF0(mtx, x, y, -z);

    func_8007FE74(mtx, (mirrorX != 0) ? -1.0f : 1.0f, (mirrorY != 0) ? -1.0f : 1.0f);
    func_8007FF58(mtx, rotX * DEG_TO_RAD);
    func_8007FF0C(mtx, rotY * DEG_TO_RAD);
    func_8007FEC8(mtx, rotZ * DEG_TO_RAD);
    func_8007FE74(mtx, scaleX, scaleY);
    func_8007FFF0(mtx, offsetX, offsetY, offsetZ);
}

// Givens rotation: rotates the plane spanned by MtxF rows axis1/axis2 by angle degrees (generalizes the axis rotations)
void MtxF_RotatePlane(MtxF* mf, f32 angle, s32 axis1, s32 axis2) {
    f32* src;
    f32 cosA;
    f32* dst;
    f32 sinA;
    s32 i;

    if (angle != 0.0f) {
        src = mf->mf[axis1];
        dst = mf->mf[axis2];

        sinA = Math_SinDeg(angle);
        cosA = Math_CosDeg(angle);

        for (i = 0; i < 4; i++) {
            f32 s = *src;
            f32 d = *dst;
            *src++ = s * cosA + d * sinA;
            *dst++ = d * cosA - s * sinA;
        }
    }
}

// Sets upper-left 3x3 of MtxF to identity (diagonal=1, off-diagonal=0)
void MtxF_SetRotationIdentity(MtxF* mf) {
    mf->xx = 1.0f;
    mf->yy = 1.0f;
    mf->zz = 1.0f;
    mf->xy = 0.0f;
    mf->xz = 0.0f;
    mf->yx = 0.0f;
    mf->yz = 0.0f;
    mf->zx = 0.0f;
    mf->zy = 0.0f;
}

// Sets MtxF translation column (xw, yw, zw)
void MtxF_SetTranslation(MtxF* mf, f32 x, f32 y, f32 z) {
    mf->xw = x;
    mf->yw = y;
    mf->zw = z;
}

// Sets MtxF perspective/homogeneous row to (0, 0, 0, 1)
void MtxF_SetWRow(MtxF* mf) {
    mf->wx = 0.0f;
    mf->wy = 0.0f;
    mf->wz = 0.0f;
    mf->ww = 1.0f;
}

// Builds a translation-only MtxF: clears rotation, sets translation, sets w=1
void MtxF_Translate(MtxF* mf, f32 x, f32 y, f32 z) {
    MtxF_SetRotationIdentity(mf);
    MtxF_SetTranslation(mf, x, y, z);
    MtxF_SetWRow(mf);
}

// Quadratic B-spline basis: computes n sets of 3 coefficients
void Spline_QuadBasisTable(f32* dst, s32 n) {
    f32 step = 1.0f / n;
    f32 t = 0.0f;
    s32 i = 0;

    if (n > 0) {
        do {
            f32 s = 1.0f - t;
            f32 s2 = s * s;
            f32 t2 = t * t;
            *dst++ = s2 * 0.5f;
            *dst++ = s * t + 0.5f;
            *dst++ = t2 * 0.5f;
            t += step;
            i++;
        } while (i != n);
    }
}

// Blends 3 s16 control triples with 3 basis weights, round-to-nearest:
// out[i] = round(a[i]*w[0] + b[i]*w[1] + c[i]*w[2])
void Spline_Blend3Vec3s(f32* weights, s16* a, s16* b, s16* c, s16* output) {
    s32 i;

    for (i = 0; i < 3; i++) {
        f32 result = *a * weights[0] + *b * weights[1] + *c * weights[2];
        if (result < 0.0f) {
            *output = (s32)(result - 0.5f);
        } else {
            *output = (s32)(result + 0.5f);
        }
        a++;
        b++;
        c++;
        output++;
    }
}

// Cubic B-spline basis functions: result[0..3] = N_i(t), t clamped to [0,1]
void Spline_CubicBasis(f32 t, f32* result) {
    f32 one;
    f32 half;
    f32 oneThird;
    f32 sixth;
    f32 tSq;
    f32 tCu;
    f32 twoThirds;

    one = 1.0f;
    half = 0.5f;
    oneThird = 1.0f / 3.0f;
    if (t > 1.0f) {
        t = 1.0f;
    }
    if (t < 0.0f) {
        t = 0.0f;
    }
    if (1) {
        sixth = half * oneThird;
    }
    tSq = t * t;
    tCu = tSq * t;
    twoThirds = oneThird + oneThird;
    result[0] = (((one - t) * (one - t)) * (one - t)) * sixth;
    result[1] = ((tCu * half) - tSq) + twoThirds;
    result[2] = (((tSq - tCu) + t) * half) + sixth;
    result[3] = tCu * sixth;
}

// Angle normalize: adjust *b by 360 if abs(*a - *b) > 180
void Math_UnwrapAngle(f32* a, f32* b) {
    if (Math_AbsF(*a - *b) > 180.0f) {
        if (*a > 0.0f) {
            *b += 360.0f;
        } else {
            *b -= 360.0f;
        }
    }
}

// Blends 4 angle control values with 4 basis weights, unwrapping each neighbor pair first
void Spline_Blend4Angles(f32* weights, f32* output, f32 a, f32 b, f32 c, f32 d) {
    Math_UnwrapAngle(&a, &b);
    Math_UnwrapAngle(&b, &c);
    Math_UnwrapAngle(&c, &d);
    *output = weights[0] * a + weights[1] * b + weights[2] * c + weights[3] * d;
}

// Blends 4 f32 control pairs with 4 basis weights: out[i] = a[i]*w[0] + b[i]*w[1] + c[i]*w[2] + d[i]*w[3]
void Spline_Blend4Vec2f(f32* weights, f32* a, f32* b, f32* c, f32* d, f32* output) {
    s32 i;

    for (i = 0; i != 2; i++) {
        *output = *a * weights[0] + *b * weights[1] + *c * weights[2] + *d * weights[3];
        a++;
        b++;
        c++;
        d++;
        output++;
    }
}

// Blends 4 f32 control triples with 4 basis weights: out[i] = a[i]*w[0] + b[i]*w[1] + c[i]*w[2] + d[i]*w[3]
void Spline_Blend4Vec3f(f32* weights, f32* a, f32* b, f32* c, f32* d, f32* output) {
    s32 i;

    for (i = 0; i != 3; i++) {
        *output = *a * weights[0] + *b * weights[1] + *c * weights[2] + *d * weights[3];
        a++;
        b++;
        c++;
        d++;
        output++;
    }
}

// Cubic Bernstein basis: computes n sets of 4 coefficients
void Spline_BernsteinBasisTable(f32* dst, s32 n) {
    f32 step = 1.0f / n;
    f32 t = 0.0f;
    s32 i = 0;

    if (n > 0) {
        do {
            f32 s = 1.0f - t;
            f32 s2 = s * s;
            f32 t2 = t * t;
            *dst++ = s2 * s;
            *dst++ = 3.0f * t * s2;
            *dst++ = t2 * 3.0f * s;
            *dst++ = t2 * t;
            t += step;
            i++;
        } while (i != n);
    }
}

// Blends 4 s16 control triples with 4 basis weights, round-to-nearest:
// out[i] = round(a[i]*w[0] + b[i]*w[1] + c[i]*w[2] + d[i]*w[3])
void Spline_Blend4Vec3s(f32* weights, s16* a, s16* b, s16* c, s16* d, s16* output) {
    s32 i;

    for (i = 0; i != 3; i++) {
        f32 result = *a * weights[0] + *b * weights[1] + *c * weights[2] + *d * weights[3];
        if (result < 0.0f) {
            *output = (s32)(result - 0.5f);
        } else {
            *output = (s32)(result + 0.5f);
        }
        a++;
        b++;
        c++;
        d++;
        output++;
    }
}

// Sine of angle in degrees, returned as float in [-1, 1]
f32 Math_SinDeg(f32 angle) {
    if (angle <= 0.0f) {
        do {
            angle += 360.0f;
        } while (angle <= 0.0f);
    }
    if (360.0f < angle) {
        do {
            angle -= 360.0f;
        } while (360.0f < angle);
    }
    return sins((u16)(s32)(angle * (32767.0f / 180.0f))) * (1.0f / 32767.0f);
}

// Cosine of angle in degrees, returned as float in [-1, 1]
f32 Math_CosDeg(f32 angle) {
    if (angle <= 0.0f) {
        do {
            angle += 360.0f;
        } while (angle <= 0.0f);
    }
    if (360.0f < angle) {
        do {
            angle -= 360.0f;
        } while (360.0f < angle);
    }
    return coss((u16)(s32)(angle * (32767.0f / 180.0f))) * (1.0f / 32767.0f);
}

// Arctan table lookup: returns the binary angle for the ratio num/denom in [0, 1] (single
// return statement is load-bearing: it keeps ugen's exit block unsplit so the
// indexed path ends in jr ra + or v0,v1,zero)
s32 Math_AtanLookup(f32 num, f32 denom) {
    u32 result;
    s32 index;

    if (denom == 0.0f) {
        result = gAtanTable[0];
    } else {
        index = (s32)(num / denom * 1024.0f + 0.5f);
        if (index >= 0x401) {
            result = gAtanTable[0];
        } else {
            result = gAtanTable[index];
        }
    }
    return result;
}

// atan2 returning a 16-bit binary angle
s16 Math_Atan2S(f32 x, f32 y) {
    u16 result;

    if (y >= 0.0f) {
        if (x >= 0.0f) {
            if (y <= x) {
                result = Math_AtanLookup(y, x);
            } else {
                result = 0x4000 - Math_AtanLookup(x, y);
            }
        } else {
            x = -x;
            if (x < y) {
                result = Math_AtanLookup(x, y) + 0x4000;
            } else {
                result = 0x8000 - Math_AtanLookup(y, x);
            }
        }
    } else {
        y = -y;
        if (x < 0.0f) {
            x = -x;
            if (y <= x) {
                result = Math_AtanLookup(y, x) + 0x8000;
            } else {
                result = 0xC000 - Math_AtanLookup(x, y);
            }
        } else {
            if (x < y) {
                result = Math_AtanLookup(x, y) + 0xC000;
            } else {
                result = -Math_AtanLookup(y, x);
            }
        }
    }

    return result;
}

// atan2 in degrees: converts Math_Atan2S's binary angle via 8.8 fixed-point, val * 360 / 65536
f32 Math_Atan2Deg(f32 x, f32 y) {
    s32 val;
    u8 low;
    s16 high;

    val = Math_Atan2S(x, y);
    low = val & 0xFF;
    high = (s16)(val >> 8);
    // u8 low + (s32) cast: u8 is needed for register allocation, (s32) avoids unsigned float conversion
    return ((f32)high + (f32)(s32)low * (1.0f / 256.0f)) * (360.0f / 256.0f);
}

// Angle chase: move *angle toward target by step degrees per call, wrapping 0-360
void Math_StepToAngle(f32* angle, f32 target, f32 step) {
    f32 diff = target - *angle;

    if (diff < -0.001f) {
        if (diff < -180.0f) {
            *angle += step;
            if (360.0f <= *angle) {
                *angle -= 360.0f;
            }
        } else {
            *angle -= step;
            if (*angle < target) {
                *angle = target;
            }
        }
    } else {
        if (diff < 180.0f) {
            *angle += step;
            if (target < *angle) {
                *angle = target;
            }
        } else {
            *angle -= step;
            if (*angle < 0.0f) {
                *angle += 360.0f;
            }
        }
    }
}

void Idle_InitVideo(void);
void Main_ThreadEntry(void* arg);

extern OSThread sMainThread;
extern STACK(sMainStack, 0x2700);
extern StackEntry sMainStackInfo;

extern OSMesgQueue sPiMgrCmdQueue;
extern OSMesg sPiMgrCmdBuff[8];

void Idle_ThreadEntry(void* arg) {
    osCreateViManager(OS_PRIORITY_VIMGR);
    Idle_InitVideo();
    osViSetSpecialFeatures(OS_VI_DITHER_FILTER_ON | OS_VI_GAMMA_DITHER_OFF | OS_VI_GAMMA_OFF);
    osViBlack(true);
    osCreatePiManager(OS_PRIORITY_PIMGR, &sPiMgrCmdQueue, sPiMgrCmdBuff, ARRAY_COUNT(sPiMgrCmdBuff));
    StackCheck_Init(&sMainStackInfo, sMainStack, STACK_TOP(sMainStack), 0, 0x1000, "main");
    osCreateThread(&sMainThread, Y_THREAD_ID_MAIN, Main_ThreadEntry, arg, STACK_TOP(sMainStack), Y_PRIORITY_MAIN);
    D_80108624 = IO_READ(SP_IMEM_START);
    D_80108628 = IO_READ(SP_DMEM_START);
    func_8006F500();
    osStartThread(&sMainThread);
    osSetThreadPri(NULL, OS_PRIORITY_IDLE);

    for (;;) {}
}

void Idle_InitVideo(void) {
    switch (osTvType) {
        case OS_TV_TYPE_PAL:
            osViSetMode(&osViModeTable[OS_VI_PAL_LAN1]);
            break;

        case OS_TV_TYPE_NTSC:
            osViSetMode(&osViModeTable[OS_VI_NTSC_LAN1]);
            break;

        case OS_TV_TYPE_MPAL:
            osViSetMode(&osViModeTable[OS_VI_MPAL_LAN1]);
            break;
    }
}

void bootclear(void) {
    bzero(SEGMENT_VRAM_END(main), osMemSize - K0_TO_PHYS(SEGMENT_VRAM_END(main)));
}

extern STACK(sBootStack, 0x2000);
extern StackEntry sBootStackInfo;

extern OSThread sIdleThread;
extern STACK(sIdleStack, 0x200);
extern StackEntry sIdleStackInfo;

void bootproc(void) {
    StackCheck_Init(&sBootStackInfo, sBootStack, STACK_TOP(sBootStack), 0, -1, "boot");
    osInitialize();
    bootclear();
    StackCheck_Init(&sIdleStackInfo, sIdleStack, STACK_TOP(sIdleStack), 0, 0x100, "idle");
    osCreateThread(&sIdleThread, Y_THREAD_ID_IDLE, Idle_ThreadEntry, NULL, STACK_TOP(sIdleStack), Y_PRIORITY_IDLE);
    osStartThread(&sIdleThread);
}

// Segment 0 = direct (physical) addressing, segment 1 = heap arena base (KSEG0 → physical)
void Segments_Init(void) {
    gSegments[0] = 0;
    gSegments[1] = (u32)D_802078B0 & 0x1FFFFFFF;
}

extern OSMesgQueue D_801004C8;
extern OSMesg D_801004E0;
extern OSMesgQueue sSiEventQueue;
extern OSMesg sSiEventBuff;

// Boot leftover: the OS_EVENT_SI registration is superseded during init by the pad manager's
// sSerialEventQueue (func_8007C540), and D_801004C8 is never referenced again anywhere.
void Si_InitEventQueues(void) {
    osCreateMesgQueue(&D_801004C8, &D_801004E0, 1);
    osCreateMesgQueue(&sSiEventQueue, &sSiEventBuff, 1);
    osSetEventMesg(OS_EVENT_SI, &sSiEventQueue, (OSMesg)1);
}

// Resolves a segmented ROM address and copies data via DMA.
// segAddr format: bits 24-27 = segment index into gRomSegments, bits 0-23 = offset within segment.
void Rom_LoadSegmented(void* dest, UNK_TYPE segAddr, size_t size) {
    DmaMgr_RequestSync(dest, gRomSegments[SEGMENT_INDEX(segAddr)] + (segAddr & 0xFFFFFF), size);
}

// Loads data via the given loader callback; every caller in the ROM passes Rom_LoadSegmented
void Rom_LoadWith(void* dest, UNK_TYPE src, size_t size, RomLoadFunc loadFn) {
    loadFn(dest, src, size);
}

// Advances to the next of the 10 ring slots, then blocks until all of that slot's pending messages arrive
void MesgQueueRing_Advance(GameMgr* mgr) {
    u8 idx;
    MesgQueueRing* ring = &mgr->mesgRing;

    ring->index++;
    if ((u8)ring->index >= 10) {
        ring->index = 0;
    }

    idx = ring->index;
    if (ring->pendingCount[idx] != 0) {
        do {
            osRecvMesg(&ring->queues[idx], NULL, OS_MESG_BLOCK);
            ring->pendingCount[idx]--;
        } while ((u8)ring->pendingCount[idx] != 0);
    }
}

// Initializes the ring's 10 message queues and clears pending counts
void MesgQueueRing_Init(GameMgr* mgr) {
    s32 i;

    mgr->mesgRing.index = 0;

    for (i = 0; i < 10; i++) {
        mgr->mesgRing.pendingCount[i] = 0;
        osCreateMesgQueue(&mgr->mesgRing.queues[i], mgr->mesgRing.msgBuf[i], 10);
    }
}

void DmaMgr_Init(void);
void func_800755E0(void*, void*, s32, s32);
void func_80071B0C(void*);
void func_80071740(s32, void*);
void func_800729DC(void*);
void func_800715E0(void);
void func_80076AA0(void*);

extern UNK_TYPE D_800FAB28;
extern STACK(D_800FFCD8, 0x500);
extern StackEntry D_801001D8;
extern UNK_TYPE D_800A6E34;
extern UNK_PTR D_800A8514;
extern s32 D_800A8518;

// One-time OS-services init: SI queues, segments, DMA, IRQ manager, subsystems
void System_Init(GameMgr* mgr) {
    Si_InitEventQueues();
    Segments_Init();
    DmaMgr_Init();
    StackCheck_Init(&D_801001D8, D_800FFCD8, STACK_TOP(D_800FFCD8), 0, 0x100, "irqmgr");
    PRINTF("SWITCH : %d\n");
    func_800755E0(&D_801001F8, &D_801001D8, 0x18, 1);
    func_80071B0C(&D_801001F8);
    func_80071740(3, &D_800A6E34);
    func_800729DC(&D_801001F8);
    func_800715E0();
    func_80076AA0((u8*)mgr + 0xE0);
    D_800A8514 = &D_800FAB28;
    D_800A8518 = 150;
}

// Wrappers over the NA sound library (asm 11120-1BB80; stripped debug strings
// "NA : Na_*" / "NAI: BgmRoutin/SE Buffer/PortRoutin" name the module). The
// bare-() wrappers forward their caller's args untouched. Callees kept func_
// until that module is decompiled; sub_handle is the library's own term.
s32 Sound_IsValidSubHandle(s16 subHandle) {
    if (subHandle < 0) {
        goto ret0;
    }
    if (subHandle < 10) {
        goto ret1;
    }
ret0:
    return 0;
ret1:
    return 1;
}

void func_800677F8() {
    func_80010A74();
}

// Fires a game event ID at the Na_Event dispatcher (49 "NA : Na_Event <NAME>"
// strings); callers set pan/volume params via SoundEventParams_SetPos* first
void Sound_PostEvent() {
    func_80011D1C();
}

void func_80067838(void* arg0, s16 subHandle, UNK_TYPE arg2) {
    if (Sound_IsValidSubHandle(subHandle) != 0) {
        func_80015E74(arg0, subHandle, arg2);
    }
}

void func_80067878(void* arg0, s16 subHandle) {
    if (Sound_IsValidSubHandle(subHandle) != 0) {
        func_80011990(arg0, subHandle);
    }
}

void func_800678B0(void) {
    func_80011C64();
}

void func_800678D0(void) {
    func_80011C3C();
}

void func_800678F0(void* arg0, s16 subHandle, UNK_TYPE arg2) {
    if (Sound_IsValidSubHandle(subHandle) != 0) {
        func_8001759C(arg0, subHandle, arg2);
    }
}

// Validated call: returns func_80011928 result if subHandle in [0,9], else 0
s32 func_80067930(void* arg0, s16 subHandle) {
    if (Sound_IsValidSubHandle(subHandle) == 0) {
        return 0;
    }
    return func_80011928(arg0, subHandle);
}

void func_80067970(void) {
    func_80010640();
}

void func_80067990(void) {
    func_80010688();
}

void func_800679B0() {
    func_80010730();
}

void func_800679D0() {
    func_800106CC();
}

// Forwards status 0-3 to func_800186E0 = "NA : Na_YoshiStatusChange %d";
// music mood reaction (per-track volume/fade changes on petal-count thresholds)
void Sound_YoshiStatusChange(void) {
    func_800186E0();
}

void func_80067A10(void) {
    func_80018E68();
}

void func_80067A30(void) {
    func_80018EE8();
}

void func_80067A50(s16 arg0) {
    func_80018C98(arg0);
}

void func_80067A78(s16 arg0, s16 arg1) {
    func_80018EBC(arg0, arg1);
}

void func_80067AAC(void) {
    func_80010558();
}

void func_80067ACC(UNK_TYPE arg0, s16 arg1) {
    func_80018CB0(arg0, arg1);
}

void func_80067AF4(s16 arg0) {
    func_80018C80(arg0);
}

void func_80067B1C(void) {
    func_80071D40();
}

// Increments D_800A6E94 and wraps 5 -> 1. VOID: the delay-slot li v0,1 is the
// wrap block's own store value moved in by as1's fallthrough-lead fill, which
// is only liveness-legal when $v0 is dead at the shared exit (void return).
// The pointer local + if(1) boundary defeat the kpicopt address web; reading
// the 1 back through the named pointer q makes it a load-class web (colored
// v0) instead of a store-only ugen t6 temp (inlining *(&ret) folds away).
// The dead `jr ra; nop` tail at 0x80067B6C is a separate empty function.
void func_80067B3C(void) {
    s32* p = &D_800A6E94;
    s32 val = *p + 1;

    if (1) {
        *p = val;
    }
    if (val >= 5) {
        s32 ret = 1;
        s32* q = &ret;
        *p = *q;
    }
}

static void func_80067B6C(void) {
}

// Clamps a coordinate to [-320, 640] and scales by 2/5 (maps ~[-128, 255])
s16 SoundEventParams_ScaleCoord(s32 coord) {
    if (coord < -320) {
        coord = -(320 + 1);
    } else if (coord >= 640 + 1) {
        coord = 640 - 1;
    }
    // coord * 10 / 25 = coord * 2 / 5
    return ((coord << 3) + (coord << 1)) / 25;
}

SoundEventParams* SoundEventParams_SetPos(s32 x, s32 y) {
    gSoundEventParams.posX = SoundEventParams_ScaleCoord(x);
    gSoundEventParams.posY = SoundEventParams_ScaleCoord(y);
    return &gSoundEventParams;
}

SoundEventParams* SoundEventParams_SetPosUnk4(UNK_TYPE unk4, UNK_TYPE x, UNK_TYPE y) {
    SoundEventParams_SetPos(x, y);
    gSoundEventParams.unk4 = unk4;
    return &gSoundEventParams;
}

SoundEventParams* SoundEventParams_SetPosUnk8(UNK_TYPE unk8, UNK_TYPE x, UNK_TYPE y) {
    SoundEventParams_SetPos(x, y);
    gSoundEventParams.unk8 = unk8;
    return &gSoundEventParams;
}

SoundEventParams* SoundEventParams_SetPosUnk4Unk8(UNK_TYPE unk4, UNK_TYPE unk8, UNK_TYPE x, UNK_TYPE y) {
    SoundEventParams_SetPos(x, y);
    gSoundEventParams.unk8 = unk8;
    gSoundEventParams.unk4 = unk4;
    return &gSoundEventParams;
}

SoundEventParams* SoundEventParams_SetPosUnkC(UNK_TYPE unkC, UNK_TYPE x, UNK_TYPE y) {
    SoundEventParams_SetPos(x, y);
    gSoundEventParams.unkC = unkC;
    return &gSoundEventParams;
}

SoundEventParams* SoundEventParams_SetPosUnk4UnkC(UNK_TYPE unk4, UNK_TYPE unkC, UNK_TYPE x, UNK_TYPE y) {
    SoundEventParams_SetPos(x, y);
    gSoundEventParams.unkC = unkC;
    gSoundEventParams.unk4 = unk4;
    return &gSoundEventParams;
}

SoundEventParams* SoundEventParams_SetUnk14(UNK_TYPE unk14) {
    SoundEventParams* p = &gSoundEventParams;

    p->unk14 = unk14;
    return p;
}

SoundEventParams* SoundEventParams_SetUnk18(UNK_TYPE unk18) {
    SoundEventParams* p = &gSoundEventParams;

    p->unk18 = unk18;
    return p;
}

SoundEventParams* SoundEventParams_SetUnk14Unk18(UNK_TYPE unk14, UNK_TYPE unk18) {
    SoundEventParams* p = &gSoundEventParams;

    p->unk14 = unk14;
    p->unk18 = unk18;
    return p;
}

SoundEventParams* SoundEventParams_SetPosUnk14(UNK_TYPE unk14, UNK_TYPE x, UNK_TYPE y) {
    SoundEventParams_SetPos(x, y);
    gSoundEventParams.unk14 = unk14;
    return &gSoundEventParams;
}

SoundEventParams* SoundEventParams_SetPosUnk4Unk14(UNK_TYPE unk4, UNK_TYPE unk14, UNK_TYPE x, UNK_TYPE y) {
    SoundEventParams_SetPosUnk14(unk14, x, y);
    gSoundEventParams.unk4 = unk4;
    return &gSoundEventParams;
}

SoundEventParams* SoundEventParams_SetPosUnk4Unk14Unk18(UNK_TYPE unk4, UNK_TYPE unk14, UNK_TYPE unk18, UNK_TYPE x,
                                                        UNK_TYPE y) {
    SoundEventParams_SetPos(x, y);
    SoundEventParams_SetUnk14Unk18(unk14, unk18);
    gSoundEventParams.unk4 = unk4;
    return &gSoundEventParams;
}

SoundEventParams* SoundEventParams_SetPosUnk8Unk14(UNK_TYPE unk8, UNK_TYPE unk14, UNK_TYPE x, UNK_TYPE y) {
    SoundEventParams_SetPos(x, y);
    gSoundEventParams.unk8 = unk8;
    gSoundEventParams.unk14 = unk14;
    return &gSoundEventParams;
}

SoundEventParams* SoundEventParams_SetUnk10(UNK_TYPE unk10) {
    SoundEventParams* p = &gSoundEventParams;

    p->unk10 = unk10;
    return p;
}

void SoundEventParams_SetPosY60(UNK_TYPE x) {
    SoundEventParams_SetPos(x, 0x3C);
}

void SoundEventParams_SetPosUnk4Y60(UNK_TYPE unk4, UNK_TYPE x) {
    SoundEventParams_SetPosUnk4(unk4, x, 0x3C);
}

void SoundEventParams_SetPosUnk8Y60(UNK_TYPE unk8, UNK_TYPE x) {
    SoundEventParams_SetPosUnk8(unk8, x, 0x3C);
}

void SoundEventParams_SetPosUnk4Unk8Y60(UNK_TYPE unk4, UNK_TYPE unk8, UNK_TYPE x) {
    SoundEventParams_SetPosUnk4Unk8(unk4, unk8, x, 0x3C);
}

void SoundEventParams_SetPosUnkCY60(UNK_TYPE unkC, UNK_TYPE x) {
    SoundEventParams_SetPosUnkC(unkC, x, 0x3C);
}

void SoundEventParams_SetPosUnk4UnkCY60(UNK_TYPE unk4, UNK_TYPE unkC, UNK_TYPE x) {
    SoundEventParams_SetPosUnk4UnkC(unk4, unkC, x, 0x3C);
}

void SoundEventParams_SetPosUnk14Y60(UNK_TYPE unk14, UNK_TYPE x) {
    SoundEventParams_SetPosUnk14(unk14, x, 0x3C);
}

void SoundEventParams_SetPosUnk8Unk14Y60(UNK_TYPE unk8, UNK_TYPE unk14, UNK_TYPE x) {
    SoundEventParams_SetPosUnk8Unk14(unk8, unk14, x, 0x3C);
}

// Finds first free slot in [0,9], installs arg1 there, returns slot index (or 10 if full)
s16 func_80067F78(void* arg0, UNK_TYPE arg1) {
    s16 i;

    for (i = 0; i < 10; i++) {
        if (func_80067930(arg0, i) == 0) {
            func_80067838(arg0, i, arg1);
            return i;
        }
    }
    return 10;
}

extern s32 gResetState;

// Initializes video, resets state flags, and triggers double scene load if gResetState == 2
void Main_SoftReset(s32 arg0) {
    s32* p = &gResetState;
    p += 0; // matching: forces access through pointer

    Idle_InitVideo();
    osViBlack(1);
    gRumbleStrength = 0;
    gRumbleTimer = 0;
    gRumbleDecay = 1;
    func_800678D0();
    func_800679B0(0);
    if (*p == 2) {
        func_80071D6C();
        func_80071D6C();
    }
}

void func_80068D24(void);
void func_8006E0FC(s32, s32, s32);

// Soft reset: Z held + START pressed triggers reinitialization
s32 Main_CheckSoftReset(GameMgr* mgr) {
    s32* p = &gResetState;
    p += 0; // matching: forces access through pointer

    if (*p != 0) {
        *(void (**)(s32))((u8*)mgr + 0x1C) = Main_SoftReset;
        return 1;
    }

    if (gGameMgr.activeMode == 7 || gGameMgr.activeMode == 8 || gGameMgr.activeMode == 5) {
        if (gGameMgr.unk4 == 0) {
            if ((gGameMgr.input->button & Z_TRIG) == Z_TRIG) {
                if ((*(u16*)((u8*)gGameMgr.input + 0xC) & START_BUTTON) == START_BUTTON) {
                    gGameMgr.unk4 = 1;
                    gRumbleStrength = 0;
                    gRumbleTimer = 0;
                    gRumbleDecay = 1;
                    func_800678D0();
                    func_800679B0(0x45);
                    func_80068D24();

                    *(s32*)((u8*)&gGameMgr + 0x3F0) = 0;

                    if (((*(u32*)((u8*)&gGameMgr + 0xFB8) << 4) >> 30) == 1) {
                        func_8006E0FC(0, 0, 9);
                    } else {
                        func_8006E0FC(0, 0, 0x29);
                    }
                    return 1;
                }
            }
        }
    }
    return 0;
}

// Instant death cheat: Z + L + A + B kills Yoshi regardless of health
s32 Main_InstantDeathCheat(void) {
    s32 v = gGameMgr.input->button;

    if ((v & Z_TRIG) == Z_TRIG && (v & L_TRIG) == L_TRIG && (v & A_BUTTON) == A_BUTTON && (v & B_BUTTON) == B_BUTTON) {
        // kill yoshi?
        s32 result = func_80047058();
        if (result != 0) {
            func_8002FC3C(result);
            return 1;
        }
    }
    return 0;
}

void Main_CheckResetOrCheat(GameMgr* mgr) {
    if (Main_CheckSoftReset(mgr) == 0) {
        Main_InstantDeathCheat();
    }
}

// Sound fade tick: counts down unk14E8 while setting volume unk14EA, then triggers fade-in/out via unk14EC
void Main_SoundFadeTick(void) {
    if (gGameMgr.unk5 == 2) {
        return;
    }
    // unk14E8: countdown timer (s32), unk14EA: volume/track param (s16), unk14EC: fade state flag (s32)
    if (*(s32*)((u8*)&gGameMgr + 0x14E8) != 0) {
        *(s32*)((u8*)&gGameMgr + 0x14E8) -= 1;
        func_80067ACC(0, *(s16*)((u8*)&gGameMgr + 0x14EA));
        if (*(s32*)((u8*)&gGameMgr + 0x14EC) != 0) {
            return;
        }
        func_800677F8(0x14);
        *(s32*)((u8*)&gGameMgr + 0x14EC) = 1;
    } else {
        if (*(s32*)((u8*)&gGameMgr + 0x14EC) == 0) {
            return;
        }
        func_800677F8(0x15);
        *(s32*)((u8*)&gGameMgr + 0x14EC) = 0;
    }
}

void Main_StopSoundFade(void) {
    if (D_800F938C != 0) {
        func_800677F8(0x15);
        func_80067ACC(0, 0);
        *(s32*)((u8*)&gGameMgr + 0x14EC) = 0;
        *(s32*)((u8*)&gGameMgr + 0x14E8) = 0;
    }
}

void Main_UpdateSoundFade(void) {
    Main_SoundFadeTick();
}

// Restricts input to Z + START and clears analog stick when unk5 is set
void Main_UpdateInput(GameMgr* mgr) {
    Input_CallUpdateFunc(mgr);
    if (gGameMgr.unk5 != 0) {
        gGameMgr.input->button &= 0x30C0; // effectively Z_TRIG | START_BUTTON (0xC0 bits are unused in hardware)
        gGameMgr.input->stick_x = 0;
        gGameMgr.input->stick_y = 0;
    }
    mgr->unk18();
}

// Allocates the frame image buffer, 64-byte aligns it, stores the aligned address in gGameMgr.frameImage.
// "FrameImage" is a best guess: payload is exactly one 320x240 16bpp frame and the 64-byte
// alignment fits an RDP image source. It is NOT one of the display framebuffers (allocated on demand,
// freed again, null-checked next to an F3DEX display list in 5E4C0.s @ 8005E558) - likely a capture/effect
// buffer. Rename once the pixel consumer is decompiled.
void Main_AllocFrameImage(void) {
    s32* allocRef;
    s32 buf;

    if (gFrameImageAlloc != 0) {
        return;
    }

    buf = gFrameImageAlloc = (s32)Heap_Alloc(SCREEN_WIDTH * SCREEN_HEIGHT * 2 + 64); // + 64 slack for 64-byte alignment
    allocRef = &gFrameImageAlloc;

    if (buf == 0) {
        Fault_HungUp("./main.c", 0x178);
        buf = *allocRef;
    }

    gGameMgr.frameImage = (u32)(buf + 0x3F) >> 6 << 6; // ALIGN64 (shift variant, macro uses & ~0x3F)
}

// Clears the raw alloc pointer and both GameMgr views of the frame image (frameImage + unkF0 latch)
void Main_ClearFrameImageRefs(void) {
    s32* a = (s32*)&gFrameImageAlloc;
    GameMgr* p = &gGameMgr;

    p->frameImage = (*a = 0); // chained assignment needed to match: forces IDO to materialize both addresses
    p->unkF0 = 0;
}

void Main_FreeFrameImage(void) {
    s32 temp = gFrameImageAlloc;

    if (temp != 0) {
        Heap_Free((Y65430UnkStruct*)temp);
        Main_ClearFrameImageRefs();
    }
}

// Register a callback into gGameMgr's callback array (max 20 slots)
// Stores {func, data} into the next callback slot and increments callbackCount.
// Pointer trick + count reset needed to match original compiler output.
s32 Main_RegisterCallback(void (*func)(UNK_TYPE), UNK_TYPE data) {
    s16 count = gGameMgr.callbackCount;
    void** entry;

    if (count >= 20) {
        return 0;
    }

    if (!func) {
        Fault_HungUp("./main.c", 424);
        return 0;
    }

    // These debug prints belong to scene lifecycle functions later in this file
    // (release = 解放, creation = 生成). Parked here to preserve rodata order;
    // move to the correct functions once they are identified.
    PRINTF("シーン情報の解放処理 +++++++++++ start\n"); // "Scene info release processing start"
    PRINTF("シーン情報の解放処理 +++++++++++ end\n");   // "Scene info release processing end"
    PRINTF("シーン情報の生成処理 +++++++++++ start\n"); // "Scene info creation processing start"
    PRINTF("シーン情報の生成処理 +++++++++++ end\n");   // "Scene info creation processing end"

    entry = (void**)&gGameMgr.callbacks[count];
    count = 0;
    entry[1] = (void*)func;
    entry[0] = (void*)data;
    gGameMgr.callbackCount++;

    return (void**)&gGameMgr.callbacks[count] != NULL;
}

// Dispatches queued callbacks, then clears count
void Main_DispatchCallbacks(GameMgr* arg0) {
    s16 count = arg0->callbackCount;
    u8* ptr = (u8*)arg0->callbacks;
    s16 i;

    if (count == 0) {
        return;
    }

    for (i = 0; i < count; i++) {
        ((void (*)(UNK_TYPE)) * (UNK_PTR*)(ptr + 4))(*(UNK_TYPE*)ptr);
        ptr += 8;
    }

    arg0->callbackCount = 0;
}

void Main_FrameCallback(GameMgr* arg0);

// Steady-state per-frame handler, invoked via gGameMgr.callback from the main loop:
// dispatches queued callbacks, advances the mesg-queue ring, latches frameImage into unkF0
void Main_FrameCallback(GameMgr* arg0) {
    func_80072FF0();
    Main_DispatchCallbacks(arg0);
    MesgQueueRing_Advance(arg0);
    gGameMgr.unkF0 = gGameMgr.frameImage;
    func_800716C0();
    func_800716F8();
}

// Initial value of gGameMgr.callback (installed at init); on its first run it
// swaps the steady-state Main_FrameCallback into place
void Main_FirstFrameCallback(GameMgr* arg0) {
    gGameMgr.unkF0 = gGameMgr.frameImage;
    func_800716C0();
    arg0->callback = (void (*)(UNK_TYPE))Main_FrameCallback;
    func_800716F8();
}

// Checks two status halfwords; clears the pending slot index and returns 1 if both are zero
s32 func_800685F0(void) {
    if (gGameMgr.unk992 != 0 || gGameMgr.unk9A6 != 0) {
        return 0;
    }

    // both branches store the same value (required for matching bnel pattern)
    if (gGameMgr.slot == 0) {
        gGameMgr.pendingSlot = 0;
    } else {
        gGameMgr.pendingSlot = 0;
    }
    return 1;
}

// Selects slot in gGameMgr: stores the index (+0x3EC = slot) and sets 9 pointers to per-slot sub-arrays
// (raw offsets are matching-load-bearing - do not convert to field accesses)
void Main_SelectSlot(s32 slot) {
    u8* base = (u8*)&gGameMgr;
    s32 idx;

    *(s32*)(base + 0x3EC) = slot;
    *(u8**)(base + 0x3F4) = base + 0x3F8 + slot * 128;
    *(u8**)(base + 0x478) = base + 0x47C + slot * 256;
    *(u8**)(base + 0x634) = base + 0x638 + slot * 664;
    *(u8**)(base + 0x970) = base + 0x974 + slot * 20;
    func_80044CA0();
    base = (u8*)&gGameMgr;
    idx = *(s32*)(base + 0x3EC);
    *(u8**)(base + 0x9DC) = base + 0x9E0 + idx * 20;
    *(u8**)(base + 0x8D0) = base + 0x8D4 + idx * 132;
    *(u8**)(base + 0xA68) = base + 0xA6C + idx * 20;
    *(u8**)(base + 0x13B8) = base + 0x13BC + idx * 300;
}

// Reset sequence: stops sound fade, clears/reinits manager sub-structs
// mgr is gGameMgr (GameMgr), registered via Main_StartModeChange -> modeResetFunc; invoked from Main_ResetMode's reset
// step
void Main_ResetSubsystems(u8* mgr) {
    Main_StopSoundFade();
    func_80052290(mgr + 0xAA0);
    func_8005A994(*(s32*)(mgr + 0x478), 0, 0);
    func_80045250();
    func_800581AC(mgr + 0xA68);
    func_8004404C(mgr + 0x958, 0);
    func_8005D160(mgr + 0x8D0);
    func_80046560();
    func_80051BE8(*(s32*)(mgr + 0x634));
    Mem_FillHalfwords(mgr + 0x478, 0x104, 0);
    func_8006D6B4(mgr + 0x57C);
}

// Sets CIC/boot-check hardware flags, resets the unk18 hook, calls 9 subsystem inits
// registered via Main_StartModeChange -> modeInitFunc; invoked at mode start by Main_ModeStartupPhase0
void Main_InitSubsystems(GameMgr* mgr) {
    D_800A6EB4 = (osCicId == 0x17D8);
    D_800A6EB8 = (*(s32*)PHYS_TO_K1(0x164) == 0x1EC6021);
    gGameMgr.unk18 = (void (*)(void))Stub_Return0;
    gGameMgr.activeMode = gGameMgr.mode;
    func_80052298((u8*)&mgr->unkAA0);
    func_8006DDCC((u8*)&mgr->unk57C);
    func_8006D1CC();
    func_800519C0((u8*)&mgr->unk634, D_800F843C, mgr->unk8D0);
    func_8006DE24((u8*)&mgr->unk57C);
    func_8006C77C((u8*)&mgr->unk3F4);
    func_8005A9BC((u8*)&mgr->unk478, D_800F843C);
    func_80058C7C();
    func_80044C58((u8*)&mgr->unk958);
}

// Conditionally calls func_8002ADE4 if func_8004700C passes and unk5 is clear
void func_80068890(UNK_TYPE arg0) {
    if (func_8004700C(arg0) != 0) {
        if (gGameMgr.unk5 == 0) {
            func_8002ADE4(arg0);
        }
    }
}

extern s32 D_800A6EBC;
extern void (*D_800F8DF8)(void);
void func_800493F4(void);

// Dispatches mode transition: invokes callback for mode 0x24, advances or resets subsystem entries
void func_800688CC(void) {
    void* entry;
    s32 flag;
    s32 mode;

    mode = *(s32*)((u8*)D_800F843C + 0x104);
    flag = 0;
    func_800493F4();

    // ^ 0: operand-order workaround
    if (mode != 0x46 && D_800A6EBC != (mode ^ 0)) {
        if (mode == 0x24) {
            D_800F8DF8();
            flag = 1;
        }
        func_800679B0(mode);
        D_800A6EBC = mode;
    } else {
        func_800679B0(0);
    }

    entry = *(void**)((u8*)D_800F843C + 0x118);
    if (entry != 0 && flag == 0) {
        func_800677F8();
    }
    entry = *(void**)((u8*)D_800F843C + 0x108);
    if (entry != 0) {
        func_80067838(entry, 0, 0);
    }
}

// Per-frame subsystem update: input scan + update calls on the same sub-structs Init/ResetSubsystems touch;
// returns func_80044B80's result (used as the ready check by Main_ModeStartupPhase1)
s32 Main_UpdateSubsystems(void* arg0) {
    s32 result;

    Main_UpdateInput(arg0);
    func_8006C848(*(s32*)((u8*)arg0 + 0x3F4));
    func_8005A484(*(s32*)((u8*)arg0 + 0x478));
    result = func_80044B80((u8*)arg0 + 0x958);
    func_8005F7D8((u8*)arg0 + 0x9C8);
    return result;
}

UNK_RET Stub_Return1(void) {
    return 1;
}

extern s32 D_800A8488;
extern u8 D_800A8520;
void Rumble_Reset(void);
void SaveData_Init(void);

// Resets mode state: clears flags, frees the frame image, reinits save data if flagged,
// then invokes the mode's modeResetFunc callback
void Main_ResetMode(void* arg0) {
    GameMgr* s0;

    Stub_Return1();
    s0 = &gGameMgr;
    s0 += 0;

    if (s0->unk20 == 0) {
        D_800A8488 = 1;
    }
    s0->unk20 = 0;

    s0->unk6 = 0;
    Main_FreeFrameImage();
    Rumble_Reset();

    if (func_80067930(*(void**)((u8*)s0->unk59C + 0x108), 0) == 1) {
        func_80067878(*(void**)((u8*)s0->unk59C + 0x108), 0);
    }
    D_800A8520 = 1;

    if (s0->unk0 == 1) {
        SaveData_Init();
        s0->unk0 = 0;
    }

    ((void (*)(void*))((GameMgr*)arg0)->modeResetFunc)(arg0);
}

// Startup phase 0: selects the pending slot, invokes modeInitFunc (+0xD0), mode dispatch, subsystem update
s32 Main_ModeStartupPhase0(void* arg0) {
    u8* p = (u8*)&gGameMgr;

    p[3] = 1;
    Main_SelectSlot(*(s32*)(p + 0x3F0));
    (*(void (**)(void*))((u8*)arg0 + 0xD0))(arg0);
    D_800F828A = 0;
    func_800688CC();
    Main_UpdateSubsystems(arg0);
    return 1;
}

// Startup phase 1: stalls until Main_UpdateSubsystems' ready check passes.
// Declared void but its table-call site uses $v0 = the callee's result (matching artifact - keep as is)
void Main_ModeStartupPhase1(void* arg0) {
    Main_UpdateSubsystems(arg0);
}

extern s32 gModeStartupPhase;
extern s32 (*gModeStartupPhases[])(void*);

// Steps through gModeStartupPhases (0 = stall on current phase); after both phases
// complete, invokes modeStartedFunc (+0xD4) and resets the index.
// The pointer local + if(1) boundary keep uopt from caching &gModeStartupPhase
// in a register (kpicopt web) so each access AT-folds like the target.
s32 Main_RunModeStartupPhases(arg0) void* arg0;
{
    s32* p = &gModeStartupPhase;
    s32 val;

    val = *p;
    if (gModeStartupPhases[val](arg0) == 0) {
        return 0;
    }

    val = *p + 1;
    if (1) {
        *p = val;
    }
    if (val < 2) {
        // val * 0 == 0; the extra val use colors its web v1 instead of v0
        return val * 0;
    }
    ((s32(*)(void*)) * (s32*)((u8*)arg0 + 0xD4))(arg0);
    gGameMgr.unk3 = 0;
    *p = 0;
    return 1;
}

// Mode-change state fn (used for 2nd+ mode change): resets the old mode, installs
// Main_FirstFrameCallback, kicks phase 0; returns 0 -> next state = Main_ModeStartupStep
s32 Main_ModeRestartStep(GameMgr* arg0) {
    func_80072FF0();
    Main_ResetMode(arg0);
    arg0->callback = (void (*)(UNK_TYPE))Main_FirstFrameCallback;
    Main_RunModeStartupPhases(arg0);
    return 0;
}

// Mode-change state fn (initial value of gModeChangeState): runs startup phases until done,
// then sets unk6 for modes 7/8 and clears D_800A8488; returns 1 when startup is complete
s32 Main_ModeStartupStep(void) {
    if (Main_RunModeStartupPhases() == 0) {
        return 0;
    }
    // Logically activeMode == 7 || activeMode == 8; the redundant == 5 / != 5 is needed for matching
    if ((gGameMgr.activeMode == 7 || gGameMgr.activeMode == 8 || gGameMgr.activeMode == 5) &&
        gGameMgr.activeMode != 5) {
        gGameMgr.unk6 = 1;
    } else {
        gGameMgr.unk6 = 0;
    }
    D_800A8488 = 0;
    return 1;
}

// tickFunc during a mode change: drives gModeChangeState each frame; when the state fn
// reports done, hands tickFunc to Main_GameTick and re-arms Main_ModeRestartStep for the next change
void Main_ModeChangeTick(GameMgr* arg0) {
    UNK_TYPE nextStep = (UNK_TYPE)Main_ModeStartupStep;

    if (((s32(*)(GameMgr*))gModeChangeState)(arg0) == 0) {
        gModeChangeState = nextStep;
    } else {
        gModeChangeState = (UNK_TYPE)Main_ModeRestartStep;
        arg0->tickFunc = (UNK_PTR)Main_GameTick;
    }
}

// Stores the 3 per-mode lifecycle callbacks and arms Main_ModeChangeTick as tickFunc
void Main_SetModeCallbacks(UNK_TYPE arg0, UNK_TYPE arg1, UNK_TYPE arg2) {
    gGameMgr.tickFunc = (UNK_PTR)Main_ModeChangeTick;
    gGameMgr.modeResetFunc = arg0;
    gGameMgr.modeInitFunc = arg1;
    gGameMgr.modeStartedFunc = arg2;
}

// Sets D_800F87F8 callback to func_800448BC and registers params via Main_SetModeCallbacks
void Main_StartModeChange(UNK_TYPE arg0, UNK_TYPE arg1, UNK_TYPE arg2) {
    D_800F87F8 = (UNK_PTR)func_800448BC;
    Main_SetModeCallbacks(arg0, arg1, arg2);
}

// Resets gGameMgr animation/state: sets counter to 0x1B, clears 12 fields
void func_80068D24(void) {
    u8* p;

    p = (u8*)&gGameMgr;
    *(s32*)(p + 0x5B4) = 0x1B;
    func_8006D1C0();
    func_8006D774(0);
    p = (u8*)&gGameMgr;
    *(s32*)(p + 0xAB0) = 0;
    *(s32*)(p + 0xAB4) = 0;
    *(s32*)(p + 0xAB8) = 0;
    *(s32*)(p + 0xABC) = 0;
    *(s32*)(p + 0xAC0) = 0;
    *(s32*)(p + 0xAC4) = 0;
    *(s32*)(p + 0xAC8) = 0;
    *(s32*)(p + 0xACC) = 0;
    *(s32*)(p + 0xAD0) = 0;
    *(s32*)(p + 0xAD4) = 0;
    *(s32*)(p + 0xB30) = 0;
    *(s32*)(p + 0xBAC) = 0;
}

void Stub_Empty(void) {
}

// Steady-state tickFunc (installed by Main_ModeChangeTick once startup completes):
// input, subsystem updates, sound fade, reset/cheat check
void Main_GameTick(GameMgr* arg0) {
    s32 unk478 = *(s32*)((u8*)arg0 + 0x478);

    D_800F828A = 0;
    Main_UpdateInput(arg0);
    if (gGameMgr.unk7 == 0 || gGameMgr.unk6 == 0) {
        func_8006C848(*(s32*)((u8*)arg0 + 0x3F4));
        func_8005A894(unk478);
    }
    func_8005A484(unk478);
    func_80044A54((u8*)arg0 + 0x958);
    func_8005F7D8((u8*)arg0 + 0x9C8);
    Main_UpdateSoundFade();
    Main_CheckResetOrCheat(arg0);
}

extern u8 D_800F9409;
void Rumble_Update(s32 /*arg0*/);

// Main loop: dispatches tickFunc, ticks frame counter, calls end-of-frame callback if set
void Main_Loop(void) {
    D_800F9409 = 1;

    for (;;) {
        ((void (*)(GameMgr*))gGameMgr.tickFunc)(&gGameMgr);
        Rumble_Update(gGameMgr.unk5 == 2);
        gGameMgr.frameCount++;
        Stub_Empty();
        func_80067AAC();
        if (gGameMgr.callback != 0) {
            ((void (*)(GameMgr*))gGameMgr.callback)(&gGameMgr);
            func_8005DB34();
        }
    }
}

GameMgr* Main_InitGameMgr(void) {
    GameMgr* p;

    Mem_FillHalfwords((MallocRecord*)&gGameMgr, 0x14F0, 0);
    p = &gGameMgr;
    p->unk0 = 1;
    return p;
}

// Main init: sets up game manager, initializes all subsystems, installs main loop callback
void Main_Init(void) {
    GameMgr* s0;
    u8* p;

    s0 = Main_InitGameMgr();
    Heap_Init();
    System_Init(s0);
    Rand_SetSeed(1);
    Rand_SetSeedF(1);
    MesgQueueRing_Init(s0);
    Input_Init(s0);
    Main_ClearFrameImageRefs();
    func_8006C95C((u8*)s0 + 0x3F4);
    func_80051C24((u8*)s0 + 0x634);
    func_800505F0();
    func_8005AA68((u8*)s0 + 0x478);
    func_80044E38((u8*)s0 + 0x958);
    func_8006DF14((u8*)s0 + 0x57C);
    func_80058160((u8*)s0 + 0xA68);
    func_8005D420((u8*)s0 + 0x8D0);
    func_8005F930(s0);
    *(void (**)(UNK_TYPE))((u8*)s0 + 0x24) = (void (*)(UNK_TYPE))Main_FirstFrameCallback;
    func_800522B8((u8*)s0 + 0xAA0);
    func_8006D1C0();
    func_80067B1C();
    p = (u8*)&gGameMgr;
    *(s32*)(p + 0xD8) = 0;
    *(s32*)(p + 0x3F0) = 0;
    func_8006E0FC(0, 0, 0);
    Main_ModeChangeTick(s0);
}

void Main_ThreadEntry(UNUSED void* arg) {
    Fault_Init();
    Main_Init();
    Main_Loop();
}

void func_800748D8(UNK_PTR, u8*);

// Rumble PWM over 4 controllers (all arrays u8[4]): strength (duty 0-100) adds into
// accum mod 100, carry = motor-on this frame; while timer > 0 it counts down, after
// that strength decays by gRumbleDecay toward 0. Flags go to padmgr's motor command
// (func_800748D8 -> osMotorStart/Stop in 740F0.s). arg0 != 0 forces motors off.
//
// Matching notes (lab/func_80069030/ v13-v59): three late-fold/`same-line constructs
// are load-bearing. `+ i * 0` keeps the gRumbleAccum materialization in-block (looks
// loop-variant at LICM, folds to 0 after). `+ arg0 * 0` on the gRumbleDecay base stays
// hoistable (invariant) but folds late, demoting that web below the 100/4 constant
// webs (t4 vs t2/t3). The merged declaration line makes the preheader materializations
// tie on line number so as1 schedules the addius in the target's LIFO order. The sum
// and rate/decay locals must stay inlined: re-reads CSE into one web (v0) and as1
// forwards the stored sum into `andi`, putting the arithmetic in ugen temps (t8/t9).
// clang-format off
void Rumble_Update(s32 arg0) {
    u8* carry = &gRumbleMotorOn; u8* timer = &gRumbleTimer; { s32 i = 0; do {
            if (arg0 != 0) {
                *carry = 0;
            } else {
                s32 val = (&gRumbleStrength)[i];

                if (val >= 101) {
                    (&gRumbleStrength)[i] = 100;
                    val = (&gRumbleStrength)[i];
                }

                if ((&gRumbleDecay + arg0 * 0)[i] >= 101) {
                    (&gRumbleDecay + arg0 * 0)[i] = 100;
                }

                {
                    u8* p = &gRumbleAccum + i * 0;
                    p[i] = p[i] + val;
                    if (p[i] >= 100) {
                        p[i] = p[i] - 100;
                        *carry = 1;
                    } else {
                        *carry = 0;
                    }
                }
            }

            carry++;

            {
                s32 tv = *timer;
                if (tv > 0) {
                    *timer = tv - 1;
                } else {
                    if ((&gRumbleDecay)[i] < (&gRumbleStrength)[i]) {
                        (&gRumbleStrength)[i] = (&gRumbleStrength)[i] - (&gRumbleDecay)[i];
                    } else {
                        (&gRumbleStrength)[i] = 0;
                    }
                }
            }

            timer++;
            i++;
        } while (i != 4);
    }

    func_800748D8(&D_8010DF40, &gRumbleMotorOn);
}
// clang-format on

// Clears channel 0 rumble state and asks padmgr to re-init the motors (cmd 3); from Main_ResetMode
void Rumble_Reset(void) {
    gRumbleStrength = 0;
    gRumbleTimer = 0;
    gRumbleDecay = 1;
    func_800748B0(&D_8010DF40);
}

typedef struct Y65520UnkStruct {
    /* 0x00 */ f32 unk0;
    /* 0x04 */ f32 unk4;
    /* 0x08 */ f32 unk8;
    /* 0x0C */ char pad0C[0x78];
    /* 0x84 */ f32 unk84;
    /* 0x88 */ f32 unk88;
    /* 0x8C */ f32 unk8C;
    /* 0x90 */ char pad90[0x1C];
    /* 0xAC */ f32 unkAC;
    /* 0xB0 */ f32 unkB0;
    /* 0xB4 */ char padB4[0x38];
    /* 0xEC */ void* unkEC;
    /* 0xF0 */ u8 unkF0;
    /* 0xF1 */ u8 unkF1;
    /* 0xF2 */ u8 unkF2;
    /* 0xF3 */ char padF3[5];
    /* 0xF8 */ u8 unkF8;
    /* 0xF9 */ u8 unkF9;
} Y65520UnkStruct; // size >= 0xFA

// 2 rows x 8 samples of one oscillation cycle: {0,1,2,1,0,-1,-2,-1} and {0,2,3.5,2,0,-2,-3.5,-2}
extern f32 D_800A6ED0[][8];
// Looks up a waveform value from D_800A6ED0[unkF2][arg2 & 7], updates unk4 via Math_AddViaThousandths,
// and accumulates into unkB0
void func_8006919C(f32* result, Y65520UnkStruct* arg1, u8 arg2) {
    *result = D_800A6ED0[arg1->unkF2][arg2 & 7];
    arg1->unk4 = Math_AddViaThousandths(arg1->unk4, *result);
    arg1->unkB0 += *result;
}

void func_8005A52C(Y65520UnkStruct* arg0);

s32 func_8006920C(Y65520UnkStruct* arg0) {
    u8 count = arg0->unkF0;
    f32 result = 0.0f;

    arg0->unkF8 = arg0->unkF9;
    arg0->unkF9 = 0;

    if (arg0->unkEC == (void*)Stub_Return0) {
        Vec3f_Copy(arg0, &arg0->unk84);
        arg0->unkAC = 0.0f;
        arg0->unkB0 = 0.0f;
    }

    if (count != 0) {
        func_8006919C(&result, arg0, count);
        arg0->unkF0 -= 1;
        arg0->unkF1 = arg0->unkF0 & 7;
    } else if (arg0->unkF1 != 0) {
        func_8006919C(&result, arg0, arg0->unkF1);
        arg0->unkF1 -= 1;
    }

    func_8005A52C(arg0);
    return (s32)result;
}

extern UNK_PTR D_800F8318;
void func_8004590C(void*, s32);

typedef struct {
    /* 0x00 */ char pad0[0x88];
    /* 0x88 */ s32 unk88;
} Y692D4Sub168; // size >= 0x8C

typedef struct {
    /* 0x00 */ char pad0[0x18];
    /* 0x18 */ s16 unk18;
    /* 0x1A */ s16 unk1A;
} Y692D4Sub174; // size >= 0x1C

typedef struct {
    /* 0x000 */ char pad0[0x34];
    /* 0x034 */ s32 x; // screen coords: bounds-checked [0,321)/[0,241), passed to coord ctor SoundEventParams_SetPos
    /* 0x038 */ s32 y;
    /* 0x03C */ char pad3C[0x12C];
    /* 0x168 */ Y692D4Sub168* unk168;
    /* 0x16C */ char pad16C[0x8];
    /* 0x174 */ Y692D4Sub174** unk174;
    /* 0x178 */ char pad178[0x74];
    /* 0x1EC */ s32 unk1EC;
} Y692D4Arg; // size >= 0x1F0

s32 func_800692D4(Y692D4Arg* arg0) {
    s32 mode;
    s32 base;

    base = arg0->unk168->unk88;

    if (((u8*)D_800F8318)[0xF8] != 0) {
        mode = arg0->x;
        if (mode >= 0 && mode < SCREEN_WIDTH + 1) {
            mode = arg0->y;
            if (mode >= 0) {
                if (mode < SCREEN_HEIGHT + 1) {
                    goto ok;
                }
            }
        }
    fail:
        return 0;
    ok:
        switch (arg0->unk1EC) {
            case 2:
                func_8004590C(arg0, base + 0x400);
                (*arg0->unk174)->unk18 = 0x4222;
                (*arg0->unk174)->unk1A = 0x2244;
                arg0->unk1EC = 0;
                Sound_PostEvent(0x26A, SoundEventParams_SetPos(arg0->x, arg0->y));
                return 1;
            case 0:
                func_8004590C(arg0, base + 0x600);
                (*arg0->unk174)->unk18 = 0x2224;
                (*arg0->unk174)->unk1A = 0x4244;
                arg0->unk1EC = 3;
                Sound_PostEvent(0x26A, SoundEventParams_SetPos(arg0->x, arg0->y));
                return 1;
            case 3:
                func_8004590C(arg0, base);
                (*arg0->unk174)->unk18 = 0x2422;
                (*arg0->unk174)->unk1A = 0x2444;
                arg0->unk1EC = 1;
                Sound_PostEvent(0x26A, SoundEventParams_SetPos(arg0->x, arg0->y));
                return 1;
            case 1:
                func_8004590C(arg0, base + 0x200);
                (*arg0->unk174)->unk18 = 0x2242;
                (*arg0->unk174)->unk1A = 0x2244;
                arg0->unk1EC = 2;
                Sound_PostEvent(0x26A, SoundEventParams_SetPos(arg0->x, arg0->y));
                return 1;
        }
    }
    return 0;
}

// Selects slot arg0 via D_800A6F10 and stores arg0 into bits 6-7 of unkFB8
void SaveData_SetSlot(s32 arg0) {
    u8 shifted;
    func_800679D0(D_800A6F10[arg0]);
    shifted = (arg0 << 3) << 3;
    gGameMgr.unkFB8 = shifted | (gGameMgr.unkFB8 & 0xFF3F);
}

// Packs count halfwords into bytes (s16[] -> u8[])
void SaveData_PackFields(u8* dest, s16* src, s32 count) {
    s32 i;
    for (i = 0; i < count; i++) {
        dest[i] = src[i];
    }
}

// Unpacks count bytes into halfwords (u8[] -> u16[])
void SaveData_UnpackFields(u16* dest, u8* src, s32 count) {
    s32 i;
    for (i = 0; i < count; i++) {
        dest[i] = src[i];
    }
}

// NOTE: gGameMgr+0xFB8 == gSaveData (0x800F8E58), so the byte offsets below land inside the
// EEPROM save image: 0xFC2 = gSaveData+0xA, 0xFEA = gSaveData+0x32, 0x13B4 = gSaveData+0x3FC
// (matching the sSaveFields scatter map entries at +0x8/+0x30/+0x3F0).

// Packs 6 halfwords into 6-byte record [index] of the stride-8 array at gSaveData+0xA
void SaveData_PackEntry(s32 index, void* src) {
    SaveData_PackFields((u8*)&gGameMgr + index * 8 + 0xFC2, src, 6);
}

// Unpacks 6-byte record [index] of the stride-8 array at gSaveData+0xA
void SaveData_UnpackEntry(s32 index, void* dest) {
    SaveData_UnpackFields(dest, (u8*)&gGameMgr + index * 8 + 0xFC2, 6);
}

// Packs 6 halfwords into record [row][col] of the stride-40 grid at gSaveData+0x32 (5 records per row)
void SaveData_PackGridEntry(s32 row, s32 col, void* src) {
    SaveData_PackFields((u8*)&gGameMgr + row * 40 + col * 8 + 0xFEA, src, 6);
}

// Unpacks record [row][col] of the stride-40 grid at gSaveData+0x32
void SaveData_UnpackGridEntry(s32 row, s32 col, void* dest) {
    SaveData_UnpackFields(dest, (u8*)&gGameMgr + row * 40 + col * 8 + 0xFEA, 6);
}

// If bit arg0 is set in unkFBC (= gSaveData+4), clears bit (arg0-1) within the lower 24 bits.
// (arg0+31 wraps mod 32 on MIPS sllv, so 1<<(arg0+31) == 1<<(arg0-1) for arg0>=1)
void func_80069728(s32 arg0) {
    s32 val = gGameMgr.unkFBC;
    s32* valp = &val; // regalloc: forces val into a1
    if (val & (1 << arg0)) {
        s32 masked = (*valp) & ((1 << (arg0 + 31)) ^ 0xFFFFFF);
        gGameMgr.unkFBC = masked;
    }
}

// Checks if any of 6 bit fields (5x 2-bit + 1x 3-bit) in the word at offset 0x13B4 (= gSaveData+0x3FC) are non-zero
int func_8006976C(void) {
    u32 val = *(u32*)((u8*)&gGameMgr + 0x13B4);

    return ((val << 3) >> 30) || ((val << 5) >> 30) || ((val << 7) >> 30) || ((val << 9) >> 30) ||
           ((val << 11) >> 30) || (*(u16*)((u8*)&gGameMgr + 0x13B4) & 7);
}

// All-zero default save image in .data (0x800 = full 16-kbit EEPROM); written when no valid save exists
typedef struct {
    u8 data[0x800];
} DefaultSaveData; // size = 0x800

extern DefaultSaveData sDefaultSaveData;

// Save-field descriptor: scatter/gather map between RAM save fields and the packed EEPROM image.
// 5 entries (gSaveData +0/+5/+8/+0x30/+0x3F0); sizes sum to 0x3FA = EepBuffer.data / checksum span.
typedef struct {
    /* 0x00 */ void* ptr;
    /* 0x04 */ s32 unk4; // not read by the load/save code (values 1/4/8/8/0xE)
    /* 0x08 */ s32 size;
} SaveField; // size = 0xC

extern SaveField sSaveFields[]; // table ends at &sDefaultSaveData (used as end sentinel)
s32 SaveData_ReadEeprom(UNK_PTR);

// Zero-fills the gSaveData region, reads the EEPROM image, then scatters it into the RAM
// save fields via sSaveFields. Returns 1 if both EEPROM copies are invalid.
s32 SaveData_Load(void) {
    u8 buf[0x400];
    u8* dst = buf;
    SaveField* entry;
    SaveField* end;

    Mem_FillHalfwords(&gSaveData, 0x400, 0);
    if (SaveData_ReadEeprom(buf) != 0) {
        return 1;
    }
    // clang-format off
    entry = sSaveFields; end = (SaveField*)&sDefaultSaveData;
    // clang-format on
    do {
        SaveField* cur = entry;
        Mem_CopyBytes(entry->ptr, dst, entry->size);
        dst += cur->size;
        entry++;
    } while (entry != end);
    return 0;
}

s32 SaveData_WriteEeprom(void* /*data*/);

// Gathers the RAM save fields via sSaveFields into a packed stack image, then writes it to EEPROM
void SaveData_Save(void) {
    u8 buf[0x400];
    u8* dst = buf;
    SaveField* entry;
    SaveField* end;

    // clang-format off
    entry = sSaveFields; end = (SaveField*)&sDefaultSaveData;
    // clang-format on
    do {
        SaveField* cur = entry;
        Mem_CopyBytes(dst, cur->ptr, cur->size);
        dst += cur->size;
        entry++;
    } while (entry != end);
    SaveData_WriteEeprom(buf);
}

// Copies sDefaultSaveData to stack and writes it to EEPROM via SaveData_WriteEeprom
void SaveData_WriteDefaults(void) {
    DefaultSaveData local = sDefaultSaveData;

    SaveData_WriteEeprom(&local);
}

void SaveData_StartEepMgr(void);

// Boot-time init: starts the eepmgr thread, loads the save; on failure re-clears the save
// region and writes the default image. Then applies the slot select held in gSaveData's
// first byte (top 2 bits) via SaveData_SetSlot.
void SaveData_Init(void) {
    SaveData_StartEepMgr();
    if (SaveData_Load() != 0) {
        Mem_FillHalfwords(&gSaveData, 0x400, 0);
        SaveData_WriteDefaults();
    }
    SaveData_SetSlot(gSaveData.allocP >> 0x1E);
}

// EEPROM data checksum: 16-bit rotate-left-by-1 with XOR over data[0..0x3F9]
u16 SaveData_Checksum(u8* data) {
    u32 crc = 0;
    s32 i;

    for (i = 0; i != 0x3FA; i++) {
        crc ^= data[i];
        crc <<= 1;
        crc = (crc & 0xFFFE) | ((crc >> 16) & 1);
    }
    return crc;
}

extern EepMgr gEepMgr;
extern EepBuffer sEepBuff[2];
extern STACK(sEepStack, 0x300);
extern EepBuffer sEepCache[2];
extern StackEntry sEepStackInfo;

// Stamps checksum + magic and writes data mirrored into both EEPROM copies; returns 1 on failure
s32 SaveData_WriteEeprom(void* data) {
    Mem_CopyHalfwords(&sEepBuff[0].data, data, ARRAY_COUNT(sEepBuff[0].data));
    sEepBuff[0].checksum = SaveData_Checksum(sEepBuff);
    sEepBuff[0].magic = EEPBUFFER_MAGIC;
    sEepBuff[1] = sEepBuff[0];

    eepmgr_SendWrite(&gEepMgr, sEepBuff);
    if (func_8007D508(&gEepMgr) != 0) {
        return 1;
    }

    return 0;
}

// Reads the EEPROM; copies out the first of the two copies that passes checksum + magic.
// Returns 1 if the read fails or both copies are invalid.
s32 SaveData_ReadEeprom(UNK_PTR dest) {
    eepmgr_SendRead(&gEepMgr, sEepBuff);

    if (func_8007D508(&gEepMgr) != 0) {
        return 1;
    }

    if ((sEepBuff[0].checksum == SaveData_Checksum(&sEepBuff[0])) && (sEepBuff[0].magic == EEPBUFFER_MAGIC)) {
        Mem_CopyHalfwords(dest, &sEepBuff[0].data, ARRAY_COUNT(sEepBuff[0].data));
        return 0;
    }

    if ((sEepBuff[1].checksum == SaveData_Checksum(&sEepBuff[1])) && (sEepBuff[1].magic == EEPBUFFER_MAGIC)) {
        Mem_CopyHalfwords(dest, &sEepBuff[1].data, ARRAY_COUNT(sEepBuff[1].data));
        return 0;
    }

    return 1;
}

void SaveData_StartEepMgr(void) {
    StackCheck_Init(&sEepStackInfo, sEepStack, STACK_TOP(sEepStack), 0, 0x100, "eepmgr");
    eepmgr_Create(&gEepMgr, &D_8010DF40, EEPROM_TYPE_16K, sEepCache, Y_THREAD_ID_EEPMGR, Y_PRIORITY_EEPMGR,
                  STACK_TOP(sEepStack));
}
