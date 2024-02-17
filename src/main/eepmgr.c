#include "eepmgr.h"

#include "global.h"

s32 D_800A9A20 = 1;
u16 D_800A9A24[] = { 0, 0x40, 0x100 };

#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/func_8007CDA0.s")

#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/func_8007CDCC.s")

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

#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/func_8007D1EC.s")

#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/func_8007D2C4.s")

#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/func_8007D380.s")

#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/func_8007D488.s")

#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/func_8007D508.s")

#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/func_8007D540.s")

#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/func_8007D590.s")

#pragma GLOBAL_ASM("asm/us/nonmatchings/main/eepmgr/D_800B2AE0.s")
