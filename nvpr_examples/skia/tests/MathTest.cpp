
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "Test.h"
#include "SkFloatingPoint.h"
#include "SkMath.h"
#include "SkPoint.h"
#include "SkRandom.h"
#include "SkColorPriv.h"

static float float_blend(int src, int dst, float unit) {
    return dst + (src - dst) * unit;
}

static int blend31(int src, int dst, int a31) {
    return dst + ((src - dst) * a31 * 2114 >> 16);
    //    return dst + ((src - dst) * a31 * 33 >> 10);
}

static int blend31_slow(int src, int dst, int a31) {
    int prod = src * a31 + (31 - a31) * dst + 16;
    prod = (prod + (prod >> 5)) >> 5;
    return prod;
}

static int blend31_round(int src, int dst, int a31) {
    int prod = (src - dst) * a31 + 16;
    prod = (prod + (prod >> 5)) >> 5;
    return dst + prod;
}

static int blend31_old(int src, int dst, int a31) {
    a31 += a31 >> 4;
    return dst + ((src - dst) * a31 >> 5);
}

static void test_blend31() {
    int failed = 0;
    int death = 0;
    for (int src = 0; src <= 255; src++) {
        for (int dst = 0; dst <= 255; dst++) {
            for (int a = 0; a <= 31; a++) {
//                int r0 = blend31(src, dst, a);
//                int r0 = blend31_round(src, dst, a);
//                int r0 = blend31_old(src, dst, a);
                int r0 = blend31_slow(src, dst, a);

                float f = float_blend(src, dst, a / 31.f);
                int r1 = (int)f;
                int r2 = SkScalarRoundToInt(SkFloatToScalar(f));

                if (r0 != r1 && r0 != r2) {
                    printf("src:%d dst:%d a:%d result:%d float:%g\n",
                                 src, dst, a, r0, f);
                    failed += 1;
                }
                if (r0 > 255) {
                    death += 1;
                    printf("death src:%d dst:%d a:%d result:%d float:%g\n",
                           src, dst, a, r0, f);
                }
            }
        }
    }
    SkDebugf("---- failed %d death %d\n", failed, death);
}

static void test_blend(skiatest::Reporter* reporter) {
    for (int src = 0; src <= 255; src++) {
        for (int dst = 0; dst <= 255; dst++) {
            for (int a = 0; a <= 255; a++) {
                int r0 = SkAlphaBlend255(src, dst, a);
                float f1 = float_blend(src, dst, a / 255.f);
                int r1 = SkScalarRoundToInt(SkFloatToScalar(f1));

                if (r0 != r1) {
                    float diff = sk_float_abs(f1 - r1);
                    diff = sk_float_abs(diff - 0.5f);
                    if (diff > (1 / 255.f)) {
#ifdef SK_DEBUG
                        SkDebugf("src:%d dst:%d a:%d result:%d float:%g\n",
                                 src, dst, a, r0, f1);
#endif
                        REPORTER_ASSERT(reporter, false);
                    }
                }
            }
        }
    }
}

#if defined(SkLONGLONG)
static int symmetric_fixmul(int a, int b) {
    int sa = SkExtractSign(a);
    int sb = SkExtractSign(b);

    a = SkApplySign(a, sa);
    b = SkApplySign(b, sb);

#if 1
    int c = (int)(((SkLONGLONG)a * b) >> 16);

    return SkApplySign(c, sa ^ sb);
#else
    SkLONGLONG ab = (SkLONGLONG)a * b;
    if (sa ^ sb) {
        ab = -ab;
    }
    return ab >> 16;
#endif
}
#endif

static void check_length(skiatest::Reporter* reporter,
                         const SkPoint& p, SkScalar targetLen) {
#ifdef SK_CAN_USE_FLOAT
    float x = SkScalarToFloat(p.fX);
    float y = SkScalarToFloat(p.fY);
    float len = sk_float_sqrt(x*x + y*y);

    len /= SkScalarToFloat(targetLen);

    REPORTER_ASSERT(reporter, len > 0.999f && len < 1.001f);
#endif
}

#if defined(SK_CAN_USE_FLOAT)

