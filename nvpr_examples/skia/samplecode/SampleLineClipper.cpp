
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
#include "SkGraphics.h"
#include "SkImageDecoder.h"
#include "SkPath.h"
#include "SkRegion.h"
#include "SkShader.h"
#include "SkUtils.h"
#include "SkXfermode.h"
#include "SkColorPriv.h"
#include "SkColorFilter.h"
#include "SkTime.h"
#include "SkRandom.h"

#include "SkLineClipper.h"
#include "SkEdgeClipper.h"

#define AUTO_ANIMATE    true

static int test0(SkPoint pts[], SkRect* clip) {
    pts[0].set(200000, 140);
    pts[1].set(-740000, 483);
    pts[2].set(1.00000102e-06f, 9.10000017e-05f);
    clip->set(0, 0, 640, 480);
    return 2;
}

///////////////////////////////////////////////////////////////////////////////

static void drawQuad(SkCanvas* canvas, const SkPoint pts[3], const SkPaint& p) {
    SkPath path;
    path.moveTo(pts[0]);
    path.quadTo(pts[1], pts[2]);
    canvas->drawPath(path, p);
}

static void drawCubic(SkCanvas* canvas, const SkPoint pts[4], const SkPaint& p) {
    SkPath path;
    path.moveTo(pts[0]);
    path.cubicTo(pts[1], pts[2], pts[3]);
    canvas->drawPath(path, p);
}

typedef void (*clipper_proc)(const SkPoint src[], const SkRect& clip,
                            SkCanvas*, const SkPaint&, const SkPaint&);

static void check_clipper(int count, const SkPoint pts[], const SkRect& clip) {
    for (int i = 0; i < count; i++) {
        SkASSERT(pts[i].fX >= clip.fLeft);
        SkASSERT(pts[i].fX <= clip.fRight);
        SkASSERT(pts[i].fY >= clip.fTop);
        SkASSERT(pts[i].fY <= clip.fBottom);
    }

    if (count > 1) {
        sk_assert_monotonic_y(pts, count);
    }
}

static void line_intersector(const SkPoint src[], const SkRect& clip,
                         SkCanvas* canvas, const SkPaint& p0, const SkPaint& p1) {
    canvas->drawPoints(SkCanvas::kLines_PointMode, 2, src, p1);
    
    SkPoint dst[2];
    if (SkLineClipper::IntersectLine(src, clip, dst)) {
        check_clipper(2, dst, clip);
        canvas->drawPoints(SkCanvas::kLines_PointMode, 2, dst, p0);
    }
}

static void line_clipper(const SkPoint src[], const SkRect& clip,
                         SkCanvas* canvas, const SkPaint& p0, const SkPaint& p1) {
    canvas->drawPoints(SkCanvas::kLines_PointMode, 2, src, p1);
    
    SkPoint dst[SkLineClipper::kMaxPoints];
    int count = SkLineClipper::ClipLine(src, clip, dst);
    for (int i = 0; i < count; i++) {
        check_clipper(2, &dst[i], clip);
        canvas->drawPoints(SkCanvas::kLines_PointMode, 2, &dst[i], p0);
    }
}

static void quad_clipper(const SkPoint src[], const SkRect& clip,
                         SkCanvas* canvas, const SkPaint& p0, const SkPaint& p1) {
    drawQuad(canvas, src, p1);
    
    SkEdgeClipper clipper;
    if (clipper.clipQuad(src, clip)) {
        SkPoint pts[4];
        SkPath::Verb verb;
        while ((verb = clipper.next(pts)) != SkPath::kDone_Verb) {
            switch (verb) {
                case SkPath::kLine_Verb:
                    check_clipper(2, pts, clip);
                    canvas->drawPoints(SkCanvas::kLines_PointMode, 2, pts, p0);
                    break;
                case SkPath::kQuad_Verb:
                    check_clipper(3, pts, clip);
                    drawQuad(canvas, pts, p0);
                    break;
                default:
                    SkASSERT(!"unexpected verb");
            }
        }
    }
}

