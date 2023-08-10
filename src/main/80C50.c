#include "global.h"

extern MtxF D_800AA350;

// Matrix_Clear, sets mf to identity matrix
void func_80080050(MtxF* mf) {
    *mf = D_800AA350;
}

// Matrix_Copy, copies src to dest
void func_80080090(MtxF* dest, MtxF* src) {
    *dest = *src;
}

// Matrix_Mult, general matrix multiply
void func_800800CC(MtxF* mfA, MtxF* mfB) {
    f32 tempX;
    f32 tempY;
    f32 tempZ;
    f32 tempW;

    tempX = mfA->mf[0][0];
    tempY = mfA->mf[1][0];
    tempZ = mfA->mf[2][0];
    tempW = mfA->mf[3][0];
    mfA->mf[0][0] = (mfB->mf[0][0] * tempX) + (mfB->mf[0][1] * tempY) + (mfB->mf[0][2] * tempZ) + (mfB->mf[0][3] * tempW);
    mfA->mf[1][0] = (mfB->mf[1][0] * tempX) + (mfB->mf[1][1] * tempY) + (mfB->mf[1][2] * tempZ) + (mfB->mf[1][3] * tempW);
    mfA->mf[2][0] = (mfB->mf[2][0] * tempX) + (mfB->mf[2][1] * tempY) + (mfB->mf[2][2] * tempZ) + (mfB->mf[2][3] * tempW);
    mfA->mf[3][0] = (mfB->mf[3][0] * tempX) + (mfB->mf[3][1] * tempY) + (mfB->mf[3][2] * tempZ) + (mfB->mf[3][3] * tempW);

    tempX = mfA->mf[0][1];
    tempY = mfA->mf[1][1];
    tempZ = mfA->mf[2][1];
    tempW = mfA->mf[3][1];
    mfA->mf[0][1] = (mfB->mf[0][0] * tempX) + (mfB->mf[0][1] * tempY) + (mfB->mf[0][2] * tempZ) + (mfB->mf[0][3] * tempW);
    mfA->mf[1][1] = (mfB->mf[1][0] * tempX) + (mfB->mf[1][1] * tempY) + (mfB->mf[1][2] * tempZ) + (mfB->mf[1][3] * tempW);
    mfA->mf[2][1] = (mfB->mf[2][0] * tempX) + (mfB->mf[2][1] * tempY) + (mfB->mf[2][2] * tempZ) + (mfB->mf[2][3] * tempW);
    mfA->mf[3][1] = (mfB->mf[3][0] * tempX) + (mfB->mf[3][1] * tempY) + (mfB->mf[3][2] * tempZ) + (mfB->mf[3][3] * tempW);

    tempX = mfA->mf[0][2];
    tempY = mfA->mf[1][2];
    tempZ = mfA->mf[2][2];
    tempW = mfA->mf[3][2];
    mfA->mf[0][2] = (mfB->mf[0][0] * tempX) + (mfB->mf[0][1] * tempY) + (mfB->mf[0][2] * tempZ) + (mfB->mf[0][3] * tempW);
    mfA->mf[1][2] = (mfB->mf[1][0] * tempX) + (mfB->mf[1][1] * tempY) + (mfB->mf[1][2] * tempZ) + (mfB->mf[1][3] * tempW);
    mfA->mf[2][2] = (mfB->mf[2][0] * tempX) + (mfB->mf[2][1] * tempY) + (mfB->mf[2][2] * tempZ) + (mfB->mf[2][3] * tempW);
    mfA->mf[3][2] = (mfB->mf[3][0] * tempX) + (mfB->mf[3][1] * tempY) + (mfB->mf[3][2] * tempZ) + (mfB->mf[3][3] * tempW);

    tempX = mfA->mf[0][3];
    tempY = mfA->mf[1][3];
    tempZ = mfA->mf[2][3];
    tempW = mfA->mf[3][3];
    mfA->mf[0][3] = (mfB->mf[0][0] * tempX) + (mfB->mf[0][1] * tempY) + (mfB->mf[0][2] * tempZ) + (mfB->mf[0][3] * tempW);
    mfA->mf[1][3] = (mfB->mf[1][0] * tempX) + (mfB->mf[1][1] * tempY) + (mfB->mf[1][2] * tempZ) + (mfB->mf[1][3] * tempW);
    mfA->mf[2][3] = (mfB->mf[2][0] * tempX) + (mfB->mf[2][1] * tempY) + (mfB->mf[2][2] * tempZ) + (mfB->mf[2][3] * tempW);
    mfA->mf[3][3] = (mfB->mf[3][0] * tempX) + (mfB->mf[3][1] * tempY) + (mfB->mf[3][2] * tempZ) + (mfB->mf[3][3] * tempW);
}