static float nextFloat(SkRandom& rand) {
    SkFloatIntUnion data;
    data.fSignBitInt = rand.nextU();
    return data.fFloat;
}

/*  returns true if a == b as resulting from (int)x. Since it is undefined
 what to do if the float exceeds 2^32-1, we check for that explicitly.
 */
static bool equal_float_native_skia(float x, uint32_t ni, uint32_t si) {
    if (!(x == x)) {    // NAN
        return si == SK_MaxS32 || si == SK_MinS32;
    }
    // for out of range, C is undefined, but skia always should return NaN32
    if (x > SK_MaxS32) {
        return si == SK_MaxS32;
    }
    if (x < -SK_MaxS32) {
        return si == SK_MinS32;
    }
    return si == ni;
}

static void assert_float_equal(skiatest::Reporter* reporter, const char op[],
                               float x, uint32_t ni, uint32_t si) {
    if (!equal_float_native_skia(x, ni, si)) {
        SkString desc;
        desc.printf("%s float %g bits %x native %x skia %x\n", op, x, ni, si);
        reporter->reportFailed(desc);
    }
}

static void test_float_cast(skiatest::Reporter* reporter, float x) {
    int ix = (int)x;
    int iix = SkFloatToIntCast(x);
    assert_float_equal(reporter, "cast", x, ix, iix);
}

static void test_float_floor(skiatest::Reporter* reporter, float x) {
    int ix = (int)floor(x);
    int iix = SkFloatToIntFloor(x);
    assert_float_equal(reporter, "floor", x, ix, iix);
}

static void test_float_round(skiatest::Reporter* reporter, float x) {
    double xx = x + 0.5;    // need intermediate double to avoid temp loss
    int ix = (int)floor(xx);
    int iix = SkFloatToIntRound(x);
    assert_float_equal(reporter, "round", x, ix, iix);
}

static void test_float_ceil(skiatest::Reporter* reporter, float x) {
    int ix = (int)ceil(x);
    int iix = SkFloatToIntCeil(x);
    assert_float_equal(reporter, "ceil", x, ix, iix);
}

static void test_float_conversions(skiatest::Reporter* reporter, float x) {
    test_float_cast(reporter, x);
    test_float_floor(reporter, x);
    test_float_round(reporter, x);
    test_float_ceil(reporter, x);
}

static void test_int2float(skiatest::Reporter* reporter, int ival) {
    float x0 = (float)ival;
    float x1 = SkIntToFloatCast(ival);
    float x2 = SkIntToFloatCast_NoOverflowCheck(ival);
    REPORTER_ASSERT(reporter, x0 == x1);
    REPORTER_ASSERT(reporter, x0 == x2);
}

static void unittest_fastfloat(skiatest::Reporter* reporter) {
    SkRandom rand;
    size_t i;

    static const float gFloats[] = {
        0.f, 1.f, 0.5f, 0.499999f, 0.5000001f, 1.f/3,
        0.000000001f, 1000000000.f,     // doesn't overflow
        0.0000000001f, 10000000000.f    // does overflow
    };
    for (i = 0; i < SK_ARRAY_COUNT(gFloats); i++) {
        test_float_conversions(reporter, gFloats[i]);
        test_float_conversions(reporter, -gFloats[i]);
    }

    for (int outer = 0; outer < 100; outer++) {
        rand.setSeed(outer);
        for (i = 0; i < 100000; i++) {
            float x = nextFloat(rand);
            test_float_conversions(reporter, x);
        }

        test_int2float(reporter, 0);
        test_int2float(reporter, 1);
        test_int2float(reporter, -1);
        for (i = 0; i < 100000; i++) {
            // for now only test ints that are 24bits or less, since we don't
            // round (down) large ints the same as IEEE...
            int ival = rand.nextU() & 0xFFFFFF;
            test_int2float(reporter, ival);
            test_int2float(reporter, -ival);
        }
    }
}

#ifdef SK_SCALAR_IS_FLOAT
static float make_zero() {
    return sk_float_sin(0);
}
#endif