static void cubic_clipper(const SkPoint src[], const SkRect& clip,
                       SkCanvas* canvas, const SkPaint& p0, const SkPaint& p1) {
    drawCubic(canvas, src, p1);
    
    SkEdgeClipper clipper;
    if (clipper.clipCubic(src, clip)) {
        SkPoint pts[4];
        SkPath::Verb verb;
        while ((verb = clipper.next(pts)) != SkPath::kDone_Verb) {
            switch (verb) {
                case SkPath::kLine_Verb:
                    check_clipper(2, pts, clip);
                    canvas->drawPoints(SkCanvas::kLines_PointMode, 2, pts, p0);
                    break;
                case SkPath::kCubic_Verb:
                 //   check_clipper(4, pts, clip);
                    drawCubic(canvas, pts, p0);
                    break;
                default:
                    SkASSERT(!"unexpected verb");
            }
        }
    }
}

static const clipper_proc gProcs[] = {
    line_intersector,
    line_clipper,
    quad_clipper,
    cubic_clipper
};

///////////////////////////////////////////////////////////////////////////////

enum {
    W = 640/3,
    H = 480/3
};

class LineClipperView : public SampleView {
    SkMSec      fNow;
    int         fCounter;
    int         fProcIndex;
    SkRect      fClip;
    SkRandom    fRand;
    SkPoint     fPts[4];

    void randPts() {
        for (size_t i = 0; i < SK_ARRAY_COUNT(fPts); i++) {
            fPts[i].set(fRand.nextUScalar1() * 640,
                        fRand.nextUScalar1() * 480);
        }
        fCounter += 1;
    }

public:
	LineClipperView() {
        fProcIndex = 0;
        fCounter = 0;
        fNow = 0;

        int x = (640 - W)/2;
        int y = (480 - H)/2;
        fClip.set(SkIntToScalar(x), SkIntToScalar(y),
                  SkIntToScalar(x + W), SkIntToScalar(y + H));
        this->randPts();
    }
    
protected:
    // overrides from SkEventSink
    virtual bool onQuery(SkEvent* evt) {
        if (SampleCode::TitleQ(*evt)) {
            SampleCode::TitleR(evt, "LineClipper");
            return true;
        }
        return this->INHERITED::onQuery(evt);
    }
    
    static void drawVLine(SkCanvas* canvas, SkScalar x, const SkPaint& paint) {
        canvas->drawLine(x, -999, x, 999, paint);
    }
    
    static void drawHLine(SkCanvas* canvas, SkScalar y, const SkPaint& paint) {
        canvas->drawLine(-999, y, 999, y, paint);
    }
    
    virtual void onDrawContent(SkCanvas* canvas) {
        SkMSec now = SampleCode::GetAnimTime();
        if (fNow != now) {
            fNow = now;
            this->randPts();
            this->inval(NULL);
        }

     //   fProcIndex = test0(fPts, &fClip);

        SkPaint paint, paint1;
        
        drawVLine(canvas, fClip.fLeft + SK_ScalarHalf, paint);
        drawVLine(canvas, fClip.fRight - SK_ScalarHalf, paint);
        drawHLine(canvas, fClip.fTop + SK_ScalarHalf, paint);
        drawHLine(canvas, fClip.fBottom - SK_ScalarHalf, paint);
        
        paint.setColor(SK_ColorLTGRAY);
        canvas->drawRect(fClip, paint);
        
        paint.setAntiAlias(true);
        paint.setColor(SK_ColorBLUE);
        paint.setStyle(SkPaint::kStroke_Style);
      //  paint.setStrokeWidth(SkIntToScalar(3));
        paint.setStrokeCap(SkPaint::kRound_Cap);
        
        paint1.setAntiAlias(true);
        paint1.setColor(SK_ColorRED);
        paint1.setStyle(SkPaint::kStroke_Style);
        gProcs[fProcIndex](fPts, fClip, canvas, paint, paint1);
        this->inval(NULL);
    }

    virtual SkView::Click* onFindClickHandler(SkScalar x, SkScalar y) {
     //   fProcIndex = (fProcIndex + 1) % SK_ARRAY_COUNT(gProcs);
        if (x < 50 && y < 50) {
            this->randPts();
        }
        this->inval(NULL);
        return NULL;
    }
        
    virtual bool onClick(Click* click) {
        return false;
    }
    
private:
    typedef SampleView INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

static SkView* MyFactory() { return new LineClipperView; }
static SkViewRegister reg(MyFactory);

