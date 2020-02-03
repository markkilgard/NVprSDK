/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "Test.h"
#include "SkAAClip.h"
#include "SkCanvas.h"
#include "SkMask.h"
#include "SkPath.h"
#include "SkRandom.h"

static bool operator==(const SkMask& a, const SkMask& b) {
    if (a.fFormat != b.fFormat || a.fBounds != b.fBounds) {
        return false;
    }
    if (!a.fImage && !b.fImage) {
        return true;
    }
    if (!a.fImage || !b.fImage) {
        return false;
    }

    size_t wbytes = a.fBounds.width();
    switch (a.fFormat) {
        case SkMask::kBW_Format:
            wbytes = (wbytes + 7) >> 3;
            break;
        case SkMask::kA8_Format:
        case SkMask::k3D_Format:
            break;
        case SkMask::kLCD16_Format:
            wbytes <<= 1;
            break;
        case SkMask::kLCD32_Format:
        case SkMask::kARGB32_Format:
            wbytes <<= 2;
            break;
        default:
            SkASSERT(!"unknown mask format");
            return false;
    }

    const int h = a.fBounds.height();
    const char* aptr = (const char*)a.fImage;
    const char* bptr = (const char*)b.fImage;
    for (int y = 0; y < h; ++y) {
        if (memcmp(aptr, bptr, wbytes)) {
            return false;
        }
        aptr += wbytes;
        bptr += wbytes;
    }
    return true;
}

static void copyToMask(const SkRegion& rgn, SkMask* mask) {
    mask->fFormat = SkMask::kA8_Format;

    if (rgn.isEmpty()) {
        mask->fBounds.setEmpty();
        mask->fRowBytes = 0;
        mask->fImage = NULL;
        return;
    }

    mask->fBounds = rgn.getBounds();
    mask->fRowBytes = mask->fBounds.width();
    mask->fImage = SkMask::AllocImage(mask->computeImageSize());
    sk_bzero(mask->fImage, mask->computeImageSize());

    SkBitmap bitmap;
    bitmap.setConfig(SkBitmap::kA8_Config, mask->fBounds.width(),
                     mask->fBounds.height(), mask->fRowBytes);
    bitmap.setPixels(mask->fImage);

    // canvas expects its coordinate system to always be 0,0 in the top/left
    // so we translate the rgn to match that before drawing into the mask.
    //
    SkRegion tmpRgn(rgn);
    tmpRgn.translate(-rgn.getBounds().fLeft, -rgn.getBounds().fTop);

    SkCanvas canvas(bitmap);
    canvas.clipRegion(tmpRgn);
    canvas.drawColor(SK_ColorBLACK);
}

static SkIRect rand_rect(SkRandom& rand, int n) {
    int x = rand.nextS() % n;
    int y = rand.nextS() % n;
    int w = rand.nextU() % n;
    int h = rand.nextU() % n;
    return SkIRect::MakeXYWH(x, y, w, h);
}

static void make_rand_rgn(SkRegion* rgn, SkRandom& rand) {
    int count = rand.nextU() % 20;
    for (int i = 0; i < count; ++i) {
        rgn->op(rand_rect(rand, 100), SkRegion::kXOR_Op);
    }
}

static bool operator==(const SkRegion& rgn, const SkAAClip& aaclip) {
    SkMask mask0, mask1;

    copyToMask(rgn, &mask0);
    aaclip.copyToMask(&mask1);
    bool eq = (mask0 == mask1);

    SkMask::FreeImage(mask0.fImage);
    SkMask::FreeImage(mask1.fImage);
    return eq;
}

static bool equalsAAClip(const SkRegion& rgn) {
    SkAAClip aaclip;
    aaclip.setRegion(rgn);
    return rgn == aaclip;
}

static void setRgnToPath(SkRegion* rgn, const SkPath& path) {
    SkIRect ir;
    path.getBounds().round(&ir);
    rgn->setPath(path, SkRegion(ir));
}

