
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "gm.h"
#include "SkColorPriv.h"
#include "SkShader.h"
#include "SkCanvas.h"
#include "SkUtils.h"

namespace skiagm {

static SkBitmap make_bitmap() {
    SkBitmap bm;

    SkColorTable* ctable = new SkColorTable(1);
    SkPMColor* c = ctable->lockColors();
    c[0] = SkPackARGB32(0x80, 0x80, 0, 0);
    ctable->unlockColors(true);

    bm.setConfig(SkBitmap::kIndex8_Config, 1, 1);
    bm.allocPixels(ctable);
    ctable->unref();

    bm.lockPixels();
    *bm.getAddr8(0, 0) = 0;
    bm.unlockPixels();
    return bm;
}

class TinyBitmapGM : public GM {
    SkBitmap    fBM;
public:
    TinyBitmapGM() {
        this->setBGColor(0xFFDDDDDD);
        fBM = make_bitmap();
    }
    
protected:
    SkString onShortName() {
        return SkString("tinybitmap");
    }

    virtual SkISize onISize() { return make_isize(100, 100); }

    virtual void onDraw(SkCanvas* canvas) {
        SkShader* s = 
            SkShader::CreateBitmapShader(fBM, SkShader::kRepeat_TileMode,
                                         SkShader::kMirror_TileMode);
        SkPaint paint;
        paint.setAlpha(0x80);
        paint.setShader(s)->unref();
        canvas->drawPaint(paint);
    }
    
private:
    typedef GM INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

static GM* MyFactory(void*) { return new TinyBitmapGM; }
static GMRegistry reg(MyFactory);

}
