#ifndef GLOBAL_H
#define GLOBAL_H

#include "ultra64.h"

#include "libc/stdbool.h"
#include "libc/stddef.h"
#include "libc/stdint.h"

#include "attributes.h"
#include "macros.h"
#include "unk.h"
#include "ys64math.h"

typedef struct Y65430UnkStruct {
    /* 0x00 */ u32 magic;
    /* 0x04 */ size_t unk04;
} Y65430UnkStruct; // size >= 0x08;

#define Y65430UnkStruct_MAGIC 0x434D5052 // 'CMPR'

// malloc
void* func_80064DD0(u32 size);

Y65430UnkStruct* func_80064D1C(u32);

void func_80064E84(Y65430UnkStruct*);

void func_8006F560(Y65430UnkStruct*, Y65430UnkStruct*);

// dma
void DmaMgr_RequestSync(void* vram, u32 vrom, u32 size);

// 80C50
void func_800800CC(MtxF* mfA, MtxF* mfB);
void func_80080914(MtxF* mf, f32, f32, f32, f32);
void func_80080824(MtxF* mf, f32 /*x*/, f32 /*y*/, f32 /*z*/);
void func_80080440(MtxF* mf, f32 /*x*/, f32 /*y*/);
void func_800804E0(MtxF* mf, f32 /*angle*/);
void func_800805C8(MtxF* mf, f32 /*angle*/);
void func_800806B0(MtxF* mf, f32 /*angle*/);
void func_80080798(MtxF* mf, f32 /*x*/, f32 /*y*/);
void func_80080C84(MtxF* mf, f32 /*x*/, f32 /*y*/, f32 /*z*/);

// loadfragment2
size_t Overlay_Load(uintptr_t vromStart, uintptr_t vromEnd, uintptr_t vramStart, uintptr_t vramEnd, void* allocatedRamAddr);
void* Overlay_AllocateAndLoad(uintptr_t vromStart, uintptr_t vromEnd, uintptr_t vramStart, uintptr_t vramEnd);


#endif
