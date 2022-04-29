#include "common.h"

typedef float f32;
extern f32 sinf(f32 x);
extern f32 cosf(f32 x);

f32 func_80081C30(f32 x) {
    return sinf(x) / cosf(x);
}

#pragma GLOBAL_ASM("asm/nonmatchings/main/mathf/func_80081C64.s")

#pragma GLOBAL_ASM("asm/nonmatchings/main/mathf/func_80081C84.s")

#pragma GLOBAL_ASM("asm/nonmatchings/main/mathf/func_80081CA4.s")

#pragma GLOBAL_ASM("asm/nonmatchings/main/mathf/func_80081CD8.s")

#pragma GLOBAL_ASM("asm/nonmatchings/main/mathf/func_80081D08.s")

#pragma GLOBAL_ASM("asm/nonmatchings/main/mathf/func_80081D28.s")

#pragma GLOBAL_ASM("asm/nonmatchings/main/mathf/func_80081D48.s")

#pragma GLOBAL_ASM("asm/nonmatchings/main/mathf/func_80081D68.s")

#pragma GLOBAL_ASM("asm/nonmatchings/main/mathf/func_80081E8C.s")

#pragma GLOBAL_ASM("asm/nonmatchings/main/mathf/func_80081F80.s")

#pragma GLOBAL_ASM("asm/nonmatchings/main/mathf/func_80081FB0.s")
