#ifndef GLOBAL_H
#define GLOBAL_H

#include "ultratypes.h"

// Libultra
f32 sinf(f32 x);
f32 cosf(f32 x);

// mtxuty-cvt
void mtxcvt_F2L(Mtx* mtx, MtxF* mf);
void mtxcvt_L2F(MtxF* mtx, Mtx* mf);

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


#endif
