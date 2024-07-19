#include "fault.h"

#include "lib/ultralib/src/libc/xstdio.h"
#include "libc/stdbool.h"
#include "libc/stddef.h"
#include "libc/stdint.h"
#include "PR/os_internal.h"

#include "global.h"

#include "macros.h"
#include "pad.h"
#include "sleep.h"
#include "stack.h"
#include "stackcheck.h"
#include "stdarg.h"
#include "terminal.h"
#include "ys64thread.h"

typedef struct FaultMgr {
    /* 0x000 */ OSThread thread;
    /* 0x1B0 */ STACK(stack, 0x400);
    /* 0x5B0 */ OSMesgQueue queue;
    /* 0x5C8 */ OSMesg msg[1];
    /* 0x5CC */ u16* framebuf;
    /* 0x5D0 */ u16 width;
    /* 0x5D2 */ u16 depth;
} FaultMgr; // size = 0x5D4

vu32 fault_exit = false;
vu32 fault_msg_id = 0;
vs32 fault_display_enable = false;

u32 sFaultFont[] = {
#include "assets/us/main/fault/sFaultFont.font.inc.c"
};

const char* sExceptionNames[] = {
    "Interrupt",
    "TLB modification",
    "TLB exception on load",
    "TLB exception on store",
    "Address error on load",
    "Address error on store",
    "Bus error on inst.",
    "Bus error on data",
    "System call exception",
    "Breakpoint exception",
    "Reserved instruction",
    "Coprocessor unusable",
    "Arithmetic overflow",
    "Trap exception",
    "Virtual coherency on inst.",
    "Floating point exception",
    "Watchpoint exception",
    "Virtual coherency on data",
};

const char* sFpExceptionNames[] = {
    "Unimplemented operation", "Invalid operation", "Division by zero", "Overflow", "Underflow", "Inexact operation",
};

u16 sFaultFontColor = GPACK_RGBA5551(255, 255, 255, 1);
static s32 sPad = 0;

Input sFaultInputs[4];
s32 D_80108690;
FaultMgr debugger;
STACK(sFaultStack, 0x400);
StackEntry sFaultStackInfo;
uintptr_t D_80109090[16];

void Fault_DrawRec(s32 x, s32 y, s32 w, s32 h) {
    u16* fb = debugger.framebuf + debugger.width * y + x;
    s32 j;
    s32 i;

    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            *fb++ = GPACK_RGBA5551(0, 0, 0, 1);
        }
        fb += debugger.width - w;
    }
}

void Fault_PrintCharImpl(s32 x, s32 y, char fmt) {
    u32* fontPtr = sFaultFont + (((fmt / 8) * 16) + ((fmt & 4) >> 2));
    u16* fb = debugger.framebuf + debugger.width * y + x;
    s32 i;
    s32 j;

    for (i = 0; i < 8; i++) {
        u32 mask = 0x10000000 << (fmt % 4);
        u32 font = *fontPtr;

        for (j = 0; j < 8; j++) {
            if (mask & font) {
                *fb = sFaultFontColor;
            }
            mask >>= 4;
            fb++;
        }
        fontPtr += 2;
        fb += debugger.width - 8;
    }
    osWritebackDCacheAll();
}

void Fault_PrintChar(s32 x, s32 y, char fmt) {
    Fault_PrintCharImpl(x, y, fmt);
}

typedef struct FaultDrawerPos {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
} FaultDrawerPos;

void* Fault_PrintCallback(char* arg, const char* fmt, size_t count) {
    FaultDrawerPos* pos = (FaultDrawerPos*)arg;

    for (; count > 0; count--) {
        Fault_PrintChar(pos->x, pos->y, *fmt++);
        pos->x += 6;
    }

    return pos;
}

