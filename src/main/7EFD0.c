#include "global.h"

typedef struct Struct8010ED90 {
    /* 0x0 */ UNK_TYPE unk0;
    /* 0x4 */ char* unk4;
} Struct8010ED90; // size = 0x8

s32 D_800A9A50 = 0;

Struct8010ED90 D_8010ED90[100];
static s32 sBssPad;

void func_8007E3D0(UNK_TYPE arg0, char* arg1) {
    if (D_800A9A50 < 100) {
        D_8010ED90[D_800A9A50].unk0 = arg0;
        D_8010ED90[D_800A9A50].unk4 = arg1;
        D_800A9A50++;
    }
}

char* func_8007E40C(UNK_TYPE arg0) {
    s32 i;

    for (i = 0; i < D_800A9A50; i++) {
        if (D_8010ED90[i].unk0 == arg0) {
            return D_8010ED90[i].unk4;
        }
    }
    return "???";
}

void func_8007E458(void) {
    D_800A9A50 = 0;
}
