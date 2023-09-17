#ifndef LIBU64_GFX_PRINT_H
#define LIBU64_GFX_PRINT_H

#include "ultra64.h"
#include "color.h"
#include "aprintf.h"
#include "libc/stdbool.h"

typedef struct gfxprint  {
    /* 0x00 */ PrintCallback proutFunc; /* Current print out func */
    /* 0x04 */ Gfx* gListp; /* Current display list to write text to */
    /* 0x08 */ s32 isOpen; /* Open state */
    /* 0x0C */ s32 posX;
    /* 0x10 */ s32 posY;
    /* 0x14 */ Color_RGBA8_u32 color;
    /* 0x18 */ s32 kanaMode; /* Japanese characters displayed as hiragana or katakana */
    /* 0x1C */ s32 isGradient; /* Gradient state */
    /* 0x20 */ s32 isShadow; /* Shadow state */
    /* 0x24 */ s32 isChanged; /* Text changed */
    /* 0x28 */ s32 offsetX;
    /* 0x2C */ s32 offsetY;
} gfxprint;

#define GFXPRINT_NUM_SUBPIXELS 4
#define GFXPRINT_PIXELS_PER_BLOCK 8
#define GFXPRINT_PAD GFXPRINT_PIXELS_PER_BLOCK * GFXPRINT_NUM_SUBPIXELS

#define GFXPRINT_KANA_MODE_KATAKANA 0
#define GFXPRINT_KANA_MODE_HIRAGANA 1

#define GFXPRINT_CLEAR_GRADIENT_CHAR '\x8A'
#define GFXPRINT_ENABLE_GRADIENT_CHAR '\x8B'

#define GFXPRINT_KATAKANA_MODE_CHAR '\x8C'
#define GFXPRINT_HIRAGANA_MODE_CHAR '\x8D'

#define GFXPRINT_UNUSED_CHAR '\x8E'

#define gfxprint_isOpened(this) (this->isOpen)
#define gfxprint_setOpened(this) (this->isOpen = true)
#define gfxprint_clrOpened(this) (this->isOpen = false)

#define gfxprint_isHiragana(this) (this->kanaMode != GFXPRINT_KANA_MODE_KATAKANA)
#define gfxprint_isKatakana(this) (this->kanaMode == GFXPRINT_KANA_MODE_KATAKANA)
#define gfxprint_setHiragana(this) (this->kanaMode = GFXPRINT_KANA_MODE_HIRAGANA)
#define gfxprint_setKatakana(this) (this->kanaMode = GFXPRINT_KANA_MODE_KATAKANA)

#define gfxprint_isGradient(this) (this->isGradient)
#define gfxprint_setGradient(this) (this->isGradient = true)
#define gfxprint_clrGradient(this) (this->isGradient = false)

#define gfxprint_isShadow(this) (this->isShadow)
#define gfxprint_setShadow(this) (this->isShadow = true)
#define gfxprint_clrShadow(this) (this->isShadow = false)

#define gfxprint_isChanged(this) (this->isChanged)
#define gfxprint_setChanged(this) (this->isChanged = true)
#define gfxprint_clrChanged(this) (this->isChanged = false)

//TODO: migrate data and add these when we can extract assets

extern u16 gfxprint_moji_tlut[]; // 0x800A9A60
extern u16 gfxprint_rainbow_tlut[]; // 0x800A9AE0

extern u8 gfxprint_font[]; // 0x800A9B08
extern u8 gfxprint_rainbow_txtr[]; // 0x800A9B00


void gfxprint_setup(gfxprint* this);
void gfxprint_color(gfxprint* this, u32 r, u32 g, u32 b, u32 a);
void gfxprint_locate(gfxprint* this, s32 x, s32 y);
void gfxprint_locate8x8(gfxprint* this, s32 x, s32 y);
void gfxprint_setoffset(gfxprint* this, s32 x, s32 y);
void gfxprint_putc1(gfxprint* this, char c);
void gfxprint_putc(gfxprint* this, char c);
void gfxprint_write(gfxprint* this, const void* buffer, s32 size, s32 n);
void gfxprint_puts(gfxprint* this, const char* buffer);
void* gfxprint_prout(void* this, const char* buffer, s32 n);
void gfxprint_init(gfxprint* this);
void gfxprint_cleanup(gfxprint* this);
void gfxprint_open(gfxprint* this, Gfx* gListp);
Gfx* gfxprint_close(gfxprint* this);
s32 gfxprint_vprintf(gfxprint* this, const char* fmt, va_list ap);
s32 gfxprint_printf(gfxprint* this, const char* fmt, ...);


#endif
