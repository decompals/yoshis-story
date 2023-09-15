#include "stackcheck.h"
#include "libc/stdbool.h"
#include "libc/stdint.h"

StackEntry* sStackInfoListStart = NULL;
StackEntry* sStackInfoListEnd = NULL;

void StackCheck_Init(StackEntry* entry, void* stackBottom, void* stackTop, u32 initValue, s32 minSpace,
                     const char* name) {
    u32* addr;

    if (entry == NULL) {
        sStackInfoListStart = NULL;
    } else {
        entry->head = stackBottom;
        entry->tail = stackTop;
        entry->initValue = initValue;
        entry->minSpace = minSpace;
        entry->name = name;

        entry->prev = sStackInfoListEnd;
        entry->next = NULL;

        if (sStackInfoListEnd != NULL) {
            sStackInfoListEnd->next = entry;
        }

        sStackInfoListEnd = entry;
        if (sStackInfoListStart == NULL) {
            sStackInfoListStart = entry;
        }

        if (entry->minSpace != -1) {
            addr = entry->head;
            while (addr < (u32*)entry->tail) {
                *addr++ = entry->initValue;
            }
        }
    }
}

void StackCheck_Cleanup(StackEntry* entry) {
    if (entry->prev == NULL) {
        sStackInfoListStart = entry->next;
    } else {
        entry->prev->next = entry->next;
    }

    if (entry->next == NULL) {
        sStackInfoListEnd = entry->prev;
    }
}

s32 StackCheck_Check(StackEntry* entry) {
    if (entry == NULL) {
        StackEntry* iter = sStackInfoListStart;
        s32 ret = 0;

        while (iter != NULL) {
            if (StackCheck_Check(iter) != STACK_STATUS_OK) {
                ret = 1;
            }
            iter = iter->next;
        }

        return ret;

    } else {
        u32* iter;
        s32 ret;
        size_t free;

        for (iter = entry->head; iter < (u32*)entry->tail; iter++) {
            if (*iter != entry->initValue) {
                break;
            }
        }

        free = (uintptr_t)iter - (uintptr_t)entry->head;
        if (free == 0) {
            ret = STACK_STATUS_OVERFLOW;
        } else if ((free < entry->minSpace) && (entry->minSpace != -1)) {
            ret = STACK_STATUS_WARNING;
        } else {
            ret = STACK_STATUS_OK;
        }

        return ret;
    }
}
