#ifndef GLOBAL_H
#define GLOBAL_H

#include "ultra64.h"

#include "libc/stddef.h"
#include "libc/stdint.h"



typedef struct {
    /* 0x0 */ f32 x;
    /* 0x4 */ f32 y;
    /* 0x8 */ f32 z;
} Vec3f;

typedef float MtxF_t[4][4];
typedef union {
    MtxF_t mf;
    struct {
        float xx, yx, zx, wx, xy, yy, zy, wy, xz, yz, zz, wz, xw, yw, zw, ww;
    };
} MtxF; // size = 0x40

// mtxuty-cvt
void mtxcvt_F2L(Mtx* mtx, MtxF* mf);
void mtxcvt_L2F(MtxF* mtx, Mtx* mf);

// malloc
void* func_80064DD0(u32 size);

// dma
s32 func_8007DF0C(void* vramStart, u32 vromStart, u32 size);

// 80C50
void func_800800CC(MtxF* mfA, MtxF* mfB);
void func_80080914(MtxF* mf, f32, f32, f32, f32);
void func_80080824(MtxF* mf, f32, f32, f32);
void func_80080440(MtxF* mf, f32, f32);
void func_800804E0(MtxF* mf, f32);
void func_800805C8(MtxF* mf, f32);
void func_800806B0(MtxF* mf, f32);
void func_80080798(MtxF* mf, f32, f32);
void func_80080C84(MtxF* mf, f32, f32, f32);

// loadfragment2
size_t Load2_LoadOverlay(uintptr_t vRomStart, uintptr_t vRomEnd, uintptr_t vRamStart, uintptr_t vRamEnd, void* allocatedVRamAddr);
void* Load2_AllocateAndLoad(uintptr_t vRomStart, uintptr_t vRomEnd, uintptr_t vRamStart, uintptr_t vRamEnd);


#endif
