/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm.h"
#include "SkCanvas.h"
#include "SkColorFilter.h"
#include "SkColorPriv.h"
#include "SkShader.h"

#include "SkBlurImageFilter.h"
#include "SkTestImageFilters.h"

#define FILTER_WIDTH    SkIntToScalar(150)
#define FILTER_HEIGHT   SkIntToScalar(200)

static SkImageFilter* make0() { return new SkDownSampleImageFilter(SK_Scalar1 / 5); }
static SkImageFilter* make1() { return new SkOffsetImageFilter(SkIntToScalar(16), SkIntToScalar(16)); }
static SkImageFilter* make2() {
    SkColorFilter* cf = SkColorFilter::CreateModeFilter(SK_ColorBLUE,
                                                        SkXfermode::kSrcIn_Mode);
    SkAutoUnref aur(cf);
    return new SkColorFilterImageFilter(cf);
}
static SkImageFilter* make3() {
    return new SkBlurImageFilter(8, 0);
}

static SkImageFilter* make4() {
    SkImageFilter* outer = new SkOffsetImageFilter(SkIntToScalar(16), SkIntToScalar(16));
    SkImageFilter* inner = new SkDownSampleImageFilter(SK_Scalar1 / 5);
    SkAutoUnref aur0(outer);
    SkAutoUnref aur1(inner);
    return new SkComposeImageFilter(outer, inner);
}
static SkImageFilter* make5() {
    SkImageFilter* first = new SkOffsetImageFilter(SkIntToScalar(16), SkIntToScalar(16));
    SkImageFilter* second = new SkDownSampleImageFilter(SK_Scalar1 / 5);
    SkAutoUnref aur0(first);
    SkAutoUnref aur1(second);
    return new SkMergeImageFilter(first, second);
}

static SkImageFilter* make6() {
    SkImageFilter* outer = new SkOffsetImageFilter(SkIntToScalar(16), SkIntToScalar(16));
    SkImageFilter* inner = new SkDownSampleImageFilter(SK_Scalar1 / 5);
    SkAutoUnref aur0(outer);
    SkAutoUnref aur1(inner);
    SkImageFilter* compose = new SkComposeImageFilter(outer, inner);
    SkAutoUnref aur2(compose);
    
    SkColorFilter* cf = SkColorFilter::CreateModeFilter(0x880000FF,
                                                        SkXfermode::kSrcIn_Mode);
    SkAutoUnref aur3(cf);
    SkImageFilter* blue = new SkColorFilterImageFilter(cf);
    SkAutoUnref aur4(blue);
    
    return new SkMergeImageFilter(compose, blue);
}

static SkImageFilter* make7() {
    SkImageFilter* outer = new SkOffsetImageFilter(SkIntToScalar(16), SkIntToScalar(16));
    SkImageFilter* inner = make3();
    SkAutoUnref aur0(outer);
    SkAutoUnref aur1(inner);
    SkImageFilter* compose = new SkComposeImageFilter(outer, inner);
    SkAutoUnref aur2(compose);
    
    SkColorFilter* cf = SkColorFilter::CreateModeFilter(0x880000FF,
                                                        SkXfermode::kSrcIn_Mode);
    SkAutoUnref aur3(cf);
    SkImageFilter* blue = new SkColorFilterImageFilter(cf);
    SkAutoUnref aur4(blue);
    
    return new SkMergeImageFilter(compose, blue);
}

static void draw0(SkCanvas* canvas) {
    SkPaint p;
    p.setAntiAlias(true);
    SkRect r = SkRect::MakeWH(FILTER_WIDTH, FILTER_HEIGHT);
    r.inset(SK_Scalar1 * 12, SK_Scalar1 * 12);
    p.setColor(SK_ColorRED);
    canvas->drawOval(r, p);
}

class TestImageFiltersGM : public skiagm::GM {
public:
    TestImageFiltersGM () {}

protected:

    virtual SkString onShortName() {
        return SkString("testimagefilters");
    }

    virtual SkISize onISize() { return SkISize::Make(700, 460); }

    virtual void onDraw(SkCanvas* canvas) {
//        this->drawSizeBounds(canvas, 0xFFCCCCCC);

        static SkImageFilter* (*gFilterProc[])() = {
            make0, make1, make2, make3, make4, make5, make6, make7
        };
        
        const SkRect bounds = SkRect::MakeWH(FILTER_WIDTH, FILTER_HEIGHT);
        
        const SkScalar dx = bounds.width() * 8 / 7;
        const SkScalar dy = bounds.height() * 8 / 7;

        canvas->translate(SkIntToScalar(8), SkIntToScalar(8));

        for (size_t i = 0; i < SK_ARRAY_COUNT(gFilterProc); ++i) {
            int ix = i % 4;
            int iy = i / 4;

            SkAutoCanvasRestore acr(canvas, true);
            canvas->translate(ix * dx, iy * dy);

            SkPaint p;
            p.setStyle(SkPaint::kStroke_Style);
            canvas->drawRect(bounds, p);
            
            SkPaint paint;
            paint.setImageFilter(gFilterProc[i]())->unref();
            canvas->saveLayer(&bounds, &paint);
            draw0(canvas);
        }
    }

private:
    typedef GM INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

static skiagm::GM* MyFactory(void*) { return new TestImageFiltersGM; }
static skiagm::GMRegistry reg(MyFactory);


