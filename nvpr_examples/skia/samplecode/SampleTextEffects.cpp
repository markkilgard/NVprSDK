
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
#include "SkColorPriv.h"
#include "SkColorFilter.h"
#include "SkTypeface.h"
#include "SkAvoidXfermode.h"

static inline SkPMColor rgb2gray(SkPMColor c) {
    unsigned r = SkGetPackedR32(c);
    unsigned g = SkGetPackedG32(c);
    unsigned b = SkGetPackedB32(c);

    unsigned x = (r * 5 + g * 7 + b * 4) >> 4;

    return SkPackARGB32(0, x, x, x) | (c & (SK_A32_MASK << SK_A32_SHIFT));
}

class SkGrayScaleColorFilter : public SkColorFilter {
public:
    virtual void filterSpan(const SkPMColor src[], int count,
                            SkPMColor result[]) {
        for (int i = 0; i < count; i++) {
            result[i] = rgb2gray(src[i]);
        }
    }
};

class SkChannelMaskColorFilter : public SkColorFilter {
public:
    SkChannelMaskColorFilter(U8CPU redMask, U8CPU greenMask, U8CPU blueMask) {
        fMask = SkPackARGB32(0xFF, redMask, greenMask, blueMask);
    }

    virtual void filterSpan(const SkPMColor src[], int count,
                            SkPMColor result[]) {
        SkPMColor mask = fMask;
        for (int i = 0; i < count; i++) {
            result[i] = src[i] & mask;
        }
    }

private:
    SkPMColor   fMask;
};

///////////////////////////////////////////////////////////

#include "SkGradientShader.h"
#include "SkLayerRasterizer.h"
#include "SkBlurMaskFilter.h"

static void r0(SkLayerRasterizer* rast, SkPaint& p) {
    p.setMaskFilter(SkBlurMaskFilter::Create(SkIntToScalar(3),
                                             SkBlurMaskFilter::kNormal_BlurStyle))->unref();
    rast->addLayer(p, SkIntToScalar(3), SkIntToScalar(3));

    p.setMaskFilter(NULL);
    p.setStyle(SkPaint::kStroke_Style);
    p.setStrokeWidth(SK_Scalar1);
    rast->addLayer(p);

    p.setAlpha(0x11);
    p.setStyle(SkPaint::kFill_Style);
    p.setXfermodeMode(SkXfermode::kSrc_Mode);
    rast->addLayer(p);
}

static void r1(SkLayerRasterizer* rast, SkPaint& p) {
    rast->addLayer(p);

    p.setAlpha(0x40);
    p.setXfermodeMode(SkXfermode::kSrc_Mode);
    p.setStyle(SkPaint::kStroke_Style);
    p.setStrokeWidth(SK_Scalar1*2);
    rast->addLayer(p);
}

static void r2(SkLayerRasterizer* rast, SkPaint& p) {
    p.setStyle(SkPaint::kStrokeAndFill_Style);
    p.setStrokeWidth(SK_Scalar1*4);
    rast->addLayer(p);

    p.setStyle(SkPaint::kStroke_Style);
    p.setStrokeWidth(SK_Scalar1*3/2);
    p.setXfermodeMode(SkXfermode::kClear_Mode);
    rast->addLayer(p);
}

static void r3(SkLayerRasterizer* rast, SkPaint& p) {
    p.setStyle(SkPaint::kStroke_Style);
    p.setStrokeWidth(SK_Scalar1*3);
    rast->addLayer(p);

    p.setAlpha(0x20);
    p.setStyle(SkPaint::kFill_Style);
    p.setXfermodeMode(SkXfermode::kSrc_Mode);
    rast->addLayer(p);
}

static void r4(SkLayerRasterizer* rast, SkPaint& p) {
    p.setAlpha(0x60);
    rast->addLayer(p, SkIntToScalar(3), SkIntToScalar(3));

    p.setAlpha(0xFF);
    p.setXfermodeMode(SkXfermode::kClear_Mode);
    rast->addLayer(p, SK_Scalar1*3/2, SK_Scalar1*3/2);

    p.setXfermode(NULL);
    rast->addLayer(p);
}

