
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "SampleCode.h"
#include "SkView.h"
#include "SkCanvas.h"
#include "SkGradientShader.h"
#include "SkPath.h"
#include "SkRegion.h"
#include "SkShader.h"
#include "SkUtils.h"
#include "Sk1DPathEffect.h"
#include "SkCornerPathEffect.h"
#include "SkPathMeasure.h"
#include "SkRandom.h"
#include "SkColorPriv.h"
#include "SkPixelXorXfermode.h"

#define CORNER_RADIUS   12
static SkScalar gPhase;

static const int gXY[] = {
    4, 0, 0, -4, 8, -4, 12, 0, 8, 4, 0, 4
};

static SkPathEffect* make_pe(int flags) {
    if (flags == 1)
        return new SkCornerPathEffect(SkIntToScalar(CORNER_RADIUS));

    SkPath  path;
    path.moveTo(SkIntToScalar(gXY[0]), SkIntToScalar(gXY[1]));
    for (unsigned i = 2; i < SK_ARRAY_COUNT(gXY); i += 2)
        path.lineTo(SkIntToScalar(gXY[i]), SkIntToScalar(gXY[i+1]));
    path.close();
    path.offset(SkIntToScalar(-6), 0);

    SkPathEffect* outer = new SkPath1DPathEffect(path, SkIntToScalar(12), gPhase, SkPath1DPathEffect::kRotate_Style);
    
    if (flags == 2)
        return outer;

    SkPathEffect* inner = new SkCornerPathEffect(SkIntToScalar(CORNER_RADIUS));

    SkPathEffect* pe = new SkComposePathEffect(outer, inner);
    outer->unref();
    inner->unref();
    return pe;
}

static SkPathEffect* make_warp_pe() {
    SkPath  path;
    path.moveTo(SkIntToScalar(gXY[0]), SkIntToScalar(gXY[1]));
    for (unsigned i = 2; i < SK_ARRAY_COUNT(gXY); i += 2)
        path.lineTo(SkIntToScalar(gXY[i]), SkIntToScalar(gXY[i+1]));
    path.close();
    path.offset(SkIntToScalar(-6), 0);

    SkPathEffect* outer = new SkPath1DPathEffect(path, SkIntToScalar(12), gPhase, SkPath1DPathEffect::kMorph_Style);
    SkPathEffect* inner = new SkCornerPathEffect(SkIntToScalar(CORNER_RADIUS));

    SkPathEffect* pe = new SkComposePathEffect(outer, inner);
    outer->unref();
    inner->unref();
    return pe;
}

///////////////////////////////////////////////////////////

#include "SkColorFilter.h"
#include "SkLayerRasterizer.h"

class testrast : public SkLayerRasterizer {
public:
    testrast() {
        SkPaint paint;
        paint.setAntiAlias(true);

#if 0        
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(SK_Scalar1*4);
        this->addLayer(paint);
    
        paint.setStrokeWidth(SK_Scalar1*1);
        paint.setXfermode(SkXfermode::kClear_Mode);
        this->addLayer(paint);
#else
        paint.setAlpha(0x66);
        this->addLayer(paint, SkIntToScalar(4), SkIntToScalar(4));
    
        paint.setAlpha(0xFF);
        this->addLayer(paint);
#endif
    }
};

class PathEffectView : public SampleView {
    SkPath  fPath;
    SkPoint fClickPt;
public:
	PathEffectView() {
        SkRandom    rand;
        int         steps = 20;
        SkScalar    dist = SkIntToScalar(400);
        SkScalar    x = SkIntToScalar(20);
        SkScalar    y = SkIntToScalar(50);
        
        fPath.moveTo(x, y);
        for (int i = 0; i < steps; i++) {
            x += dist/steps;
            SkScalar tmpY = y + SkIntToScalar(rand.nextS() % 25);
            if (i == steps/2) {
                fPath.moveTo(x, tmpY);
            } else {
                fPath.lineTo(x, tmpY);
            }
        }

        {
            SkRect  oval;
            oval.set(SkIntToScalar(20), SkIntToScalar(30),
                     SkIntToScalar(100), SkIntToScalar(60));
            oval.offset(x, 0);
            fPath.addRoundRect(oval, SkIntToScalar(8), SkIntToScalar(8));
        }
        
        fClickPt.set(SkIntToScalar(200), SkIntToScalar(200));
        
        this->setBGColor(0xFFDDDDDD);
    }
	
protected:
    // overrides from SkEventSink
    virtual bool onQuery(SkEvent* evt) {
        if (SampleCode::TitleQ(*evt)) {
            SampleCode::TitleR(evt, "PathEffects");
            return true;
        }
        return this->INHERITED::onQuery(evt);
    }
    
    virtual void onDrawContent(SkCanvas* canvas) {
        gPhase -= SampleCode::GetAnimSecondsDelta() * 40;
        this->inval(NULL);
        
        SkPaint paint;
        
#if 0
        paint.setAntiAlias(true);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(SkIntToScalar(5));
        canvas->drawPath(fPath, paint);
        paint.setStrokeWidth(0);
        
        paint.setColor(SK_ColorWHITE);
        paint.setPathEffect(make_pe(1))->unref();
        canvas->drawPath(fPath, paint);
#endif
        
        canvas->translate(0, SkIntToScalar(50));
        
        paint.setColor(SK_ColorBLUE);
        paint.setPathEffect(make_pe(2))->unref();
        canvas->drawPath(fPath, paint);
        
        canvas->translate(0, SkIntToScalar(50));
        
        paint.setARGB(0xFF, 0, 0xBB, 0);
        paint.setPathEffect(make_pe(3))->unref();
        canvas->drawPath(fPath, paint);
        
        canvas->translate(0, SkIntToScalar(50));

        paint.setARGB(0xFF, 0, 0, 0);
        paint.setPathEffect(make_warp_pe())->unref();
        paint.setRasterizer(new testrast)->unref();
        canvas->drawPath(fPath, paint);
    }
    
private:
    typedef SampleView INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

static SkView* MyFactory() { return new PathEffectView; }
static SkViewRegister reg(MyFactory);

