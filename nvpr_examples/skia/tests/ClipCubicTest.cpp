
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "Test.h"

#include "SkCanvas.h"
#include "SkPaint.h"
#include "SkCubicClipper.h"
#include "SkGeometry.h"

// Currently the supersampler blitter uses int16_t for its index into an array
// the width of the clip. Test that we don't crash/assert if we try to draw
// with a device/clip that is larger.
static void test_giantClip() {
    SkBitmap bm;
    bm.setConfig(SkBitmap::kARGB_8888_Config, 64919, 1);
    bm.allocPixels();
    SkCanvas canvas(bm);
    canvas.clear(0);
    
    SkPath path;
    path.moveTo(0, 0); path.lineTo(1, 0); path.lineTo(33, 1);
    SkPaint paint;
    paint.setAntiAlias(true);
    canvas.drawPath(path, paint);
}

static void PrintCurve(const char *name, const SkPoint crv[4]) {
    printf("%s: %.10g, %.10g, %.10g, %.10g, %.10g, %.10g, %.10g, %.10g\n",
            name,
            (float)crv[0].fX, (float)crv[0].fY,
            (float)crv[1].fX, (float)crv[1].fY,
            (float)crv[2].fX, (float)crv[2].fY,
            (float)crv[3].fX, (float)crv[3].fY);

}


static bool CurvesAreEqual(const SkPoint c0[4],
                           const SkPoint c1[4],
                           float tol) {
    for (int i = 0; i < 4; i++) {
        if (SkScalarAbs(c0[i].fX - c1[i].fX) > SkFloatToScalar(tol) ||
            SkScalarAbs(c0[i].fY - c1[i].fY) > SkFloatToScalar(tol)
        ) {
            PrintCurve("c0", c0);
            PrintCurve("c1", c1);
            return false;
        }
    }
    return true;
}


static SkPoint* SetCurve(float x0, float y0,
                         float x1, float y1,
                         float x2, float y2,
                         float x3, float y3,
                         SkPoint crv[4]) {
    crv[0].fX = SkFloatToScalar(x0);   crv[0].fY = SkFloatToScalar(y0);
    crv[1].fX = SkFloatToScalar(x1);   crv[1].fY = SkFloatToScalar(y1);
    crv[2].fX = SkFloatToScalar(x2);   crv[2].fY = SkFloatToScalar(y2);
    crv[3].fX = SkFloatToScalar(x3);   crv[3].fY = SkFloatToScalar(y3);
    return crv;
}


static void TestCubicClipping(skiatest::Reporter* reporter) {
    static SkPoint crv[4] = {
        { SkIntToScalar(0), SkIntToScalar(0)  },
        { SkIntToScalar(2), SkIntToScalar(3)  },
        { SkIntToScalar(1), SkIntToScalar(10) },
        { SkIntToScalar(4), SkIntToScalar(12) }
    };

    SkCubicClipper clipper;
    SkPoint clipped[4], shouldbe[4];
    SkIRect clipRect;
    bool success;
    const float tol = SkFloatToScalar(1e-4);

    // Test no clip, with plenty of room.
    clipRect.set(-2, -2, 6, 14);
    clipper.setClip(clipRect);
    success = clipper.clipCubic(crv, clipped);
    REPORTER_ASSERT(reporter, success == true);
    REPORTER_ASSERT(reporter, CurvesAreEqual(clipped, SetCurve(
        0, 0, 2, 3, 1, 10, 4, 12, shouldbe), tol));

    // Test no clip, touching first point.
    clipRect.set(-2, 0, 6, 14);
    clipper.setClip(clipRect);
    success = clipper.clipCubic(crv, clipped);
    REPORTER_ASSERT(reporter, success == true);
    REPORTER_ASSERT(reporter, CurvesAreEqual(clipped, SetCurve(
        0, 0, 2, 3, 1, 10, 4, 12, shouldbe), tol));

    // Test no clip, touching last point.
    clipRect.set(-2, -2, 6, 12);
    clipper.setClip(clipRect);
    success = clipper.clipCubic(crv, clipped);
    REPORTER_ASSERT(reporter, success == true);
    REPORTER_ASSERT(reporter, CurvesAreEqual(clipped, SetCurve(
        0, 0, 2, 3, 1, 10, 4, 12, shouldbe), tol));

    // Test all clip.
    clipRect.set(-2, 14, 6, 20);
    clipper.setClip(clipRect);
    success = clipper.clipCubic(crv, clipped);
    REPORTER_ASSERT(reporter, success == false);

    // Test clip at 1.
    clipRect.set(-2, 1, 6, 14);
    clipper.setClip(clipRect);
    success = clipper.clipCubic(crv, clipped);
    REPORTER_ASSERT(reporter, success == true);
    REPORTER_ASSERT(reporter, CurvesAreEqual(clipped, SetCurve(
        0.5126125216, 1,
        1.841195941,  4.337081432,
        1.297019958,  10.19801331,
        4,            12,
        shouldbe), tol));

    // Test clip at 2.
    clipRect.set(-2, 2, 6, 14);
    clipper.setClip(clipRect);
    success = clipper.clipCubic(crv, clipped);
    REPORTER_ASSERT(reporter, success == true);
    REPORTER_ASSERT(reporter, CurvesAreEqual(clipped, SetCurve(
        00.8412352204, 2,
        1.767683744,   5.400758266,
        1.55052948,    10.36701965,
        4,             12,
        shouldbe), tol));

    // Test clip at 11.
    clipRect.set(-2, -2, 6, 11);
    clipper.setClip(clipRect);
    success = clipper.clipCubic(crv, clipped);
    REPORTER_ASSERT(reporter, success == true);
    REPORTER_ASSERT(reporter, CurvesAreEqual(clipped, SetCurve(
        0,           0,
        1.742904663, 2.614356995,
        1.207521796, 8.266430855,
        3.026495695, 11,
        shouldbe), tol));

    // Test clip at 10.
    clipRect.set(-2, -2, 6, 10);
    clipper.setClip(clipRect);
    success = clipper.clipCubic(crv, clipped);
    REPORTER_ASSERT(reporter, success == true);
    REPORTER_ASSERT(reporter, CurvesAreEqual(clipped, SetCurve(
        0,           0,
        1.551193237, 2.326789856,
        1.297736168, 7.059780121,
        2.505550385, 10,
        shouldbe), tol));

    test_giantClip();
}




#include "TestClassDef.h"
DEFINE_TESTCLASS("CubicClipper", CubicClippingTestClass, TestCubicClipping)
