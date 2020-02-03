
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "Test.h"
#include "SkPath.h"
#include "SkLineClipper.h"
#include "SkEdgeClipper.h"

#include "SkCanvas.h"
static void test_hairclipping(skiatest::Reporter* reporter) {
    SkBitmap bm;
    bm.setConfig(SkBitmap::kARGB_8888_Config, 4, 4);
    bm.allocPixels();
    bm.eraseColor(SK_ColorWHITE);
    
    SkPaint paint;
    paint.setAntiAlias(true);

    SkCanvas canvas(bm);
    canvas.clipRect(SkRect::MakeWH(SkIntToScalar(4), SkIntToScalar(2)));
    canvas.drawLine(SkFloatToScalar(1.5), SkFloatToScalar(1.5),
                    SkFloatToScalar(3.5), SkFloatToScalar(3.5), paint);
    
    /**
     *  We had a bug where we misinterpreted the bottom of the clip, and
     *  would draw another pixel (to the right in this case) on the same
     *  last scanline. i.e. we would draw to [2,1], even though this hairline
     *  should just draw to [1,1], [2,2], [3,3] modulo the clip.
     *
     *  The result of this entire draw should be that we only draw to [1,1]
     *
     *  Fixed in rev. 3366
     */
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            bool nonWhite = (1 == y) && (1 == x);
            SkPMColor c = *bm.getAddr32(x, y);
            if (nonWhite) {
                REPORTER_ASSERT(reporter, 0xFFFFFFFF != c);
            } else {
                REPORTER_ASSERT(reporter, 0xFFFFFFFF == c);
            }
        }
    }
}

static void test_edgeclipper(skiatest::Reporter* reporter) {
    SkEdgeClipper clipper;
    
    const SkPoint pts[] = {
        { SkFloatToScalar(3.0995476e+010),  SkFloatToScalar(42.929779) },
        { SkFloatToScalar(-3.0995163e+010), SkFloatToScalar(51.050385) },
        { SkFloatToScalar(-3.0995157e+010), SkFloatToScalar(51.050392) },
        { SkFloatToScalar(-3.0995134e+010), SkFloatToScalar(51.050400) },
    };

    const SkRect clip = { 0, 0, SkIntToScalar(300), SkIntToScalar(200) };

    // this should not assert, even though our choppers do a poor numerical
    // job when computing their t values.
    // http://code.google.com/p/skia/issues/detail?id=444
    clipper.clipCubic(pts, clip);
}

static void test_intersectline(skiatest::Reporter* reporter) {
    static const SkScalar L = 0;
    static const SkScalar T = 0;
    static const SkScalar R = SkIntToScalar(100);
    static const SkScalar B = SkIntToScalar(100);
    static const SkScalar CX = SkScalarHalf(L + R);
    static const SkScalar CY = SkScalarHalf(T + B);
    static const SkRect gR = { L, T, R, B };

    size_t i;
    SkPoint dst[2];

    static const SkPoint gEmpty[] = {
        // sides
        { L, CY }, { L - 10, CY },
        { R, CY }, { R + 10, CY },
        { CX, T }, { CX, T - 10 },
        { CX, B }, { CX, B + 10 },
        // corners
        { L, T }, { L - 10, T - 10 },
        { L, B }, { L - 10, B + 10 },
        { R, T }, { R + 10, T - 10 },
        { R, B }, { R + 10, B + 10 },
    };
    for (i = 0; i < SK_ARRAY_COUNT(gEmpty); i += 2) {
        bool valid = SkLineClipper::IntersectLine(&gEmpty[i], gR, dst);
        if (valid) {
            SkDebugf("----- [%d] %g %g -> %g %g\n", i/2, dst[0].fX, dst[0].fY, dst[1].fX, dst[1].fY);
        }
        REPORTER_ASSERT(reporter, !valid);
    }
    
    static const SkPoint gFull[] = {
        // diagonals, chords
        { L, T }, { R, B },
        { L, B }, { R, T },
        { CX, T }, { CX, B },
        { L, CY }, { R, CY },
        { CX, T }, { R, CY },
        { CX, T }, { L, CY },
        { L, CY }, { CX, B },
        { R, CY }, { CX, B },
        // edges
        { L, T }, { L, B },
        { R, T }, { R, B },
        { L, T }, { R, T },
        { L, B }, { R, B },
    };
    for (i = 0; i < SK_ARRAY_COUNT(gFull); i += 2) {
        bool valid = SkLineClipper::IntersectLine(&gFull[i], gR, dst);
        if (!valid || memcmp(&gFull[i], dst, sizeof(dst))) {
            SkDebugf("++++ [%d] %g %g -> %g %g\n", i/2, dst[0].fX, dst[0].fY, dst[1].fX, dst[1].fY);
        }
        REPORTER_ASSERT(reporter, valid && !memcmp(&gFull[i], dst, sizeof(dst)));
    }
    
    static const SkPoint gPartial[] = {
        { L - 10, CY }, { CX, CY }, { L, CY }, { CX, CY },
        { CX, T - 10 }, { CX, CY }, { CX, T }, { CX, CY },
        { R + 10, CY }, { CX, CY }, { R, CY }, { CX, CY },
        { CX, B + 10 }, { CX, CY }, { CX, B }, { CX, CY },
        // extended edges
        { L, T - 10 }, { L, B + 10 }, { L, T }, { L, B },
        { R, T - 10 }, { R, B + 10 }, { R, T }, { R, B },
        { L - 10, T }, { R + 10, T }, { L, T }, { R, T },
        { L - 10, B }, { R + 10, B }, { L, B }, { R, B },
    };
    for (i = 0; i < SK_ARRAY_COUNT(gPartial); i += 4) {
        bool valid = SkLineClipper::IntersectLine(&gPartial[i], gR, dst);
        if (!valid || memcmp(&gPartial[i+2], dst, sizeof(dst))) {
            SkDebugf("++++ [%d] %g %g -> %g %g\n", i/2, dst[0].fX, dst[0].fY, dst[1].fX, dst[1].fY);
        }
        REPORTER_ASSERT(reporter, valid &&
                                  !memcmp(&gPartial[i+2], dst, sizeof(dst)));
    }
    
}

void TestClipper(skiatest::Reporter* reporter) {
    test_intersectline(reporter);
    test_edgeclipper(reporter);
    test_hairclipping(reporter);
}

#include "TestClassDef.h"
DEFINE_TESTCLASS("Clipper", TestClipperClass, TestClipper)
