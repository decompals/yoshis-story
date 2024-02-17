#include "eepmgr.h"

#include "global.h"
#include "sleep.h"

#define LOOP()   \
    while (true) \
    (void)0

s32 D_800A9A20 = 1;
u16 D_800A9A24[] = { 0, 0x200 / 8, 0x800 / 8 };

#define OS_CYCLES_TO_USEC_ALT(c) (((u64)(c) * (1000000LL / 15625LL)) / (osClockRate / 15625LL))

void func_8007CDA0(EepMgr* eepmgr) {
    eepmgr->unk044 = func_800744B0(eepmgr->unk218);
}

void func_8007CDCC(EepMgr* eepmgr) {
    func_8007451C(eepmgr->unk218, eepmgr->unk044);
    eepmgr->unk044 = NULL;
}

void func_8007CDFC(EepMgr* eepmgr) {
    s32 status;
    OSTime before;
    OSTime after;

    func_8007CDA0(eepmgr);

    eepmgr->unk225 = 1;
    before = osGetTime();
    status = osEepromProbe(eepmgr->unk044);
    after = osGetTime();
    eepmgr->unk225 = 0;

    func_8007CDCC(eepmgr);

    if (D_800A9A20 >= 3) {
        (void)OS_CYCLES_TO_USEC_ALT(after - before);
    }
    if (status == 0) {
        if (D_800A9A20 != 0) {}
    } else if (status == 1) {
        if (D_800A9A20 != 0) {}
    } else if (status == 2) {
        if (D_800A9A20 != 0) {}
    } else {
        LOOP();
    }

    if (D_800A9A20 != 0) {}

    if (eepmgr->unk21C != status) {
        LOOP();
    }
}

void func_8007CF50(EepMgr* eepmgr, s32 arg1, s32 arg2, void* arg3) {
    bzero(eepmgr, sizeof(EepMgr));
    eepmgr->unk218 = arg1;
    eepmgr->unk220 = arg3;
    eepmgr->unk21C = arg2;
    eepmgr->unk21E = D_800A9A24[eepmgr->unk21C];
}

s32 func_8007CFB0(EepMgr* eepmgr, u8 arg1, EepRequest2* arg2) {
    s32 status;
    OSTime before;
    OSTime after;

    func_8007CDA0(eepmgr);

    eepmgr->unk225 = 1;
    before = osGetTime();
    status = osEepromRead(eepmgr->unk044, arg1, (u8*)arg2);
    after = osGetTime();
    eepmgr->unk225 = 0;

    func_8007CDCC(eepmgr);

    if (D_800A9A20 >= 2) {
        (void)OS_CYCLES_TO_USEC_ALT(after - before);
    }

    if (status != 0) {
        return -1;
    }

    if (D_800A9A20 != 0) {}

    return 0;
}

s32 func_8007D0B0(EepMgr* eepmgr, u8 arg1, EepRequest2* arg2) {
    s32 status;
    OSTime before;
    OSTime after;

    func_8007CDA0(eepmgr);

    eepmgr->unk225 = 2;
    before = osGetTime();
    status = osEepromWrite(eepmgr->unk044, arg1, (u8*)arg2);
    after = osGetTime();
    eepmgr->unk225 = 0;

    func_8007CDCC(eepmgr);

    if (D_800A9A20 >= 2) {
        (void)OS_CYCLES_TO_USEC_ALT(after - before);
    }

    if (status != 0) {
        return -1;
    }

    if (D_800A9A20 != 0) {}

    csleep((15 * osClockRate) / 1000ULL);
    return 0;
}

#ifdef NON_MATCHING
s32 func_8007D1EC(EepMgr* eepmgr, EepRequest2* arg1) {
    EepRequest2* var_s0;
    EepRequest2* var_s1;
    s32 var_s2;

    if (eepmgr->unk21C == 0) {
        return -1;
    }
    if (eepmgr->unk224 == 0) {
        return -1;
    }

    for (var_s1 = eepmgr->unk220, var_s0 = arg1, var_s2 = 0; var_s2 < eepmgr->unk21E; var_s1++, var_s0++, var_s2++) {
        if ((var_s1->unk0 == var_s0->unk0) && (var_s1->unk4 == var_s0->unk4)) {
            continue;
        }

        if (func_8007D0B0(eepmgr, var_s2, var_s0)) {
            return -1;
        }
        var_s1->unk0 = var_s0->unk0;
        var_s1->unk4 = var_s0->unk4;
    }

    return 0;
}
#else
s32 func_8007D1EC(EepMgr* eepmgr, EepRequest2* arg1);
#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/func_8007D1EC.s")
#endif

s32 func_8007D2C4(EepMgr* eepmgr, EepRequest2* arg1) {
    s32 var_s0;
    EepRequest2* req;

    if (eepmgr->unk21C == 0) {
        return -1;
    }

    if (eepmgr->unk224 == 0) {
        for (req = eepmgr->unk220, var_s0 = 0; var_s0 < eepmgr->unk21E; req++, var_s0++) {
            if (func_8007CFB0(eepmgr, var_s0, req)) {
                return -1;
            }
        }
        eepmgr->unk224 = 1;
    }

    bcopy(eepmgr->unk220, arg1, eepmgr->unk21E * 8);
    return 0;
}

void func_8007D380(void* arg) {
    EepRequest* req = NULL;
    EepMgr* eepmgr = (EepMgr*)arg;
    void* tmp;

    func_8007CDFC(eepmgr);
    do {
        osRecvMesg(&eepmgr->unk014, (OSMesg*)&req, OS_MESG_BLOCK);
        switch (req->unk0) {
            case 8:
                if (1) {}
                osSendMesg(&eepmgr->unk02C, (OSMesg)func_8007D2C4(eepmgr, req->unk4), OS_MESG_BLOCK);
                break;

            case 9:
                if (1) {}
                osSendMesg(&eepmgr->unk02C, (OSMesg)func_8007D1EC(eepmgr, req->unk4), OS_MESG_BLOCK);
                break;

                //! FAKE:
                tmp = req->unk4;
            default:
                if (tmp != NULL) {}
        }
    } while (req->unk0 != 4);
}

void func_8007D488(EepMgr* eepmgr, s32 arg1, s32 arg2, void* arg3, s32 id, s32 priority, void* stack) {
    func_8007CF50(eepmgr, arg1, arg2, arg3);
    osCreateMesgQueue(&eepmgr->unk014, eepmgr->unk000, ARRAY_COUNT(eepmgr->unk000));
    osCreateMesgQueue(&eepmgr->unk02C, eepmgr->unk004, ARRAY_COUNT(eepmgr->unk004));
    osCreateThread(&eepmgr->thread, id, func_8007D380, eepmgr, stack, priority);
    osStartThread(&eepmgr->thread);
}

void func_8007D508(EepMgr* eepmgr);
#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/func_8007D508.s")

void func_8007D540(EepMgr* eepmgr, s32 arg1);
#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/func_8007D540.s")

void func_8007D590(EepMgr* eepmgr, s32 arg1);
#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/func_8007D590.s")

#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/D_800B2AE0.s")