// Scale x and y
void func_80080440(MtxF* mf, f32 x, f32 y) {
    if (x != 1.0f) {
        mf->mf[0][0] *= x;
        mf->mf[0][1] *= x;
        mf->mf[0][2] *= x;
        mf->mf[0][3] *= x;
    }
    if (y != 1.0f) {
        mf->mf[1][0] *= y;
        mf->mf[1][1] *= y;
        mf->mf[1][2] *= y;
        mf->mf[1][3] *= y;
    }
}

void func_800804E0(MtxF* mf, f32 angle) {
    f32 sin = sinf(angle);
    f32 cos = cosf(angle);
    f32 tempX;
    f32 tempY;

    tempX = mf->mf[0][0];
    tempY = mf->mf[1][0];
    mf->mf[0][0] = tempX * cos + tempY * sin;
    mf->mf[1][0] = tempY * cos - tempX * sin;

    tempX = mf->mf[0][1];
    tempY = mf->mf[1][1];
    mf->mf[0][1] = tempX * cos + tempY * sin;
    mf->mf[1][1] = tempY * cos - tempX * sin;

    tempX = mf->mf[0][2];
    tempY = mf->mf[1][2];
    mf->mf[0][2] = tempX * cos + tempY * sin;
    mf->mf[1][2] = tempY * cos - tempX * sin;

    tempX = mf->mf[0][3];
    tempY = mf->mf[1][3];
    mf->mf[0][3] = tempX * cos + tempY * sin;
    mf->mf[1][3] = tempY * cos - tempX * sin;
}

void func_800805C8(MtxF* mf, f32 angle) {
    f32 sin = sinf(angle);
    f32 cos = cosf(angle);
    f32 tempZ;
    f32 tempX;

    tempZ = mf->mf[2][0];
    tempX = mf->mf[0][0];
    mf->mf[2][0] = tempZ * cos + tempX * sin;
    mf->mf[0][0] = tempX * cos - tempZ * sin;

    tempZ = mf->mf[2][1];
    tempX = mf->mf[0][1];
    mf->mf[2][1] = tempZ * cos + tempX * sin;
    mf->mf[0][1] = tempX * cos - tempZ * sin;

    tempZ = mf->mf[2][2];
    tempX = mf->mf[0][2];
    mf->mf[2][2] = tempZ * cos + tempX * sin;
    mf->mf[0][2] = tempX * cos - tempZ * sin;

    tempZ = mf->mf[2][3];
    tempX = mf->mf[0][3];
    mf->mf[2][3] = tempZ * cos + tempX * sin;
    mf->mf[0][3] = tempX * cos - tempZ * sin;
}

void func_800806B0(MtxF* mf, f32 angle) {
    f32 sin = sinf(angle);
    f32 cos = cosf(angle);
    f32 tempY;
    f32 tempZ;

    tempY = mf->mf[1][0];
    tempZ = mf->mf[2][0];
    mf->mf[1][0] = tempY * cos + tempZ * sin;
    mf->mf[2][0] = tempZ * cos - tempY * sin;

    tempY = mf->mf[1][1];
    tempZ = mf->mf[2][1];
    mf->mf[1][1] = tempY * cos + tempZ * sin;
    mf->mf[2][1] = tempZ * cos - tempY * sin;

    tempY = mf->mf[1][2];
    tempZ = mf->mf[2][2];
    mf->mf[1][2] = tempY * cos + tempZ * sin;
    mf->mf[2][2] = tempZ * cos - tempY * sin;
    
    tempY = mf->mf[1][3];
    tempZ = mf->mf[2][3];
    mf->mf[1][3] = tempY * cos + tempZ * sin;
    mf->mf[2][3] = tempZ * cos - tempY * sin;
}

