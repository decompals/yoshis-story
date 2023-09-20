#include "vimode.h"
#include "attributes.h"
#include "macros.h"
#include "libc/stdbool.h"
#include "lib/ultralib/src/io/viint.h"
#include "pad.h"

void ViMode_LogPrint(UNUSED OSViMode* osViMode) {
}

/**
 * Configures the custom OSViMode for this ViMode
 *
 * @param viMode        ViMode to configure the custom OSViMode for
 * @param type          Identifying type for the OSViMode
 * @param tvType        TV Type: NTSC, PAL, MPAL or FPAL
 * @param loRes         Boolean: true = low resolution, false = high resolution.
 *                      Corresponds to "L" or "H" in libultra VI mode names
 * @param antialiasOff  Boolean: true = point-sampling, false = anti-aliasing.
 *                      Corresponds to "P" or "A" in libultra VI mode names
 * @param modeN         Boolean: controls interlacing mode, different based on resolution.
 *                      Corresponds to "N" or "F" in libultra VI mode names
 * @param fb16Bit       Bolean: true = 16-bit framebuffer, false = 32-bit framebuffer.
 *                      Corresponds to "1" or "2" in libultra VI mode names
 * @param width         Screen width
 * @param height        Screen height
 * @param leftAdjust    Left edge adjustment
 * @param rightAdjust   Right edge adjustment
 * @param upperAdjust   Upper edge adjustment
 * @param lowerAdjust   Lower edge adjustment
 */
