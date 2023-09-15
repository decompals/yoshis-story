#include "global.h"
#include "mtxconv.h"

// clang-format off
//! TODO: Determine where to put these macros
#define IPART(x) (((s32)((x) * 0x10000) >> 16) & 0xFFFF)
#define FPART(x)  ((s32)((x) * 0x10000) & 0xFFFF)

#define gdSPDefMtx( \
        xx, xy, xz, xw, \
        yx, yy, yz, yw, \
        zx, zy, zz, zw, \
        wx, wy, wz, ww) \
    { { \
        (IPART(xx) << 16) | IPART(yx), \
        (IPART(zx) << 16) | IPART(wx), \
        (IPART(xy) << 16) | IPART(yy), \
        (IPART(zy) << 16) | IPART(wy), \
        (IPART(xz) << 16) | IPART(yz), \
        (IPART(zz) << 16) | IPART(wz), \
        (IPART(xw) << 16) | IPART(yw), \
        (IPART(zw) << 16) | IPART(ww), \
        (FPART(xx) << 16) | FPART(yx), \
        (FPART(zx) << 16) | FPART(wx), \
        (FPART(xy) << 16) | FPART(yy), \
        (FPART(zy) << 16) | FPART(wy), \
        (FPART(xz) << 16) | FPART(yz), \
        (FPART(zz) << 16) | FPART(wz), \
        (FPART(xw) << 16) | FPART(yw), \
        (FPART(zw) << 16) | FPART(ww), \
    } }

Mtx gMtxClear = gdSPDefMtx(
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
);
// clang-format on

void func_8007FCF0(Mtx* arg0) {
    *arg0 = gMtxClear;
}

void func_8007FD30(Mtx* arg0, Mtx* arg1) {
    *arg0 = *arg1;
}

void func_8007FD6C(Mtx* arg0, Mtx* arg1) {
    MtxF sp58;
    MtxF sp18;

    MtxConv_L2F(&sp58, arg0);
    MtxConv_L2F(&sp18, arg1);
    func_800800CC(&sp58, &sp18);
    MtxConv_F2L(arg0, &sp58);
}

void func_8007FDBC(void) {
}

void func_8007FDC4(Mtx* arg0, f32 arg1, f32 arg2, f32 arg3, f32 arg4) {
    MtxF sp20;

    MtxConv_L2F(&sp20, arg0);
    func_80080914(&sp20, arg1, arg2, arg3, arg4);
    MtxConv_F2L(arg0, &sp20);
}

void func_8007FE20(Mtx* arg0, f32 arg1, f32 arg2, f32 arg3) {
    MtxF sp18;

    MtxConv_L2F(&sp18, arg0);
    func_80080824(&sp18, arg1, arg2, arg3);
    MtxConv_F2L(arg0, &sp18);
}

void func_8007FE74(Mtx* arg0, f32 arg1, f32 arg2) {
    MtxF sp18;

    MtxConv_L2F(&sp18, arg0);
    func_80080440(&sp18, arg1, arg2);
    MtxConv_F2L(arg0, &sp18);
}

void func_8007FEC0(void) {
}

void func_8007FEC8(Mtx* arg0, f32 arg1) {
    MtxF sp18;

    MtxConv_L2F(&sp18, arg0);
    func_800804E0(&sp18, arg1);
    MtxConv_F2L(arg0, &sp18);
}

void func_8007FF0C(Mtx* arg0, f32 arg1) {
    MtxF sp18;

    MtxConv_L2F(&sp18, arg0);
    func_800805C8(&sp18, arg1);
    MtxConv_F2L(arg0, &sp18);
}

void func_8007FF50(void) {
}

void func_8007FF58(Mtx* arg0, f32 arg1) {
    MtxF sp18;

    MtxConv_L2F(&sp18, arg0);
    func_800806B0(&sp18, arg1);
    MtxConv_F2L(arg0, &sp18);
}

void func_8007FF9C(void) {
}

void func_8007FFA4(Mtx* arg0, f32 arg1, f32 arg2) {
    MtxF sp18;

    MtxConv_L2F(&sp18, arg0);
    func_80080798(&sp18, arg1, arg2);
    MtxConv_F2L(arg0, &sp18);
}

void func_8007FFF0(Mtx* arg0, f32 arg1, f32 arg2, f32 arg3) {
    MtxF sp18;

    MtxConv_L2F(&sp18, arg0);
    func_80080C84(&sp18, arg1, arg2, arg3);
    MtxConv_F2L(arg0, &sp18);
}

void func_80080044(void) {
}
