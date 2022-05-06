#ifndef GLOBAL_H
#define GLOBAL_H

typedef signed char s8;
typedef unsigned char u8;
typedef signed short int s16;
typedef unsigned short int u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long int s64;
typedef unsigned long long int u64;
typedef volatile u8 vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile u64 vu64;
typedef volatile s8 vs8;
typedef volatile s16 vs16;
typedef volatile s32 vs32;
typedef volatile s64 vs64;
typedef float f32;
typedef double f64;
typedef void* TexturePtr;
typedef long int Mtx_t[4][4];
typedef union {
    Mtx_t m;
    long long int forc_structure_alignment;
} Mtx;
typedef float MtxF_t[4][4];
typedef union {
    MtxF_t mf;
    struct {
        float xx, xy, xz, xw,
              yx, yy, yz, yw,
              zx, zy, zz, zw,
              wx, wy, wz, ww;
    };
} MtxF;



void mtxcvt_F2L(Mtx* mtx, MtxF* mf);
void mtxcvt_L2F(MtxF* mtx, Mtx* mf);

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