#include "SkDiscretePathEffect.h"

static void r5(SkLayerRasterizer* rast, SkPaint& p) {
    rast->addLayer(p);

    p.setPathEffect(new SkDiscretePathEffect(SK_Scalar1*4, SK_Scalar1*3))->unref();
    p.setXfermodeMode(SkXfermode::kSrcOut_Mode);
    rast->addLayer(p);
}

static void r6(SkLayerRasterizer* rast, SkPaint& p) {
    rast->addLayer(p);

    p.setAntiAlias(false);
    SkLayerRasterizer* rast2 = new SkLayerRasterizer;
    r5(rast2, p);
    p.setRasterizer(rast2)->unref();
    p.setXfermodeMode(SkXfermode::kClear_Mode);
    rast->addLayer(p);
}

#include "Sk2DPathEffect.h"

static SkPathEffect* MakeDotEffect(SkScalar radius, const SkMatrix& matrix) {
    SkPath path;
    path.addCircle(0, 0, radius);
    return new SkPath2DPathEffect(matrix, path);
}

static void r7(SkLayerRasterizer* rast, SkPaint& p) {
    SkMatrix    lattice;
    lattice.setScale(SK_Scalar1*6, SK_Scalar1*6, 0, 0);
    lattice.postSkew(SK_Scalar1/3, 0, 0, 0);
    p.setPathEffect(MakeDotEffect(SK_Scalar1*4, lattice))->unref();
    rast->addLayer(p);
}

static void r8(SkLayerRasterizer* rast, SkPaint& p) {
    rast->addLayer(p);

    SkMatrix    lattice;
    lattice.setScale(SK_Scalar1*6, SK_Scalar1*6, 0, 0);
    lattice.postSkew(SK_Scalar1/3, 0, 0, 0);
    p.setPathEffect(MakeDotEffect(SK_Scalar1*2, lattice))->unref();
    p.setXfermodeMode(SkXfermode::kClear_Mode);
    rast->addLayer(p);

    p.setPathEffect(NULL);
    p.setXfermode(NULL);
    p.setStyle(SkPaint::kStroke_Style);
    p.setStrokeWidth(SK_Scalar1);
    rast->addLayer(p);
}

class Line2DPathEffect : public Sk2DPathEffect {
public:
    Line2DPathEffect(SkScalar width, const SkMatrix& matrix)
        : Sk2DPathEffect(matrix), fWidth(width) {}

	virtual bool filterPath(SkPath* dst, const SkPath& src, SkScalar* width) {
        if (this->INHERITED::filterPath(dst, src, width)) {
            *width = fWidth;
            return true;
        }
        return false;
    }

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(Line2DPathEffect)

protected:
	virtual void nextSpan(int u, int v, int ucount, SkPath* dst) {
        if (ucount > 1) {
            SkPoint	src[2], dstP[2];

            src[0].set(SkIntToScalar(u) + SK_ScalarHalf,
                       SkIntToScalar(v) + SK_ScalarHalf);
            src[1].set(SkIntToScalar(u+ucount) + SK_ScalarHalf,
                       SkIntToScalar(v) + SK_ScalarHalf);
            this->getMatrix().mapPoints(dstP, src, 2);

            dst->moveTo(dstP[0]);
            dst->lineTo(dstP[1]);
        }
    }

    Line2DPathEffect(SkFlattenableReadBuffer& buffer) : INHERITED(buffer) {
        fWidth = buffer.readScalar();
    }
    virtual void flatten(SkFlattenableWriteBuffer& buffer) const SK_OVERRIDE {
        this->INHERITED::flatten(buffer);
        buffer.writeScalar(fWidth);
    }

private:
    SkScalar fWidth;

    typedef Sk2DPathEffect INHERITED;
};