void ViMode_Configure(ViMode* viMode, s32 type, s32 tvType, s32 loRes, s32 antialiasOff, s32 modeN, s32 fb16Bit, s32 width,
                   s32 height, s32 leftAdjust, s32 rightAdjust, s32 upperAdjust, s32 lowerAdjust) {
    s32 hiRes;
    s32 antialiasOn;
    s32 modeF;
    s32 fb32Bit;
    s32 hiResDeflicker; // deflickered interlacing
    s32 hiResInterlaced;
    s32 loResDeinterlaced;
    s32 loResInterlaced;
    s32 modeLAN1; // L=(lo res) A=(antialias)     N=(deinterlace)        1=(16-bit)
    s32 modeLPN2; // L=(lo res) P=(point-sampled) N=(deinterlace)        2=(32-bit)
    s32 modeHPN2; // H=(hi res) P=(point-sampled) N=(normal interlacing) 2=(32-bit)
    s32 yScaleLo;
    s32 yScaleHiEvenField;
    s32 yScaleHiOddField;

    hiRes = !loRes;
    antialiasOn = !antialiasOff;
    modeF = !modeN;
    fb32Bit = !fb16Bit;

    hiResDeflicker = hiRes && modeF;
    hiResInterlaced = hiRes && modeN;
    loResDeinterlaced = loRes && modeN;
    loResInterlaced = loRes && modeF;

    modeLAN1 = loRes && antialiasOn && modeN && fb16Bit;
    modeLPN2 = loRes && antialiasOff && modeN && fb32Bit;
    modeHPN2 = hiRes && antialiasOff && modeN && fb32Bit;

    upperAdjust &= ~1;
    lowerAdjust &= ~1;

    yScaleLo =
        (hiResDeflicker ? 2 : 1) * ((height << 11) / (SCREEN_HEIGHT * 2 + lowerAdjust - upperAdjust) / (loRes ? 1 : 2));

    yScaleHiEvenField = modeF ? (loResInterlaced ? (F210(0.25) << 16) : (F210(0.5) << 16)) : 0;
    yScaleHiOddField = modeF ? (loResInterlaced ? (F210(0.75) << 16) : (F210(0.5) << 16)) : 0;

    viMode->customViMode.type = type;
    viMode->customViMode.comRegs.ctrl = VI_CTRL_PIXEL_ADV_3 | VI_CTRL_GAMMA_ON | VI_CTRL_GAMMA_DITHER_ON |
                                        (!loResDeinterlaced ? VI_CTRL_SERRATE_ON : 0) |
                                        (antialiasOn ? VI_CTRL_DIVOT_ON : 0) |
                                        (fb32Bit ? VI_CTRL_TYPE_32 : VI_CTRL_TYPE_16);

    if (modeLAN1) {
        // Anti-aliased, fetch extra lines as-needed
        viMode->customViMode.comRegs.ctrl |= VI_CTRL_ANTIALIAS_MODE_1;
    } else if (modeLPN2 | modeHPN2) {
        // Point-sampled, resampling disabled
        viMode->customViMode.comRegs.ctrl |= VI_CTRL_ANTIALIAS_MODE_3;
    } else {
        if (antialiasOff) {
            // Point-sampled, resampling enabled
            viMode->customViMode.comRegs.ctrl |= VI_CTRL_ANTIALIAS_MODE_2;
        } else {
            // Anti-aliased, always fetch extra lines
            viMode->customViMode.comRegs.ctrl |= 0;
        }
    }

    viMode->customViMode.comRegs.width = width * (hiResInterlaced ? 2 : 1);

    if (tvType == OS_TV_NTSC) {
        viMode->customViMode.comRegs.burst = BURST(57, 34, 5, 62);
        viMode->customViMode.comRegs.vSync = VSYNC(524);
        viMode->customViMode.comRegs.hSync = HSYNC(3093, 0);
        viMode->customViMode.comRegs.leap = LEAP(3093, 3093);
        viMode->customViMode.comRegs.hStart = HSTART(108, 748);
        viMode->customViMode.fldRegs[0].vStart = START(37, 511);
        viMode->customViMode.fldRegs[0].vBurst = BURST(4, 2, 14, 0);
    } else if (tvType == OS_TV_PAL) {
        viMode->customViMode.comRegs.burst = BURST(58, 35, 4, 64);
        viMode->customViMode.comRegs.vSync = VSYNC(624);
        viMode->customViMode.comRegs.hSync = HSYNC(3177, 21);
        viMode->customViMode.comRegs.leap = LEAP(3183, 3182);
        viMode->customViMode.comRegs.hStart = HSTART(128, 768);
        viMode->customViMode.fldRegs[0].vStart = START(95, 569);
        viMode->customViMode.fldRegs[0].vBurst = BURST(107, 2, 9, 0);
    } else if (tvType == OS_TV_MPAL) {
        viMode->customViMode.comRegs.burst = BURST(57, 30, 5, 70);
        viMode->customViMode.comRegs.vSync = VSYNC(524);
        viMode->customViMode.comRegs.hSync = HSYNC(3088, 0);
        viMode->customViMode.comRegs.leap = LEAP(3100, 3100);
        viMode->customViMode.comRegs.hStart = HSTART(108, 748);
        viMode->customViMode.fldRegs[0].vStart = START(37, 511);
        viMode->customViMode.fldRegs[0].vBurst = BURST(4, 2, 14, 0);
    }

    viMode->customViMode.fldRegs[1].vStart = viMode->customViMode.fldRegs[0].vStart;

    viMode->customViMode.comRegs.hStart += (leftAdjust << 16) + (s16)rightAdjust;
    viMode->customViMode.fldRegs[0].vStart += (upperAdjust << 16) + (s16)lowerAdjust;
    viMode->customViMode.fldRegs[1].vStart += (upperAdjust << 16) + (s16)lowerAdjust;

    viMode->customViMode.fldRegs[1].vBurst = viMode->customViMode.fldRegs[0].vBurst;

    if (loResDeinterlaced) {
        viMode->customViMode.comRegs.vSync++;
        if (tvType == OS_TV_MPAL) {
            viMode->customViMode.comRegs.hSync += HSYNC(1, 4);
        }
        if (tvType == OS_TV_MPAL) {
            viMode->customViMode.comRegs.leap += LEAP((u16)-4, (u16)-2);
        }
    } else {
        viMode->customViMode.fldRegs[0].vStart += START((u16)-3, (u16)-2);
        if (tvType == OS_TV_MPAL) {
            viMode->customViMode.fldRegs[0].vBurst += BURST((u8)-2, (u8)-1, 12, -1);
        }
        if (tvType == OS_TV_PAL) {
            viMode->customViMode.fldRegs[1].vBurst += BURST((u8)-2, (u8)-1, 2, 0);
        }
    }

    viMode->customViMode.comRegs.xScale = (width << 10) / (SCREEN_WIDTH * 2 + rightAdjust - leftAdjust);
    viMode->customViMode.comRegs.vCurrent = VCURRENT(0);

    viMode->customViMode.fldRegs[0].origin = ORIGIN(width * 2 * (fb16Bit ? 1 : 2));
    viMode->customViMode.fldRegs[1].origin = ORIGIN(width * 2 * (fb16Bit ? 1 : 2) * (loRes ? 1 : 2));

    viMode->customViMode.fldRegs[0].yScale = yScaleLo | yScaleHiEvenField;
    viMode->customViMode.fldRegs[1].yScale = yScaleLo | yScaleHiOddField;

    viMode->customViMode.fldRegs[0].vIntr = VINTR(2);
    viMode->customViMode.fldRegs[1].vIntr = VINTR(2);
}