s32 Fault_Printf(s32 x, s32 y, const char* fmt, ...) {
    va_list ap;
    FaultDrawerPos pos;
    s32 ret;

    va_start(ap, fmt);

    pos.x = x;
    pos.y = y;
    ret = _Printf((outfun*)*Fault_PrintCallback, (char*)&pos, fmt, ap);

    va_end(ap);

    return ret;
}

void Fault_Sleep(s32 ms) {
    u64 cycles = (ms * osClockRate) / 1000ULL;

    csleep(cycles);
}

void Fault_PrintFReg(s32 x, s32 y, s32 index, f32* value) {
    u32 raw = *(u32*)value;
    s32 exp = ((raw & 0x7F800000) >> 0x17) - 0x7F;

    if ((exp >= -0x7E && exp < 0x80) || raw == 0) {
        Fault_Printf(x, y, "F%02d:%.7e", index, *value);
    } else {
        Fault_Printf(x, y, "F%02d:-------------", index);
    }
}

void Fault_LogFReg(s32 index, f32* value) {
    u32 raw = *(u32*)value;
    s32 exp = ((raw & 0x7F800000) >> 0x17) - 0x7F;

    if (((exp >= -0x7E) && (exp < 0x80)) || raw == 0) {
        osSyncPrintf("F%02d:%.7e     ", index, *value);
    } else {
        osSyncPrintf("F%02d:-------------     ", index);
    }
}

void Fault_PrintFPCSR(s32 x, s32 y, u32 value) {
    s32 i;
    u32 flag = FPCSR_CE;

    Fault_Printf(x, y, "FPCSR:%08XH", value);

    // Go through each of the six causes and print the name of
    // the first cause that is set
    for (i = 0; i < ARRAY_COUNT(sFpExceptionNames); i++) {
        if (value & flag) {
            Fault_Printf(x + 100, y, "(%s)", sFpExceptionNames[i]);
            break;
        }
        flag >>= 1;
    }
}

void Fault_LogFPCSR(u32 value) {
    s32 i;
    u32 flag = FPCSR_CE;

    osSyncPrintf("FPCSR:%08XH  ", value);
    for (i = 0; i < ARRAY_COUNT(sFpExceptionNames); i++) {
        if (value & flag) {
            osSyncPrintf("(%s)\n", sFpExceptionNames[i]);
            break;
        }
        flag >>= 1;
    }
}

