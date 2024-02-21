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
        // osEepromProbe execution time=%d us
        (void)"osEepromProbe 実行時間=%d us\n";
        (void)OS_CYCLES_TO_USEC_ALT(after - before);
    }
    if (type == 0) {
        if (gEepmgrLogSeverity != 0) {
            // No EEPROM
            (void)"ＥＥＰＲＯＭ が ありません\n";
        }
    } else if (type == EEPROM_TYPE_4K) {
        if (gEepmgrLogSeverity != 0) {
            // 4K bit EEPROM detected
            (void)"４ＫビットＥＥＰＲＯＭを検出\n";
        }
    } else if (type == EEPROM_TYPE_16K) {
        if (gEepmgrLogSeverity != 0) {
            // 16K bit EEPROM detected
            (void)"１６ＫビットＥＥＰＲＯＭを検出\n";
        }
    } else {
        LOOP();
    }

    if (gEepmgrLogSeverity != 0) {}

    if (eepmgr->type != type) {
        LOOP();
    }
}

void eepmgr_Setup(EepMgr* eepmgr, UNK_PTR arg1, s32 type, EepBuffer* buffer) {
    bzero(eepmgr, sizeof(EepMgr));
    eepmgr->unk218 = arg1;
    eepmgr->cache = buffer;
    eepmgr->type = type;
    eepmgr->numBlocks = sMaxBlocks[eepmgr->type];
}

s32 eepmgr_Read(EepMgr* eepmgr, u8 addr, void* buffer) {
    s32 status;
    OSTime before;
    OSTime after;

    func_8007CDA0(eepmgr);

    eepmgr->operation = 1;
    before = osGetTime();
    status = osEepromRead(eepmgr->unk044, addr, buffer);
    after = osGetTime();
    eepmgr->operation = 0;

    func_8007CDCC(eepmgr);

    if (gEepmgrLogSeverity >= 2) {
        // osEepromRead execution time=%d us
        (void)"osEepromRead 実行時間=%d us\n";
        (void)OS_CYCLES_TO_USEC_ALT(after - before);
    }

    if (status != 0) {
        // EEPROM interface circuit unresponsive (READ)
        (void)"ＥＥＰＲＯＭ インターフェース回路反応なし (ＲＥＡＤ)\n";
        return -1;
    }

    if (gEepmgrLogSeverity != 0) {
        (void)"EEPROM READ  %02X: %02X %02X %02X %02X %02X %02X %02X %02X\n";
    }

    return 0;
}

s32 eepmgr_Write(EepMgr* eepmgr, u8 addr, void* buffer) {
    s32 status;
    OSTime before;
    OSTime after;

    func_8007CDA0(eepmgr);

    eepmgr->operation = 2;
    before = osGetTime();
    status = osEepromWrite(eepmgr->unk044, addr, buffer);
    after = osGetTime();
    eepmgr->operation = 0;

    func_8007CDCC(eepmgr);

    if (gEepmgrLogSeverity >= 2) {
        // osEepromWrite execution time=%d us
        (void)"osEepromWrite 実行時間=%d us\n";
        (void)OS_CYCLES_TO_USEC_ALT(after - before);
    }

    if (status != 0) {
        // EEPROM interface circuit unresponsive (WRITE)
        (void)"ＥＥＰＲＯＭ インターフェース回路反応なし (ＷＲＩＴＥ)\n";
        return -1;
    }

    if (gEepmgrLogSeverity != 0) {
        (void)"EEPROM WRITE %02X: %02X %02X %02X %02X %02X %02X %02X %02X\n";
    }

    // Sleep 15ms
    csleep((15 * osClockRate) / 1000ULL);
    return 0;
}

