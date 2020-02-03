
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "SampleCode.h"
#include "SkView.h"
#include "SkCanvas.h"
#include "Sk64.h"
#include "SkCornerPathEffect.h"
#include "SkGradientShader.h"
#include "SkGraphics.h"
#include "SkImageDecoder.h"
#include "SkKernel33MaskFilter.h"
#include "SkPath.h"
#include "SkRandom.h"
#include "SkRegion.h"
#include "SkShader.h"
#include "SkUtils.h"
#include "SkColorPriv.h"
#include "SkColorFilter.h"
#include "SkTime.h"
#include "SkTypeface.h"
#include "SkXfermode.h"

#include "SkStream.h"
#include "SkXMLParser.h"
#include "SkColorPriv.h"
#include "SkImageDecoder.h"

static void setNamedTypeface(SkPaint* paint, const char name[]) {
    SkTypeface* face = SkTypeface::CreateFromName(name, SkTypeface::kNormal);
    paint->setTypeface(face);
    SkSafeUnref(face);
}

#if 0
static int newscale(U8CPU a, U8CPU b, int shift) {
    unsigned prod = a * b + (1 << (shift - 1));
    return (prod + (prod >> shift)) >> shift;
}

static void test_srcover565(SkCanvas* canvas) {
    const int width = 32;
    SkBitmap bm1, bm2, bm3;
    bm1.setConfig(SkBitmap::kRGB_565_Config, width, 256); bm1.allocPixels(NULL);
    bm2.setConfig(SkBitmap::kRGB_565_Config, width, 256); bm2.allocPixels(NULL);
    bm3.setConfig(SkBitmap::kRGB_565_Config, width, 256); bm3.allocPixels(NULL);

    int rgb = 0x18;
    int r = rgb >> 3;
    int g = rgb >> 2;
    uint16_t dst = SkPackRGB16(r, g, r);
    for (int alpha = 0; alpha <= 255; alpha++) {
        SkPMColor pm = SkPreMultiplyARGB(alpha, rgb, rgb, rgb);
        uint16_t newdst = SkSrcOver32To16(pm, dst);
        sk_memset16(bm1.getAddr16(0, alpha), newdst, bm1.width());

        int ia = 255 - alpha;
        int iscale = SkAlpha255To256(ia);
        int dr = (SkGetPackedR32(pm) + (r * iscale >> 5)) >> 3;
        int dg = (SkGetPackedG32(pm) + (g * iscale >> 6)) >> 2;

        sk_memset16(bm2.getAddr16(0, alpha), SkPackRGB16(dr, dg, dr), bm2.width());

        int dr2 = (SkMulDiv255Round(alpha, rgb) + newscale(r, ia, 5)) >> 3;
        int dg2 = (SkMulDiv255Round(alpha, rgb) + newscale(g, ia, 6)) >> 2;

        sk_memset16(bm3.getAddr16(0, alpha), SkPackRGB16(dr2, dg2, dr2), bm3.width());

//        if (mr != dr || mg != dg)
        {
//            SkDebugf("[%d] macro [%d %d] inline [%d %d] new [%d %d]\n", alpha, mr, mg, dr, dg, dr2, dg2);
        }
    }

    SkScalar dx = SkIntToScalar(width+4);

    canvas->drawBitmap(bm1, 0, 0, NULL); canvas->translate(dx, 0);
    canvas->drawBitmap(bm2, 0, 0, NULL); canvas->translate(dx, 0);
    canvas->drawBitmap(bm3, 0, 0, NULL); canvas->translate(dx, 0);

    SkRect rect = { 0, 0, SkIntToScalar(bm1.width()), SkIntToScalar(bm1.height()) };
    SkPaint p;
    p.setARGB(0xFF, rgb, rgb, rgb);
    canvas->drawRect(rect, p);
}
#endif

static void make_bitmaps(int w, int h, SkBitmap* src, SkBitmap* dst) {
    src->setConfig(SkBitmap::kARGB_8888_Config, w, h);
    src->allocPixels();
    src->eraseColor(0);

    SkCanvas c(*src);
    SkPaint p;
    SkRect r;
    SkScalar ww = SkIntToScalar(w);
    SkScalar hh = SkIntToScalar(h);

    p.setAntiAlias(true);
    p.setColor(0xFFFFCC44);
    r.set(0, 0, ww*3/4, hh*3/4);
    c.drawOval(r, p);

    dst->setConfig(SkBitmap::kARGB_8888_Config, w, h);
    dst->allocPixels();
    dst->eraseColor(0);
    c.setBitmapDevice(*dst);

    p.setColor(0xFF66AAFF);
    r.set(ww/3, hh/3, ww*19/20, hh*19/20);
    c.drawRect(r, p);
}

static uint16_t gBG[] = { 0xFFFF, 0xCCCF, 0xCCCF, 0xFFFF };

class XfermodesView : public SampleView {
    SkBitmap    fBG;
    SkBitmap    fSrcB, fDstB;

    void draw_mode(SkCanvas* canvas, SkXfermode* mode, int alpha,
                   SkScalar x, SkScalar y) {
        SkPaint p;

        canvas->drawBitmap(fSrcB, x, y, &p);
        p.setAlpha(alpha);
        p.setXfermode(mode);
        canvas->drawBitmap(fDstB, x, y, &p);
    }

public:
    const static int W = 64;
    const static int H = 64;
    bool fOnce;

