#include "global.h"

#include "pad.h"
#include "stack.h"
#include "stackcheck.h"
#include "ys64thread.h"

OSMesgQueue sSerialEventQueue;
OSMesg sSerialMsgBuff[1];
STACK(sPadStack, 0x400);
StackEntry sPadStackInfo;

void func_8007C540(void) {
    osCreateMesgQueue(&sSerialEventQueue, sSerialMsgBuff, ARRAY_COUNT(sSerialMsgBuff));
    osSetEventMesg(OS_EVENT_SI, &sSerialEventQueue, NULL);
    StackCheck_Init(&sPadStackInfo, sPadStack, STACK_TOP(sPadStack), 0, 0x100, "padmgr");
    func_80075020(&D_8010DF40, &sSerialEventQueue, &D_801001F8, Y_THREAD_ID_PAD, Y_PRIORITY_PAD, STACK_TOP(sPadStack));
}

void func_8007C5EC(void) {
    func_80074C88(&D_8010DF40, &D_8010DEE0, 1);
}

void func_8007C61C(void) {
    func_800744B0(&D_8010DF40);
}

void func_8007C640(OSMesgQueue* arg0) {
    func_8007451C(&D_8010DF40, arg0);
}
