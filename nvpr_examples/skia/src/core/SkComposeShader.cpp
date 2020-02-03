
/*
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include "SkComposeShader.h"
#include "SkColorFilter.h"
#include "SkColorPriv.h"
#include "SkColorShader.h"
#include "SkXfermode.h"

///////////////////////////////////////////////////////////////////////////////

SkComposeShader::SkComposeShader(SkShader* sA, SkShader* sB, SkXfermode* mode) {
    fShaderA = sA;  sA->ref();
    fShaderB = sB;  sB->ref();
    // mode may be null
    fMode = mode;
    SkSafeRef(mode);
}

SkComposeShader::SkComposeShader(SkFlattenableReadBuffer& buffer) :
    INHERITED(buffer) {
    fShaderA = static_cast<SkShader*>(buffer.readFlattenable());
    if (NULL == fShaderA) {
        fShaderA = SkNEW_ARGS(SkColorShader, (0));
    }
    fShaderB = static_cast<SkShader*>(buffer.readFlattenable());
    if (NULL == fShaderB) {
        fShaderB = SkNEW_ARGS(SkColorShader, (0));
    }
    fMode = static_cast<SkXfermode*>(buffer.readFlattenable());
}

SkComposeShader::~SkComposeShader() {
    SkSafeUnref(fMode);
    fShaderB->unref();
    fShaderA->unref();
}

void SkComposeShader::beginSession() {
    this->INHERITED::beginSession();
    fShaderA->beginSession();
    fShaderB->beginSession();
}

void SkComposeShader::endSession() {
    fShaderA->endSession();
    fShaderB->endSession();
    this->INHERITED::endSession();
}

class SkAutoAlphaRestore {
public:
    SkAutoAlphaRestore(SkPaint* paint, uint8_t newAlpha) {
        fAlpha = paint->getAlpha();
        fPaint = paint;
        paint->setAlpha(newAlpha);
    }

    ~SkAutoAlphaRestore() {
        fPaint->setAlpha(fAlpha);
    }
private:
    SkPaint*    fPaint;
    uint8_t     fAlpha;
};

void SkComposeShader::flatten(SkFlattenableWriteBuffer& buffer) const {
    this->INHERITED::flatten(buffer);
    buffer.writeFlattenable(fShaderA);
    buffer.writeFlattenable(fShaderB);
    buffer.writeFlattenable(fMode);
}

/*  We call setContext on our two worker shaders. However, we
    always let them see opaque alpha, and if the paint really
    is translucent, then we apply that after the fact.
*/
bool SkComposeShader::setContext(const SkBitmap& device,
                                 const SkPaint& paint,
                                 const SkMatrix& matrix) {
    if (!this->INHERITED::setContext(device, paint, matrix)) {
        return false;
    }

    // we preconcat our localMatrix (if any) with the device matrix
    // before calling our sub-shaders

    SkMatrix tmpM;

    (void)this->getLocalMatrix(&tmpM);
    tmpM.setConcat(matrix, tmpM);

    SkAutoAlphaRestore  restore(const_cast<SkPaint*>(&paint), 0xFF);

    return  fShaderA->setContext(device, paint, tmpM) &&
            fShaderB->setContext(device, paint, tmpM);
}

// larger is better (fewer times we have to loop), but we shouldn't
// take up too much stack-space (each element is 4 bytes)
#define TMP_COLOR_COUNT     64

void SkComposeShader::shadeSpan(int x, int y, SkPMColor result[], int count) {
    SkShader*   shaderA = fShaderA;
    SkShader*   shaderB = fShaderB;
    SkXfermode* mode = fMode;
    unsigned    scale = SkAlpha255To256(this->getPaintAlpha());

    SkPMColor   tmp[TMP_COLOR_COUNT];

    if (NULL == mode) {   // implied SRC_OVER
        // TODO: when we have a good test-case, should use SkBlitRow::Proc32
        // for these loops
        do {
            int n = count;
            if (n > TMP_COLOR_COUNT) {
                n = TMP_COLOR_COUNT;
            }

            shaderA->shadeSpan(x, y, result, n);
            shaderB->shadeSpan(x, y, tmp, n);

            if (256 == scale) {
                for (int i = 0; i < n; i++) {
                    result[i] = SkPMSrcOver(tmp[i], result[i]);
                }
            } else {
                for (int i = 0; i < n; i++) {
                    result[i] = SkAlphaMulQ(SkPMSrcOver(tmp[i], result[i]),
                                            scale);
                }
            }

            result += n;
            x += n;
            count -= n;
        } while (count > 0);
    } else {    // use mode for the composition
        do {
            int n = count;
            if (n > TMP_COLOR_COUNT) {
                n = TMP_COLOR_COUNT;
            }

            shaderA->shadeSpan(x, y, result, n);
            shaderB->shadeSpan(x, y, tmp, n);
            mode->xfer32(result, tmp, n, NULL);

            if (256 == scale) {
                for (int i = 0; i < n; i++) {
                    result[i] = SkAlphaMulQ(result[i], scale);
                }
            }

            result += n;
            x += n;
            count -= n;
        } while (count > 0);
    }
}

