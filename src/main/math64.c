#include "global.h"
#include "math64.h"
#include "libc/math.h"
#include "fp.h"

f32 Math_FTanF(f32 x) {
    return sinf(x) / cosf(x);
}

f32 Math_FFloorF(f32 x) {
    return floorf(x);
}

f32 Math_FCeilF(f32 x) {
    return ceilf(x);
}

f64 Math_FAbs(f64 x) {
    return ABS(x);
}

f32 Math_FAbsF(f32 x) {
    return ABS(x);
}

f32 Math_FRoundF(f32 x) {
    return roundf(x);
}

f32 Math_FTruncF(f32 x) {
    return truncf(x);
}

f32 Math_FNearbyIntF(f32 x) {
    return nearbyintf(x);
}

f32 Math_FAtanContFracF(f32 x) {
    s32 sector;
    f32 z;
    f32 conv;
    f32 sq;
    s32 i;

    if (x > 1.0f) {
        sector = 1;
        x = 1.0f / x;
    } else if (x < -1.0f) {
        sector = -1;
        x = 1.0f / x;
    } else {
        sector = 0;
    }

    sq = x * x;
    conv = 0.0f;
    z = 24.0f;
    for (i = 24; i != 0; i--) {
        conv = z * z * sq / (2.0f * z + 1.0f + conv);
        z -= 1.0f;
    }

    if (sector > 0) {
        return ((f32)M_PI / 2) - (x / (1.0f + conv));
    } else if (sector < 0) {
        return -((f32)M_PI / 2) - (x / (1.0f + conv));
    } else {
        return x / (1.0f + conv);
    }
}

f32 fatan2(f32 y, f32 x) {
    if ((y == 0.0f) && (x == 0.0f)) {
        return 0.0f;
    } else if (x == 0.0f) {
        if (y < 0.0f) {
            return -((f32)M_PI / 2);
        } else {
            return (f32)M_PI / 2;
        }
    } else if (x < 0.0f) {
        if (y < 0.0f) {
            return -((f32)M_PI - Math_FAtanContFracF(fabs((f64)(y / x))));
        } else {
            return (f32)M_PI - Math_FAtanContFracF(fabs((f64)(y / x)));
        }
    } else {
        return Math_FAtanContFracF(y / x);
    }
}

f32 Math_FAsinF(f32 x) {
    return fatan2(x, sqrtf(1.0f - (x * x)));
}

f32 Math_FAcosF(f32 x) {
    return ((f32)M_PI / 2) - Math_FAsinF(x);
}