void Fault_PrintThreadContext(OSThread* thread) {
    __OSThreadContext* ctx = &thread->context;
    s32 y = 20;
    s16 causeStrIndex = (thread->context.cause >> 2) & 0x1F;

    if (causeStrIndex == (EXC_WATCH >> CAUSE_EXCSHIFT)) {
        causeStrIndex = 16;
    }
    if (causeStrIndex == (EXC_VCED >> CAUSE_EXCSHIFT)) {
        causeStrIndex = 17;
    }

    Fault_DrawRec(22, 16, 276, 24);
    Fault_Printf(30, y, "THREAD:%d  (%s)", thread->id, sExceptionNames[causeStrIndex]);

    y += 9;
    Fault_Printf(30, y, "PC:%08XH   SR:%08XH   VA:%08XH", (uintptr_t)ctx->pc, (uintptr_t)ctx->sr,
                 (uintptr_t)ctx->badvaddr);

    y += 13;
    Fault_DrawRec(22, 40, 276, 184);
    Fault_Printf(30, y, "AT:%08XH   V0:%08XH   V1:%08XH", (uintptr_t)ctx->at, (uintptr_t)ctx->v0, (uintptr_t)ctx->v1);

    y += 9;
    Fault_Printf(30, y, "A0:%08XH   A1:%08XH   A2:%08XH", (uintptr_t)ctx->a0, (uintptr_t)ctx->a1, (uintptr_t)ctx->a2);

    y += 9;
    Fault_Printf(30, y, "A3:%08XH   T0:%08XH   T1:%08XH", (uintptr_t)ctx->a3, (uintptr_t)ctx->t0, (uintptr_t)ctx->t1);

    y += 9;
    Fault_Printf(30, y, "T2:%08XH   T3:%08XH   T4:%08XH", (uintptr_t)ctx->t2, (uintptr_t)ctx->t3, (uintptr_t)ctx->t4);

    y += 9;
    Fault_Printf(30, y, "T5:%08XH   T6:%08XH   T7:%08XH", (uintptr_t)ctx->t5, (uintptr_t)ctx->t6, (uintptr_t)ctx->t7);

    y += 9;
    Fault_Printf(30, y, "S0:%08XH   S1:%08XH   S2:%08XH", (uintptr_t)ctx->s0, (uintptr_t)ctx->s1, (uintptr_t)ctx->s2);

    y += 9;
    Fault_Printf(30, y, "S3:%08XH   S4:%08XH   S5:%08XH", (uintptr_t)ctx->s3, (uintptr_t)ctx->s4, (uintptr_t)ctx->s5);

    y += 9;
    Fault_Printf(30, y, "S6:%08XH   S7:%08XH   T8:%08XH", (uintptr_t)ctx->s6, (uintptr_t)ctx->s7, (uintptr_t)ctx->t8);

    y += 9;
    Fault_Printf(30, y, "T9:%08XH   GP:%08XH   SP:%08XH", (uintptr_t)ctx->t9, (uintptr_t)ctx->gp, (uintptr_t)ctx->sp);

    y += 9;
    Fault_Printf(30, y, "S8:%08XH   RA:%08XH", (uintptr_t)ctx->s8, (uintptr_t)ctx->ra);

    y += 13;
    Fault_PrintFPCSR(30, y, ctx->fpcsr);

    y += 13;
    Fault_PrintFReg(30, y, 0, &ctx->fp0.f.f_even);
    Fault_PrintFReg(160, y, 2, &ctx->fp2.f.f_even);

    y += 9;
    Fault_PrintFReg(30, y, 4, &ctx->fp4.f.f_even);
    Fault_PrintFReg(160, y, 6, &ctx->fp6.f.f_even);

    y += 9;
    Fault_PrintFReg(30, y, 8, &ctx->fp8.f.f_even);
    Fault_PrintFReg(160, y, 10, &ctx->fp10.f.f_even);

    y += 9;
    Fault_PrintFReg(30, y, 12, &ctx->fp12.f.f_even);
    Fault_PrintFReg(160, y, 14, &ctx->fp14.f.f_even);

    y += 9;
    Fault_PrintFReg(30, y, 16, &ctx->fp16.f.f_even);
    Fault_PrintFReg(160, y, 18, &ctx->fp18.f.f_even);

    y += 9;
    Fault_PrintFReg(30, y, 20, &ctx->fp20.f.f_even);
    Fault_PrintFReg(160, y, 22, &ctx->fp22.f.f_even);

    y += 9;
    Fault_PrintFReg(30, y, 24, &ctx->fp24.f.f_even);
    Fault_PrintFReg(160, y, 26, &ctx->fp26.f.f_even);

    y += 9;
    Fault_PrintFReg(30, y, 28, &ctx->fp28.f.f_even);
    Fault_PrintFReg(160, y, 30, &ctx->fp30.f.f_even);

    osWritebackDCacheAll();
}

