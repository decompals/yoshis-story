#ifndef EEPMGR_H
#define EEPMGR_H

#include "ultra64.h"
#include "unk.h"


typedef struct EepRequest {
    /* 0x0 */ s16 type;
    /* 0x4 */ u64* buffer;
} EepRequest;

typedef struct EepMgr {
    /* 0x000 */ OSMesg unk000[4];
    /* 0x010 */ OSMesg unk004[1];
    /* 0x014 */ OSMesgQueue unk014;
    /* 0x02C */ OSMesgQueue unk02C;
    /* 0x044 */ OSMesgQueue* unk044;
    /* 0x048 */ EepRequest unk048;
    /* 0x050 */ char unk_04C[0x68 - 0x50];
    /* 0x068 */ OSThread thread;
    /* 0x218 */ UNK_PTR unk218;
    /* 0x21C */ u8 type;
    /* 0x21E */ u16 numBlocks;
    /* 0x220 */ u64* cache;
    /* 0x224 */ u8 cached;
    /* 0x225 */ u8 operation;
} EepMgr; // size = 0x228

#endif
