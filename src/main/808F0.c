#include "global.h"

extern Mtx D_800AA310; // Identity mtx

void func_8007FCF0(Mtx* arg0) {
    *arg0 = D_800AA310;
}

void func_8007FD30(Mtx* arg0, Mtx* arg1) {
    *arg0 = *arg1;
}

void func_8007FD6C(Mtx* arg0, Mtx* arg1) {
    MtxF sp58;
    MtxF sp18;

    mtxcvt_L2F(&sp58, arg0);
    mtxcvt_L2F(&sp18, arg1);
    func_800800CC(&sp58, &sp18);
    mtxcvt_F2L(arg0, &sp58);
}

void func_8007FDBC(void) {
}

void func_8007FDC4(Mtx* arg0, f32 arg1, f32 arg2, f32 arg3, f32 arg4) {
    MtxF sp20;

    mtxcvt_L2F(&sp20, arg0);
    func_80080914(&sp20, arg1, arg2, arg3, arg4);
    mtxcvt_F2L(arg0, &sp20);
}

void func_8007FE20(Mtx* arg0, f32 arg1, f32 arg2, f32 arg3) {
    MtxF sp18;

    mtxcvt_L2F(&sp18, arg0);
    func_80080824(&sp18, arg1, arg2, arg3);
    mtxcvt_F2L(arg0, &sp18);
}

void func_8007FE74(Mtx* arg0, f32 arg1, f32 arg2) {
    MtxF sp18;

    mtxcvt_L2F(&sp18, arg0);
    func_80080440(&sp18, arg1, arg2);
    mtxcvt_F2L(arg0, &sp18);
}

void func_8007FEC0(void) {
}

void func_8007FEC8(Mtx* arg0, f32 arg1) {
    MtxF sp18;

    mtxcvt_L2F(&sp18, arg0);
    func_800804E0(&sp18, arg1);
    mtxcvt_F2L(arg0, &sp18);
}

void func_8007FF0C(Mtx* arg0, f32 arg1) {
    MtxF sp18;

    mtxcvt_L2F(&sp18, arg0);
    func_800805C8(&sp18, arg1);
    mtxcvt_F2L(arg0, &sp18);
}

void func_8007FF50(void) {
}

void func_8007FF58(Mtx* arg0, f32 arg1) {
    MtxF sp18;

    mtxcvt_L2F(&sp18, arg0);
    func_800806B0(&sp18, arg1);
    mtxcvt_F2L(arg0, &sp18);
}

void func_8007FF9C(void) {
}

void func_8007FFA4(Mtx* arg0, f32 arg1, f32 arg2) {
    MtxF sp18;

    mtxcvt_L2F(&sp18, arg0);
    func_80080798(&sp18, arg1, arg2);
    mtxcvt_F2L(arg0, &sp18);
}

void func_8007FFF0(Mtx* arg0, f32 arg1, f32 arg2, f32 arg3) {
    MtxF sp18;

    mtxcvt_L2F(&sp18, arg0);
    func_80080C84(&sp18, arg1, arg2, arg3);
    mtxcvt_F2L(arg0, &sp18);
}

void func_80080044(void) {
}
