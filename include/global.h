#ifndef GLOBAL_H
#define GLOBAL_H

#include "ultra64.h"

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"

#include "attributes.h"
#include "eepmgr.h"
#include "macros.h"
#include "unk.h"
#include "ys64math.h"

struct Input;

#define Y65430UnkStruct_MAGIC 0x434D5052 // 'CMPR'

typedef void (*Y65520UnkFunc)(void*, UNK_TYPE, size_t);

typedef struct MallocRecord {
    /* 0x00 */ u32 allocP;
    /* 0x04 */ UNK_TYPE requestSize;
    /* 0x08 */ UNK_TYPE totalFree;
    /* 0x0C */ UNK_TYPE totalAlloc;
    /* 0x10 */ UNK_TYPE blockCnt;
} MallocRecord; // size >= 0x14

typedef struct Y65430UnkStruct {
    /* 0x00 */ u32 magic;
    /* 0x04 */ size_t unk04;
} Y65430UnkStruct; // size >= 0x08;

typedef struct Y511F0UnkStruct {
    /* 0x0 */ uintptr_t unk0;
    /* 0x4 */ s16 unk4;
    /* 0x8 */ uintptr_t unk8;
    /* 0xC */ uintptr_t unkC;
} Y511F0UnkStruct; // size = 0x10

typedef struct Y511F0UnkStruct2 {
    /* 0x0 */ char unk0[0x04];
    /* 0x4 */ UNK_TYPE unk4;
} Y511F0UnkStruct2; // size >= 0x8

// Functions

// 511F0
Y511F0UnkStruct2* func_80050C8C(u16 arg0);

// 65430
Y65430UnkStruct* func_80064D1C(u32);
void* func_80064DD0(u32 size); // malloc?
void func_80064E84(Y65430UnkStruct*);

// 65520
s32 func_80064FEC(void);
void func_80065528(void* arg0, void* arg1, s32 arg2);
void func_80065728(MallocRecord*, s32, s32);
void func_8006758C(void*, UNK_TYPE, size_t);
void func_800675D0(void*, UNK_TYPE, size_t, Y65520UnkFunc);

// 6DF40
void func_8006F500(void);

// 70160
void func_8006F560(Y65430UnkStruct*, Y65430UnkStruct*);

// 740F0
OSMesgQueue* func_800744B0(UNK_PTR arg0);
void func_800748B0(UNK_PTR arg0);
void func_8007451C(UNK_PTR arg0, OSMesgQueue* arg1);
void func_80074C88(UNK_PTR, struct Input*, s32);
void func_80075020(UNK_PTR, OSMesgQueue*, UNK_PTR, s32, s32, void*);

// dma
void DmaMgr_RequestSync(void* ram, u32 vrom, u32 size);

// 80C50
void func_800800CC(MtxF* mfA, MtxF* mfB);
void func_80080914(MtxF* mf, f32, f32, f32, f32);
void func_80080824(MtxF* mf, f32 x, f32 y, f32 z);
void func_80080440(MtxF* mf, f32 x, f32 y);
void func_800804E0(MtxF* mf, f32 angle);
void func_800805C8(MtxF* mf, f32 angle);
void func_800806B0(MtxF* mf, f32 angle);
void func_80080798(MtxF* mf, f32 x, f32 y);
void func_80080C84(MtxF* mf, f32 x, f32 y, f32 z);

// loadfragment2
size_t Overlay_Load(uintptr_t vromStart, uintptr_t vromEnd, uintptr_t vramStart, uintptr_t vramEnd, void* allocatedRamAddr);
void* Overlay_AllocateAndLoad(uintptr_t vromStart, uintptr_t vromEnd, uintptr_t vramStart, uintptr_t vramEnd);

// Data

// 92170
extern MallocRecord mallocRecord;

// Bss

extern UNK_TYPE D_800DA83C;
extern Y511F0UnkStruct D_800DA840[];
extern MallocRecord D_800F8E58;
extern s32 D_800F93F0;
extern s32 D_800F93F4;
extern u8 D_800FAAF8;
extern u8 D_800FAB1C;
extern u8 D_800FAB20;
extern UNK_TYPE D_801001F8;
extern UNK_TYPE D_80108620;
extern UNK_TYPE D_80108624;
extern UNK_TYPE D_80108628;
extern struct Input D_8010DEE0;
extern UNK_TYPE D_8010DF40;


#endif