s32 eepmgr_HandleWrite(EepMgr* eepmgr, EepBuffer* buffer) {
    u64* bufferIter;
    u64* cacheIter;
    s32 i;

    if (eepmgr->type == 0) {
        // No EEPROM
        (void)"ＥＥＰＲＯＭ が ありません\n";
        return -1;
    }

    if (!eepmgr->cached) {
        // EEPROM not cached
        (void)"ＥＥＰＲＯＭ が キャッシュされていません\n";
        // Please read it once and then write it.
        (void)"一度読み込んでから、書くようにしてください\n";
        return -1;
    }

    cacheIter = (u64*)eepmgr->cache;
    bufferIter = (u64*)buffer;
    for (i = 0; i < eepmgr->numBlocks; i++) {
        if (*bufferIter != *cacheIter) {
            if (eepmgr_Write(eepmgr, i, bufferIter) != 0) {
                return -1;
            }

            *cacheIter = *bufferIter;
        }

        cacheIter++;
        bufferIter++;
    }

    return 0;
}

s32 eepmgr_HandleRead(EepMgr* eepmgr, EepBuffer* buffer) {
    u64* cacheIter;
    s32 i;

    if (eepmgr->type == 0) {
        // No EEPROM
        (void)"ＥＥＰＲＯＭ が ありません\n";
        return -1;
    }

    if (!eepmgr->cached) {
        cacheIter = (u64*)eepmgr->cache;
        for (i = 0; i < eepmgr->numBlocks; i++) {
            if (eepmgr_Read(eepmgr, i, cacheIter) != 0) {
                return -1;
            }
            cacheIter++;
        }
        eepmgr->cached = true;
    }

    bcopy(eepmgr->cache, buffer, eepmgr->numBlocks * EEPROM_BLOCK_SIZE);
    return 0;
}

void eepmgr_ThreadEntry(void* arg) {
    EepRequest* req = NULL;
    EepMgr* eepmgr = (EepMgr*)arg;

    if (gEepmgrLogSeverity != 0) {
        // EEPROM manager thread execution starts
        (void)"ＥＥＰＲＯＭマネージャスレッド実行開始\n";
    }
    eepmgr_Probe(eepmgr);
    do {
        if (gEepmgrLogSeverity != 0) {
            // eepmgr: Waiting for command message
            (void)"eepmgr:コマンドメッセージ待ち\n";
        }

        osRecvMesg(&eepmgr->unk014, (OSMesg*)&req, OS_MESG_BLOCK);
        switch (req->type) {
            case EEP_READ_MSG:
                if (gEepmgrLogSeverity != 0) {
                    // eepmgr: Load command received
                    (void)"eepmgr:ロードコマンド受信\n";
                }
                osSendMesg(&eepmgr->unk02C, (OSMesg)eepmgr_HandleRead(eepmgr, req->buffer), OS_MESG_BLOCK);
                break;

            case EEP_WRITE_MSG:
                if (gEepmgrLogSeverity != 0) {
                    // eepmgr: Receive save command
                    (void)"eepmgr:セーブコマンド受信\n";
                }
                osSendMesg(&eepmgr->unk02C, (OSMesg)eepmgr_HandleWrite(eepmgr, req->buffer), OS_MESG_BLOCK);
                break;

            default:
                break;
        }
    } while (req->type != 4);

    if (gEepmgrLogSeverity != 0) {
        // EEPROM manager thread execution ends
        (void)"ＥＥＰＲＯＭマネージャスレッド実行終了\n";
    }
}

void eepmgr_Create(EepMgr* eepmgr, UNK_PTR arg1, s32 type, EepBuffer* buffer, s32 id, s32 priority, void* stack) {
    (void)"eepmgr_Create(%08x, %08x, %d, %08x, %d, %d, %08x)\n";
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

void eepmgr_SendRead(EepMgr* eepmgr, EepBuffer* buffer) {
    EepRequest* req = &eepmgr->unk048;

    if (req->type != 0) {
        LOOP();
    }

    req->type = EEP_READ_MSG;
    req->buffer = buffer;
    osSendMesg(&eepmgr->unk014, &eepmgr->unk048, OS_MESG_BLOCK);
}

void eepmgr_SendWrite(EepMgr* eepmgr, EepBuffer* buffer) {
    EepRequest* req = &eepmgr->unk048;

    if (req->type != 0) {
        LOOP();
    }

    req->type = EEP_WRITE_MSG;
    req->buffer = buffer;
    osSendMesg(&eepmgr->unk014, &eepmgr->unk048, OS_MESG_BLOCK);
}
