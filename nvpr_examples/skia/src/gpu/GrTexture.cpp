
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include "GrTexture.h"

#include "GrContext.h"
#include "GrGpu.h"
#include "GrRenderTarget.h"

bool GrTexture::readPixels(int left, int top, int width, int height,
                           GrPixelConfig config, void* buffer,
                           size_t rowBytes) {
    // go through context so that all necessary flushing occurs
    GrContext* context = this->getContext();
    if (NULL == context) {
        return false;
    }
    return context->readTexturePixels(this,
                                      left, top,
                                      width, height,
                                      config, buffer, rowBytes);
}

void GrTexture::writePixels(int left, int top, int width, int height,
                            GrPixelConfig config, const void* buffer,
                            size_t rowBytes) {
    // go through context so that all necessary flushing occurs
    GrContext* context = this->getContext();
    if (NULL == context) {
        return;
    }
    context->writeTexturePixels(this,
                                left, top,
                                width, height,
                                config, buffer, rowBytes);
}

void GrTexture::releaseRenderTarget() {
    if (NULL != fRenderTarget) {
        GrAssert(fRenderTarget->asTexture() == this);
        fRenderTarget->onTextureReleaseRenderTarget();
        fRenderTarget->unref();
        fRenderTarget = NULL;
    }
}

void GrTexture::onAbandon() {
    if (NULL != fRenderTarget) {
        fRenderTarget->abandon();
    }
}

