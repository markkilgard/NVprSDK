
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "SampleCode.h"
#include "SkView.h"
#include "SkCanvas.h"
#include "SkDevice.h"
#include "SkPaint.h"
#include "SkShader.h"

static SkBitmap createBitmap(int n) {
    SkBitmap bitmap;
    bitmap.setConfig(SkBitmap::kARGB_8888_Config, n, n);
    bitmap.allocPixels();
    bitmap.eraseColor(0);
    
    SkCanvas canvas(bitmap);
    SkRect r;
    r.set(0, 0, SkIntToScalar(n), SkIntToScalar(n));
    SkPaint paint;
    paint.setAntiAlias(true);
    
    paint.setColor(SK_ColorRED);
    canvas.drawOval(r, paint);
    paint.setColor(SK_ColorBLUE);
    paint.setStrokeWidth(SkIntToScalar(n)/15);
    paint.setStyle(SkPaint::kStroke_Style);
    canvas.drawLine(0, 0, r.fRight, r.fBottom, paint);
    canvas.drawLine(0, r.fBottom, r.fRight, 0, paint);
    
    return bitmap;
}

class MipMapView : public SampleView {
    SkBitmap fBitmap;
    enum {
        N = 64
    };
    bool fOnce;
public:
    MipMapView() {
        fOnce = false;
    }
    
    void init() {
        if (fOnce) {
            return;
        }
        fOnce = true;

        fBitmap = createBitmap(N);
        
        fWidth = N;
    }
    
protected:
    // overrides from SkEventSink
    virtual bool onQuery(SkEvent* evt) {
        if (SampleCode::TitleQ(*evt)) {
            SampleCode::TitleR(evt, "MipMaps");
            return true;
        }
        return this->INHERITED::onQuery(evt);
    }
    
    void drawN(SkCanvas* canvas, const SkBitmap& bitmap) {
        SkAutoCanvasRestore acr(canvas, true);
        for (int i = N; i > 1; i >>= 1) {
            canvas->drawBitmap(bitmap, 0, 0, NULL);
            canvas->translate(SkIntToScalar(N + 8), 0);
            canvas->scale(SK_ScalarHalf, SK_ScalarHalf);
        }
    }
    
    void drawN2(SkCanvas* canvas, const SkBitmap& bitmap) {
        SkBitmap bg;
        bg.setConfig(SkBitmap::kARGB_8888_Config, N, N);
        bg.allocPixels();
        
        SkAutoCanvasRestore acr(canvas, true);
        for (int i = 0; i < 6; i++) {
            bg.eraseColor(0);
            SkCanvas c(bg);
            c.scale(SK_Scalar1 / (1 << i), SK_Scalar1 / (1 << i));
            c.drawBitmap(bitmap, 0, 0, NULL);

            canvas->save();
            canvas->scale(SkIntToScalar(1 << i), SkIntToScalar(1 << i));
            canvas->drawBitmap(bg, 0, 0, NULL);
            canvas->restore();
            canvas->translate(SkIntToScalar(N + 8), 0);
        }
    }
    
    virtual void onDrawContent(SkCanvas* canvas) {
        this->init();
        canvas->translate(SkIntToScalar(10), SkIntToScalar(10));
        
        canvas->scale(1.00000001f, 0.9999999f);

        drawN2(canvas, fBitmap);

        canvas->translate(0, SkIntToScalar(N + 8));
        SkBitmap bitmap(fBitmap);
        bitmap.buildMipMap();
        drawN2(canvas, bitmap);

        SkScalar time = SampleCode::GetAnimScalar(SkIntToScalar(1)/4,
                                                  SkIntToScalar(2));
        if (time >= SK_Scalar1) {
            time = SkIntToScalar(2) - time;
        }
        fWidth = 8 + SkScalarRound(N * time);

        SkRect dst;
        dst.set(0, 0, SkIntToScalar(fWidth), SkIntToScalar(fWidth));

        SkPaint paint;
        paint.setFilterBitmap(true);
        paint.setAntiAlias(true);

        canvas->translate(0, SkIntToScalar(N + 8));
        canvas->drawBitmapRect(fBitmap, NULL, dst, NULL);
        canvas->translate(SkIntToScalar(N + 8), 0);
        canvas->drawBitmapRect(fBitmap, NULL, dst, &paint);
        canvas->translate(-SkIntToScalar(N + 8), SkIntToScalar(N + 8));
        canvas->drawBitmapRect(bitmap, NULL, dst, NULL);
        canvas->translate(SkIntToScalar(N + 8), 0);
        canvas->drawBitmapRect(bitmap, NULL, dst, &paint);
        
        SkShader* s = SkShader::CreateBitmapShader(bitmap,
                                                   SkShader::kRepeat_TileMode,
                                                   SkShader::kRepeat_TileMode);
        paint.setShader(s)->unref();
        SkMatrix m;
        m.setScale(SkIntToScalar(fWidth) / N,
                   SkIntToScalar(fWidth) / N);
        s->setLocalMatrix(m);
        SkRect r;
        r.set(0, 0, SkIntToScalar(4*N), SkIntToScalar(5*N/2));
        r.offset(SkIntToScalar(N + 12), -SkIntToScalar(N + 4));
        canvas->drawRect(r, paint);
        
        this->inval(NULL);
    }
    
private:
    int fWidth;

    typedef SampleView INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

static SkView* MyFactory() { return new MipMapView; }
static SkViewRegister reg(MyFactory);

