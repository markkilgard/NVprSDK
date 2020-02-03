/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkBenchmark.h"
#include "SkCanvas.h"
#include "SkPaint.h"
#include "SkRandom.h"
#include "SkString.h"
#include "SkShader.h"

enum VertFlags {
    kColors_VertFlag,
    kTexture_VertFlag,
};

class VertBench : public SkBenchmark {
    SkString fName;
    enum {
        W = 640,
        H = 480,
        ROW = 20,
        COL = 20,
        PTS = (ROW + 1) * (COL + 1),
        IDX = ROW * COL * 6,
        N = SkBENCHLOOP(10)
    };

    SkPoint fPts[PTS];
    SkColor fColors[PTS];
    SkPoint fTex[PTS];
    uint16_t fIdx[IDX];

    static void load_2_tris(uint16_t idx[], int x, int y, int rb) {
        int n = y * rb + x;
        idx[0] = n; idx[1] = n + 1; idx[2] = rb + n + 1;
        idx[3] = n; idx[4] = rb + n + 1; idx[5] = n + rb;
    }
    
public:
    VertBench(void* param) : INHERITED(param) {
        const SkScalar dx = SkIntToScalar(W) / COL;
        const SkScalar dy = SkIntToScalar(H) / COL;

        SkPoint* pts = fPts;
        uint16_t* idx = fIdx;

        SkScalar yy = 0;
        for (int y = 0; y <= ROW; y++) {
            SkScalar xx = 0;
            for (int x = 0; x <= COL; ++x) {
                pts->set(xx, yy);
                pts += 1;
                xx += dx;
                
                if (x < COL && y < ROW) {
                    load_2_tris(idx, x, y, COL + 1);
                    for (int i = 0; i < 6; i++) {
                        SkASSERT(idx[i] < PTS);
                    }
                    idx += 6;
                }
            }
            yy += dy;
        }
        SkASSERT(PTS == pts - fPts);
        SkASSERT(IDX == idx - fIdx);

        SkRandom rand;
        for (int i = 0; i < PTS; ++i) {
            fColors[i] = rand.nextU() | (0xFF << 24);
        }

        fName.set("verts");
    }

protected:
    virtual const char* onGetName() { return fName.c_str(); }
    virtual void onDraw(SkCanvas* canvas) {
        SkPaint paint;
        this->setupPaint(&paint);

        for (int i = 0; i < N; i++) {
            canvas->drawVertices(SkCanvas::kTriangles_VertexMode, PTS,
                                 fPts, NULL, fColors, NULL, fIdx, IDX, paint);
        }
    }
private:
    typedef SkBenchmark INHERITED;
};

///////////////////////////////////////////////////////////////////////////////

static SkBenchmark* Fact(void* p) { return SkNEW_ARGS(VertBench, (p)); }

static BenchRegistry gReg(Fact);
