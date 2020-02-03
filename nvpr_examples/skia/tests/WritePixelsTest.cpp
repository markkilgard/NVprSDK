
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "Test.h"
#include "SkCanvas.h"
#include "SkRegion.h"
#include "SkGpuDevice.h"

static const int DEV_W = 100, DEV_H = 100;
static const SkIRect DEV_RECT = SkIRect::MakeWH(DEV_W, DEV_H);
static const SkRect DEV_RECT_S = SkRect::MakeWH(DEV_W * SK_Scalar1, 
                                                DEV_H * SK_Scalar1);
static const U8CPU DEV_PAD = 0xee;

namespace {
SkPMColor getCanvasColor(int x, int y) {
    SkASSERT(x >= 0 && x < DEV_W);
    SkASSERT(y >= 0 && y < DEV_H);

    U8CPU r = x;
    U8CPU g = y;
    U8CPU b = 0xc;

    U8CPU a = 0x0;
    switch ((x+y) % 5) {
        case 0:
            a = 0xff;
            break;
        case 1:
            a = 0x80;
            break;
        case 2:
            a = 0xCC;
            break;
        case 3:
            a = 0x00;
            break;
        case 4:
            a = 0x01;
            break;
    }
    return SkPremultiplyARGBInline(a, r, g, b);
}

bool config8888IsPremul(SkCanvas::Config8888 config8888) {
    switch (config8888) {
        case SkCanvas::kNative_Premul_Config8888:
        case SkCanvas::kBGRA_Premul_Config8888:
        case SkCanvas::kRGBA_Premul_Config8888:
            return true;
        case SkCanvas::kNative_Unpremul_Config8888:
        case SkCanvas::kBGRA_Unpremul_Config8888:
        case SkCanvas::kRGBA_Unpremul_Config8888:
            return false;
        default:
            SkASSERT(0);
            return false;
    }
}

// assumes any premu/.unpremul has been applied
uint32_t packConfig8888(SkCanvas::Config8888 config8888,
                        U8CPU a, U8CPU r, U8CPU g, U8CPU b) {
    uint32_t r32;
    uint8_t* result = reinterpret_cast<uint8_t*>(&r32);
    switch (config8888) {
        case SkCanvas::kNative_Premul_Config8888:
        case SkCanvas::kNative_Unpremul_Config8888:
            r32 = SkPackARGB32NoCheck(a, r, g, b);
            break;
        case SkCanvas::kBGRA_Premul_Config8888:
        case SkCanvas::kBGRA_Unpremul_Config8888:
            result[0] = b;
            result[1] = g;
            result[2] = r;
            result[3] = a;
            break;
        case SkCanvas::kRGBA_Premul_Config8888:
        case SkCanvas::kRGBA_Unpremul_Config8888:
            result[0] = r;
            result[1] = g;
            result[2] = b;
            result[3] = a;
            break;
        default:
            SkASSERT(0);
            return 0;
    }
    return r32;
}

uint32_t getBitmapColor(int x, int y, int w, int h, SkCanvas::Config8888 config8888) {
    int n = y * w + x;
    U8CPU b = n & 0xff;
    U8CPU g = (n >> 8) & 0xff;
    U8CPU r = (n >> 16) & 0xff;
    U8CPU a = 0;
    switch ((x+y) % 5) {
        case 4:
            a = 0xff;
            break;
        case 3:
            a = 0x80;
            break;
        case 2:
            a = 0xCC;
            break;
        case 1:
            a = 0x01;
            break;
        case 0:
            a = 0x00;
            break;
    }
    if (config8888IsPremul(config8888)) {
        r = SkMulDiv255Ceiling(r, a);
        g = SkMulDiv255Ceiling(g, a);
        b = SkMulDiv255Ceiling(b, a);
    }
    return packConfig8888(config8888, a, r, g , b);
}

void fillCanvas(SkCanvas* canvas) {
    static SkBitmap bmp;
    if (bmp.isNull()) {
        bmp.setConfig(SkBitmap::kARGB_8888_Config, DEV_W, DEV_H);
        bool alloc = bmp.allocPixels();
        SkASSERT(alloc);
        SkAutoLockPixels alp(bmp);
        intptr_t pixels = reinterpret_cast<intptr_t>(bmp.getPixels());
        for (int y = 0; y < DEV_H; ++y) {
            for (int x = 0; x < DEV_W; ++x) {
                SkPMColor* pixel = reinterpret_cast<SkPMColor*>(pixels + y * bmp.rowBytes() + x * bmp.bytesPerPixel());
                *pixel = getCanvasColor(x, y);
            }
        }
    }
    canvas->save();
    canvas->setMatrix(SkMatrix::I());
    canvas->clipRect(DEV_RECT_S, SkRegion::kReplace_Op);
    SkPaint paint;
    paint.setXfermodeMode(SkXfermode::kSrc_Mode);
    canvas->drawBitmap(bmp, 0, 0, &paint);
    canvas->restore();
}

SkPMColor convertConfig8888ToPMColor(SkCanvas::Config8888 config8888,
                                     uint32_t color,
                                     bool* premul) {
    const uint8_t* c = reinterpret_cast<uint8_t*>(&color);
    U8CPU a,r,g,b;
    *premul = false;
    switch (config8888) {
        case SkCanvas::kNative_Premul_Config8888:
            return color;
        case SkCanvas::kNative_Unpremul_Config8888:
            *premul = true;
            a = SkGetPackedA32(color);
            r = SkGetPackedR32(color);
            g = SkGetPackedG32(color);
            b = SkGetPackedB32(color);
            break;
        case SkCanvas::kBGRA_Unpremul_Config8888:
            *premul = true; // fallthru
        case SkCanvas::kBGRA_Premul_Config8888:
            a = static_cast<U8CPU>(c[3]);
            r = static_cast<U8CPU>(c[2]);
            g = static_cast<U8CPU>(c[1]);
            b = static_cast<U8CPU>(c[0]);
            break;
        case SkCanvas::kRGBA_Unpremul_Config8888:
            *premul = true; // fallthru
        case SkCanvas::kRGBA_Premul_Config8888:
            a = static_cast<U8CPU>(c[3]);
            r = static_cast<U8CPU>(c[0]);
            g = static_cast<U8CPU>(c[1]);
            b = static_cast<U8CPU>(c[2]);
            break;
        default:
            GrCrash("Unexpected Config8888");
    }
    if (*premul) {
        r = SkMulDiv255Ceiling(r, a);
        g = SkMulDiv255Ceiling(g, a);
        b = SkMulDiv255Ceiling(b, a);
    }
    return SkPackARGB32(a, r, g, b);
}

bool checkPixel(SkPMColor a, SkPMColor b, bool didPremulConversion) {
    if (!didPremulConversion) {
        return a == b;
    }
    int32_t aA = static_cast<int32_t>(SkGetPackedA32(a));
    int32_t aR = static_cast<int32_t>(SkGetPackedR32(a));
    int32_t aG = static_cast<int32_t>(SkGetPackedG32(a));
    int32_t aB = SkGetPackedB32(a);

    int32_t bA = static_cast<int32_t>(SkGetPackedA32(b));
    int32_t bR = static_cast<int32_t>(SkGetPackedR32(b));
    int32_t bG = static_cast<int32_t>(SkGetPackedG32(b));
    int32_t bB = static_cast<int32_t>(SkGetPackedB32(b));

    return aA == bA &&
           SkAbs32(aR - bR) <= 1 &&
           SkAbs32(aG - bG) <= 1 &&
           SkAbs32(aB - bB) <= 1;
}

bool checkWrite(skiatest::Reporter* reporter,
                SkCanvas* canvas,
                const SkBitmap& bitmap,
                int writeX, int writeY,
                SkCanvas::Config8888 config8888) {
    SkDevice* dev = canvas->getDevice();
    if (!dev) {
        return false;
    }
    SkBitmap devBmp = dev->accessBitmap(false);
    if (devBmp.width() != DEV_W ||
        devBmp.height() != DEV_H ||
        devBmp.config() != SkBitmap::kARGB_8888_Config ||
        devBmp.isNull()) {
        return false;
    }
    SkAutoLockPixels alp(devBmp);

    intptr_t canvasPixels = reinterpret_cast<intptr_t>(devBmp.getPixels());
    size_t canvasRowBytes = devBmp.rowBytes();
    SkIRect writeRect = SkIRect::MakeXYWH(writeX, writeY, bitmap.width(), bitmap.height());
    bool success = true;
    for (int cy = 0; cy < DEV_H; ++cy) {
        const SkPMColor* canvasRow = reinterpret_cast<const SkPMColor*>(canvasPixels);
        for (int cx = 0; cx < DEV_W; ++cx) {
            SkPMColor canvasPixel = canvasRow[cx];
            if (writeRect.contains(cx, cy)) {
                int bx = cx - writeX;
                int by = cy - writeY;
                uint32_t bmpColor8888 = getBitmapColor(bx, by, bitmap.width(), bitmap.height(), config8888);
                bool mul;
                SkPMColor bmpPMColor = convertConfig8888ToPMColor(config8888, bmpColor8888, &mul);
                bool check;
                REPORTER_ASSERT(reporter, check = checkPixel(bmpPMColor, canvasPixel, mul));
                if (!check) {
                    success = false;
                }
            } else {
                bool check;
                SkPMColor testColor = getCanvasColor(cx, cy);
                REPORTER_ASSERT(reporter, check = (canvasPixel == testColor));
                if (!check) {
                    success = false;
                }
            }
        }
        if (cy != DEV_H -1) {
            const char* pad = reinterpret_cast<const char*>(canvasPixels + 4 * DEV_W);
            for (size_t px = 0; px < canvasRowBytes - 4 * DEV_W; ++px) {
                bool check;
                REPORTER_ASSERT(reporter, check = (pad[px] == static_cast<char>(DEV_PAD)));
                if (!check) {
                    success = false;
                }
            }
        }
        canvasPixels += canvasRowBytes;
    }

    return success;
}

enum DevType {
    kRaster_DevType,
    kGpu_DevType,
};

struct CanvasConfig {
    DevType fDevType;
    bool fTightRowBytes;
};

static const CanvasConfig gCanvasConfigs[] = {
    {kRaster_DevType, true},
    {kRaster_DevType, false},
#ifdef SK_SCALAR_IS_FLOAT
    {kGpu_DevType, true}, // row bytes has no meaning on gpu devices
#endif
};

bool setupCanvas(SkCanvas* canvas, const CanvasConfig& c, GrContext* grCtx) {
    switch (c.fDevType) {
        case kRaster_DevType: {
            SkBitmap bmp;
            size_t rowBytes = c.fTightRowBytes ? 0 : 4 * DEV_W + 100;
            bmp.setConfig(SkBitmap::kARGB_8888_Config, DEV_W, DEV_H, rowBytes);
            if (!bmp.allocPixels()) {
                return false;
            }
            // if rowBytes isn't tight then set the padding to a known value
            if (rowBytes) {
                SkAutoLockPixels alp(bmp);
                memset(bmp.getPixels(), DEV_PAD, bmp.getSafeSize());
            }
            canvas->setDevice(new SkDevice(bmp))->unref();
            } break;
        case kGpu_DevType:
            canvas->setDevice(new SkGpuDevice(grCtx,
                                              SkBitmap::kARGB_8888_Config,
                                              DEV_W, DEV_H))->unref();
            break;
    }
    return true;
}

bool setupBitmap(SkBitmap* bitmap,
              SkCanvas::Config8888 config8888,
              int w, int h,
              bool tightRowBytes) {
    size_t rowBytes = tightRowBytes ? 0 : 4 * w + 60;
    bitmap->setConfig(SkBitmap::kARGB_8888_Config, w, h, rowBytes);
    if (!bitmap->allocPixels()) {
        return false;
    }
    SkAutoLockPixels alp(*bitmap);
    intptr_t pixels = reinterpret_cast<intptr_t>(bitmap->getPixels());
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint32_t* pixel = reinterpret_cast<uint32_t*>(pixels + y * bitmap->rowBytes() + x * 4);
            *pixel = getBitmapColor(x, y, w, h, config8888);
        }
    }
    return true;
}