	XfermodesView() {
        fOnce = false;
    }
    
    void init() {
        if (fOnce) {
            return;
        }
        fOnce = true;

        const int W = 64;
        const int H = 64;

        fBG.setConfig(SkBitmap::kARGB_4444_Config, 2, 2, 4);
        fBG.setPixels(gBG);
        fBG.setIsOpaque(true);

        make_bitmaps(W, H, &fSrcB, &fDstB);
    }

protected:
    // overrides from SkEventSink
    virtual bool onQuery(SkEvent* evt) {
        if (SampleCode::TitleQ(*evt)) {
            SampleCode::TitleR(evt, "Xfermodes");
            return true;
        }
        return this->INHERITED::onQuery(evt);
    }

    virtual void onDrawContent(SkCanvas* canvas) {
        this->init();

        canvas->translate(SkIntToScalar(10), SkIntToScalar(20));

        const struct {
            SkXfermode::Mode  fMode;
            const char*         fLabel;
        } gModes[] = {
            { SkXfermode::kClear_Mode,    "Clear"     },
            { SkXfermode::kSrc_Mode,      "Src"       },
            { SkXfermode::kDst_Mode,      "Dst"       },
            { SkXfermode::kSrcOver_Mode,  "SrcOver"   },
            { SkXfermode::kDstOver_Mode,  "DstOver"   },
            { SkXfermode::kSrcIn_Mode,    "SrcIn"     },
            { SkXfermode::kDstIn_Mode,    "DstIn"     },
            { SkXfermode::kSrcOut_Mode,   "SrcOut"    },
            { SkXfermode::kDstOut_Mode,   "DstOut"    },
            { SkXfermode::kSrcATop_Mode,  "SrcATop"   },
            { SkXfermode::kDstATop_Mode,  "DstATop"   },
            { SkXfermode::kXor_Mode,      "Xor"       },

            { SkXfermode::kPlus_Mode,         "Plus"          },
            { SkXfermode::kMultiply_Mode,     "Multiply"      },
            { SkXfermode::kScreen_Mode,       "Screen"        },
            { SkXfermode::kOverlay_Mode,      "Overlay"       },
            { SkXfermode::kDarken_Mode,       "Darken"        },
            { SkXfermode::kLighten_Mode,      "Lighten"       },
            { SkXfermode::kColorDodge_Mode,   "ColorDodge"    },
            { SkXfermode::kColorBurn_Mode,    "ColorBurn"     },
            { SkXfermode::kHardLight_Mode,    "HardLight"     },
            { SkXfermode::kSoftLight_Mode,    "SoftLight"     },
            { SkXfermode::kDifference_Mode,   "Difference"    },
            { SkXfermode::kExclusion_Mode,    "Exclusion"     },
        };

        const SkScalar w = SkIntToScalar(W);
        const SkScalar h = SkIntToScalar(H);
        SkShader* s = SkShader::CreateBitmapShader(fBG,
                                                   SkShader::kRepeat_TileMode,
                                                   SkShader::kRepeat_TileMode);
        SkMatrix m;
        m.setScale(SkIntToScalar(6), SkIntToScalar(6));
        s->setLocalMatrix(m);

        SkPaint labelP;
        labelP.setAntiAlias(true);
        labelP.setLCDRenderText(true);
        labelP.setTextAlign(SkPaint::kCenter_Align);
        setNamedTypeface(&labelP, "Menlo Regular");
//        labelP.setTextSize(SkIntToScalar(11));

        const int W = 5;

        SkScalar x0 = 0;
        for (int twice = 0; twice < 2; twice++) {
            SkScalar x = x0, y = 0;
            for (size_t i = 0; i < SK_ARRAY_COUNT(gModes); i++) {
                SkXfermode* mode = SkXfermode::Create(gModes[i].fMode);
                SkAutoUnref aur(mode);
                SkRect r;
                r.set(x, y, x+w, y+h);

                SkPaint p;
                p.setStyle(SkPaint::kFill_Style);
                p.setShader(s);
                canvas->drawRect(r, p);

                canvas->saveLayer(&r, NULL, SkCanvas::kARGB_ClipLayer_SaveFlag);
         //       canvas->save();
                draw_mode(canvas, mode, twice ? 0x88 : 0xFF, r.fLeft, r.fTop);
                canvas->restore();

                r.inset(-SK_ScalarHalf, -SK_ScalarHalf);
                p.setStyle(SkPaint::kStroke_Style);
                p.setShader(NULL);
                canvas->drawRect(r, p);

#if 1
                canvas->drawText(gModes[i].fLabel, strlen(gModes[i].fLabel),
                                 x + w/2, y - labelP.getTextSize()/2, labelP);
#endif
                x += w + SkIntToScalar(10);
                if ((i % W) == W - 1) {
                    x = x0;
                    y += h + SkIntToScalar(30);
                }
            }
            x0 += SkIntToScalar(400);
        }
        s->unref();
    }

private:
    typedef SampleView INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

static SkView* MyFactory() { return new XfermodesView; }
static SkViewRegister reg(MyFactory);

