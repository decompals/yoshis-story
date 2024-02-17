#include "eepmgr.h"

#include "global.h"

s32 D_800A9A20 = 1;
u16 D_800A9A24[] = { 0, 0x40, 0x100 };

#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/func_8007CDA0.s")

#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/func_8007CDCC.s")

void func_8007CDFC(EepMgr* eepmgr);
#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/func_8007CDFC.s")

void func_8007CF50(EepMgr* eepmgr, s32 arg1, s32 arg2, s32 arg3) {
    bzero(eepmgr, sizeof(EepMgr));
    eepmgr->unk218 = arg1;
    eepmgr->unk220 = arg3;
    eepmgr->unk21C = arg2;
    eepmgr->unk21E = D_800A9A24[eepmgr->unk21C];
}

#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/func_8007CFB0.s")

#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/func_8007D0B0.s")

OSMesg func_8007D1EC(EepMgr* eepmgr, void* arg1);
#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/func_8007D1EC.s")

OSMesg func_8007D2C4(EepMgr* eepmgr, void* arg1);
#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/func_8007D2C4.s")

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
                osSendMesg(&eepmgr->unk02C, func_8007D2C4(eepmgr, req->unk4), OS_MESG_BLOCK);
                break;

            case 9:
                if (1) {}
                osSendMesg(&eepmgr->unk02C, func_8007D1EC(eepmgr, req->unk4), OS_MESG_BLOCK);
                break;

                //! FAKE:
                tmp = req->unk4;
            default:
                if (tmp != NULL) {}
        }
    } while (req->unk0 != 4);
}

void func_8007D488(EepMgr* eepmgr, s32 arg1, s32 arg2, s32 arg3, s32 id, s32 priority, void* stack) {
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
