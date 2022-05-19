/**
 * @file loadfragment2.c
 *
 * Functions used to process and relocate overlays
 *
 */
#include "global.h"
#include "loadfragment2.h"

s32 gLoad2LogSeverity = 2;

void DoRelocation(void* allocatedVRamAddr, OverlayRelocationSection* ovl, uintptr_t vRamStart) {
    u32 sections[4];
    u32* relocDataP;
    u32 reloc;
    uintptr_t relocatedAddress;
    u32 i;
    u32* luiInstRef;
    uintptr_t allocu32 = (uintptr_t)allocatedVRamAddr;
    u32* regValP;
    u32* luiRefs[32];
    u32 luiVals[32];
    u32 isLoNeg;

    if (gLoad2LogSeverity >= 3) {}

    sections[0] = 0;
    sections[1] = allocu32;
    sections[2] = allocu32 + ovl->textSize;
    sections[3] = sections[2] + ovl->dataSize;

    for (i = 0; i < ovl->nRelocations; i++) {
        reloc = ovl->relocations[i];
        relocDataP = (u32*)(sections[RELOC_SECTION(reloc)] + RELOC_OFFSET(reloc));

        switch (RELOC_TYPE(reloc)) {
            case R_MIPS_32 << 24:
                // Handles 32-bit address relocation, used for things such as jump tables and pointers in data.
                // Just relocate the full address.

                // Check address is valid for relocation
                if ((*relocDataP & 0xF000000) == 0) {
                    *relocDataP = RELOCATE_ADDR(*relocDataP, vRamStart, allocu32);
                } else if (gLoad2LogSeverity >= 3) {
                }
                break;

            case R_MIPS_26 << 24:
                // Handles 26-bit address relocation, used for jumps and jals.
                // Extract the address from the target field of the J-type MIPS instruction.
                // Relocate the address and update the instruction.

                *relocDataP =
                    (*relocDataP & 0xFC000000) |
                    ((RELOCATE_ADDR(PHYS_TO_K0((*relocDataP & 0x3FFFFFF) << 2), vRamStart, allocu32) & 0xFFFFFFF) >> 2);
                break;

            case R_MIPS_HI16 << 24:
                // Handles relocation for a hi/lo pair, part 1.
                // Store the reference to the LUI instruction (hi) using the `rt` register of the instruction.
                // This will be updated later in the `R_MIPS_LO16` section.

                luiRefs[(*relocDataP >> 0x10) & 0x1F] = relocDataP;
                luiVals[(*relocDataP >> 0x10) & 0x1F] = *relocDataP;
                break;

            case R_MIPS_LO16 << 24:
                // Handles relocation for a hi/lo pair, part 2.
                // Grab the stored LUI (hi) from the `R_MIPS_HI16` section using the `rs` register of the instruction.
                // The full address is calculated, relocated, and then used to update both the LUI and lo instructions.
                // If the lo part is negative, add 1 to the LUI.
                // Note: The lo instruction is assumed to have a signed immediate.

                luiInstRef = luiRefs[(*relocDataP >> 0x15) & 0x1F];
                regValP = &luiVals[(*relocDataP >> 0x15) & 0x1F];

                // Check address is valid for relocation
                if ((((*luiInstRef << 0x10) + (s16)*relocDataP) & 0x0F000000) == 0) {
                    relocatedAddress = RELOCATE_ADDR((*regValP << 0x10) + (s16)*relocDataP, vRamStart, allocu32);
                    isLoNeg = (relocatedAddress & 0x8000) ? 1 : 0;
                    *luiInstRef = (*luiInstRef & 0xFFFF0000) | (((relocatedAddress >> 0x10) & 0xFFFF) + isLoNeg);
                    *relocDataP = (*relocDataP & 0xFFFF0000) | (relocatedAddress & 0xFFFF);
                } else if (gLoad2LogSeverity >= 3) {
                }
                break;
        }
    }
}

size_t func_8008169C(uintptr_t vRomStart, uintptr_t vRomEnd, uintptr_t vRamStart, uintptr_t vRamEnd,
                     void* allocatedVRamAddr) {
    s32 pad[2];
    s32 size = vRomEnd - vRomStart;
    void* end;
    OverlayRelocationSection* ovl;

    if (gLoad2LogSeverity >= 3) {}
    if (gLoad2LogSeverity >= 3) {}

    end = (uintptr_t)allocatedVRamAddr + size;
    func_8007DF0C(allocatedVRamAddr, vRomStart, size);

    ovl = (OverlayRelocationSection*)((uintptr_t)end - ((s32*)end)[-1]);

    if (gLoad2LogSeverity >= 3) {}
    if (gLoad2LogSeverity >= 3) {}

    DoRelocation(allocatedVRamAddr, ovl, vRamStart);

    if (ovl->bssSize != 0) {
        if (gLoad2LogSeverity >= 3) {}
        bzero(end, ovl->bssSize);
    }

    size = vRamEnd - vRamStart;

    osWritebackDCache(allocatedVRamAddr, size);
    osInvalICache(allocatedVRamAddr, size);

    if (gLoad2LogSeverity >= 3) {}

    return size;
}

void* func_80081744(uintptr_t vRomStart, uintptr_t vRomEnd, uintptr_t vRamStart, uintptr_t vRamEnd) {
    void* allocatedVRamAddr = func_80064DD0(vRamEnd - vRamStart);

    if (allocatedVRamAddr != NULL) {
        func_8008169C(vRomStart, vRomEnd, vRamStart, vRamEnd, allocatedVRamAddr);
    }

    return allocatedVRamAddr;
}
