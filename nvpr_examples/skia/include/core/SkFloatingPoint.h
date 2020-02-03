
/*
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef SkFloatingPoint_DEFINED
#define SkFloatingPoint_DEFINED

#include "SkTypes.h"

#ifdef SK_CAN_USE_FLOAT

#include <math.h>
#include <float.h>
#include "SkFloatBits.h"

// If math.h had powf(float, float), I could remove this wrapper
static inline float sk_float_pow(float base, float exp) {
    return static_cast<float>(pow(static_cast<double>(base),
                                  static_cast<double>(exp)));
}

static inline float sk_float_copysign(float x, float y) {
    int32_t xbits = SkFloat2Bits(x);
    int32_t ybits = SkFloat2Bits(y);
    return SkBits2Float((xbits & 0x7FFFFFFF) | (ybits & 0x80000000));
}

#ifdef SK_BUILD_FOR_WINCE
    #define sk_float_sqrt(x)        (float)::sqrt(x)
    #define sk_float_sin(x)         (float)::sin(x)
    #define sk_float_cos(x)         (float)::cos(x)
    #define sk_float_tan(x)         (float)::tan(x)
    #define sk_float_acos(x)        (float)::acos(x)
    #define sk_float_asin(x)        (float)::asin(x)
    #define sk_float_atan2(y,x)     (float)::atan2(y,x)
    #define sk_float_abs(x)         (float)::fabs(x)
    #define sk_float_mod(x,y)       (float)::fmod(x,y)
    #define sk_float_exp(x)         (float)::exp(x)
    #define sk_float_log(x)         (float)::log(x)
    #define sk_float_floor(x)       (float)::floor(x)
    #define sk_float_ceil(x)        (float)::ceil(x)
#else
    #define sk_float_sqrt(x)        sqrtf(x)
    #define sk_float_sin(x)         sinf(x)
    #define sk_float_cos(x)         cosf(x)
    #define sk_float_tan(x)         tanf(x)
    #define sk_float_floor(x)       floorf(x)
    #define sk_float_ceil(x)        ceilf(x)
#ifdef SK_BUILD_FOR_MAC
    #define sk_float_acos(x)        static_cast<float>(acos(x))
    #define sk_float_asin(x)        static_cast<float>(asin(x))
#else
    #define sk_float_acos(x)        acosf(x)
    #define sk_float_asin(x)        asinf(x)
#endif
    #define sk_float_atan2(y,x)     atan2f(y,x)
    #define sk_float_abs(x)         fabsf(x)
    #define sk_float_mod(x,y)       fmodf(x,y)
    #define sk_float_exp(x)         expf(x)
    #define sk_float_log(x)         logf(x)
#endif

#ifdef SK_BUILD_FOR_WIN
    #define sk_float_isfinite(x)    _finite(x)
    #define sk_float_isnan(x)       _isnan(x)
    static inline int sk_float_isinf(float x) {
        int32_t bits = SkFloat2Bits(x);
        return (bits << 1) == (0xFF << 24);
    }
#else
    #define sk_float_isfinite(x)    isfinite(x)
    #define sk_float_isnan(x)       isnan(x)
    #define sk_float_isinf(x)       isinf(x)
#endif

#ifdef SK_USE_FLOATBITS
    #define sk_float_floor2int(x)   SkFloatToIntFloor(x)
    #define sk_float_round2int(x)   SkFloatToIntRound(x)
    #define sk_float_ceil2int(x)    SkFloatToIntCeil(x)
#else
    #define sk_float_floor2int(x)   (int)sk_float_floor(x)
    #define sk_float_round2int(x)   (int)sk_float_floor((x) + 0.5f)
    #define sk_float_ceil2int(x)    (int)sk_float_ceil(x)
#endif

#endif
#endif
