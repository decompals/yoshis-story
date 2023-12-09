#include "global.h"
#include "stack.h"
#include "stackcheck.h"
#include "ys64thread.h"

OSThread sDmaThread;
STACK(sDmaStack, 0x300);
StackEntry sDmaStackInfo;
OSMesgQueue D_8010ED10;
OSMesg D_8010ED28[10];
OSMesgQueue D_8010ED50;
OSMesg D_8010ED68[10];

typedef struct DmaRequest {
    /* 0x00 */ u32 unk00;
    /* 0x04 */ void* unk04;
    /* 0x08 */ size_t unk08;
    /* 0x0C */ void* unk0C;
    /* 0x10 */ u8 unk10;
    /* 0x11 */ u8 unk11;
    /* 0x14 */ u32 unk14;
    /* 0x18 */ void* unk18;
    /* 0x1C */ size_t unk1C;
    /* 0x20 */ OSIoMesg ioMsg;
    /* 0x38 */ s32 pri;
    /* 0x3C */ OSMesgQueue* queue;
    /* 0x40 */ OSMesg msg;
} DmaRequest; // size = 0x44

void DmaMgr_Read(OSIoMesg* ioMsg, s32 pri, s32 op, uintptr_t vrom, void* ram, size_t size, OSMesgQueue* endQueue,
                 size_t buffSize) {
    OSMesgQueue startQueue;
    OSMesg msg;

    osCreateMesgQueue(&startQueue, &msg, 1);
    while (buffSize < size) {
        osPiStartDma(ioMsg, pri, op, vrom, ram, buffSize, &startQueue);
        osRecvMesg(&startQueue, NULL, OS_MESG_BLOCK);
        size -= buffSize;
        vrom += buffSize;
        ram = (void*)((uintptr_t)ram + buffSize);
    }
    osPiStartDma(ioMsg, pri, op, vrom, ram, size, endQueue);
}

// DmaMgr_Uncompress?
// CMPR = LZW compression ?
s32 func_8007DC5C(DmaRequest* req) {
    Y65430UnkStruct* temp_v0;

    if (!req->unk10) {
        return 0;
    }

    if (req->unk1C < 0x10) {
        return 0;
    }

    temp_v0 = (Y65430UnkStruct*)req->unk18;
    req->unk10 = false;
    if (temp_v0->magic == Y65430UnkStruct_MAGIC) {
        req->unk11 = 1;
        req->unk14 += 0x10;
        req->unk1C = temp_v0->unk04;
        if (req->unk0C != NULL) {
            req->unk18 = req->unk0C;
        } else {
            req->unk18 = func_80064D1C(req->unk1C);
            if (req->unk18 == NULL) {
                return -1;
            }
        }
    } else {
        req->unk1C = req->unk08;
    }

    osSendMesg(&D_8010ED50, (OSMesg)req, OS_MESG_BLOCK);
    osInvalDCache(req->unk18, req->unk1C);

    (void)"osPiStartDma(%08x, %08x, OS_READ, %08x, %08x, %08x, %08x)\n";

    DmaMgr_Read(&req->ioMsg, req->pri, OS_READ, req->unk14, req->unk18, req->unk1C, &D_8010ED10, 0x1000);
    return 1;
}

void DmaMgr_ThreadEntry(UNUSED void* arg) {
    DmaRequest* req;
    OSMesg msg;

    // "DMA manager thread execution start"
    (void)"ＤＭＡマネージャスレッド実行開始\n";

    while (true) {
        osRecvMesg(&D_8010ED10, &msg, OS_MESG_BLOCK);
        osRecvMesg(&D_8010ED50, &msg, OS_MESG_BLOCK);

        req = (DmaRequest*)msg;
        if (func_8007DC5C(req) != 0) {
            continue;
        }

        if (req->unk11 != 0) {
            func_8006F560(req->unk18, req->unk04);
            if (req->unk0C == NULL) {
                func_80064E84(req->unk18);
            }
        }
        if (req->queue != NULL) {
            osSendMesg(req->queue, req->msg, OS_MESG_NOBLOCK);
        }
    }
}

void DmaMgr_SendRequest(DmaRequest* req, void* ram, u32 vrom, size_t size, void* arg4, OSMesgQueue* queue, OSMesg msg) {
    req->unk00 = vrom;
    req->unk04 = ram;
    req->unk08 = size;
    req->unk0C = arg4;
    req->pri = OS_MESG_PRI_NORMAL;
    req->queue = queue;
    req->msg = msg;
    req->unk11 = 0;
    req->unk14 = vrom;
    req->unk18 = ram;
    if (size < 0x10) {
        req->unk10 = false;
        req->unk1C = size;
    } else {
        req->unk10 = true;
        req->unk1C = 0x10;
    }

    osSendMesg(&D_8010ED50, (OSMesg)req, OS_MESG_BLOCK);
    osInvalDCache(req->unk04, req->unk1C);

    (void)"osPiStartDma(%08x, OS_MESG_PRI_HIGH, OS_READ, %08x, %08x, %08x, %08x)\n";
    DmaMgr_Read(&req->ioMsg, OS_MESG_PRI_HIGH, OS_READ, req->unk14, req->unk18, req->unk1C, &D_8010ED10, req->unk1C);
}

void DmaMgr_RequestSync(void* ram, u32 vrom, u32 size) {
    DmaRequest req;
    OSMesgQueue queue;
    OSMesg msg;

    osCreateMesgQueue(&queue, &msg, 1);
    DmaMgr_SendRequest(&req, ram, vrom, size, NULL, &queue, NULL);
    osRecvMesg(&queue, NULL, OS_MESG_BLOCK);
}

void DmaMgr_Init(void) {
    osCreateMesgQueue(&D_8010ED10, D_8010ED28, ARRAY_COUNT(D_8010ED28));
    osCreateMesgQueue(&D_8010ED50, D_8010ED68, ARRAY_COUNT(D_8010ED68));
    StackCheck_Init(&sDmaStackInfo, sDmaStack, STACK_TOP(sDmaStack), 0, 0x100, "dmamgr");
    osCreateThread(&sDmaThread, Y_THREAD_ID_DMA, DmaMgr_ThreadEntry, NULL, &sDmaStackInfo, Y_PRIORITY_DMA);
    osStartThread(&sDmaThread);
}