void WritePixelsTest(skiatest::Reporter* reporter, GrContext* context) {
    SkCanvas canvas;
    
    const SkIRect testRects[] = {
        // entire thing
        DEV_RECT,
        // larger on all sides
        SkIRect::MakeLTRB(-10, -10, DEV_W + 10, DEV_H + 10),
        // fully contained
        SkIRect::MakeLTRB(DEV_W / 4, DEV_H / 4, 3 * DEV_W / 4, 3 * DEV_H / 4),
        // outside top left
        SkIRect::MakeLTRB(-10, -10, -1, -1),
        // touching top left corner
        SkIRect::MakeLTRB(-10, -10, 0, 0),
        // overlapping top left corner
        SkIRect::MakeLTRB(-10, -10, DEV_W / 4, DEV_H / 4),
        // overlapping top left and top right corners
        SkIRect::MakeLTRB(-10, -10, DEV_W  + 10, DEV_H / 4),
        // touching entire top edge
        SkIRect::MakeLTRB(-10, -10, DEV_W  + 10, 0),
        // overlapping top right corner
        SkIRect::MakeLTRB(3 * DEV_W / 4, -10, DEV_W  + 10, DEV_H / 4),
        // contained in x, overlapping top edge
        SkIRect::MakeLTRB(DEV_W / 4, -10, 3 * DEV_W  / 4, DEV_H / 4),
        // outside top right corner
        SkIRect::MakeLTRB(DEV_W + 1, -10, DEV_W + 10, -1),
        // touching top right corner
        SkIRect::MakeLTRB(DEV_W, -10, DEV_W + 10, 0),
        // overlapping top left and bottom left corners
        SkIRect::MakeLTRB(-10, -10, DEV_W / 4, DEV_H + 10),
        // touching entire left edge
        SkIRect::MakeLTRB(-10, -10, 0, DEV_H + 10),
        // overlapping bottom left corner
        SkIRect::MakeLTRB(-10, 3 * DEV_H / 4, DEV_W / 4, DEV_H + 10),
        // contained in y, overlapping left edge
        SkIRect::MakeLTRB(-10, DEV_H / 4, DEV_W / 4, 3 * DEV_H / 4),
        // outside bottom left corner
        SkIRect::MakeLTRB(-10, DEV_H + 1, -1, DEV_H + 10),
        // touching bottom left corner
        SkIRect::MakeLTRB(-10, DEV_H, 0, DEV_H + 10),
        // overlapping bottom left and bottom right corners
        SkIRect::MakeLTRB(-10, 3 * DEV_H / 4, DEV_W + 10, DEV_H + 10),
        // touching entire left edge
        SkIRect::MakeLTRB(0, DEV_H, DEV_W, DEV_H + 10),
        // overlapping bottom right corner
        SkIRect::MakeLTRB(3 * DEV_W / 4, 3 * DEV_H / 4, DEV_W + 10, DEV_H + 10),
        // overlapping top right and bottom right corners
        SkIRect::MakeLTRB(3 * DEV_W / 4, -10, DEV_W + 10, DEV_H + 10),
    };

    for (size_t i = 0; i < SK_ARRAY_COUNT(gCanvasConfigs); ++i) {
        REPORTER_ASSERT(reporter, setupCanvas(&canvas, gCanvasConfigs[i], context));

        static const SkCanvas::Config8888 gReadConfigs[] = {
            SkCanvas::kNative_Premul_Config8888,
            SkCanvas::kNative_Unpremul_Config8888,
            SkCanvas::kBGRA_Premul_Config8888,
            SkCanvas::kBGRA_Unpremul_Config8888,
            SkCanvas::kRGBA_Premul_Config8888,
            SkCanvas::kRGBA_Unpremul_Config8888,
        };
        for (size_t r = 0; r < SK_ARRAY_COUNT(testRects); ++r) {
            const SkIRect& rect = testRects[r];
            for (int tightBmp = 0; tightBmp < 2; ++tightBmp) {
                for (size_t c = 0; c < SK_ARRAY_COUNT(gReadConfigs); ++c) {
                    fillCanvas(&canvas);
                    SkCanvas::Config8888 config8888 = gReadConfigs[c];
                    SkBitmap bmp;
                    REPORTER_ASSERT(reporter, setupBitmap(&bmp, config8888, rect.width(), rect.height(), SkToBool(tightBmp)));
                    canvas.writePixels(bmp, rect.fLeft, rect.fTop, config8888);
                    REPORTER_ASSERT(reporter, checkWrite(reporter, &canvas, bmp, rect.fLeft, rect.fTop, config8888));
                }
            }
        }
    }
}
}

#include "TestClassDef.h"
DEFINE_GPUTESTCLASS("WritePixels", WritePixelsTestClass, WritePixelsTest)