// aaclip.setRegion should create idential masks to the region
static void test_rgn(skiatest::Reporter* reporter) {
    SkRandom rand;
    for (int i = 0; i < 1000; i++) {
        SkRegion rgn;
        make_rand_rgn(&rgn, rand);
        REPORTER_ASSERT(reporter, equalsAAClip(rgn));
    }

    {
        SkRegion rgn;
        SkPath path;
        path.addCircle(0, 0, SkIntToScalar(30));
        setRgnToPath(&rgn, path);
        REPORTER_ASSERT(reporter, equalsAAClip(rgn));

        path.reset();
        path.moveTo(0, 0);
        path.lineTo(SkIntToScalar(100), 0);
        path.lineTo(SkIntToScalar(100 - 20), SkIntToScalar(20));
        path.lineTo(SkIntToScalar(20), SkIntToScalar(20));
        setRgnToPath(&rgn, path);
        REPORTER_ASSERT(reporter, equalsAAClip(rgn));
    }
}

static const SkRegion::Op gRgnOps[] = {
    SkRegion::kDifference_Op,
    SkRegion::kIntersect_Op,
    SkRegion::kUnion_Op,
    SkRegion::kXOR_Op,
    SkRegion::kReverseDifference_Op,
    SkRegion::kReplace_Op
};

static const char* gRgnOpNames[] = {
    "DIFF", "INTERSECT", "UNION", "XOR", "REVERSE_DIFF", "REPLACE"
};

static void imoveTo(SkPath& path, int x, int y) {
    path.moveTo(SkIntToScalar(x), SkIntToScalar(y));
}

static void icubicTo(SkPath& path, int x0, int y0, int x1, int y1, int x2, int y2) {
    path.cubicTo(SkIntToScalar(x0), SkIntToScalar(y0),
                 SkIntToScalar(x1), SkIntToScalar(y1),
                 SkIntToScalar(x2), SkIntToScalar(y2));
}

static void test_path_bounds(skiatest::Reporter* reporter) {
    SkPath path;
    SkAAClip clip;
    const int height = 40;
    const SkScalar sheight = SkIntToScalar(height);

    path.addOval(SkRect::MakeWH(sheight, sheight));
    REPORTER_ASSERT(reporter, sheight == path.getBounds().height());
    clip.setPath(path, NULL, true);
    REPORTER_ASSERT(reporter, height == clip.getBounds().height());

    // this is the trimmed height of this cubic (with aa). The critical thing
    // for this test is that it is less than height, which represents just
    // the bounds of the path's control-points.
    //
    // This used to fail until we tracked the MinY in the BuilderBlitter.
    //
    const int teardrop_height = 12;
    path.reset();
    imoveTo(path, 0, 20);
    icubicTo(path, 40, 40, 40, 0, 0, 20);
    REPORTER_ASSERT(reporter, sheight == path.getBounds().height());
    clip.setPath(path, NULL, true);
    REPORTER_ASSERT(reporter, teardrop_height == clip.getBounds().height());
}

static void test_empty(skiatest::Reporter* reporter) {
    SkAAClip clip0, clip1;

    REPORTER_ASSERT(reporter, clip0.isEmpty());
    REPORTER_ASSERT(reporter, clip0.getBounds().isEmpty());
    REPORTER_ASSERT(reporter, clip1 == clip0);

    clip0.translate(10, 10);    // should have no effect on empty
    REPORTER_ASSERT(reporter, clip0.isEmpty());
    REPORTER_ASSERT(reporter, clip0.getBounds().isEmpty());
    REPORTER_ASSERT(reporter, clip1 == clip0);

    SkIRect r = { 10, 10, 40, 50 };
    clip0.setRect(r);
    REPORTER_ASSERT(reporter, !clip0.isEmpty());
    REPORTER_ASSERT(reporter, !clip0.getBounds().isEmpty());
    REPORTER_ASSERT(reporter, clip0 != clip1);
    REPORTER_ASSERT(reporter, clip0.getBounds() == r);

    clip0.setEmpty();
    REPORTER_ASSERT(reporter, clip0.isEmpty());
    REPORTER_ASSERT(reporter, clip0.getBounds().isEmpty());
    REPORTER_ASSERT(reporter, clip1 == clip0);

    SkMask mask;
    mask.fImage = NULL;
    clip0.copyToMask(&mask);
    REPORTER_ASSERT(reporter, NULL == mask.fImage);
    REPORTER_ASSERT(reporter, mask.fBounds.isEmpty());
}

