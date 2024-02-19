#include "eepmgr.h"

#include "global.h"
#include "sleep.h"

#define LOOP()   \
    while (true) \
    (void)0

s32 gEepmgrLogSeverity = 1;
static u16 sMaxBlocks[] = { 0, EEPROM_MAXBLOCKS, EEP16K_MAXBLOCKS };

#define EEP_READ_MSG 8
#define EEP_WRITE_MSG 9

//! TODO: Probably a libultra macro 
#define OS_CYCLES_TO_USEC_ALT(c) (((u64)(c) * (1000000LL / 15625LL)) / (osClockRate / 15625LL))

void func_8007CDA0(EepMgr* eepmgr) {
    eepmgr->unk044 = func_800744B0(eepmgr->unk218);
}

void func_8007CDCC(EepMgr* eepmgr) {
    func_8007451C(eepmgr->unk218, eepmgr->unk044);
    eepmgr->unk044 = NULL;
}

void eepmgr_Probe(EepMgr* eepmgr) {
    s32 type;
    OSTime before;
    OSTime after;

    func_8007CDA0(eepmgr);

    eepmgr->operation = 1;
    before = osGetTime();
    type = osEepromProbe(eepmgr->unk044);
    after = osGetTime();
    eepmgr->operation = 0;

    func_8007CDCC(eepmgr);

    if (gEepmgrLogSeverity >= 3) {
        (void)OS_CYCLES_TO_USEC_ALT(after - before);
    }
    if (type == 0) {
        if (gEepmgrLogSeverity != 0) {}
    } else if (type == EEPROM_TYPE_4K) {
        if (gEepmgrLogSeverity != 0) {}
    } else if (type == EEPROM_TYPE_16K) {
        if (gEepmgrLogSeverity != 0) {}
    } else {
        LOOP();
    }

    if (gEepmgrLogSeverity != 0) {}

    if (eepmgr->type != type) {
        LOOP();
    }
}

void eepmgr_Setup(EepMgr* eepmgr, s32 arg1, s32 type, u64* buffer) {
    bzero(eepmgr, sizeof(EepMgr));
    eepmgr->unk218 = arg1;
    eepmgr->buffer = buffer;
    eepmgr->type = type;
    eepmgr->numBlocks = sMaxBlocks[eepmgr->type];
}

s32 eepmgr_Read(EepMgr* eepmgr, u8 arg1, u64* buffer) {
    s32 status;
    OSTime before;
    OSTime after;

    func_8007CDA0(eepmgr);

    eepmgr->operation = 1;
    before = osGetTime();
    status = osEepromRead(eepmgr->unk044, arg1, (u8*)buffer);
    after = osGetTime();
    eepmgr->operation = 0;

    func_8007CDCC(eepmgr);

    if (gEepmgrLogSeverity >= 2) {
        (void)OS_CYCLES_TO_USEC_ALT(after - before);
    }

    if (status != 0) {
        return -1;
    }

    if (gEepmgrLogSeverity != 0) {}

    return 0;
}

s32 eepmgr_Write(EepMgr* eepmgr, u8 addr, u64* buffer) {
    s32 status;
    OSTime before;
    OSTime after;

    func_8007CDA0(eepmgr);

    eepmgr->operation = 2;
    before = osGetTime();
    status = osEepromWrite(eepmgr->unk044, addr, (u8*)buffer);
    after = osGetTime();
    eepmgr->operation = 0;

    func_8007CDCC(eepmgr);

    if (gEepmgrLogSeverity >= 2) {
        (void)OS_CYCLES_TO_USEC_ALT(after - before);
    }

    if (status != 0) {
        return -1;
    }

    if (gEepmgrLogSeverity != 0) {}

    // Sleep 15ms
    csleep((15 * osClockRate) / 1000ULL);
    return 0;
}