void Fault_LogThreadContext(OSThread* thread) {
    __OSThreadContext* ctx = &thread->context;
    s16 causeStrIndex = (thread->context.cause >> 2) & 0x1F;

    if (causeStrIndex == (EXC_WATCH >> CAUSE_EXCSHIFT)) {
        causeStrIndex = 16;
    }
    if (causeStrIndex == (EXC_VCED >> CAUSE_EXCSHIFT)) {
        causeStrIndex = 17;
    }

    osSyncPrintf("\n");
    osSyncPrintf("THREAD(%08x)\n", thread);
    osSyncPrintf("THREAD ID:%d (%s)\n", thread->id, sExceptionNames[causeStrIndex]);

    osSyncPrintf("PC:%08XH   SR:%08XH   VA:%08XH\n", (uintptr_t)ctx->pc, (uintptr_t)ctx->sr, (uintptr_t)ctx->badvaddr);
    osSyncPrintf("AT:%08XH   V0:%08XH   V1:%08XH\n", (uintptr_t)ctx->at, (uintptr_t)ctx->v0, (uintptr_t)ctx->v1);
    osSyncPrintf("A0:%08XH   A1:%08XH   A2:%08XH\n", (uintptr_t)ctx->a0, (uintptr_t)ctx->a1, (uintptr_t)ctx->a2);
    osSyncPrintf("A3:%08XH   T0:%08XH   T1:%08XH\n", (uintptr_t)ctx->a3, (uintptr_t)ctx->t0, (uintptr_t)ctx->t1);
    osSyncPrintf("T2:%08XH   T3:%08XH   T4:%08XH\n", (uintptr_t)ctx->t2, (uintptr_t)ctx->t3, (uintptr_t)ctx->t4);
    osSyncPrintf("T5:%08XH   T6:%08XH   T7:%08XH\n", (uintptr_t)ctx->t5, (uintptr_t)ctx->t6, (uintptr_t)ctx->t7);
    osSyncPrintf("S0:%08XH   S1:%08XH   S2:%08XH\n", (uintptr_t)ctx->s0, (uintptr_t)ctx->s1, (uintptr_t)ctx->s2);
    osSyncPrintf("S3:%08XH   S4:%08XH   S5:%08XH\n", (uintptr_t)ctx->s3, (uintptr_t)ctx->s4, (uintptr_t)ctx->s5);
    osSyncPrintf("S6:%08XH   S7:%08XH   T8:%08XH\n", (uintptr_t)ctx->s6, (uintptr_t)ctx->s7, (uintptr_t)ctx->t8);
    osSyncPrintf("T9:%08XH   GP:%08XH   SP:%08XH\n", (uintptr_t)ctx->t9, (uintptr_t)ctx->gp, (uintptr_t)ctx->sp);
    osSyncPrintf("S8:%08XH   RA:%08XH           \n", (uintptr_t)ctx->s8, (uintptr_t)ctx->ra);
    osSyncPrintf("\n");

    Fault_LogFPCSR(ctx->fpcsr);
    osSyncPrintf("\n");

    Fault_LogFReg(0, &ctx->fp0.f.f_even);
    Fault_LogFReg(2, &ctx->fp2.f.f_even);
    osSyncPrintf("\n");
    Fault_LogFReg(4, &ctx->fp4.f.f_even);
    Fault_LogFReg(6, &ctx->fp6.f.f_even);
    osSyncPrintf("\n");
    Fault_LogFReg(8, &ctx->fp8.f.f_even);
    Fault_LogFReg(10, &ctx->fp10.f.f_even);
    osSyncPrintf("\n");
    Fault_LogFReg(12, &ctx->fp12.f.f_even);
    Fault_LogFReg(14, &ctx->fp14.f.f_even);
    osSyncPrintf("\n");
    Fault_LogFReg(16, &ctx->fp16.f.f_even);
    Fault_LogFReg(18, &ctx->fp18.f.f_even);
    osSyncPrintf("\n");
    Fault_LogFReg(20, &ctx->fp20.f.f_even);
    Fault_LogFReg(22, &ctx->fp22.f.f_even);
    osSyncPrintf("\n");
    Fault_LogFReg(24, &ctx->fp24.f.f_even);
    Fault_LogFReg(26, &ctx->fp26.f.f_even);
    osSyncPrintf("\n");
    Fault_LogFReg(28, &ctx->fp28.f.f_even);
    Fault_LogFReg(30, &ctx->fp30.f.f_even);
    osSyncPrintf("\n");
}

