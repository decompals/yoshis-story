#ifndef EEPMGR_H
#define EEPMGR_H

#include "ultra64.h"


typedef struct EepMgr {
    /* 0x000 */ char unk_000[0x218];
    /* 0x218 */ s32 unk218;
    /* 0x21C */ u8 unk21C;
    /* 0x21D */ char unk_21D[0x1];
    /* 0x21E */ u16 unk21E;
    /* 0x220 */ s32 unk220;
    /* 0x224 */ char unk_224[0x4];
} EepMgr; // size = 0x228

#endif
