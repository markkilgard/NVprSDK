
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "gm.h"

namespace skiagm {

static const char* gConfigNames[] = {
    "unknown config",
    "A1",
    "A8",
    "Index8",
    "565",
    "4444",
    "8888"
};

SkBitmap::Config gConfigs[] = {
  SkBitmap::kRGB_565_Config,
  SkBitmap::kARGB_4444_Config,
  SkBitmap::kARGB_8888_Config,
};

#define NUM_CONFIGS (sizeof(gConfigs) / sizeof(SkBitmap::Config))

static void draw_checks(SkCanvas* canvas, int width, int height) {
    SkPaint paint;
    paint.setColor(SK_ColorRED);
    canvas->drawRectCoords(0, 0, width / 2, height / 2, paint);
    paint.setColor(SK_ColorGREEN);
    canvas->drawRectCoords(width / 2, 0, width, height / 2, paint);
    paint.setColor(SK_ColorBLUE);
    canvas->drawRectCoords(0, height / 2, width / 2, height, paint);
    paint.setColor(SK_ColorYELLOW);
    canvas->drawRectCoords(width / 2, height / 2, width, height, paint);
}

class BitmapCopyGM : public GM {
public:
    SkBitmap    fDst[NUM_CONFIGS];

    BitmapCopyGM() {
        this->setBGColor(0xFFDDDDDD);
    }

protected:
    virtual SkString onShortName() {
        return SkString("bitmapcopy");
    }

    virtual SkISize onISize() {
        return make_isize(540, 330);
    }

    virtual void onDraw(SkCanvas* canvas) {
        SkPaint paint;
        SkScalar horizMargin(SkIntToScalar(10));
        SkScalar vertMargin(SkIntToScalar(10));

        draw_checks(canvas, 40, 40);
        SkBitmap src = canvas->getDevice()->accessBitmap(false);

        for (unsigned i = 0; i < NUM_CONFIGS; ++i) {
            if (!src.deepCopyTo(&fDst[i], gConfigs[i])) {
                src.copyTo(&fDst[i], gConfigs[i]);
            }
        }

        canvas->clear(0xFFDDDDDD);
        paint.setAntiAlias(true);
        SkScalar width = SkIntToScalar(40);
        SkScalar height = SkIntToScalar(40);
        if (paint.getFontSpacing() > height) {
            height = paint.getFontSpacing();
        }
        for (unsigned i = 0; i < NUM_CONFIGS; i++) {
            const char* name = gConfigNames[src.config()];
            SkScalar textWidth = paint.measureText(name, strlen(name));
            if (textWidth > width) {
                width = textWidth;
            }
        }
        SkScalar horizOffset = width + horizMargin;
        SkScalar vertOffset = height + vertMargin;
        canvas->translate(SkIntToScalar(20), SkIntToScalar(20));

        for (unsigned i = 0; i < NUM_CONFIGS; i++) {
            canvas->save();
            // Draw destination config name
            const char* name = gConfigNames[fDst[i].config()];
            SkScalar textWidth = paint.measureText(name, strlen(name));
            SkScalar x = (width - textWidth) / SkScalar(2);
            SkScalar y = paint.getFontSpacing() / SkScalar(2);
            canvas->drawText(name, strlen(name), x, y, paint);

            // Draw destination bitmap
            canvas->translate(0, vertOffset);
            x = (width - 40) / SkScalar(2);
            canvas->drawBitmap(fDst[i], x, 0, &paint);
            canvas->restore();

            canvas->translate(horizOffset, 0);
        }
    }

    virtual uint32_t onGetFlags() const { return kSkipPicture_Flag; }

private:
    typedef GM INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

static GM* MyFactory(void*) { return new BitmapCopyGM; }
static GMRegistry reg(MyFactory);

}
