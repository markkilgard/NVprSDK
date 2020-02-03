
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "SampleCode.h"
#include "SkCanvas.h"
#include "SkPaint.h"
#include "SkGpuDevice.h"

static void make_bitmap(SkBitmap* bitmap, GrContext* ctx, SkIRect* center) {
    SkDevice* dev;
    SkCanvas canvas;
    
    const int kFixed = 28;
    const int kStretchy = 8;
    const int kSize = 2*kFixed + kStretchy;

    if (ctx) {
        dev = new SkGpuDevice(ctx, SkBitmap::kARGB_8888_Config, kSize, kSize);
        *bitmap = dev->accessBitmap(false);
    } else {
        bitmap->setConfig(SkBitmap::kARGB_8888_Config, kSize, kSize);
        bitmap->allocPixels();
        dev = new SkDevice(*bitmap);
    }

    canvas.setDevice(dev)->unref();
    canvas.clear(0);

    SkRect r = SkRect::MakeWH(SkIntToScalar(kSize), SkIntToScalar(kSize));
    const SkScalar strokeWidth = SkIntToScalar(6);
    const SkScalar radius = SkIntToScalar(kFixed) - strokeWidth/2;

    center->setXYWH(kFixed, kFixed, kStretchy, kStretchy);

    SkPaint paint;
    paint.setAntiAlias(true);

    paint.setColor(0xFFFF0000);
    canvas.drawRoundRect(r, radius, radius, paint);
    r.setXYWH(SkIntToScalar(kFixed), 0, SkIntToScalar(kStretchy), SkIntToScalar(kSize));
    paint.setColor(0x8800FF00);
    canvas.drawRect(r, paint);
    r.setXYWH(0, SkIntToScalar(kFixed), SkIntToScalar(kSize), SkIntToScalar(kStretchy));
    paint.setColor(0x880000FF);
    canvas.drawRect(r, paint);
}


class NinePatchView : public SampleView {
public:
	NinePatchView() {}

protected:
    // overrides from SkEventSink
    virtual bool onQuery(SkEvent* evt) {
        if (SampleCode::TitleQ(*evt)) {
            SampleCode::TitleR(evt, "NinePatch");
            return true;
        }
        return this->INHERITED::onQuery(evt);
    }

    virtual void onDrawContent(SkCanvas* canvas) {
        SkBitmap bm;
        SkIRect center;
        make_bitmap(&bm, SampleCode::GetGr(), &center);

        // amount of bm that should not be stretched (unless we have to)
        const SkScalar fixed = SkIntToScalar(bm.width() - center.width());

        const SkTSize<SkScalar> size[] = {
            { fixed * 4 / 5, fixed * 4 / 5 },   // shrink in both axes
            { fixed * 4 / 5, fixed * 4 },       // shrink in X
            { fixed * 4,     fixed * 4 / 5 },   // shrink in Y
            { fixed * 4,     fixed * 4 }
        };

        canvas->drawBitmap(bm, SkIntToScalar(10), SkIntToScalar(10), NULL);

        SkScalar x = SkIntToScalar(100);
        SkScalar y = SkIntToScalar(100);

        SkPaint paint;
        paint.setFilterBitmap(true);

        for (int iy = 0; iy < 2; ++iy) {
            for (int ix = 0; ix < 2; ++ix) {
                int i = ix * 2 + iy;
                SkRect r = SkRect::MakeXYWH(x + ix * fixed, y + iy * fixed,
                                            size[i].width(), size[i].height());
                canvas->drawBitmapNine(bm, center, r, &paint);
            }
        }
    }
    
private:
    SkScalar fX, fY;
    typedef SampleView INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

static SkView* MyFactory() { return new NinePatchView; }
static SkViewRegister reg(MyFactory);

