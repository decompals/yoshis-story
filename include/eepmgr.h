#ifndef EEPMGR_H
#define EEPMGR_H

#include "ultra64.h"
#include "unk.h"

#define EEPBUFFER_MAGIC 0x81317531

typedef struct EepBuffer {
    /* 0x000 */ u8 data[0x3FA];
    /* 0x3FA */ u16 unk3FA;
    /* 0x3FC */ s32 magic;
} EepBuffer; // size = 0x400

typedef struct EepRequest {
    /* 0x0 */ s16 type;
    /* 0x4 */ EepBuffer* buffer;
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
    /* 0x220 */ EepBuffer* cache;
    /* 0x224 */ u8 cached;
    /* 0x225 */ u8 operation;
} EepMgr; // size = 0x228


void eepmgr_Create(EepMgr* eepmgr, UNK_PTR arg1, s32 type, EepBuffer* buffer, s32 id, s32 priority, void* stack);
s32 func_8007D508(EepMgr* eepmgr);
void eepmgr_SendRead(EepMgr* eepmgr, EepBuffer* buffer);
void eepmgr_SendWrite(EepMgr* eepmgr, EepBuffer* buffer);

#endif