void ViMode_Save(UNUSED ViMode* viMode) {
}

void ViMode_Load(ViMode* viMode) {
    viMode->editState = VI_MODE_EDIT_STATE_INACTIVE;
    viMode->viWidth = 0;
    viMode->viHeight = 0;
    viMode->upperAdjust = 0;
    viMode->lowerAdjust = 0;
    viMode->leftAdjust = 0;
    viMode->rightAdjust = 0;
}

void ViMode_Init(ViMode* viMode) {
    viMode->editState = VI_MODE_EDIT_STATE_INACTIVE;
    viMode->viWidth = SCREEN_WIDTH;
    viMode->viHeight = SCREEN_HEIGHT;
    viMode->leftAdjust = 0;
    viMode->rightAdjust = 0;
    viMode->upperAdjust = 0;
    viMode->lowerAdjust = 0;
    viMode->viFeatures = OS_VI_DITHER_FILTER_ON | OS_VI_GAMMA_OFF;
    viMode->tvType = osTvType;
    viMode->fb16Bit = true;
    viMode->modeN = true;
    viMode->antialiasOff = false;
    viMode->loRes = true;

    ViMode_Save(viMode);
}

void ViMode_Destroy(UNUSED ViMode* viMode) {
}

void ViMode_ConfigureFeatures(ViMode* viMode, s32 viFeatures) {
    u32 ctrl = viMode->customViMode.comRegs.ctrl;

    if (viFeatures & OS_VI_GAMMA_ON) {
        ctrl |= VI_CTRL_GAMMA_ON;
    }
    if (viFeatures & OS_VI_GAMMA_OFF) {
        ctrl &= ~VI_CTRL_GAMMA_ON;
    }
    if (viFeatures & OS_VI_GAMMA_DITHER_ON) {
        ctrl |= VI_CTRL_GAMMA_DITHER_ON;
    }
    if (viFeatures & OS_VI_GAMMA_DITHER_OFF) {
        ctrl &= ~VI_CTRL_GAMMA_DITHER_ON;
    }
    if (viFeatures & OS_VI_DIVOT_ON) {
        ctrl |= VI_CTRL_DIVOT_ON;
    }
    if (viFeatures & OS_VI_DIVOT_OFF) {
        ctrl &= ~VI_CTRL_DIVOT_ON;
    }
    viMode->customViMode.comRegs.ctrl = ctrl;
}

