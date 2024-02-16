#ifndef MATH64_H
#define MATH64_H

#include "ultra64.h"

#ifdef __sgi
#pragma intrinsic(sqrtf);
#endif

f32 Math_FTanF(f32 x);
f32 Math_FFloorF(f32 x);
f32 Math_FCeilF(f32 x);
f64 Math_FAbs(f64 x);
f32 Math_FAbsF(f32 x);
f32 Math_FRoundF(f32 x);
f32 Math_FTruncF(f32 x);
f32 Math_FNearbyIntF(f32 x);
f32 fatan2(f32 y, f32 x);
f32 Math_FAsinF(f32 x);
f32 Math_FAcosF(f32 x);


#endif
