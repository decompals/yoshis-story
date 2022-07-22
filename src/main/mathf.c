#include "global.h"
#include "fp.h"

// tanf?
f32 func_80081C30(f32 x) {
    return sinf(x) / cosf(x);
}

// Wrapper for floorf
f32 func_80081C64(f32 x) {
    return floorf(x);
}

// Wrapper for ceilf
f32 func_80081C84(f32 x) {
    return ceilf(x);
}

// abs_d?
f64 func_80081CA4(f64 x) {
    return ABS(x);
}

// abs_f?
f32 func_80081CD8(f32 x) {
    return ABS(x);
}

// Wrapper for roundf
f32 func_80081D08(f32 x) {
    return roundf(x);
}

// Wrapper for truncf
f32 func_80081D28(f32 x) {
    return truncf(x);
}

// Wrapper for nearbyintf
f32 func_80081D48(f32 x) {
    return nearbyintf(x);
}

/**
 * @brief arctan using a continued fraction.
 *
 * Uses the formulae
 * \f[
 *  \arctan x = \frac{\pi}{2} - \arctan \Big( \frac{1}{x} \Big) \qquad (x > 0)
 * \f]
 * and
 * \f[
 *  \arctan x = -\frac{\pi}{2} - \arctan \Big( \frac{1}{x} \Big) \qquad (x < 0)
 * \f]
 * to bring \p x into the range \f[ [0,1] \f], then uses the continued fraction formula
 * \f[
 *  \arctan x = \frac{x}{1 +} \frac{x^2}{3 +} \frac{4x^2}{5 +} \frac{9x^2}{7 +} \frac{16x^2}{9 +} \dotso
 * \f]
 * (https://dlmf.nist.gov/4.25.E4)
 *
 * @remark Rather unnecessarily it uses 25 terms of this continued fraction! Ocarina of Time and later use only 9 terms,
 * enough to get to the maximum accuracy possible with this method using only floats.
 *
 * @param[in] x 
 * @return f32 arctan x
 */
f32 func_80081D68(f32 x) {
    s32 sector;
    f32 conv;
    f32 sq;
    f32 z;
    s32 i;

    sector = 1;
    if (x > 1.0f) {
        x = 1.0f / x;
    } else {
        if (x < -1.0f) {
            sector = -1;
            x = 1.0f / x;
        } else {
            sector = 0;
        }
    }

    sq = x * x;
    conv = 0.0f;
    z = 24.0f;

    for (i = 24; i > 0; i--) {
        conv = SQ(z) * sq / (2.0f * z + 1.0f + conv);
        z -= 1.0f;
    }

    if (sector > 0) {
        return M_PI / 2 - (x / (1.0f + conv));
    } else if (sector < 0) {
        return -M_PI / 2 - (x / (1.0f + conv));
    } else {
        return (x / (1.0f + conv));
    }
}

// atan2
f32 func_80081E8C(f32 arg0, f32 arg1) {
    if ((arg0 == 0.0f) && (arg1 == 0.0f)) {
        return 0.0f;
    } else if (arg1 == 0.0f) {
        if (arg0 < 0.0f) {
            return -M_PI / 2;
        } else {
            return M_PI / 2;
        }
    } else if (arg1 < 0.0f) {
        if (arg0 < 0.0f) {
            return -(M_PI - func_80081D68(fabs(arg0 / arg1)));
        } else {
            return M_PI - func_80081D68(fabs(arg0 / arg1));
        }
    } else {
        return func_80081D68(arg0 / arg1);
    }
}

// asin
f32 func_80081F80(f32 x) {
    return func_80081E8C(x, sqrtf(1.0f - SQ(x)));
}

// acos
#pragma GLOBAL_ASM("asm/nonmatchings/main/mathf/func_80081FB0.s")
// f32 func_80081FB0(f32 x) {
//     return M_PI / 2 - func_80081F80(x);
// }