OSThread* FindFaultedThread(void) {
    OSThread* thread = __osGetActiveQueue();

    while (thread->priority != -1) {
        if ((thread->priority > OS_PRIORITY_IDLE) && (thread->priority < OS_PRIORITY_APPMAX) &&
            (thread->flags & (OS_FLAG_CPU_BREAK | OS_FLAG_FAULT))) {
            return thread;
        }
        thread = thread->tlnext;
    }
    return NULL;
}

//! TODO: Old libultra header should use this as OS_CYCLES_TO_USEC?
#define OS_CYCLES_TO_USEC_ALT(c) (((u64)(c) * (1000000LL / 15625LL)) / (osClockRate / 15625LL))

void Fault_WaitButtonCombo(void) {
    Input* input = &sFaultInputs[0];
    s32 trigger;
    s32 button;
    OSTime start;
    s32 x;
    s32 y;
    UNUSED s32 pad[4];

    osSyncPrintf(
        VT_FGCOL(WHITE) "KeyWaitB (ＬＲＺ " VT_FGCOL(WHITE) "上" VT_FGCOL(YELLOW) "下 " VT_FGCOL(YELLOW) "上" VT_FGCOL(WHITE) "下 " VT_FGCOL(WHITE) "左" VT_FGCOL(
            YELLOW) "左 " VT_FGCOL(YELLOW) "右" VT_FGCOL(WHITE) "右 " VT_FGCOL(GREEN) "Ｂ" VT_FGCOL(BLUE) "Ａ" VT_FGCOL(RED) "START" VT_FGCOL(WHITE) ")" VT_RST
                                                                                                                                                     "\n");
    x = 0;
    y = 0;
    do {
        Fault_Sleep(16);
        func_80074C88(&D_8010DF40, sFaultInputs, 0);
        trigger = pad_trigger(input);
        button = pad_button(input);
        if ((button == 0) && (y == 1)) {
            y = 0;
            osSyncPrintf("x=%d y=%d\n", x, y);
        } else if (trigger != 0) {
            if (y == 1) {
                x = 0;
                osSyncPrintf("x=%d y=%d\n", x, y);
            }
            switch (x) {
                case 0:
                    if ((button == (Z_TRIG | R_TRIG | L_TRIG)) && (trigger == Z_TRIG)) {
                        x = 1;
                        y = 1;
                        start = osGetTime();
                    }
                    break;

                case 1:
                    if (trigger == U_JPAD) {
                        x = 2;
                    } else {
                        x = 0;
                    }
                    break;

                case 2:
                    x = 3;
                    if (trigger == D_CBUTTONS) {
                        y = 1;
                    } else {
                        x = 0;
                    }
                    break;

                case 3:
                    if (trigger == U_CBUTTONS) {
                        x = 4;
                    } else {
                        x = 0;
                    }
                    break;

                case 4:
                    x = 5;
                    if (trigger == D_JPAD) {
                        y = 1;
                    } else {
                        x = 0;
                    }
                    break;

                case 5:
                    if (trigger == L_JPAD) {
                        x = 6;
                    } else {
                        x = 0;
                    }
                    break;

                case 6:
                    x = 7;
                    if (trigger == L_CBUTTONS) {
                        y = 1;
                    } else {
                        x = 0;
                    }
                    break;

                case 7:
                    if (trigger == R_CBUTTONS) {
                        x = 8;
                    } else {
                        x = 0;
                    }
                    break;

                case 8:
                    x = 9;
                    if (trigger == R_JPAD) {
                        y = 1;
                    } else {
                        x = 0;
                    }
                    break;

                case 9:
                    if (trigger == (A_BUTTON | B_BUTTON)) {
                        x = 10;
                    } else if (trigger == A_BUTTON) {
                        x = 91;
                    } else if (trigger == B_BUTTON) {
                        x = 92;
                    } else {
                        x = 0;
                    }
                    break;

                case 91:
                    if (trigger == B_BUTTON) {
                        x = 10;
                    } else {
                        x = 0;
                    }
                    break;

                case 92:
                    if (trigger == A_BUTTON) {
                        x = 10;
                    } else {
                        x = 0;
                    }
                    break;

                case 10:
                    if ((button == (A_BUTTON | B_BUTTON | START_BUTTON)) && (trigger == START_BUTTON)) {
                        f32 inputTime = OS_CYCLES_TO_USEC_ALT(osGetTime() - start) / 1000000.0f;

                        // Input time seconds
                        osSyncPrintf("入力時間 %f 秒\n", inputTime);

                        if (inputTime <= 50.0f) {
                            x = 11;
                        } else {
                            x = 0;
                        }
                    } else {
                        x = 0;
                    }

                    break;

                default:
                    break;
            }
        }
    } while (x != 11);
}