void func_80080798(MtxF* mf, f32 x, f32 y) {
    mf->mf[3][0] += (mf->mf[0][0] * x) + (mf->mf[1][0] * y);
    mf->mf[3][1] += (mf->mf[0][1] * x) + (mf->mf[1][1] * y);
    mf->mf[3][2] += (mf->mf[0][2] * x) + (mf->mf[1][2] * y);
    mf->mf[3][3] += (mf->mf[0][3] * x) + (mf->mf[1][3] * y);
}

void func_80080824(MtxF* mf, f32 x, f32 y, f32 z) {
    if (x != 1.0f) {
        mf->mf[0][0] *= x;
        mf->mf[0][1] *= x;
        mf->mf[0][2] *= x;
        mf->mf[0][3] *= x;
    }
    if (y != 1.0f) {
        mf->mf[1][0] *= y;
        mf->mf[1][1] *= y;
        mf->mf[1][2] *= y;
        mf->mf[1][3] *= y;
    }
    if (z != 1.0f) {
        mf->mf[2][0] *= z;
        mf->mf[2][1] *= z;
        mf->mf[2][2] *= z;
        mf->mf[2][3] *= z;
    }
}

void func_80080914(MtxF* mf, f32 arg1, f32 arg2, f32 arg3, f32 arg4);
#pragma GLOBAL_ASM("asm/nonmatchings/main/80C50/func_80080914.s")

void func_80080C84(MtxF* mf, f32 x, f32 y, f32 z) {
    s32 i;

    for (i = 0; i < 4; i++) {
        mf->mf[3][i] += (mf->mf[0][i] * x) + (mf->mf[1][i] * y) + (mf->mf[2][i] * z);        
    }
}

void func_80080D54(MtxF* mf, f32 x, f32 y) {
    if (x != 1.0f) {
        mf->mf[0][0] *= x;
        mf->mf[1][0] *= x;
        mf->mf[2][0] *= x;
        mf->mf[3][0] *= x;
    }
    if (y != 1.0f) {
        mf->mf[0][1]*= y;
        mf->mf[1][1] *= y;
        mf->mf[2][1] *= y;
        mf->mf[3][1] *= y;
    }
}

void func_80080DF4(MtxF* mf, f32 angle) {
    f32 sin = sinf(angle);
    f32 cos = cosf(angle);
    f32 tempY;
    f32 tempZ;

    tempY = mf->mf[0][1];
    tempZ = mf->mf[0][2];
    mf->mf[0][1] = cos * tempY - sin * tempZ;
    mf->mf[0][2] = sin * tempY + cos * tempZ;
    
    tempY = mf->mf[1][1];
    tempZ = mf->mf[1][2];
    mf->mf[1][1] = cos * tempY - sin * tempZ;
    mf->mf[1][2] = sin * tempY + cos * tempZ;
    
    tempY = mf->mf[2][1];
    tempZ = mf->mf[2][2];
    mf->mf[2][1] = cos * tempY - sin * tempZ;
    mf->mf[2][2] = sin * tempY + cos * tempZ;

    tempY = mf->mf[3][1];
    tempZ = mf->mf[3][2];
    mf->mf[3][1] = cos * tempY - sin * tempZ;
    mf->mf[3][2] = sin * tempY + cos * tempZ;
}

void func_80080EE0(MtxF* mf, f32 angle) {
    f32 sin = sinf(angle);
    f32 cos = cosf(angle);
    f32 tempZ;
    f32 tempX;

    tempZ = mf->mf[0][2];
    tempX = mf->mf[0][0];
    mf->mf[0][2] = cos * tempZ - sin * tempX;
    mf->mf[0][0] = sin * tempZ + cos * tempX;
    
    tempZ = mf->mf[1][2];
    tempX = mf->mf[1][0];
    mf->mf[1][2] = cos * tempZ - sin * tempX;
    mf->mf[1][0] = sin * tempZ + cos * tempX;
    
    tempZ = mf->mf[2][2];
    tempX = mf->mf[2][0];
    mf->mf[2][2] = cos * tempZ - sin * tempX;
    mf->mf[2][0] = sin * tempZ + cos * tempX;

    tempZ = mf->mf[3][2];
    tempX = mf->mf[3][0];
    mf->mf[3][2] = cos * tempZ - sin * tempX;
    mf->mf[3][0] = sin * tempZ + cos * tempX;
}

