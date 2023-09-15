#ifndef GLOBAL_H
#define GLOBAL_H

#include "ultra64.h"

#include "libc/stddef.h"
#include "libc/stdint.h"

#include "attributes.h"
#include "macros.h"
#include "ys64math.h"

// malloc
void* func_80064DD0(u32 size);

// dma
s32 func_8007DF0C(void* vramStart, u32 vromStart, u32 size);

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
size_t Load2_LoadOverlay(uintptr_t vromStart, uintptr_t vromEnd, uintptr_t vramStart, uintptr_t vramEnd, void* allocatedRamAddr);
void* Load2_AllocateAndLoad(uintptr_t vromStart, uintptr_t vromEnd, uintptr_t vramStart, uintptr_t vramEnd);


#endif