void Fault_WaitInputImpl(void) {
    Input* input = &sFaultInputs[0];

    do {
        Fault_Sleep(16);
        func_80074C88(&D_8010DF40, sFaultInputs, 0);
    } while (!(pad_trigger(input) &
               (A_BUTTON | B_BUTTON | START_BUTTON | U_CBUTTONS | L_CBUTTONS | R_CBUTTONS | D_CBUTTONS)));
}

void Fault_WaitInput(void) {
    Fault_WaitInputImpl();
}

void func_80070848(Y511F0UnkStruct* arg0, s32 index) {
    sFaultFontColor = GPACK_RGBA5551(255, 255, 255, 1);
    Fault_Printf(32, index * 8, "%08x-%08x:%08x-%08x(%04x)", arg0->unk0, (arg0->unk0 - arg0->unk8) + arg0->unkC,
                 arg0->unk8, arg0->unkC, arg0->unk4);
}

s32 func_800708D4(s32 arg0) {
    Y511F0UnkStruct* var_s0 = &D_800DA840[arg0];
    s32 j;
    s32 i;

    for (j = 4, i = arg0; (i < D_800DA83C) && (j < 28); i++) {
        if (var_s0->unk8 != 0) {
            func_80070848(var_s0, j);
            j++;
        }
        var_s0++;
    }

    return i;
}

// Overlays?
void func_80070988(OSThread* thread) {
    __OSThreadContext* ctx = &thread->context;
    Y511F0UnkStruct* var_a0;
    s32 sp44;
    s32 var_a2;
    s32 offset;
    s32 var_t0;
    Y511F0UnkStruct* var_a02;

    Fault_DrawRec(22, 16, 276, 208);
    offset = 0;
    sp44 = -1;
    var_t0 = 0;
    var_a0 = &D_800DA840[0];
    var_a2 = 0;
    for (var_a2 = 0; var_a2 < D_800DA83C; var_a2++) {
        if (var_a0->unk8 != 0) {
            if ((ctx->pc >= var_a0->unk0) && (ctx->pc < ((var_a0->unk0 + var_a0->unkC) - var_a0->unk8))) {
                offset = var_a0->unk8 - var_a0->unk0;
                sp44 = var_a2;
            }
        } else {
            var_t0++;
        }
        var_a0++;
    }

    sFaultFontColor = GPACK_RGBA5551(120, 255, 0, 1);
    Fault_Printf(28, 16, "MAX:%d(%d)", D_800DA83C, var_t0);
    Fault_Printf(95, 16, "PC:%08x", (uintptr_t)ctx->pc + offset);
    Fault_Printf(165, 16, "RA:%08x", (uintptr_t)ctx->ra + offset);
    Fault_Printf(235, 16, "OFF:%06x", offset);
    if (sp44 >= 0) {
        sFaultFontColor = GPACK_RGBA5551(255, 255, 0, 1);
        var_a02 = &D_800DA840[sp44];
        Fault_Printf(32, 24, "%08x-%08x:%08x-%08x(%04x)", var_a02->unk0,
                     (var_a02->unk0 - var_a02->unk8) + var_a02->unkC, var_a02->unk8, var_a02->unkC, var_a02->unk4);
    }
    D_80108690 = func_800708D4(D_80108690);
    sFaultFontColor = GPACK_RGBA5551(255, 255, 255, 1);
}