void func_80080FCC(MtxF* mf, f32 angle) {
    f32 sin = sinf(angle);
    f32 cos = cosf(angle);
    f32 tempX;
    f32 tempY;

    tempX = mf->mf[0][0];
    tempY = mf->mf[0][1];
    mf->mf[0][0] = cos * tempX - sin * tempY;
    mf->mf[0][1] = sin * tempX + cos * tempY;
    
    tempX = mf->mf[1][0];
    tempY = mf->mf[1][1];
    mf->mf[1][0] = cos * tempX - sin * tempY;
    mf->mf[1][1] = sin * tempX + cos * tempY;
    
    tempX = mf->mf[2][0];
    tempY = mf->mf[2][1];
    mf->mf[2][0] = cos * tempX - sin * tempY;
    mf->mf[2][1] = sin * tempX + cos * tempY;

    tempX = mf->mf[3][0];
    tempY = mf->mf[3][1];
    mf->mf[3][0] = cos * tempX - sin * tempY;
    mf->mf[3][1] = sin * tempX + cos * tempY;
}

void func_800810B8(MtxF* mf, f32 x, f32 y) {
    f32 temp;

    temp = mf->mf[0][3];
    if (temp != 0.0f) {
        mf->mf[0][0] += temp * x;
        mf->mf[0][1] += temp * y;
    }
    temp = mf->mf[1][3];
    if (temp != 0.0f) {
        mf->mf[1][0] += temp * x;
        mf->mf[1][1] += temp * y;
    }
    temp = mf->mf[2][3];
    if (temp != 0.0f) {
        mf->mf[2][0] += temp * x;
        mf->mf[2][1] += temp * y;
    }
    temp = mf->mf[3][3];
    if (temp != 0.0f) {
        if (temp != 1.0f) {
            mf->mf[3][0] += temp * x;
            mf->mf[3][1] += temp * y;
        } else {
            mf->mf[3][0] += x;
            mf->mf[3][1] += y;
        }
    }
}

void func_800811D0(MtxF* mf, f32 x, f32 y, f32 z) {
    f32 temp;

    temp = mf->mf[0][3];
    if (temp != 0.0f) {
        mf->mf[0][0] += temp * x;
        mf->mf[0][1] += temp * y;
        mf->mf[0][2] += temp * z;
    }
    temp = mf->mf[1][3];
    if (temp != 0.0f) {
        mf->mf[1][0] += temp * x;
        mf->mf[1][1] += temp * y;
        mf->mf[1][2] += temp * z;
    }
    temp = mf->mf[2][3];
    if (temp != 0.0f) {
        mf->mf[2][0] += temp * x;
        mf->mf[2][1] += temp * y;
        mf->mf[2][2] += temp * z;
    }
    temp = mf->mf[3][3];
    if (temp != 0.0f) {
        if (temp != 1.0f) {
            mf->mf[3][0] += temp * x;
            mf->mf[3][1] += temp * y;
            mf->mf[3][2] += temp * z;
        } else {
            mf->mf[3][0] += x;
            mf->mf[3][1] += y;
            mf->mf[3][2] += z;
        }
    }
}

void func_80081344(Vec3f* dest, Vec3f* src, MtxF* mf) {
    f32 x = src->x;
    f32 y = src->y;
    f32 z = src->z;

    dest->x = (x * mf->mf[0][0]) + (y * mf->mf[1][0]) + (z * mf->mf[2][0]) + mf->mf[3][0];
    dest->y = (x * mf->mf[0][1]) + (y * mf->mf[1][1]) + (z * mf->mf[2][1]) + mf->mf[3][1];
    dest->z = (x * mf->mf[0][2]) + (y * mf->mf[1][2]) + (z * mf->mf[2][2]) + mf->mf[3][2];
}

void func_800813E0(Vec3f* dest, Vec3f* src, MtxF* mf) {
    f32 x = src->x;
    f32 y = src->y;
    
    dest->x = ((x * mf->mf[0][0]) + (y * mf->mf[1][0])) + mf->mf[3][0];
    dest->y = (x * mf->mf[0][1]) + (y * mf->mf[1][1]) + mf->mf[3][1];
}