static void unittest_isfinite(skiatest::Reporter* reporter) {
#ifdef SK_SCALAR_IS_FLOAT
    float nan = sk_float_asin(2);
    float inf = 1.0 / make_zero();
    float big = 3.40282e+038;

    REPORTER_ASSERT(reporter, !SkScalarIsNaN(inf));
    REPORTER_ASSERT(reporter, !SkScalarIsNaN(-inf));
    REPORTER_ASSERT(reporter, !SkScalarIsFinite(inf));
    REPORTER_ASSERT(reporter, !SkScalarIsFinite(-inf));
#else
    SkFixed nan = SK_FixedNaN;
    SkFixed big = SK_FixedMax;
#endif

    REPORTER_ASSERT(reporter,  SkScalarIsNaN(nan));
    REPORTER_ASSERT(reporter, !SkScalarIsNaN(big));
    REPORTER_ASSERT(reporter, !SkScalarIsNaN(-big));
    REPORTER_ASSERT(reporter, !SkScalarIsNaN(0));
    
    REPORTER_ASSERT(reporter, !SkScalarIsFinite(nan));
    REPORTER_ASSERT(reporter,  SkScalarIsFinite(big));
    REPORTER_ASSERT(reporter,  SkScalarIsFinite(-big));
    REPORTER_ASSERT(reporter,  SkScalarIsFinite(0));
}

#endif

static void test_muldiv255(skiatest::Reporter* reporter) {
#ifdef SK_CAN_USE_FLOAT
    for (int a = 0; a <= 255; a++) {
        for (int b = 0; b <= 255; b++) {
            int ab = a * b;
            float s = ab / 255.0f;
            int round = (int)floorf(s + 0.5f);
            int trunc = (int)floorf(s);

            int iround = SkMulDiv255Round(a, b);
            int itrunc = SkMulDiv255Trunc(a, b);

            REPORTER_ASSERT(reporter, iround == round);
            REPORTER_ASSERT(reporter, itrunc == trunc);

            REPORTER_ASSERT(reporter, itrunc <= iround);
            REPORTER_ASSERT(reporter, iround <= a);
            REPORTER_ASSERT(reporter, iround <= b);
        }
    }
#endif
}

static void test_muldiv255ceiling(skiatest::Reporter* reporter) {
    for (int c = 0; c <= 255; c++) {
        for (int a = 0; a <= 255; a++) {
            int product = (c * a + 255);
            int expected_ceiling = (product + (product >> 8)) >> 8;
            int webkit_ceiling = (c * a + 254) / 255;
            REPORTER_ASSERT(reporter, expected_ceiling == webkit_ceiling);
            int skia_ceiling = SkMulDiv255Ceiling(c, a);
            REPORTER_ASSERT(reporter, skia_ceiling == webkit_ceiling);
        }
    }
}

static void test_copysign(skiatest::Reporter* reporter) {
    static const int32_t gTriples[] = {
        // x, y, expected result
        0, 0, 0,
        0, 1, 0,
        0, -1, 0,
        1, 0, 1,
        1, 1, 1,
        1, -1, -1,
        -1, 0, 1,
        -1, 1, 1,
        -1, -1, -1,
    };
    for (size_t i = 0; i < SK_ARRAY_COUNT(gTriples); i += 3) {
        REPORTER_ASSERT(reporter,
                        SkCopySign32(gTriples[i], gTriples[i+1]) == gTriples[i+2]);
#ifdef SK_CAN_USE_FLOAT
        float x = (float)gTriples[i];
        float y = (float)gTriples[i+1];
        float expected = (float)gTriples[i+2];
        REPORTER_ASSERT(reporter, sk_float_copysign(x, y) == expected);
#endif
    }

    SkRandom rand;
    for (int j = 0; j < 1000; j++) {
        int ix = rand.nextS();
        REPORTER_ASSERT(reporter, SkCopySign32(ix, ix) == ix);
        REPORTER_ASSERT(reporter, SkCopySign32(ix, -ix) == -ix);
        REPORTER_ASSERT(reporter, SkCopySign32(-ix, ix) == ix);
        REPORTER_ASSERT(reporter, SkCopySign32(-ix, -ix) == -ix);

        SkScalar sx = rand.nextSScalar1();
        REPORTER_ASSERT(reporter, SkScalarCopySign(sx, sx) == sx);
        REPORTER_ASSERT(reporter, SkScalarCopySign(sx, -sx) == -sx);
        REPORTER_ASSERT(reporter, SkScalarCopySign(-sx, sx) == sx);
        REPORTER_ASSERT(reporter, SkScalarCopySign(-sx, -sx) == -sx);
    }
}

