#include "global.h"
#include "macros.h"

void csleep(OSTime t) {
    OSMesgQueue mq;
    OSMesg msg[1];
    OSTimer timer;

    osCreateMesgQueue(&mq, msg, ARRAY_COUNT(msg));
    osSetTimer(&timer, t, 0, &mq, NULL);
    osRecvMesg(&mq, NULL, OS_MESG_BLOCK);
}