void Fault_PrintDebuggerInfo(void) {
    Fault_DrawRec(22, 16, 276, 208);
    sFaultFontColor = GPACK_RGBA5551(120, 120, 255, 1);
    Fault_Printf(24, 32, "mallocRecord.allocP=%08x", mallocRecord.allocP);
    Fault_Printf(24, 40, "mallocRecord.requestSize=%d", mallocRecord.requestSize);
    Fault_Printf(24, 48, "mallocRecord.totalFree=%d", mallocRecord.totalFree);
    Fault_Printf(24, 56, "mallocRecord.totalAlloc=%d", mallocRecord.totalAlloc);
    Fault_Printf(24, 64, "mallocRecord.blockCnt=%d", mallocRecord.blockCnt);
    Fault_Printf(24, 80, "debugger.framebuf=%08x", debugger.framebuf);
    Fault_Printf(24, 88, "debugger.width=%d", debugger.width);
    Fault_Printf(24, 96, "debugger.depth=%d", debugger.depth);
    sFaultFontColor = GPACK_RGBA5551(255, 255, 255, 1);
}

void Fault_PrintStackTrace(OSThread* thread) {
    s32 x;
    s32 y;
    u32* sp = (u32*)(u32)thread->context.sp;

    Fault_DrawRec(22, 16, 276, 208);
    Fault_Printf(36, 18, "Stack Trace %08x", sp);
    for (y = 28; y < 226; y += 9) {
        Fault_Printf(28, y, "%06x", sp);
        for (x = 82; x < 290; x += 52) {
            Fault_Printf(x, y, "%08x", *sp++);
        }
    }
}

void func_80070DA0(void) {
    s32 i;

    Fault_DrawRec(22, 16, 276, 208);
    Fault_Printf(36, 18, "ROM DEBUG");

    for (i = 0; i < ARRAY_COUNT(D_80109090); i++) {
        Fault_Printf(((i % 4) * 52) + 82, ((i / 4) * 9) + 28, "%08x", D_80109090[i]);
    }
}

void Fault_ResumeThread(OSThread* thread) {
    thread->context.cause = 0;
    thread->context.fpcsr = 0;
    thread->context.pc += 4;
    *((u32*)thread->context.pc) = 0x0000000D; // write in a break instruction
    osWritebackDCache((void*)thread->context.pc, 4);
    osInvalICache((void*)thread->context.pc, 4);
    osStartThread(thread);
}

void Fault_DisplayFramebuffer(void) {
    osViBlack(false);
    if (osViGetCurrentFramebuffer() >= (void*)0x80100000) {
        debugger.framebuf = osViGetCurrentFramebuffer();
    } else {
        debugger.framebuf = (void*)(PHYS_TO_K0(osMemSize) - sizeof(u16[SCREEN_HEIGHT][SCREEN_WIDTH]));
        osViSwapBuffer(debugger.framebuf);
    }
}

#define FAULT_MSG_CPU_BREAK ((OSMesg)1)
#define FAULT_MSG_FAULT ((OSMesg)2)
#define FAULT_MSG_UNK ((OSMesg)3)
#define FAULT_MSG_HUNG_UP ((OSMesg)4)