static void rand_irect(SkIRect* r, int N, SkRandom& rand) {
    r->setXYWH(0, 0, rand.nextU() % N, rand.nextU() % N);
    int dx = rand.nextU() % (2*N);
    int dy = rand.nextU() % (2*N);
    // use int dx,dy to make the subtract be signed
    r->offset(N - dx, N - dy);
}

static void test_irect(skiatest::Reporter* reporter) {
    SkRandom rand;

    for (int i = 0; i < 10000; i++) {
        SkAAClip clip0, clip1;
        SkRegion rgn0, rgn1;
        SkIRect r0, r1;

        rand_irect(&r0, 10, rand);
        rand_irect(&r1, 10, rand);
        clip0.setRect(r0);
        clip1.setRect(r1);
        rgn0.setRect(r0);
        rgn1.setRect(r1);
        for (size_t j = 0; j < SK_ARRAY_COUNT(gRgnOps); ++j) {
            SkRegion::Op op = gRgnOps[j];
            SkAAClip clip2;
            SkRegion rgn2;
            bool nonEmptyAA = clip2.op(clip0, clip1, op);
            bool nonEmptyBW = rgn2.op(rgn0, rgn1, op);
            if (nonEmptyAA != nonEmptyBW || clip2.getBounds() != rgn2.getBounds()) {
                SkDebugf("[%d %d %d %d] %s [%d %d %d %d] = BW:[%d %d %d %d] AA:[%d %d %d %d]\n",
                         r0.fLeft, r0.fTop, r0.right(), r0.bottom(),
                         gRgnOpNames[j],
                         r1.fLeft, r1.fTop, r1.right(), r1.bottom(),
                         rgn2.getBounds().fLeft, rgn2.getBounds().fTop,
                         rgn2.getBounds().right(), rgn2.getBounds().bottom(),
                         clip2.getBounds().fLeft, clip2.getBounds().fTop,
                         clip2.getBounds().right(), clip2.getBounds().bottom());
            }
            REPORTER_ASSERT(reporter, nonEmptyAA == nonEmptyBW);
            REPORTER_ASSERT(reporter, clip2.getBounds() == rgn2.getBounds());
            
            SkMask maskBW, maskAA;
            copyToMask(rgn2, &maskBW);
            clip2.copyToMask(&maskAA);
            SkAutoMaskFreeImage freeBW(maskBW.fImage);
            SkAutoMaskFreeImage freeAA(maskAA.fImage);
            REPORTER_ASSERT(reporter, maskBW == maskAA);
        }
    }
}

static void test_path_with_hole(skiatest::Reporter* reporter) {
    static const uint8_t gExpectedImage[] = {
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
    };
    SkMask expected;
    expected.fBounds.set(0, 0, 4, 6);
    expected.fRowBytes = 4;
    expected.fFormat = SkMask::kA8_Format;
    expected.fImage = (uint8_t*)gExpectedImage;

    SkPath path;
    path.addRect(SkRect::MakeXYWH(0, 0,
                                  SkIntToScalar(4), SkIntToScalar(2)));
    path.addRect(SkRect::MakeXYWH(0, SkIntToScalar(4),
                                  SkIntToScalar(4), SkIntToScalar(2)));

    for (int i = 0; i < 2; ++i) {
        SkAAClip clip;
        clip.setPath(path, NULL, 1 == i);
        
        SkMask mask;
        clip.copyToMask(&mask);
        SkAutoMaskFreeImage freeM(mask.fImage);

        REPORTER_ASSERT(reporter, expected == mask);
    }
}

static void test_regressions(skiatest::Reporter* reporter) {
    // these should not assert in the debug build
    // bug was introduced in rev. 3209
    {
        SkAAClip clip;
        SkRect r;
        r.fLeft = SkFloatToScalar(129.892181);
        r.fTop = SkFloatToScalar(10.3999996);
        r.fRight = SkFloatToScalar(130.892181); 
        r.fBottom = SkFloatToScalar(20.3999996);
        clip.setRect(r, true);
    }
}

static void TestAAClip(skiatest::Reporter* reporter) {
    test_empty(reporter);
    test_path_bounds(reporter);
    test_irect(reporter);
    test_rgn(reporter);
    test_path_with_hole(reporter);
    test_regressions(reporter);
}

#include "TestClassDef.h"
DEFINE_TESTCLASS("AAClip", AAClipTestClass, TestAAClip)