static void TestMath(skiatest::Reporter* reporter) {
    int         i;
    int32_t     x;
    SkRandom    rand;

    // these should assert
#if 0
    SkToS8(128);
    SkToS8(-129);
    SkToU8(256);
    SkToU8(-5);

    SkToS16(32768);
    SkToS16(-32769);
    SkToU16(65536);
    SkToU16(-5);

    if (sizeof(size_t) > 4) {
        SkToS32(4*1024*1024);
        SkToS32(-4*1024*1024);
        SkToU32(5*1024*1024);
        SkToU32(-5);
    }
#endif

    test_muldiv255(reporter);
    test_muldiv255ceiling(reporter);
    test_copysign(reporter);

    {
        SkScalar x = SK_ScalarNaN;
        REPORTER_ASSERT(reporter, SkScalarIsNaN(x));
    }

    for (i = 1; i <= 10; i++) {
        x = SkCubeRootBits(i*i*i, 11);
        REPORTER_ASSERT(reporter, x == i);
    }

    x = SkFixedSqrt(SK_Fixed1);
    REPORTER_ASSERT(reporter, x == SK_Fixed1);
    x = SkFixedSqrt(SK_Fixed1/4);
    REPORTER_ASSERT(reporter, x == SK_Fixed1/2);
    x = SkFixedSqrt(SK_Fixed1*4);
    REPORTER_ASSERT(reporter, x == SK_Fixed1*2);

    x = SkFractSqrt(SK_Fract1);
    REPORTER_ASSERT(reporter, x == SK_Fract1);
    x = SkFractSqrt(SK_Fract1/4);
    REPORTER_ASSERT(reporter, x == SK_Fract1/2);
    x = SkFractSqrt(SK_Fract1/16);
    REPORTER_ASSERT(reporter, x == SK_Fract1/4);

    for (i = 1; i < 100; i++) {
        x = SkFixedSqrt(SK_Fixed1 * i * i);
        REPORTER_ASSERT(reporter, x == SK_Fixed1 * i);
    }

    for (i = 0; i < 1000; i++) {
        int value = rand.nextS16();
        int max = rand.nextU16();

        int clamp = SkClampMax(value, max);
        int clamp2 = value < 0 ? 0 : (value > max ? max : value);
        REPORTER_ASSERT(reporter, clamp == clamp2);
    }

    for (i = 0; i < 10000; i++) {
        SkPoint p;

        p.setLength(rand.nextS(), rand.nextS(), SK_Scalar1);
        check_length(reporter, p, SK_Scalar1);
        p.setLength(rand.nextS() >> 13, rand.nextS() >> 13, SK_Scalar1);
        check_length(reporter, p, SK_Scalar1);
    }

    {
        SkFixed result = SkFixedDiv(100, 100);
        REPORTER_ASSERT(reporter, result == SK_Fixed1);
        result = SkFixedDiv(1, SK_Fixed1);
        REPORTER_ASSERT(reporter, result == 1);
    }

#ifdef SK_CAN_USE_FLOAT
    unittest_fastfloat(reporter);
    unittest_isfinite(reporter);
#endif

#ifdef SkLONGLONG
    for (i = 0; i < 10000; i++) {
        SkFixed numer = rand.nextS();
        SkFixed denom = rand.nextS();
        SkFixed result = SkFixedDiv(numer, denom);
        SkLONGLONG check = ((SkLONGLONG)numer << 16) / denom;

        (void)SkCLZ(numer);
        (void)SkCLZ(denom);

        REPORTER_ASSERT(reporter, result != (SkFixed)SK_NaN32);
        if (check > SK_MaxS32) {
            check = SK_MaxS32;
        } else if (check < -SK_MaxS32) {
            check = SK_MinS32;
        }
        REPORTER_ASSERT(reporter, result == (int32_t)check);

        result = SkFractDiv(numer, denom);
        check = ((SkLONGLONG)numer << 30) / denom;

        REPORTER_ASSERT(reporter, result != (SkFixed)SK_NaN32);
        if (check > SK_MaxS32) {
            check = SK_MaxS32;
        } else if (check < -SK_MaxS32) {
            check = SK_MinS32;
        }
        REPORTER_ASSERT(reporter, result == (int32_t)check);

        // make them <= 2^24, so we don't overflow in fixmul
        numer = numer << 8 >> 8;
        denom = denom << 8 >> 8;

        result = SkFixedMul(numer, denom);
        SkFixed r2 = symmetric_fixmul(numer, denom);
        //        SkASSERT(result == r2);

        result = SkFixedMul(numer, numer);
        r2 = SkFixedSquare(numer);
        REPORTER_ASSERT(reporter, result == r2);

#ifdef SK_CAN_USE_FLOAT
        if (numer >= 0 && denom >= 0) {
            SkFixed mean = SkFixedMean(numer, denom);
            float prod = SkFixedToFloat(numer) * SkFixedToFloat(denom);
            float fm = sk_float_sqrt(sk_float_abs(prod));
            SkFixed mean2 = SkFloatToFixed(fm);
            int diff = SkAbs32(mean - mean2);
            REPORTER_ASSERT(reporter, diff <= 1);
        }

        {
            SkFixed mod = SkFixedMod(numer, denom);
            float n = SkFixedToFloat(numer);
            float d = SkFixedToFloat(denom);
            float m = sk_float_mod(n, d);
            // ensure the same sign
            REPORTER_ASSERT(reporter, mod == 0 || (mod < 0) == (m < 0));
            int diff = SkAbs32(mod - SkFloatToFixed(m));
            REPORTER_ASSERT(reporter, (diff >> 7) == 0);
        }
#endif
    }
#endif

#ifdef SK_CAN_USE_FLOAT
    for (i = 0; i < 10000; i++) {
        SkFract x = rand.nextU() >> 1;
        double xx = (double)x / SK_Fract1;
        SkFract xr = SkFractSqrt(x);
        SkFract check = SkFloatToFract(sqrt(xx));
        REPORTER_ASSERT(reporter, xr == check ||
                                  xr == check-1 ||
                                  xr == check+1);

        xr = SkFixedSqrt(x);
        xx = (double)x / SK_Fixed1;
        check = SkFloatToFixed(sqrt(xx));
        REPORTER_ASSERT(reporter, xr == check || xr == check-1);

        xr = SkSqrt32(x);
        xx = (double)x;
        check = (int32_t)sqrt(xx);
        REPORTER_ASSERT(reporter, xr == check || xr == check-1);
    }
#endif

#if !defined(SK_SCALAR_IS_FLOAT) && defined(SK_CAN_USE_FLOAT)
    {
        SkFixed s, c;
        s = SkFixedSinCos(0, &c);
        REPORTER_ASSERT(reporter, s == 0);
        REPORTER_ASSERT(reporter, c == SK_Fixed1);
    }

    int maxDiff = 0;
    for (i = 0; i < 1000; i++) {
        SkFixed rads = rand.nextS() >> 10;
        double frads = SkFixedToFloat(rads);

        SkFixed s, c;
        s = SkScalarSinCos(rads, &c);

        double fs = sin(frads);
        double fc = cos(frads);

        SkFixed is = SkFloatToFixed(fs);
        SkFixed ic = SkFloatToFixed(fc);

        maxDiff = SkMax32(maxDiff, SkAbs32(is - s));
        maxDiff = SkMax32(maxDiff, SkAbs32(ic - c));
    }
    SkDebugf("SinCos: maximum error = %d\n", maxDiff);
#endif

#ifdef SK_SCALAR_IS_FLOAT
    test_blend(reporter);
#endif

    // disable for now
//    test_blend31();
}

#include "TestClassDef.h"
DEFINE_TESTCLASS("Math", MathTestClass, TestMath)
