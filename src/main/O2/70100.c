#include "global.h"

#define WAIT_ON_IOBUSY_ALT(stat)       \
    do {                               \
        stat = IO_READ(PI_STATUS_REG); \
    } while (stat & (PI_STATUS_IO_BUSY | PI_STATUS_DMA_BUSY))

#define ADDRESS_1 (PI_DOM1_ADDR2 + 0x458) // 0x10000458
#define ADDRESS_2 (PI_DOM1_ADDR2 + 0x45C) // 0x1000045C

void func_8006F500(void) {
    register u32 stat;
    u16 addr1Val;
    u16 addr2Val;

    WAIT_ON_IOBUSY_ALT(stat);
    addr1Val = IO_READ(ADDRESS_1);

    WAIT_ON_IOBUSY_ALT(stat);
    addr2Val = IO_READ(ADDRESS_2);

    D_80108620 = (addr1Val << 16) | addr2Val;
}