static void r9(SkLayerRasterizer* rast, SkPaint& p) {
    rast->addLayer(p);

    SkMatrix    lattice;
    lattice.setScale(SK_Scalar1, SK_Scalar1*6, 0, 0);
    lattice.postRotate(SkIntToScalar(30), 0, 0);
    p.setPathEffect(new Line2DPathEffect(SK_Scalar1*2, lattice))->unref();
    p.setXfermodeMode(SkXfermode::kClear_Mode);
    rast->addLayer(p);

    p.setPathEffect(NULL);
    p.setXfermode(NULL);
    p.setStyle(SkPaint::kStroke_Style);
    p.setStrokeWidth(SK_Scalar1);
    rast->addLayer(p);
}

typedef void (*raster_proc)(SkLayerRasterizer*, SkPaint&);

static const raster_proc gRastProcs[] = {
    r0, r1, r2, r3, r4, r5, r6, r7, r8, r9
};

static const struct {
    SkColor fMul, fAdd;
} gLightingColors[] = {
    { 0x808080, 0x800000 }, // general case
    { 0x707070, 0x707070 }, // no-pin case
    { 0xFFFFFF, 0x800000 }, // just-add case
    { 0x808080, 0x000000 }, // just-mul case
    { 0xFFFFFF, 0x000000 }  // identity case
};

#include "SkXfermode.h"

static void apply_shader(SkPaint* paint, int index) {
    raster_proc proc = gRastProcs[index];
    if (proc)
    {
        SkPaint p;
        SkLayerRasterizer*  rast = new SkLayerRasterizer;

        p.setAntiAlias(true);
        proc(rast, p);
        paint->setRasterizer(rast)->unref();
    }

#if 0
    SkScalar dir[] = { SK_Scalar1, SK_Scalar1, SK_Scalar1 };
    paint->setMaskFilter(SkBlurMaskFilter::CreateEmboss(dir, SK_Scalar1/4, SkIntToScalar(4), SkIntToScalar(3)))->unref();
#endif
    paint->setColor(SK_ColorBLUE);
}

static int gRastIndex;

class TextEffectView : public SampleView {
    SkTypeface* fFace;
public:
	TextEffectView() {
        fFace = SkTypeface::CreateFromFile("/Users/reed/Downloads/p052024l.pfb");
    }

    virtual ~TextEffectView() {
        SkSafeUnref(fFace);
    }

protected:
    // overrides from SkEventSink
    virtual bool onQuery(SkEvent* evt) {
        if (SampleCode::TitleQ(*evt)) {
            SampleCode::TitleR(evt, "Text Effects");
            return true;
        }
        return this->INHERITED::onQuery(evt);
    }

    virtual void onDrawContent(SkCanvas* canvas) {
        canvas->save();
//        canvas->scale(SK_Scalar1*2, SK_Scalar1*2, 0, 0);

        SkPaint     paint;

        paint.setAntiAlias(true);
        paint.setTextSize(SkIntToScalar(56));
        paint.setTypeface(SkTypeface::CreateFromName("sans-serif",
                                                     SkTypeface::kBold));

        SkScalar    x = SkIntToScalar(20);
        SkScalar    y = paint.getTextSize();

        SkString str("TextEffects");

        paint.setTypeface(fFace);

        for (size_t i = 0; i < SK_ARRAY_COUNT(gRastProcs); i++) {
            apply_shader(&paint, i);

          //  paint.setMaskFilter(NULL);
          //  paint.setColor(SK_ColorBLACK);

#if 1
            int index = i % SK_ARRAY_COUNT(gLightingColors);
            paint.setColorFilter(SkColorFilter::CreateLightingFilter(
                                    gLightingColors[index].fMul,
                                    gLightingColors[index].fAdd))->unref();
#endif

            canvas->drawText(str.c_str(), str.size(), x, y, paint);

            y += paint.getFontSpacing();
        }

        canvas->restore();
    }

    virtual SkView::Click* onFindClickHandler(SkScalar x, SkScalar y) {
        gRastIndex = (gRastIndex + 1) % SK_ARRAY_COUNT(gRastProcs);
        this->inval(NULL);

        return this->INHERITED::onFindClickHandler(x, y);
    }

    virtual bool onClick(Click* click) {
        return this->INHERITED::onClick(click);
    }

private:
    typedef SampleView INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

static SkView* MyFactory() { return new TextEffectView; }
static SkViewRegister reg(MyFactory);