s32 eepmgr_HandleWrite(EepMgr* eepmgr, u64* buffer) {
    u64* readBuffer;
    u64* iter;
    s32 i;

    if (eepmgr->type == 0) {
        return -1;
    }

    if (!eepmgr->hasData) {
        return -1;
    }

    iter = eepmgr->buffer;
    readBuffer = buffer;
    for (i = 0; i < eepmgr->numBlocks; i++) {
        if (*readBuffer != *iter) {
            if (eepmgr_Write(eepmgr, i, readBuffer) != 0) {
                return -1;
            }

            *iter = *readBuffer;
        }

        iter++;
        readBuffer++;
    }

    return 0;
}

s32 eepmgr_HandleRead(EepMgr* eepmgr, u64* arg1) {
    u64* iter;
    s32 i;

    if (eepmgr->type == 0) {
        return -1;
    }

    if (!eepmgr->hasData) {
        iter = eepmgr->buffer;
        for (i = 0; i < eepmgr->numBlocks; i++) {
            if (eepmgr_Read(eepmgr, i, iter) != 0) {
                return -1;
            }
            iter++;
        }
        eepmgr->hasData = true;
    }

    bcopy(eepmgr->buffer, arg1, eepmgr->numBlocks * EEPROM_BLOCK_SIZE);
    return 0;
}

void eepmgr_ThreadEntry(void* arg) {
    EepRequest* req = NULL;
    EepMgr* eepmgr = (EepMgr*)arg;
    void* tmp;

    eepmgr_Probe(eepmgr);
    do {
        osRecvMesg(&eepmgr->unk014, (OSMesg*)&req, OS_MESG_BLOCK);
        switch (req->type) {
            case EEP_READ_MSG:
                if (1) {}
                osSendMesg(&eepmgr->unk02C, (OSMesg)eepmgr_HandleRead(eepmgr, req->buffer), OS_MESG_BLOCK);
                break;

            case EEP_WRITE_MSG:
                if (1) {}
                osSendMesg(&eepmgr->unk02C, (OSMesg)eepmgr_HandleWrite(eepmgr, req->buffer), OS_MESG_BLOCK);
                break;

                //! FAKE:
                tmp = req->buffer;
            default:
                if (tmp != NULL) {}
        }
    } while (req->type != 4);
}

void eepmgr_Create(EepMgr* eepmgr, s32 arg1, s32 type, u64* buffer, s32 id, s32 priority, void* stack) {
    eepmgr_Setup(eepmgr, arg1, type, buffer);
    osCreateMesgQueue(&eepmgr->unk014, eepmgr->unk000, ARRAY_COUNT(eepmgr->unk000));
    osCreateMesgQueue(&eepmgr->unk02C, eepmgr->unk004, ARRAY_COUNT(eepmgr->unk004));
    osCreateThread(&eepmgr->thread, id, eepmgr_ThreadEntry, eepmgr, stack, priority);
    osStartThread(&eepmgr->thread);
}

s32 func_8007D508(EepMgr* eepmgr) {
    s32 msg;

    osRecvMesg(&eepmgr->unk02C, (OSMesg*)&msg, OS_MESG_BLOCK);
    eepmgr->unk048.type = 0;
    return msg;
}

void eepmgr_SendRead(EepMgr* eepmgr, void* buffer) {
    EepRequest* req = &eepmgr->unk048;

    if (req->type != 0) {
        LOOP();
    }

    req->type = EEP_READ_MSG;
    req->buffer = buffer;
    osSendMesg(&eepmgr->unk014, &eepmgr->unk048, OS_MESG_BLOCK);
}

void eepmgr_SendWrite(EepMgr* eepmgr, void* buffer) {
    EepRequest* req = &eepmgr->unk048;

    if (req->type != 0) {
        LOOP();
    }

    req->type = EEP_WRITE_MSG;
    req->buffer = buffer;
    osSendMesg(&eepmgr->unk014, &eepmgr->unk048, OS_MESG_BLOCK);
}

#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/D_800B2AE0.s")
