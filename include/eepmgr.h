#ifndef EEPMGR_H
#define EEPMGR_H

#include "ultra64.h"


typedef struct EepMgr {
    /* 0x000 */ OSMesg unk000[4];
    /* 0x010 */ OSMesg unk004[1];
    /* 0x014 */ OSMesgQueue unk014;
    /* 0x02C */ OSMesgQueue unk02C;
    /* 0x044 */ char unk_044[0x68 - 0x44];
    /* 0x068 */ OSThread thread;
    /* 0x218 */ s32 unk218;
    /* 0x21C */ u8 unk21C;
    /* 0x21D */ char unk_21D[0x1];
    /* 0x21E */ u16 unk21E;
    /* 0x220 */ s32 unk220;
    /* 0x224 */ char unk_224[0x4];
} EepMgr; // size = 0x228

#endif
