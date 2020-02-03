
/*
 * Copyright 2010 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include "GrGpu.h"

#include "GrBufferAllocPool.h"
#include "GrClipIterator.h"
#include "GrContext.h"
#include "GrIndexBuffer.h"
#include "GrPathRenderer.h"
#include "GrStencilBuffer.h"
#include "GrVertexBuffer.h"

// probably makes no sense for this to be less than a page
static const size_t VERTEX_POOL_VB_SIZE = 1 << 18;
static const int VERTEX_POOL_VB_COUNT = 4;
static const size_t INDEX_POOL_IB_SIZE = 1 << 16;
static const int INDEX_POOL_IB_COUNT = 4;

////////////////////////////////////////////////////////////////////////////////

extern void gr_run_unittests();

#define DEBUG_INVAL_BUFFER    0xdeadcafe
#define DEBUG_INVAL_START_IDX -1

GrGpu::GrGpu()
    : fClipInStencil(false)
    , fContext(NULL)
    , fResetTimestamp(kExpiredTimestamp+1)
    , fVertexPool(NULL)
    , fIndexPool(NULL)
    , fVertexPoolUseCnt(0)
    , fIndexPoolUseCnt(0)
    , fQuadIndexBuffer(NULL)
    , fUnitSquareVertexBuffer(NULL)
    , fPathRendererChain(NULL)
    , fContextIsDirty(true)
    , fResourceHead(NULL) {

#if GR_DEBUG
    //gr_run_unittests();
#endif
        
    fGeomPoolStateStack.push_back();
#if GR_DEBUG
    GeometryPoolState& poolState = fGeomPoolStateStack.back();
    poolState.fPoolVertexBuffer = (GrVertexBuffer*)DEBUG_INVAL_BUFFER;
    poolState.fPoolStartVertex = DEBUG_INVAL_START_IDX;
    poolState.fPoolIndexBuffer = (GrIndexBuffer*)DEBUG_INVAL_BUFFER;
    poolState.fPoolStartIndex = DEBUG_INVAL_START_IDX;
#endif
    resetStats();
}

GrGpu::~GrGpu() {
    this->releaseResources();
}

void GrGpu::abandonResources() {

    while (NULL != fResourceHead) {
        fResourceHead->abandon();
    }

    GrAssert(NULL == fQuadIndexBuffer || !fQuadIndexBuffer->isValid());
    GrAssert(NULL == fUnitSquareVertexBuffer ||
             !fUnitSquareVertexBuffer->isValid());
    GrSafeSetNull(fQuadIndexBuffer);
    GrSafeSetNull(fUnitSquareVertexBuffer);
    delete fVertexPool;
    fVertexPool = NULL;
    delete fIndexPool;
    fIndexPool = NULL;
    // in case path renderer has any GrResources, start from scratch
    GrSafeSetNull(fPathRendererChain);
}

void GrGpu::releaseResources() {

    while (NULL != fResourceHead) {
        fResourceHead->release();
    }

    GrAssert(NULL == fQuadIndexBuffer || !fQuadIndexBuffer->isValid());
    GrAssert(NULL == fUnitSquareVertexBuffer ||
             !fUnitSquareVertexBuffer->isValid());
    GrSafeSetNull(fQuadIndexBuffer);
    GrSafeSetNull(fUnitSquareVertexBuffer);
    delete fVertexPool;
    fVertexPool = NULL;
    delete fIndexPool;
    fIndexPool = NULL;
    // in case path renderer has any GrResources, start from scratch
    GrSafeSetNull(fPathRendererChain);
}

void GrGpu::insertResource(GrResource* resource) {
    GrAssert(NULL != resource);
    GrAssert(this == resource->getGpu());
    GrAssert(NULL == resource->fNext);
    GrAssert(NULL == resource->fPrevious);

    resource->fNext = fResourceHead;
    if (NULL != fResourceHead) {
        GrAssert(NULL == fResourceHead->fPrevious);
        fResourceHead->fPrevious = resource;
    }
    fResourceHead = resource;
}

void GrGpu::removeResource(GrResource* resource) {
    GrAssert(NULL != resource);
    GrAssert(NULL != fResourceHead);

    if (fResourceHead == resource) {
        GrAssert(NULL == resource->fPrevious);
        fResourceHead = resource->fNext;
    } else {
        GrAssert(NULL != fResourceHead);
        resource->fPrevious->fNext = resource->fNext;
    }
    if (NULL != resource->fNext) {
        resource->fNext->fPrevious = resource->fPrevious;
    }
    resource->fNext = NULL;
    resource->fPrevious = NULL;
}


void GrGpu::unimpl(const char msg[]) {
#if GR_DEBUG
    GrPrintf("--- GrGpu unimplemented(\"%s\")\n", msg);
#endif
}

////////////////////////////////////////////////////////////////////////////////

GrTexture* GrGpu::createTexture(const GrTextureDesc& desc,
                                const void* srcData, size_t rowBytes) {
    this->handleDirtyContext();
    GrTexture* tex = this->onCreateTexture(desc, srcData, rowBytes);
    if (NULL != tex && 
        (kRenderTarget_GrTextureFlagBit & desc.fFlags) &&
        !(kNoStencil_GrTextureFlagBit & desc.fFlags)) {
        GrAssert(NULL != tex->asRenderTarget());
        // TODO: defer this and attach dynamically
        if (!this->attachStencilBufferToRenderTarget(tex->asRenderTarget())) {
            tex->unref();
            return NULL;
        }
    }
    return tex;
}

bool GrGpu::attachStencilBufferToRenderTarget(GrRenderTarget* rt) {
    GrAssert(NULL == rt->getStencilBuffer());
    GrStencilBuffer* sb = 
        this->getContext()->findStencilBuffer(rt->width(),
                                              rt->height(),
                                              rt->numSamples());
    if (NULL != sb) {
        rt->setStencilBuffer(sb);
        bool attached = this->attachStencilBufferToRenderTarget(sb, rt);
        if (!attached) {
            rt->setStencilBuffer(NULL);
        }
        return attached;
    }
    if (this->createStencilBufferForRenderTarget(rt,
                                                 rt->width(), rt->height())) {
        rt->getStencilBuffer()->ref();
        rt->getStencilBuffer()->transferToCacheAndLock();

        // Right now we're clearing the stencil buffer here after it is
        // attached to an RT for the first time. When we start matching
        // stencil buffers with smaller color targets this will no longer
        // be correct because it won't be guaranteed to clear the entire
        // sb.
        // We used to clear down in the GL subclass using a special purpose
        // FBO. But iOS doesn't allow a stencil-only FBO. It reports unsupported
        // FBO status.
        GrDrawState::AutoRenderTargetRestore artr(this->drawState(), rt);
        this->clearStencil();
        return true;
    } else {
        return false;
    }
}

GrTexture* GrGpu::createPlatformTexture(const GrPlatformTextureDesc& desc) {
    this->handleDirtyContext();
    GrTexture* tex = this->onCreatePlatformTexture(desc);
    if (NULL == tex) {
        return NULL;
    }
    // TODO: defer this and attach dynamically
    GrRenderTarget* tgt = tex->asRenderTarget();
    if (NULL != tgt &&
        !this->attachStencilBufferToRenderTarget(tgt)) {
        tex->unref();
        return NULL;
    } else {
        return tex;
    }
}

GrRenderTarget* GrGpu::createPlatformRenderTarget(const GrPlatformRenderTargetDesc& desc) {
    this->handleDirtyContext();
    return this->onCreatePlatformRenderTarget(desc);
}

GrVertexBuffer* GrGpu::createVertexBuffer(uint32_t size, bool dynamic) {
    this->handleDirtyContext();
    return this->onCreateVertexBuffer(size, dynamic);
}

GrIndexBuffer* GrGpu::createIndexBuffer(uint32_t size, bool dynamic) {
    this->handleDirtyContext();
    return this->onCreateIndexBuffer(size, dynamic);
}

void GrGpu::clear(const GrIRect* rect, GrColor color) {
    if (NULL == this->getDrawState().getRenderTarget()) {
        return;
    }
    this->handleDirtyContext();
    this->onClear(rect, color);
}

void GrGpu::forceRenderTargetFlush() {
    this->handleDirtyContext();
    this->onForceRenderTargetFlush();
}

bool GrGpu::readPixels(GrRenderTarget* target,
                       int left, int top, int width, int height,
                       GrPixelConfig config, void* buffer,
                       size_t rowBytes, bool invertY) {
    GrAssert(GrPixelConfigIsUnpremultiplied(config) ==
             GrPixelConfigIsUnpremultiplied(target->config()));
    this->handleDirtyContext();
    return this->onReadPixels(target, left, top, width, height,
                              config, buffer, rowBytes, invertY);
}

void GrGpu::writeTexturePixels(GrTexture* texture,
                               int left, int top, int width, int height,
                               GrPixelConfig config, const void* buffer,
                               size_t rowBytes) {
    GrAssert(GrPixelConfigIsUnpremultiplied(config) ==
             GrPixelConfigIsUnpremultiplied(texture->config()));
    this->handleDirtyContext();
    this->onWriteTexturePixels(texture, left, top, width, height,
                               config, buffer, rowBytes);
}

void GrGpu::resolveRenderTarget(GrRenderTarget* target) {
    GrAssert(target);
    this->handleDirtyContext();
    this->onResolveRenderTarget(target);
}


////////////////////////////////////////////////////////////////////////////////

static const int MAX_QUADS = 1 << 12; // max possible: (1 << 14) - 1;

GR_STATIC_ASSERT(4 * MAX_QUADS <= 65535);

static inline void fill_indices(uint16_t* indices, int quadCount) {
    for (int i = 0; i < quadCount; ++i) {
        indices[6 * i + 0] = 4 * i + 0;
        indices[6 * i + 1] = 4 * i + 1;
        indices[6 * i + 2] = 4 * i + 2;
        indices[6 * i + 3] = 4 * i + 0;
        indices[6 * i + 4] = 4 * i + 2;
        indices[6 * i + 5] = 4 * i + 3;
    }
}

const GrIndexBuffer* GrGpu::getQuadIndexBuffer() const {
    if (NULL == fQuadIndexBuffer) {
        static const int SIZE = sizeof(uint16_t) * 6 * MAX_QUADS;
        GrGpu* me = const_cast<GrGpu*>(this);
        fQuadIndexBuffer = me->createIndexBuffer(SIZE, false);
        if (NULL != fQuadIndexBuffer) {
            uint16_t* indices = (uint16_t*)fQuadIndexBuffer->lock();
            if (NULL != indices) {
                fill_indices(indices, MAX_QUADS);
                fQuadIndexBuffer->unlock();
            } else {
                indices = (uint16_t*)GrMalloc(SIZE);
                fill_indices(indices, MAX_QUADS);
                if (!fQuadIndexBuffer->updateData(indices, SIZE)) {
                    fQuadIndexBuffer->unref();
                    fQuadIndexBuffer = NULL;
                    GrCrash("Can't get indices into buffer!");
                }
                GrFree(indices);
            }
        }
    }

    return fQuadIndexBuffer;
}

const GrVertexBuffer* GrGpu::getUnitSquareVertexBuffer() const {
    if (NULL == fUnitSquareVertexBuffer) {

        static const GrPoint DATA[] = {
            { 0,            0 },
            { GR_Scalar1,   0 },
            { GR_Scalar1,   GR_Scalar1 },
            { 0,            GR_Scalar1 }
#if 0
            GrPoint(0,         0),
            GrPoint(GR_Scalar1,0),
            GrPoint(GR_Scalar1,GR_Scalar1),
            GrPoint(0,         GR_Scalar1)
#endif
        };
        static const size_t SIZE = sizeof(DATA);

        GrGpu* me = const_cast<GrGpu*>(this);
        fUnitSquareVertexBuffer = me->createVertexBuffer(SIZE, false);
        if (NULL != fUnitSquareVertexBuffer) {
            if (!fUnitSquareVertexBuffer->updateData(DATA, SIZE)) {
                fUnitSquareVertexBuffer->unref();
                fUnitSquareVertexBuffer = NULL;
                GrCrash("Can't get vertices into buffer!");
            }
        }
    }

    return fUnitSquareVertexBuffer;
}

////////////////////////////////////////////////////////////////////////////////

const GrStencilSettings* GrGpu::GetClipStencilSettings(void) {
    // stencil settings to use when clip is in stencil
    GR_STATIC_CONST_SAME_STENCIL_STRUCT(sClipStencilSettings,
        kKeep_StencilOp,
        kKeep_StencilOp,
        kAlwaysIfInClip_StencilFunc,
        0x0000,
        0x0000,
        0x0000);
    return GR_CONST_STENCIL_SETTINGS_PTR_FROM_STRUCT_PTR(&sClipStencilSettings);
}

// mapping of clip-respecting stencil funcs to normal stencil funcs
// mapping depends on whether stencil-clipping is in effect.
static const GrStencilFunc gGrClipToNormalStencilFunc[2][kClipStencilFuncCount] = {
    {// Stencil-Clipping is DISABLED, effectively always inside the clip
        // In the Clip Funcs
        kAlways_StencilFunc,          // kAlwaysIfInClip_StencilFunc
        kEqual_StencilFunc,           // kEqualIfInClip_StencilFunc
        kLess_StencilFunc,            // kLessIfInClip_StencilFunc
        kLEqual_StencilFunc,          // kLEqualIfInClip_StencilFunc
        // Special in the clip func that forces user's ref to be 0.
        kNotEqual_StencilFunc,        // kNonZeroIfInClip_StencilFunc
                                      // make ref 0 and do normal nequal.
    },
    {// Stencil-Clipping is ENABLED
        // In the Clip Funcs
        kEqual_StencilFunc,           // kAlwaysIfInClip_StencilFunc
                                      // eq stencil clip bit, mask
                                      // out user bits.

        kEqual_StencilFunc,           // kEqualIfInClip_StencilFunc
                                      // add stencil bit to mask and ref

        kLess_StencilFunc,            // kLessIfInClip_StencilFunc
        kLEqual_StencilFunc,          // kLEqualIfInClip_StencilFunc
                                      // for both of these we can add
                                      // the clip bit to the mask and
                                      // ref and compare as normal
        // Special in the clip func that forces user's ref to be 0.
        kLess_StencilFunc,            // kNonZeroIfInClip_StencilFunc
                                      // make ref have only the clip bit set
                                      // and make comparison be less
                                      // 10..0 < 1..user_bits..
    }
};

GrStencilFunc GrGpu::ConvertStencilFunc(bool stencilInClip, GrStencilFunc func) {
    GrAssert(func >= 0);
    if (func >= kBasicStencilFuncCount) {
        GrAssert(func < kStencilFuncCount);
        func = gGrClipToNormalStencilFunc[stencilInClip ? 1 : 0][func - kBasicStencilFuncCount];
        GrAssert(func >= 0 && func < kBasicStencilFuncCount);
    }
    return func;
}

void GrGpu::ConvertStencilFuncAndMask(GrStencilFunc func,
                                      bool clipInStencil,
                                      unsigned int clipBit,
                                      unsigned int userBits,
                                      unsigned int* ref,
                                      unsigned int* mask) {
    if (func < kBasicStencilFuncCount) {
        *mask &= userBits;
        *ref &= userBits;
    } else {
        if (clipInStencil) {
            switch (func) {
                case kAlwaysIfInClip_StencilFunc:
                    *mask = clipBit;
                    *ref = clipBit;
                    break;
                case kEqualIfInClip_StencilFunc:
                case kLessIfInClip_StencilFunc:
                case kLEqualIfInClip_StencilFunc:
                    *mask = (*mask & userBits) | clipBit;
                    *ref = (*ref & userBits) | clipBit;
                    break;
                case kNonZeroIfInClip_StencilFunc:
                    *mask = (*mask & userBits) | clipBit;
                    *ref = clipBit;
                    break;
                default:
                    GrCrash("Unknown stencil func");
            }
        } else {
            *mask &= userBits;
            *ref &= userBits;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

#define VISUALIZE_COMPLEX_CLIP 0

#if VISUALIZE_COMPLEX_CLIP
    #include "GrRandom.h"
    GrRandom gRandom;
    #define SET_RANDOM_COLOR drawState->setColor(0xff000000 | gRandom.nextU());
#else
    #define SET_RANDOM_COLOR
#endif

namespace {
// determines how many elements at the head of the clip can be skipped and
// whether the initial clear should be to the inside- or outside-the-clip value,
// and what op should be used to draw the first element that isn't skipped.
int process_initial_clip_elements(const GrClip& clip,
                                  const GrRect& bounds,
                                  bool* clearToInside,
                                  GrSetOp* startOp) {

    // logically before the first element of the clip stack is 
    // processed the clip is entirely open. However, depending on the
    // first set op we may prefer to clear to 0 for performance. We may
    // also be able to skip the initial clip paths/rects. We loop until
    // we cannot skip an element.
    int curr;
    bool done = false;
    *clearToInside = true;
    int count = clip.getElementCount();

    for (curr = 0; curr < count && !done; ++curr) {
        switch (clip.getOp(curr)) {
            case kReplace_SetOp:
                // replace ignores everything previous
                *startOp = kReplace_SetOp;
                *clearToInside = false;
                done = true;
                break;
            case kIntersect_SetOp:
                // if this element contains the entire bounds then we
                // can skip it.
                if (kRect_ClipType == clip.getElementType(curr)
                    && clip.getRect(curr).contains(bounds)) {
                    break;
                }
                // if everything is initially clearToInside then intersect is
                // same as clear to 0 and treat as a replace. Otherwise,
                // set stays empty.
                if (*clearToInside) {
                    *startOp = kReplace_SetOp;
                    *clearToInside = false;
                    done = true;
                }
                break;
                // we can skip a leading union.
            case kUnion_SetOp:
                // if everything is initially outside then union is
                // same as replace. Otherwise, every pixel is still 
                // clearToInside
                if (!*clearToInside) {
                    *startOp = kReplace_SetOp;
                    done = true;
                }
                break;
            case kXor_SetOp:
                // xor is same as difference or replace both of which
                // can be 1-pass instead of 2 for xor.
                if (*clearToInside) {
                    *startOp = kDifference_SetOp;
                } else {
                    *startOp = kReplace_SetOp;
                }
                done = true;
                break;
            case kDifference_SetOp:
                // if all pixels are clearToInside then we have to process the
                // difference, otherwise it has no effect and all pixels
                // remain outside.
                if (*clearToInside) {
                    *startOp = kDifference_SetOp;
                    done = true;
                }
                break;
            case kReverseDifference_SetOp:
                // if all pixels are clearToInside then reverse difference
                // produces empty set. Otherise it is same as replace
                if (*clearToInside) {
                    *clearToInside = false;
                } else {
                    *startOp = kReplace_SetOp;
                    done = true;
                }
                break;
            default:
                GrCrash("Unknown set op.");
        }
    }
    return done ? curr-1 : count;
}
}

bool GrGpu::setupClipAndFlushState(GrPrimitiveType type) {
    const GrIRect* r = NULL;
    GrIRect clipRect;

    GrDrawState* drawState = this->drawState();
    GrRenderTarget* rt = drawState->getRenderTarget();

    // GrDrawTarget should have filtered this for us
    GrAssert(NULL != rt);

    if (drawState->isClipState()) {

        GrRect bounds;
        GrRect rtRect;
        rtRect.setLTRB(0, 0,
                       GrIntToScalar(rt->width()), GrIntToScalar(rt->height()));
        if (fClip.hasConservativeBounds()) {
            bounds = fClip.getConservativeBounds();
            if (!bounds.intersect(rtRect)) {
                bounds.setEmpty();
            }
        } else {
            bounds = rtRect;
        }

        bounds.roundOut(&clipRect);
        if  (clipRect.isEmpty()) {
            clipRect.setLTRB(0,0,0,0);
        }
        r = &clipRect;

        // use the stencil clip if we can't represent the clip as a rectangle.
        fClipInStencil = !fClip.isRect() && !fClip.isEmpty() && 
                         !bounds.isEmpty();

        // TODO: dynamically attach a SB when needed.
        GrStencilBuffer* stencilBuffer = rt->getStencilBuffer();
        if (fClipInStencil && NULL == stencilBuffer) {
            return false;
        }

        if (fClipInStencil &&
            stencilBuffer->mustRenderClip(fClip, rt->width(), rt->height())) {

            stencilBuffer->setLastClip(fClip, rt->width(), rt->height());

            // we set the current clip to the bounds so that our recursive
            // draws are scissored to them. We use the copy of the complex clip
            // we just stashed on the SB to render from. We set it back after
            // we finish drawing it into the stencil.
            const GrClip& clip = stencilBuffer->getLastClip();
            fClip.setFromRect(bounds);

            AutoStateRestore asr(this, GrDrawTarget::kReset_ASRInit);
            drawState = this->drawState();
            drawState->setRenderTarget(rt);
            AutoGeometryPush agp(this);

            this->flushScissor(NULL);
#if !VISUALIZE_COMPLEX_CLIP
            drawState->enableState(GrDrawState::kNoColorWrites_StateBit);
#endif

            int count = clip.getElementCount();
            int clipBit = stencilBuffer->bits();
            SkASSERT((clipBit <= 16) &&
                     "Ganesh only handles 16b or smaller stencil buffers");
            clipBit = (1 << (clipBit-1));
            
            bool clearToInside;
            GrSetOp startOp = kReplace_SetOp; // suppress warning
            int start = process_initial_clip_elements(clip,
                                                      rtRect,
                                                      &clearToInside,
                                                      &startOp);

            this->clearStencilClip(clipRect, clearToInside);

            // walk through each clip element and perform its set op
            // with the existing clip.
            for (int c = start; c < count; ++c) {
                GrPathFill fill;
                bool fillInverted;
                // enabled at bottom of loop
                drawState->disableState(kModifyStencilClip_StateBit);

                bool canRenderDirectToStencil; // can the clip element be drawn
                                               // directly to the stencil buffer
                                               // with a non-inverted fill rule
                                               // without extra passes to
                                               // resolve in/out status.

                GrPathRenderer* pr = NULL;
                const GrPath* clipPath = NULL;
                if (kRect_ClipType == clip.getElementType(c)) {
                    canRenderDirectToStencil = true;
                    fill = kEvenOdd_PathFill;
                    fillInverted = false;
                    // there is no point in intersecting a screen filling
                    // rectangle.
                    if (kIntersect_SetOp == clip.getOp(c) &&
                        clip.getRect(c).contains(rtRect)) {
                        continue;
                    }
                } else {
                    fill = clip.getPathFill(c);
                    fillInverted = GrIsFillInverted(fill);
                    fill = GrNonInvertedFill(fill);
                    clipPath = &clip.getPath(c);
                    pr = this->getClipPathRenderer(*clipPath, fill);
                    if (NULL == pr) {
                        fClipInStencil = false;
                        fClip = clip;
                        return false;
                    }
                    canRenderDirectToStencil =
                        !pr->requiresStencilPass(*clipPath, fill, this);
                }

                GrSetOp op = (c == start) ? startOp : clip.getOp(c);
                int passes;
                GrStencilSettings stencilSettings[GrStencilSettings::kMaxStencilClipPasses];

                bool canDrawDirectToClip; // Given the renderer, the element,
                                          // fill rule, and set operation can
                                          // we render the element directly to
                                          // stencil bit used for clipping.
                canDrawDirectToClip =
                    GrStencilSettings::GetClipPasses(op,
                                                     canRenderDirectToStencil,
                                                     clipBit,
                                                     fillInverted,
                                                     &passes, stencilSettings);

                // draw the element to the client stencil bits if necessary
                if (!canDrawDirectToClip) {
                    GR_STATIC_CONST_SAME_STENCIL(gDrawToStencil,
                        kIncClamp_StencilOp,
                        kIncClamp_StencilOp,
                        kAlways_StencilFunc,
                        0xffff,
                        0x0000,
                        0xffff);
                    SET_RANDOM_COLOR
                    if (kRect_ClipType == clip.getElementType(c)) {
                        *drawState->stencil() = gDrawToStencil;
                        this->drawSimpleRect(clip.getRect(c), NULL, 0);
                    } else {
                        if (canRenderDirectToStencil) {
                            *drawState->stencil() = gDrawToStencil;
                            pr->drawPath(*clipPath, fill, NULL, this, 0, false);
                        } else {
                            pr->drawPathToStencil(*clipPath, fill, this);
                        }
                    }
                }

                // now we modify the clip bit by rendering either the clip
                // element directly or a bounding rect of the entire clip.
                drawState->enableState(kModifyStencilClip_StateBit);
                for (int p = 0; p < passes; ++p) {
                    *drawState->stencil() = stencilSettings[p];
                    if (canDrawDirectToClip) {
                        if (kRect_ClipType == clip.getElementType(c)) {
                            SET_RANDOM_COLOR
                            this->drawSimpleRect(clip.getRect(c), NULL, 0);
                        } else {
                            SET_RANDOM_COLOR
                            pr->drawPath(*clipPath, fill, NULL, this, 0, false);
                        }
                    } else {
                        SET_RANDOM_COLOR
                        this->drawSimpleRect(bounds, NULL, 0);
                    }
                }
            }
            // restore clip
            fClip = clip;
            // recusive draws would have disabled this since they drew with
            // the clip bounds as clip.
            fClipInStencil = true;
        }
    }

    // Must flush the scissor after graphics state
    if (!this->flushGraphicsState(type)) {
        return false;
    }
    this->flushScissor(r);
    return true;
}

GrPathRenderer* GrGpu::getClipPathRenderer(const GrPath& path,
                                           GrPathFill fill) {
    if (NULL == fPathRendererChain) {
        fPathRendererChain = 
            new GrPathRendererChain(this->getContext(),
                                    GrPathRendererChain::kNonAAOnly_UsageFlag);
    }
    return fPathRendererChain->getPathRenderer(path, fill, this, false);
}


////////////////////////////////////////////////////////////////////////////////

void GrGpu::geometrySourceWillPush() {
    const GeometrySrcState& geoSrc = this->getGeomSrc();
    if (kArray_GeometrySrcType == geoSrc.fVertexSrc ||
        kReserved_GeometrySrcType == geoSrc.fVertexSrc) {
        this->finalizeReservedVertices();
    }
    if (kArray_GeometrySrcType == geoSrc.fIndexSrc ||
        kReserved_GeometrySrcType == geoSrc.fIndexSrc) {
        this->finalizeReservedIndices();
    }
    GeometryPoolState& newState = fGeomPoolStateStack.push_back();
#if GR_DEBUG
    newState.fPoolVertexBuffer = (GrVertexBuffer*)DEBUG_INVAL_BUFFER;
    newState.fPoolStartVertex = DEBUG_INVAL_START_IDX;
    newState.fPoolIndexBuffer = (GrIndexBuffer*)DEBUG_INVAL_BUFFER;
    newState.fPoolStartIndex = DEBUG_INVAL_START_IDX;
#endif
}

void GrGpu::geometrySourceWillPop(const GeometrySrcState& restoredState) {
    // if popping last entry then pops are unbalanced with pushes
    GrAssert(fGeomPoolStateStack.count() > 1);
    fGeomPoolStateStack.pop_back();
}

void GrGpu::onDrawIndexed(GrPrimitiveType type,
                          int startVertex,
                          int startIndex,
                          int vertexCount,
                          int indexCount) {

    this->handleDirtyContext();

    if (!this->setupClipAndFlushState(type)) {
        return;
    }

#if GR_COLLECT_STATS
    fStats.fVertexCnt += vertexCount;
    fStats.fIndexCnt  += indexCount;
    fStats.fDrawCnt   += 1;
#endif

    int sVertex = startVertex;
    int sIndex = startIndex;
    setupGeometry(&sVertex, &sIndex, vertexCount, indexCount);

    this->onGpuDrawIndexed(type, sVertex, sIndex,
                           vertexCount, indexCount);
}

void GrGpu::onDrawNonIndexed(GrPrimitiveType type,
                           int startVertex,
                           int vertexCount) {
    this->handleDirtyContext();

    if (!this->setupClipAndFlushState(type)) {
        return;
    }
#if GR_COLLECT_STATS
    fStats.fVertexCnt += vertexCount;
    fStats.fDrawCnt   += 1;
#endif

    int sVertex = startVertex;
    setupGeometry(&sVertex, NULL, vertexCount, 0);

    this->onGpuDrawNonIndexed(type, sVertex, vertexCount);
}

void GrGpu::finalizeReservedVertices() {
    GrAssert(NULL != fVertexPool);
    fVertexPool->unlock();
}

void GrGpu::finalizeReservedIndices() {
    GrAssert(NULL != fIndexPool);
    fIndexPool->unlock();
}

void GrGpu::prepareVertexPool() {
    if (NULL == fVertexPool) {
        GrAssert(0 == fVertexPoolUseCnt);
        fVertexPool = new GrVertexBufferAllocPool(this, true,
                                                  VERTEX_POOL_VB_SIZE,
                                                  VERTEX_POOL_VB_COUNT);
        fVertexPool->releaseGpuRef();
    } else if (!fVertexPoolUseCnt) {
        // the client doesn't have valid data in the pool
        fVertexPool->reset();
    }
}

void GrGpu::prepareIndexPool() {
    if (NULL == fIndexPool) {
        GrAssert(0 == fIndexPoolUseCnt);
        fIndexPool = new GrIndexBufferAllocPool(this, true,
                                                INDEX_POOL_IB_SIZE,
                                                INDEX_POOL_IB_COUNT);
        fIndexPool->releaseGpuRef();
    } else if (!fIndexPoolUseCnt) {
        // the client doesn't have valid data in the pool
        fIndexPool->reset();
    }
}

bool GrGpu::onReserveVertexSpace(GrVertexLayout vertexLayout,
                                 int vertexCount,
                                 void** vertices) {
    GeometryPoolState& geomPoolState = fGeomPoolStateStack.back();
    
    GrAssert(vertexCount > 0);
    GrAssert(NULL != vertices);
    
    this->prepareVertexPool();
    
    *vertices = fVertexPool->makeSpace(vertexLayout,
                                       vertexCount,
                                       &geomPoolState.fPoolVertexBuffer,
                                       &geomPoolState.fPoolStartVertex);
    if (NULL == *vertices) {
        return false;
    }
    ++fVertexPoolUseCnt;
    return true;
}

bool GrGpu::onReserveIndexSpace(int indexCount, void** indices) {
    GeometryPoolState& geomPoolState = fGeomPoolStateStack.back();
    
    GrAssert(indexCount > 0);
    GrAssert(NULL != indices);

    this->prepareIndexPool();

    *indices = fIndexPool->makeSpace(indexCount,
                                     &geomPoolState.fPoolIndexBuffer,
                                     &geomPoolState.fPoolStartIndex);
    if (NULL == *indices) {
        return false;
    }
    ++fIndexPoolUseCnt;
    return true;
}

void GrGpu::releaseReservedVertexSpace() {
    const GeometrySrcState& geoSrc = this->getGeomSrc();
    GrAssert(kReserved_GeometrySrcType == geoSrc.fVertexSrc);
    size_t bytes = geoSrc.fVertexCount * VertexSize(geoSrc.fVertexLayout);
    fVertexPool->putBack(bytes);
    --fVertexPoolUseCnt;
}

void GrGpu::releaseReservedIndexSpace() {
    const GeometrySrcState& geoSrc = this->getGeomSrc();
    GrAssert(kReserved_GeometrySrcType == geoSrc.fIndexSrc);
    size_t bytes = geoSrc.fIndexCount * sizeof(uint16_t);
    fIndexPool->putBack(bytes);
    --fIndexPoolUseCnt;
}

void GrGpu::onSetVertexSourceToArray(const void* vertexArray, int vertexCount) {
    this->prepareVertexPool();
    GeometryPoolState& geomPoolState = fGeomPoolStateStack.back();
#if GR_DEBUG
    bool success =
#endif
    fVertexPool->appendVertices(this->getVertexLayout(),
                                vertexCount,
                                vertexArray,
                                &geomPoolState.fPoolVertexBuffer,
                                &geomPoolState.fPoolStartVertex);
    ++fVertexPoolUseCnt;
    GR_DEBUGASSERT(success);
}

void GrGpu::onSetIndexSourceToArray(const void* indexArray, int indexCount) {
    this->prepareIndexPool();
    GeometryPoolState& geomPoolState = fGeomPoolStateStack.back();
#if GR_DEBUG
    bool success =
#endif
    fIndexPool->appendIndices(indexCount,
                              indexArray,
                              &geomPoolState.fPoolIndexBuffer,
                              &geomPoolState.fPoolStartIndex);
    ++fIndexPoolUseCnt;
    GR_DEBUGASSERT(success);
}

void GrGpu::releaseVertexArray() {
    // if vertex source was array, we stowed data in the pool
    const GeometrySrcState& geoSrc = this->getGeomSrc();
    GrAssert(kArray_GeometrySrcType == geoSrc.fVertexSrc);
    size_t bytes = geoSrc.fVertexCount * VertexSize(geoSrc.fVertexLayout);
    fVertexPool->putBack(bytes);
    --fVertexPoolUseCnt;
}

void GrGpu::releaseIndexArray() {
    // if index source was array, we stowed data in the pool
    const GeometrySrcState& geoSrc = this->getGeomSrc();
    GrAssert(kArray_GeometrySrcType == geoSrc.fIndexSrc);
    size_t bytes = geoSrc.fIndexCount * sizeof(uint16_t);
    fIndexPool->putBack(bytes);
    --fIndexPoolUseCnt;
}

////////////////////////////////////////////////////////////////////////////////

const GrGpuStats& GrGpu::getStats() const {
    return fStats;
}

void GrGpu::resetStats() {
    memset(&fStats, 0, sizeof(fStats));
}

void GrGpu::printStats() const {
    if (GR_COLLECT_STATS) {
     GrPrintf(
     "-v-------------------------GPU STATS----------------------------v-\n"
     "Stats collection is: %s\n"
     "Draws: %04d, Verts: %04d, Indices: %04d\n"
     "ProgChanges: %04d, TexChanges: %04d, RTChanges: %04d\n"
     "TexCreates: %04d, RTCreates:%04d\n"
     "-^--------------------------------------------------------------^-\n",
     (GR_COLLECT_STATS ? "ON" : "OFF"),
    fStats.fDrawCnt, fStats.fVertexCnt, fStats.fIndexCnt,
    fStats.fProgChngCnt, fStats.fTextureChngCnt, fStats.fRenderTargetChngCnt,
    fStats.fTextureCreateCnt, fStats.fRenderTargetCreateCnt);
    }
}

