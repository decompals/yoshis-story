#include "global.h"

void func_80064830(Y65430UnkStruct* arg0) {
    func_80064E84(arg0);
}

void* func_80064850(u16 arg0) {
    s32 sp1C = func_80050C8C(arg0)->unk4;
    void* sp18 = func_80064DD0(0x18);

    func_800675D0(sp18, sp1C, 0x18, func_8006758C);
    return sp18;
}

void func_800648A4(Y65430UnkStruct** arg0) {
    if (*arg0 != NULL) {
        func_80064830(*arg0);
        *arg0 = NULL;
    }
}

s32 func_800648DC(void** arg0, u16 arg1) {
    void* temp_v0 = func_80064850(arg1);

    if (temp_v0 == NULL) {
        return 0;
    }
    *arg0 = temp_v0;
    return 1;
}