void ViMode_Update(ViMode* viMode, Input* input) {
    ViMode_Load(viMode);
    if (viMode->editState == VI_MODE_EDIT_STATE_COMPARE) {
        ViMode_LogPrint(&osViModeNtscLan1);
        ViMode_LogPrint(&viMode->customViMode);
        viMode->editState = VI_MODE_EDIT_STATE_NEG_1;
    } else if ((viMode->editState == VI_MODE_EDIT_STATE_ACTIVE) || (viMode->editState == VI_MODE_EDIT_STATE_SET)) {
        if (pad_push_also(input, START_BUTTON | U_CBUTTONS | R_CBUTTONS)) {
            ViMode_Init(viMode);
        }

        if (pad_push_also(input, U_CBUTTONS)) {
            if (pad_push_also(input, U_JPAD)) {
                viMode->upperAdjust--;
            }
            if (pad_push_also(input, D_JPAD)) {
                viMode->upperAdjust++;
            }
            if (pad_push_also(input, L_JPAD)) {
                viMode->leftAdjust--;
            }
            if (pad_push_also(input, R_JPAD)) {
                viMode->leftAdjust++;
            }
        }

        if (pad_push_also(input, R_CBUTTONS)) {
            if (pad_push_also(input, U_JPAD)) {
                viMode->lowerAdjust--;
            }
            if (pad_push_also(input, D_JPAD)) {
                viMode->lowerAdjust++;
            }
            if (pad_push_also(input, L_JPAD)) {
                viMode->rightAdjust--;
            }
            if (pad_push_also(input, R_JPAD)) {
                viMode->rightAdjust++;
            }
        }

        if (pad_push_also(input, D_CBUTTONS)) {
            if (pad_on_trigger(input, U_JPAD)) {
                viMode->loRes = !viMode->loRes;
            }
            if (pad_on_trigger(input, D_JPAD)) {
                viMode->antialiasOff = !viMode->antialiasOff;
            }
            if (pad_on_trigger(input, L_JPAD)) {
                viMode->modeN = !viMode->modeN;
            }
            if (pad_on_trigger(input, R_JPAD)) {
                viMode->fb16Bit = !viMode->fb16Bit;
            }
        }

        if (viMode->leftAdjust < 0) {
            viMode->leftAdjust = 0;
        }
        if (viMode->upperAdjust < 0) {
            viMode->upperAdjust = 0;
        }
        if (viMode->rightAdjust > 0) {
            viMode->rightAdjust = 0;
        }
        if (viMode->lowerAdjust > 0) {
            viMode->lowerAdjust = 0;
        }

        ViMode_Configure(viMode, OS_VI_MPAL_LPN1, osTvType, viMode->loRes, viMode->antialiasOff, viMode->modeN,
                      viMode->fb16Bit, viMode->viWidth, viMode->viHeight, viMode->leftAdjust, viMode->rightAdjust,
                      viMode->upperAdjust, viMode->lowerAdjust);
        ViMode_ConfigureFeatures(viMode, viMode->viFeatures);

        if (viMode->editState == VI_MODE_EDIT_STATE_SET) {
            ViMode_LogPrint(&viMode->customViMode);
            viMode->editState = VI_MODE_EDIT_STATE_ACTIVE;
            osViSetMode(&viMode->customViMode);
        }
    }

    ViMode_Save(viMode);
}

void func_8007EE28(ViMode* viMode, s32 arg1, s32 arg2) {
    viMode->upperAdjust = arg1 * 2;
    viMode->lowerAdjust = (-(-arg2) * 2) - SCREEN_HEIGHT * 2;
    viMode->viHeight = arg2 - arg1;
    ViMode_Configure(viMode, OS_VI_MPAL_LPN1, osTvType, viMode->loRes, viMode->antialiasOff, viMode->modeN,
                  viMode->fb16Bit, viMode->viWidth, viMode->viHeight, viMode->leftAdjust, viMode->rightAdjust,
                  viMode->upperAdjust, viMode->lowerAdjust);
    ViMode_ConfigureFeatures(viMode, viMode->viFeatures);
    osViSetMode(&viMode->customViMode);
    osViSetSpecialFeatures(viMode->viFeatures);
}