void Fault_ThreadEntry(UNUSED void* arg) {
    OSMesg msg;
    OSThread* faultedThread;

    while (true) {
        osSetEventMesg(OS_EVENT_CPU_BREAK, &debugger.queue, FAULT_MSG_CPU_BREAK);
        osSetEventMesg(OS_EVENT_FAULT, &debugger.queue, FAULT_MSG_FAULT);
        do {
            osRecvMesg(&debugger.queue, &msg, 1);
            if (msg == FAULT_MSG_CPU_BREAK) {
                fault_msg_id = (u32)FAULT_MSG_CPU_BREAK;
                // "Fault manager: OS_EVENT_CPU_BREAK received"
                osSyncPrintf("フォルトマネージャ:OS_EVENT_CPU_BREAKを受信しました\n");
            } else if (msg == FAULT_MSG_FAULT) {
                fault_msg_id = (u32)FAULT_MSG_FAULT;
                // "Fault manager: OS_EVENT_FAULT received"
                osSyncPrintf("フォルトマネージャ:OS_EVENT_FAULTを受信しました\n");
            } else {
                fault_msg_id = (u32)FAULT_MSG_UNK;
                // "Fault manager: received an unknown message"
                osSyncPrintf("フォルトマネージャ:不明なメッセージを受信しました\n");
            }
            faultedThread = __osGetCurrFaultedThread();
            osSyncPrintf("__osGetCurrFaultedThread()=%08x\n", faultedThread);
            if (faultedThread == NULL) {
                faultedThread = FindFaultedThread();
                osSyncPrintf("FindFaultedThread()=%08x\n", faultedThread);
            }
        } while (faultedThread == NULL);

        Fault_LogThreadContext(faultedThread);
        osSyncPrintf("%d %s %d:%s = %d\n", osGetThreadId(NULL), "fault.c", 1150, "fault_display_enable",
                     fault_display_enable);
        while (!fault_display_enable) {
            Fault_Sleep(1000);
        }
        Fault_Sleep(500);
        Fault_WaitButtonCombo();
        do {
            Fault_DisplayFramebuffer();
            D_80108690 = 0;

            do {
                func_80070988(faultedThread);
                Fault_WaitInput();
            } while (D_80108690 < D_800DA83C);

            Fault_PrintDebuggerInfo();
            Fault_WaitInput();
            Fault_PrintThreadContext(faultedThread);
            Fault_WaitInput();
            func_80070DA0();
            Fault_WaitInput();
            Fault_PrintStackTrace(faultedThread);
            Fault_WaitInput();
        } while (!fault_exit);

        while (!fault_exit) {}

        Fault_ResumeThread(faultedThread);
    }
}

void Fault_SetFramebuffer(u16* fb, u16 width, u16 depth) {
    debugger.framebuf = fb;
    debugger.width = width;
    debugger.depth = depth;
}

void Fault_Init(void) {
    fault_display_enable = true;
    debugger.framebuf = (void*)(PHYS_TO_K0(osMemSize) - sizeof(u16[SCREEN_HEIGHT][SCREEN_WIDTH]));
    debugger.width = SCREEN_WIDTH;
    debugger.depth = 16;
    osCreateMesgQueue(&debugger.queue, debugger.msg, ARRAY_COUNT(debugger.msg));
    StackCheck_Init(&sFaultStackInfo, sFaultStack, STACK_TOP(sFaultStack), 0, 0x100, "fault");
    osCreateThread(&debugger.thread, Y_THREAD_ID_FAULT, Fault_ThreadEntry, NULL, STACK_TOP(sFaultStack), Y_PRIORITY_FAULT);
    osStartThread(&debugger.thread);
}

void Fault_HungUp(const char* file, s32 line) {
    fault_msg_id = (u32)FAULT_MSG_HUNG_UP;
    osSyncPrintf("HungUp %s:%d", file, line);
    osSyncPrintf("%d %s %d:%s = %d\n", osGetThreadId(NULL), "fault.c", 1247, "fault_display_enable",
                 fault_display_enable);

    while (!fault_display_enable) {
        Fault_Sleep(1000);
    }
    Fault_Sleep(500);
    Fault_WaitButtonCombo();

    do {
        Fault_DisplayFramebuffer();
        Fault_DrawRec(22, 16, 276, 24);
        Fault_Printf(30, 20, "HungUp %s:%d", file, line);
        Fault_Sleep(1000);
    } while (true);
}
