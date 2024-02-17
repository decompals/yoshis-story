#ifndef EEPMGR_H
#define EEPMGR_H

#include "ultra64.h"

typedef struct EepRequest {
    /* 0x0 */ s16 unk0;
    /* 0x4 */ void* unk4;
} EepRequest;

typedef struct EepRequest2 {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ s32 unk4;
} EepRequest2;

typedef struct EepMgr {
    /* 0x000 */ OSMesg unk000[4];
    /* 0x010 */ OSMesg unk004[1];
    /* 0x014 */ OSMesgQueue unk014;
    /* 0x02C */ OSMesgQueue unk02C;
    /* 0x044 */ OSMesgQueue* unk044;
    /* 0x048 */ char unk_048[0x68 - 0x48];
    /* 0x068 */ OSThread thread;
    /* 0x218 */ s32 unk218;
    /* 0x21C */ u8 unk21C;
    /* 0x21E */ u16 unk21E;
    /* 0x220 */ EepRequest2* unk220;
    /* 0x224 */ u8 unk224;
    /* 0x225 */ u8 unk225;
} EepMgr; // size = 0x228

#endif
