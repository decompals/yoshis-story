#ifndef FAULT_H
#define FAULT_H

#include "ultra64.h"

void Fault_SetFramebuffer(u16* fb, u16 width, u16 depth);
void Fault_Init(void);
void Fault_HungUp(const char* file, s32 line);

#endif
