
/*
 * Copyright 2008 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include "SkCanvas.h"
#include "SkBounder.h"
#include "SkDevice.h"
#include "SkDraw.h"
#include "SkDrawFilter.h"
#include "SkDrawLooper.h"
#include "SkPicture.h"
#include "SkRasterClip.h"
#include "SkScalarCompare.h"
#include "SkTemplates.h"
#include "SkTextFormatParams.h"
#include "SkTLazy.h"
#include "SkUtils.h"

//#define SK_TRACE_SAVERESTORE

#ifdef SK_TRACE_SAVERESTORE
    static int gLayerCounter;
    static void inc_layer() { ++gLayerCounter; printf("----- inc layer %d\n", gLayerCounter); }
    static void dec_layer() { --gLayerCounter; printf("----- dec layer %d\n", gLayerCounter); }

    static int gRecCounter;
    static void inc_rec() { ++gRecCounter; printf("----- inc rec %d\n", gRecCounter); }
    static void dec_rec() { --gRecCounter; printf("----- dec rec %d\n", gRecCounter); }

    static int gCanvasCounter;
    static void inc_canvas() { ++gCanvasCounter; printf("----- inc canvas %d\n", gCanvasCounter); }
    static void dec_canvas() { --gCanvasCounter; printf("----- dec canvas %d\n", gCanvasCounter); }
#else
    #define inc_layer()
    #define dec_layer()
    #define inc_rec()
    #define dec_rec()
    #define inc_canvas()
    #define dec_canvas()
#endif

typedef SkTLazy<SkPaint> SkLazyPaint;

///////////////////////////////////////////////////////////////////////////////
// Helpers for computing fast bounds for quickReject tests

static SkCanvas::EdgeType paint2EdgeType(const SkPaint* paint) {
    return paint != NULL && paint->isAntiAlias() ?
            SkCanvas::kAA_EdgeType : SkCanvas::kBW_EdgeType;
}

///////////////////////////////////////////////////////////////////////////////

/*  This is the record we keep for each SkDevice that the user installs.
    The clip/matrix/proc are fields that reflect the top of the save/restore
    stack. Whenever the canvas changes, it marks a dirty flag, and then before
    these are used (assuming we're not on a layer) we rebuild these cache
    values: they reflect the top of the save stack, but translated and clipped
    by the device's XY offset and bitmap-bounds.
*/
struct DeviceCM {
    DeviceCM*           fNext;
    SkDevice*           fDevice;
    SkRasterClip        fClip;
    const SkMatrix*     fMatrix;
    SkPaint*            fPaint; // may be null (in the future)
    // optional, related to canvas' external matrix
    const SkMatrix*     fMVMatrix;
    const SkMatrix*     fExtMatrix;

    DeviceCM(SkDevice* device, int x, int y, const SkPaint* paint)
            : fNext(NULL) {
        if (NULL != device) {
            device->ref();
            device->lockPixels();
        }
        fDevice = device;
        fPaint = paint ? SkNEW_ARGS(SkPaint, (*paint)) : NULL;
    }

    ~DeviceCM() {
        if (NULL != fDevice) {
            fDevice->unlockPixels();
            fDevice->unref();
        }
        SkDELETE(fPaint);
    }

    void updateMC(const SkMatrix& totalMatrix, const SkRasterClip& totalClip,
                  const SkClipStack& clipStack, SkRasterClip* updateClip) {
        int x = fDevice->getOrigin().x();
        int y = fDevice->getOrigin().y();
        int width = fDevice->width();
        int height = fDevice->height();

        if ((x | y) == 0) {
            fMatrix = &totalMatrix;
            fClip = totalClip;
        } else {
            fMatrixStorage = totalMatrix;
            fMatrixStorage.postTranslate(SkIntToScalar(-x),
                                         SkIntToScalar(-y));
            fMatrix = &fMatrixStorage;

            totalClip.translate(-x, -y, &fClip);
        }

        fClip.op(SkIRect::MakeWH(width, height), SkRegion::kIntersect_Op);

        // intersect clip, but don't translate it (yet)

        if (updateClip) {
            updateClip->op(SkIRect::MakeXYWH(x, y, width, height),
                           SkRegion::kDifference_Op);
        }

        fDevice->setMatrixClip(*fMatrix, fClip.forceGetBW(), clipStack);

#ifdef SK_DEBUG
        if (!fClip.isEmpty()) {
            SkIRect deviceR;
            deviceR.set(0, 0, width, height);
            SkASSERT(deviceR.contains(fClip.getBounds()));
        }
#endif
        // default is to assume no external matrix
        fMVMatrix = NULL;
        fExtMatrix = NULL;
    }

    // can only be called after calling updateMC()
    void updateExternalMatrix(const SkMatrix& extM, const SkMatrix& extI) {
        fMVMatrixStorage.setConcat(extI, *fMatrix);
        fMVMatrix = &fMVMatrixStorage;
        fExtMatrix = &extM; // assumes extM has long life-time (owned by canvas)
    }

private:
    SkMatrix    fMatrixStorage, fMVMatrixStorage;
};

/*  This is the record we keep for each save/restore level in the stack.
    Since a level optionally copies the matrix and/or stack, we have pointers
    for these fields. If the value is copied for this level, the copy is
    stored in the ...Storage field, and the pointer points to that. If the
    value is not copied for this level, we ignore ...Storage, and just point
    at the corresponding value in the previous level in the stack.
*/
class SkCanvas::MCRec {
public:
    MCRec*          fNext;
    SkMatrix*       fMatrix;        // points to either fMatrixStorage or prev MCRec
    SkRasterClip*   fRasterClip;    // points to either fRegionStorage or prev MCRec
    SkDrawFilter*   fFilter;        // the current filter (or null)

    DeviceCM*   fLayer;
    /*  If there are any layers in the stack, this points to the top-most
        one that is at or below this level in the stack (so we know what
        bitmap/device to draw into from this level. This value is NOT
        reference counted, since the real owner is either our fLayer field,
        or a previous one in a lower level.)
    */
    DeviceCM*   fTopLayer;

    MCRec(const MCRec* prev, int flags) {
        if (NULL != prev) {
            if (flags & SkCanvas::kMatrix_SaveFlag) {
                fMatrixStorage = *prev->fMatrix;
                fMatrix = &fMatrixStorage;
            } else {
                fMatrix = prev->fMatrix;
            }

            if (flags & SkCanvas::kClip_SaveFlag) {
                fRasterClipStorage = *prev->fRasterClip;
                fRasterClip = &fRasterClipStorage;
            } else {
                fRasterClip = prev->fRasterClip;
            }

            fFilter = prev->fFilter;
            SkSafeRef(fFilter);

            fTopLayer = prev->fTopLayer;
        } else {   // no prev
            fMatrixStorage.reset();

            fMatrix     = &fMatrixStorage;
            fRasterClip = &fRasterClipStorage;
            fFilter     = NULL;
            fTopLayer   = NULL;
        }
        fLayer = NULL;

        // don't bother initializing fNext
        inc_rec();
    }
    ~MCRec() {
        SkSafeUnref(fFilter);
        SkDELETE(fLayer);
        dec_rec();
    }

private:
    SkMatrix        fMatrixStorage;
    SkRasterClip    fRasterClipStorage;
};

class SkDrawIter : public SkDraw {
public:
    SkDrawIter(SkCanvas* canvas, bool skipEmptyClips = true) {
        canvas = canvas->canvasForDrawIter();
        fCanvas = canvas;
        canvas->updateDeviceCMCache();

        fClipStack = &canvas->getTotalClipStack();
        fBounder = canvas->getBounder();
        fCurrLayer = canvas->fMCRec->fTopLayer;
        fSkipEmptyClips = skipEmptyClips;
    }

    bool next() {
        // skip over recs with empty clips
        if (fSkipEmptyClips) {
            while (fCurrLayer && fCurrLayer->fClip.isEmpty()) {
                fCurrLayer = fCurrLayer->fNext;
            }
        }

        const DeviceCM* rec = fCurrLayer;
        if (rec && rec->fDevice) {

            fMatrix = rec->fMatrix;
            fClip   = &((SkRasterClip*)&rec->fClip)->forceGetBW();
            fRC     = &rec->fClip;
            fDevice = rec->fDevice;
            fBitmap = &fDevice->accessBitmap(true);
            fPaint  = rec->fPaint;
            fMVMatrix = rec->fMVMatrix;
            fExtMatrix = rec->fExtMatrix;
            SkDEBUGCODE(this->validate();)

            fCurrLayer = rec->fNext;
            if (fBounder) {
                fBounder->setClip(fClip);
            }
            // fCurrLayer may be NULL now

            fCanvas->prepareForDeviceDraw(fDevice, *fMatrix, *fClip, *fClipStack);
            return true;
        }
        return false;
    }

    SkDevice* getDevice() const { return fDevice; }
    int getX() const { return fDevice->getOrigin().x(); }
    int getY() const { return fDevice->getOrigin().y(); }
    const SkMatrix& getMatrix() const { return *fMatrix; }
    const SkRegion& getClip() const { return *fClip; }
    const SkPaint* getPaint() const { return fPaint; }

private:
    SkCanvas*       fCanvas;
    const DeviceCM* fCurrLayer;
    const SkPaint*  fPaint;     // May be null.
    SkBool8         fSkipEmptyClips;

    typedef SkDraw INHERITED;
};

/////////////////////////////////////////////////////////////////////////////

class AutoDrawLooper {
public:
    AutoDrawLooper(SkCanvas* canvas, const SkPaint& paint,
                   bool skipLayerForImageFilter = false) : fOrigPaint(paint) {
        fCanvas = canvas;
        fLooper = paint.getLooper();
        fFilter = canvas->getDrawFilter();
        fPaint = NULL;
        fSaveCount = canvas->getSaveCount();
        fDoClearImageFilter = false;
        fDone = false;

        if (!skipLayerForImageFilter && fOrigPaint.getImageFilter()) {
            SkPaint tmp;
            tmp.setImageFilter(fOrigPaint.getImageFilter());
            // it would be nice if we had a guess at the bounds, instead of null
            (void)canvas->internalSaveLayer(NULL, &tmp,
                                    SkCanvas::kARGB_ClipLayer_SaveFlag, true);
            // we'll clear the imageFilter for the actual draws in next(), so
            // it will only be applied during the restore().
            fDoClearImageFilter = true;
        }

        if (fLooper) {
            fLooper->init(canvas);
        }
    }

    ~AutoDrawLooper() {
        if (fDoClearImageFilter) {
            fCanvas->internalRestore();
        }
        SkASSERT(fCanvas->getSaveCount() == fSaveCount);
    }

    const SkPaint& paint() const {
        SkASSERT(fPaint);
        return *fPaint;
    }

    bool next(SkDrawFilter::Type drawType);

private:
    SkLazyPaint     fLazyPaint;
    SkCanvas*       fCanvas;
    const SkPaint&  fOrigPaint;
    SkDrawLooper*   fLooper;
    SkDrawFilter*   fFilter;
    const SkPaint*  fPaint;
    int             fSaveCount;
    bool            fDoClearImageFilter;
    bool            fDone;
};

bool AutoDrawLooper::next(SkDrawFilter::Type drawType) {
    fPaint = NULL;
    if (fDone) {
        return false;
    }

    if (fLooper || fFilter || fDoClearImageFilter) {
        SkPaint* paint = fLazyPaint.set(fOrigPaint);

        if (fDoClearImageFilter) {
            paint->setImageFilter(NULL);
        }

        if (fLooper && !fLooper->next(fCanvas, paint)) {
            fDone = true;
            return false;
        }
        if (fFilter) {
            fFilter->filter(paint, drawType);
            if (NULL == fLooper) {
                // no looper means we only draw once
                fDone = true;
            }
        }
        fPaint = paint;

        // if we only came in here for the imagefilter, mark us as done
        if (!fLooper && !fFilter) {
            fDone = true;
        }
    } else {
        fDone = true;
        fPaint = &fOrigPaint;
    }

    // call this after any possible paint modifiers
    if (fPaint->nothingToDraw()) {
        fPaint = NULL;
        return false;
    }
    return true;
}

/*  Stack helper for managing a SkBounder. In the destructor, if we were
    given a bounder, we call its commit() method, signifying that we are
    done accumulating bounds for that draw.
*/
class SkAutoBounderCommit {
public:
    SkAutoBounderCommit(SkBounder* bounder) : fBounder(bounder) {}
    ~SkAutoBounderCommit() {
        if (NULL != fBounder) {
            fBounder->commit();
        }
    }
private:
    SkBounder*  fBounder;
};

#include "SkColorPriv.h"

class AutoValidator {
public:
    AutoValidator(SkDevice* device) : fDevice(device) {}
    ~AutoValidator() {
#ifdef SK_DEBUG
        const SkBitmap& bm = fDevice->accessBitmap(false);
        if (bm.config() == SkBitmap::kARGB_4444_Config) {
            for (int y = 0; y < bm.height(); y++) {
                const SkPMColor16* p = bm.getAddr16(0, y);
                for (int x = 0; x < bm.width(); x++) {
                    SkPMColor16 c = p[x];
                    SkPMColor16Assert(c);
                }
            }
        }
#endif
    }
private:
    SkDevice* fDevice;
};

////////// macros to place around the internal draw calls //////////////////

#define LOOPER_BEGIN_DRAWDEVICE(paint, type)                        \
/*    AutoValidator   validator(fMCRec->fTopLayer->fDevice); */     \
    AutoDrawLooper  looper(this, paint, true);                      \
    while (looper.next(type)) {                                     \
        SkAutoBounderCommit ac(fBounder);                           \
        SkDrawIter          iter(this);

#define LOOPER_BEGIN(paint, type)                                   \
/*    AutoValidator   validator(fMCRec->fTopLayer->fDevice); */     \
    AutoDrawLooper  looper(this, paint);                            \
    while (looper.next(type)) {                                     \
        SkAutoBounderCommit ac(fBounder);                           \
        SkDrawIter          iter(this);

#define LOOPER_END    }

////////////////////////////////////////////////////////////////////////////

SkDevice* SkCanvas::init(SkDevice* device) {
    fBounder = NULL;
    fLocalBoundsCompareType.setEmpty();
    fLocalBoundsCompareTypeDirty = true;
    fLocalBoundsCompareTypeBW.setEmpty();
    fLocalBoundsCompareTypeDirtyBW = true;
    fLastDeviceToGainFocus = NULL;
    fDeviceCMDirty = false;
    fSaveLayerCount = 0;

    fMCRec = (MCRec*)fMCStack.push_back();
    new (fMCRec) MCRec(NULL, 0);

    fMCRec->fLayer = SkNEW_ARGS(DeviceCM, (NULL, 0, 0, NULL));
    fMCRec->fTopLayer = fMCRec->fLayer;
    fMCRec->fNext = NULL;

    fExternalMatrix.reset();
    fExternalInverse.reset();
    fUseExternalMatrix = false;

    return this->setDevice(device);
}

SkCanvas::SkCanvas()
: fMCStack(sizeof(MCRec), fMCRecStorage, sizeof(fMCRecStorage)) {
    inc_canvas();

    this->init(NULL);
}

SkCanvas::SkCanvas(SkDevice* device)
        : fMCStack(sizeof(MCRec), fMCRecStorage, sizeof(fMCRecStorage)) {
    inc_canvas();

    this->init(device);
}

SkCanvas::SkCanvas(const SkBitmap& bitmap)
        : fMCStack(sizeof(MCRec), fMCRecStorage, sizeof(fMCRecStorage)) {
    inc_canvas();

    this->init(SkNEW_ARGS(SkDevice, (bitmap)))->unref();
}

SkCanvas::~SkCanvas() {
    // free up the contents of our deque
    this->restoreToCount(1);    // restore everything but the last
    SkASSERT(0 == fSaveLayerCount);

    this->internalRestore();    // restore the last, since we're going away

    SkSafeUnref(fBounder);

    dec_canvas();
}

SkBounder* SkCanvas::setBounder(SkBounder* bounder) {
    SkRefCnt_SafeAssign(fBounder, bounder);
    return bounder;
}

SkDrawFilter* SkCanvas::getDrawFilter() const {
    return fMCRec->fFilter;
}

SkDrawFilter* SkCanvas::setDrawFilter(SkDrawFilter* filter) {
    SkRefCnt_SafeAssign(fMCRec->fFilter, filter);
    return filter;
}

///////////////////////////////////////////////////////////////////////////////

void SkCanvas::flush() {
    SkDevice* device = this->getDevice();
    if (device) {
        device->flush();
    }
}

SkISize SkCanvas::getDeviceSize() const {
    SkDevice* d = this->getDevice();
    return d ? SkISize::Make(d->width(), d->height()) : SkISize::Make(0, 0);
}

SkDevice* SkCanvas::getDevice() const {
    // return root device
    SkDeque::F2BIter iter(fMCStack);
    MCRec*           rec = (MCRec*)iter.next();
    SkASSERT(rec && rec->fLayer);
    return rec->fLayer->fDevice;
}

SkDevice* SkCanvas::getTopDevice(bool updateMatrixClip) const {
    if (updateMatrixClip) {
        const_cast<SkCanvas*>(this)->updateDeviceCMCache();
    }
    return fMCRec->fTopLayer->fDevice;
}

SkDevice* SkCanvas::setDevice(SkDevice* device) {
    // return root device
    SkDeque::F2BIter iter(fMCStack);
    MCRec*           rec = (MCRec*)iter.next();
    SkASSERT(rec && rec->fLayer);
    SkDevice*       rootDevice = rec->fLayer->fDevice;

    if (rootDevice == device) {
        return device;
    }

    /* Notify the devices that they are going in/out of scope, so they can do
       things like lock/unlock their pixels, etc.
    */
    if (device) {
        device->lockPixels();
    }
    if (rootDevice) {
        rootDevice->unlockPixels();
    }

    SkRefCnt_SafeAssign(rec->fLayer->fDevice, device);
    rootDevice = device;

    fDeviceCMDirty = true;

    /*  Now we update our initial region to have the bounds of the new device,
        and then intersect all of the clips in our stack with these bounds,
        to ensure that we can't draw outside of the device's bounds (and trash
                                                                     memory).

    NOTE: this is only a partial-fix, since if the new device is larger than
        the previous one, we don't know how to "enlarge" the clips in our stack,
        so drawing may be artificially restricted. Without keeping a history of
        all calls to canvas->clipRect() and canvas->clipPath(), we can't exactly
        reconstruct the correct clips, so this approximation will have to do.
        The caller really needs to restore() back to the base if they want to
        accurately take advantage of the new device bounds.
    */

    SkIRect bounds;
    if (device) {
        bounds.set(0, 0, device->width(), device->height());
    } else {
        bounds.setEmpty();
    }
    // now jam our 1st clip to be bounds, and intersect the rest with that
    rec->fRasterClip->setRect(bounds);
    while ((rec = (MCRec*)iter.next()) != NULL) {
        (void)rec->fRasterClip->op(bounds, SkRegion::kIntersect_Op);
    }

    return device;
}

SkDevice* SkCanvas::setBitmapDevice(const SkBitmap& bitmap) {
    SkDevice* device = this->setDevice(SkNEW_ARGS(SkDevice, (bitmap)));
    device->unref();
    return device;
}

bool SkCanvas::readPixels(SkBitmap* bitmap,
                          int x, int y,
                          Config8888 config8888) {
    SkDevice* device = this->getDevice();
    if (!device) {
        return false;
    }
    return device->readPixels(bitmap, x, y, config8888);
}

bool SkCanvas::readPixels(const SkIRect& srcRect, SkBitmap* bitmap) {
    SkDevice* device = this->getDevice();

    SkIRect bounds;
    bounds.set(0, 0, device->width(), device->height());
    if (!bounds.intersect(srcRect)) {
        return false;
    }

    SkBitmap tmp;
    tmp.setConfig(SkBitmap::kARGB_8888_Config, bounds.width(),
                                               bounds.height());
    if (this->readPixels(&tmp, bounds.fLeft, bounds.fTop)) {
        bitmap->swap(tmp);
        return true;
    } else {
        return false;
    }
}

void SkCanvas::writePixels(const SkBitmap& bitmap, int x, int y,
                           Config8888 config8888) {
    SkDevice* device = this->getDevice();
    if (device) {
        device->writePixels(bitmap, x, y, config8888);
    }
}

SkCanvas* SkCanvas::canvasForDrawIter() {
    return this;
}

//////////////////////////////////////////////////////////////////////////////

void SkCanvas::updateDeviceCMCache() {
    if (fDeviceCMDirty) {
        const SkMatrix& totalMatrix = this->getTotalMatrix();
        const SkRasterClip& totalClip = *fMCRec->fRasterClip;
        DeviceCM*       layer = fMCRec->fTopLayer;

        if (NULL == layer->fNext) {   // only one layer
            layer->updateMC(totalMatrix, totalClip, fClipStack, NULL);
            if (fUseExternalMatrix) {
                layer->updateExternalMatrix(fExternalMatrix,
                                            fExternalInverse);
            }
        } else {
            SkRasterClip clip(totalClip);
            do {
                layer->updateMC(totalMatrix, clip, fClipStack, &clip);
                if (fUseExternalMatrix) {
                    layer->updateExternalMatrix(fExternalMatrix,
                                                fExternalInverse);
                }
            } while ((layer = layer->fNext) != NULL);
        }
        fDeviceCMDirty = false;
    }
}

void SkCanvas::prepareForDeviceDraw(SkDevice* device, const SkMatrix& matrix,
                                    const SkRegion& clip,
                                    const SkClipStack& clipStack) {
    SkASSERT(device);
    if (fLastDeviceToGainFocus != device) {
        device->gainFocus(this, matrix, clip, clipStack);
        fLastDeviceToGainFocus = device;
    }
}

///////////////////////////////////////////////////////////////////////////////

int SkCanvas::internalSave(SaveFlags flags) {
    int saveCount = this->getSaveCount(); // record this before the actual save

    MCRec* newTop = (MCRec*)fMCStack.push_back();
    new (newTop) MCRec(fMCRec, flags);    // balanced in restore()

    newTop->fNext = fMCRec;
    fMCRec = newTop;

    fClipStack.save();
    SkASSERT(fClipStack.getSaveCount() == this->getSaveCount() - 1);

    return saveCount;
}

int SkCanvas::save(SaveFlags flags) {
    // call shared impl
    return this->internalSave(flags);
}

#define C32MASK (1 << SkBitmap::kARGB_8888_Config)
#define C16MASK (1 << SkBitmap::kRGB_565_Config)
#define C8MASK  (1 << SkBitmap::kA8_Config)

static SkBitmap::Config resolve_config(SkCanvas* canvas,
                                       const SkIRect& bounds,
                                       SkCanvas::SaveFlags flags,
                                       bool* isOpaque) {
    *isOpaque = (flags & SkCanvas::kHasAlphaLayer_SaveFlag) == 0;

#if 0
    // loop through and union all the configs we may draw into
    uint32_t configMask = 0;
    for (int i = canvas->countLayerDevices() - 1; i >= 0; --i)
    {
        SkDevice* device = canvas->getLayerDevice(i);
        if (device->intersects(bounds))
            configMask |= 1 << device->config();
    }

    // if the caller wants alpha or fullcolor, we can't return 565
    if (flags & (SkCanvas::kFullColorLayer_SaveFlag |
                 SkCanvas::kHasAlphaLayer_SaveFlag))
        configMask &= ~C16MASK;

    switch (configMask) {
    case C8MASK:    // if we only have A8, return that
        return SkBitmap::kA8_Config;

    case C16MASK:   // if we only have 565, return that
        return SkBitmap::kRGB_565_Config;

    default:
        return SkBitmap::kARGB_8888_Config; // default answer
    }
#else
    return SkBitmap::kARGB_8888_Config; // default answer
#endif
}

static bool bounds_affects_clip(SkCanvas::SaveFlags flags) {
    return (flags & SkCanvas::kClipToLayer_SaveFlag) != 0;
}

bool SkCanvas::clipRectBounds(const SkRect* bounds, SaveFlags flags,
                               SkIRect* intersection) {
    SkIRect clipBounds;
    if (!this->getClipDeviceBounds(&clipBounds)) {
        return false;
    }
    SkIRect ir;
    if (NULL != bounds) {
        SkRect r;

        this->getTotalMatrix().mapRect(&r, *bounds);
        r.roundOut(&ir);
        // early exit if the layer's bounds are clipped out
        if (!ir.intersect(clipBounds)) {
            if (bounds_affects_clip(flags)) {
                fMCRec->fRasterClip->setEmpty();
            }
            return false;
        }
    } else {    // no user bounds, so just use the clip
        ir = clipBounds;
    }

    fClipStack.clipDevRect(ir, SkRegion::kIntersect_Op);

    // early exit if the clip is now empty
    if (bounds_affects_clip(flags) &&
        !fMCRec->fRasterClip->op(ir, SkRegion::kIntersect_Op)) {
        return false;
    }

    if (intersection) {
        *intersection = ir;
    }
    return true;
}

int SkCanvas::saveLayer(const SkRect* bounds, const SkPaint* paint,
                        SaveFlags flags) {
    return this->internalSaveLayer(bounds, paint, flags, false);
}

int SkCanvas::internalSaveLayer(const SkRect* bounds, const SkPaint* paint,
                                SaveFlags flags, bool justForImageFilter) {
    // do this before we create the layer. We don't call the public save() since
    // that would invoke a possibly overridden virtual
    int count = this->internalSave(flags);

    fDeviceCMDirty = true;

    SkIRect ir;
    if (!this->clipRectBounds(bounds, flags, &ir)) {
        return count;
    }

    // Kill the imagefilter if our device doesn't allow it
    SkLazyPaint lazyP;
    if (paint && paint->getImageFilter()) {
        if (!this->getTopDevice()->allowImageFilter(paint->getImageFilter())) {
            if (justForImageFilter) {
                // early exit if the layer was just for the imageFilter
                return count;
            }
            SkPaint* p = lazyP.set(*paint);
            p->setImageFilter(NULL);
            paint = p;
        }
    }

    bool isOpaque;
    SkBitmap::Config config = resolve_config(this, ir, flags, &isOpaque);

    SkDevice* device;
    if (paint && paint->getImageFilter()) {
        device = this->createCompatibleDevice(config, ir.width(), ir.height(),
                                              isOpaque);
    } else {
        device = this->createLayerDevice(config, ir.width(), ir.height(),
                                         isOpaque);
    }
    if (NULL == device) {
        SkDebugf("Unable to create device for layer.");
        return count;
    }

    device->setOrigin(ir.fLeft, ir.fTop);
    DeviceCM* layer = SkNEW_ARGS(DeviceCM, (device, ir.fLeft, ir.fTop, paint));
    device->unref();

    layer->fNext = fMCRec->fTopLayer;
    fMCRec->fLayer = layer;
    fMCRec->fTopLayer = layer;    // this field is NOT an owner of layer

    fSaveLayerCount += 1;
    return count;
}

int SkCanvas::saveLayerAlpha(const SkRect* bounds, U8CPU alpha,
                             SaveFlags flags) {
    if (0xFF == alpha) {
        return this->saveLayer(bounds, NULL, flags);
    } else {
        SkPaint tmpPaint;
        tmpPaint.setAlpha(alpha);
        return this->saveLayer(bounds, &tmpPaint, flags);
    }
}

void SkCanvas::restore() {
    // check for underflow
    if (fMCStack.count() > 1) {
        this->internalRestore();
    }
}

void SkCanvas::internalRestore() {
    SkASSERT(fMCStack.count() != 0);

    fDeviceCMDirty = true;
    fLocalBoundsCompareTypeDirty = true;
    fLocalBoundsCompareTypeDirtyBW = true;

    fClipStack.restore();
    // reserve our layer (if any)
    DeviceCM* layer = fMCRec->fLayer;   // may be null
    // now detach it from fMCRec so we can pop(). Gets freed after its drawn
    fMCRec->fLayer = NULL;

    // now do the normal restore()
    fMCRec->~MCRec();       // balanced in save()
    fMCStack.pop_back();
    fMCRec = (MCRec*)fMCStack.back();

    /*  Time to draw the layer's offscreen. We can't call the public drawSprite,
        since if we're being recorded, we don't want to record this (the
        recorder will have already recorded the restore).
    */
    if (NULL != layer) {
        if (layer->fNext) {
            const SkIPoint& origin = layer->fDevice->getOrigin();
            this->internalDrawDevice(layer->fDevice, origin.x(), origin.y(),
                                     layer->fPaint);
            // reset this, since internalDrawDevice will have set it to true
            fDeviceCMDirty = true;

            SkASSERT(fSaveLayerCount > 0);
            fSaveLayerCount -= 1;
        }
        SkDELETE(layer);
    }

    SkASSERT(fClipStack.getSaveCount() == this->getSaveCount() - 1);
}

int SkCanvas::getSaveCount() const {
    return fMCStack.count();
}

void SkCanvas::restoreToCount(int count) {
    // sanity check
    if (count < 1) {
        count = 1;
    }

    int n = this->getSaveCount() - count;
    for (int i = 0; i < n; ++i) {
        this->restore();
    }
}

bool SkCanvas::isDrawingToLayer() const {
    return fSaveLayerCount > 0;
}

/////////////////////////////////////////////////////////////////////////////

// can't draw it if its empty, or its too big for a fixed-point width or height
static bool reject_bitmap(const SkBitmap& bitmap) {
    return  bitmap.width() <= 0 || bitmap.height() <= 0
#ifndef SK_ALLOW_OVER_32K_BITMAPS
            || bitmap.width() > 32767 || bitmap.height() > 32767
#endif
            ;
}

void SkCanvas::internalDrawBitmap(const SkBitmap& bitmap, const SkIRect* srcRect,
                                const SkMatrix& matrix, const SkPaint* paint) {
    if (reject_bitmap(bitmap)) {
        return;
    }

    SkLazyPaint lazy;
    if (NULL == paint) {
        paint = lazy.init();
    }
    this->commonDrawBitmap(bitmap, srcRect, matrix, *paint);
}

#include "SkImageFilter.h"

class DeviceImageFilterProxy : public SkImageFilter::Proxy {
public:
    DeviceImageFilterProxy(SkDevice* device) : fDevice(device) {}

    virtual SkDevice* createDevice(int w, int h) SK_OVERRIDE {
        return fDevice->createCompatibleDevice(SkBitmap::kARGB_8888_Config,
                                               w, h, false);
    }
    virtual bool canHandleImageFilter(SkImageFilter* filter) SK_OVERRIDE {
        return fDevice->canHandleImageFilter(filter);
    }
    virtual bool filterImage(SkImageFilter* filter, const SkBitmap& src,
                             const SkMatrix& ctm,
                             SkBitmap* result, SkIPoint* offset) SK_OVERRIDE {
        return fDevice->filterImage(filter, src, ctm, result, offset);
    }
    
private:
    SkDevice* fDevice;
};

void SkCanvas::internalDrawDevice(SkDevice* srcDev, int x, int y,
                                  const SkPaint* paint) {
    SkPaint tmp;
    if (NULL == paint) {
        tmp.setDither(true);
        paint = &tmp;
    }

    LOOPER_BEGIN_DRAWDEVICE(*paint, SkDrawFilter::kBitmap_Type)
    while (iter.next()) {
        SkDevice* dstDev = iter.fDevice;
        paint = &looper.paint();
        SkImageFilter* filter = paint->getImageFilter();
        SkIPoint pos = { x - iter.getX(), y - iter.getY() };
        if (filter && !dstDev->canHandleImageFilter(filter)) {
            DeviceImageFilterProxy proxy(dstDev);
            SkBitmap dst;
            const SkBitmap& src = srcDev->accessBitmap(false);
            if (filter->filterImage(&proxy, src, *iter.fMatrix, &dst, &pos)) {
                SkPaint tmp(*paint);
                tmp.setImageFilter(NULL);
                dstDev->drawSprite(iter, dst, pos.x(), pos.y(), tmp);
            }
        } else {
            dstDev->drawDevice(iter, srcDev, pos.x(), pos.y(), *paint);
        }
    }
    LOOPER_END
}

void SkCanvas::drawSprite(const SkBitmap& bitmap, int x, int y,
                          const SkPaint* paint) {
    SkDEBUGCODE(bitmap.validate();)
    
    if (reject_bitmap(bitmap)) {
        return;
    }
    
    SkPaint tmp;
    if (NULL == paint) {
        paint = &tmp;
    }
    
    LOOPER_BEGIN_DRAWDEVICE(*paint, SkDrawFilter::kBitmap_Type)
    
    while (iter.next()) {
        paint = &looper.paint();
        SkImageFilter* filter = paint->getImageFilter();
        SkIPoint pos = { x - iter.getX(), y - iter.getY() };
        if (filter && !iter.fDevice->canHandleImageFilter(filter)) {
            DeviceImageFilterProxy proxy(iter.fDevice);
            SkBitmap dst;
            if (filter->filterImage(&proxy, bitmap, *iter.fMatrix, &dst, &pos)) {
                SkPaint tmp(*paint);
                tmp.setImageFilter(NULL);
                iter.fDevice->drawSprite(iter, dst, pos.x(), pos.y(), tmp);
            }
        } else {
            iter.fDevice->drawSprite(iter, bitmap, pos.x(), pos.y(), *paint);
        }
    }
    LOOPER_END
}

/////////////////////////////////////////////////////////////////////////////

bool SkCanvas::translate(SkScalar dx, SkScalar dy) {
    fDeviceCMDirty = true;
    fLocalBoundsCompareTypeDirty = true;
    fLocalBoundsCompareTypeDirtyBW = true;
    return fMCRec->fMatrix->preTranslate(dx, dy);
}

bool SkCanvas::scale(SkScalar sx, SkScalar sy) {
    fDeviceCMDirty = true;
    fLocalBoundsCompareTypeDirty = true;
    fLocalBoundsCompareTypeDirtyBW = true;
    return fMCRec->fMatrix->preScale(sx, sy);
}

bool SkCanvas::rotate(SkScalar degrees) {
    fDeviceCMDirty = true;
    fLocalBoundsCompareTypeDirty = true;
    fLocalBoundsCompareTypeDirtyBW = true;
    return fMCRec->fMatrix->preRotate(degrees);
}

bool SkCanvas::skew(SkScalar sx, SkScalar sy) {
    fDeviceCMDirty = true;
    fLocalBoundsCompareTypeDirty = true;
    fLocalBoundsCompareTypeDirtyBW = true;
    return fMCRec->fMatrix->preSkew(sx, sy);
}

bool SkCanvas::concat(const SkMatrix& matrix) {
    fDeviceCMDirty = true;
    fLocalBoundsCompareTypeDirty = true;
    fLocalBoundsCompareTypeDirtyBW = true;
    return fMCRec->fMatrix->preConcat(matrix);
}

void SkCanvas::setMatrix(const SkMatrix& matrix) {
    fDeviceCMDirty = true;
    fLocalBoundsCompareTypeDirty = true;
    fLocalBoundsCompareTypeDirtyBW = true;
    *fMCRec->fMatrix = matrix;
}

// this is not virtual, so it must call a virtual method so that subclasses
// will see its action
void SkCanvas::resetMatrix() {
    SkMatrix matrix;

    matrix.reset();
    this->setMatrix(matrix);
}

//////////////////////////////////////////////////////////////////////////////

bool SkCanvas::clipRect(const SkRect& rect, SkRegion::Op op, bool doAA) {
    AutoValidateClip avc(this);

    fDeviceCMDirty = true;
    fLocalBoundsCompareTypeDirty = true;
    fLocalBoundsCompareTypeDirtyBW = true;

    if (fMCRec->fMatrix->rectStaysRect()) {
        // for these simpler matrices, we can stay a rect ever after applying
        // the matrix. This means we don't have to a) make a path, and b) tell
        // the region code to scan-convert the path, only to discover that it
        // is really just a rect.
        SkRect      r;

        fMCRec->fMatrix->mapRect(&r, rect);
        fClipStack.clipDevRect(r, op, doAA);
        return fMCRec->fRasterClip->op(r, op, doAA);
    } else {
        // since we're rotate or some such thing, we convert the rect to a path
        // and clip against that, since it can handle any matrix. However, to
        // avoid recursion in the case where we are subclassed (e.g. Pictures)
        // we explicitly call "our" version of clipPath.
        SkPath  path;

        path.addRect(rect);
        return this->SkCanvas::clipPath(path, op, doAA);
    }
}

static bool clipPathHelper(const SkCanvas* canvas, SkRasterClip* currClip,
                           const SkPath& devPath, SkRegion::Op op, bool doAA) {
    // base is used to limit the size (and therefore memory allocation) of the
    // region that results from scan converting devPath.
    SkRegion base;

    if (SkRegion::kIntersect_Op == op) {
        // since we are intersect, we can do better (tighter) with currRgn's
        // bounds, than just using the device. However, if currRgn is complex,
        // our region blitter may hork, so we do that case in two steps.
        if (currClip->isRect()) {
            return currClip->setPath(devPath, *currClip, doAA);
        } else {
            base.setRect(currClip->getBounds());
            SkRasterClip clip;
            clip.setPath(devPath, base, doAA);
            return currClip->op(clip, op);
        }
    } else {
        const SkDevice* device = canvas->getDevice();
        base.setRect(0, 0, device->width(), device->height());

        if (SkRegion::kReplace_Op == op) {
            return currClip->setPath(devPath, base, doAA);
        } else {
            SkRasterClip clip;
            clip.setPath(devPath, base, doAA);
            return currClip->op(clip, op);
        }
    }
}

bool SkCanvas::clipPath(const SkPath& path, SkRegion::Op op, bool doAA) {
    AutoValidateClip avc(this);

    fDeviceCMDirty = true;
    fLocalBoundsCompareTypeDirty = true;
    fLocalBoundsCompareTypeDirtyBW = true;

    SkPath devPath;
    path.transform(*fMCRec->fMatrix, &devPath);

    // Check if the transfomation, or the original path itself
    // made us empty. Note this can also happen if we contained NaN
    // values. computing the bounds detects this, and will set our
    // bounds to empty if that is the case. (see SkRect::set(pts, count))
    if (devPath.getBounds().isEmpty()) {
        // resetting the path will remove any NaN or other wanky values
        // that might upset our scan converter.
        devPath.reset();
    }

    // if we called path.swap() we could avoid a deep copy of this path
    fClipStack.clipDevPath(devPath, op, doAA);

    return clipPathHelper(this, fMCRec->fRasterClip, devPath, op, doAA);
}

bool SkCanvas::clipRegion(const SkRegion& rgn, SkRegion::Op op) {
    AutoValidateClip avc(this);

    fDeviceCMDirty = true;
    fLocalBoundsCompareTypeDirty = true;
    fLocalBoundsCompareTypeDirtyBW = true;

    // todo: signal fClipStack that we have a region, and therefore (I guess)
    // we have to ignore it, and use the region directly?
    fClipStack.clipDevRect(rgn.getBounds());

    return fMCRec->fRasterClip->op(rgn, op);
}

#ifdef SK_DEBUG
void SkCanvas::validateClip() const {
    // construct clipRgn from the clipstack
    const SkDevice* device = this->getDevice();
    SkIRect ir;
    ir.set(0, 0, device->width(), device->height());
    SkRasterClip tmpClip(ir);

    SkClipStack::B2FIter                iter(fClipStack);
    const SkClipStack::B2FIter::Clip*   clip;
    while ((clip = iter.next()) != NULL) {
        if (clip->fPath) {
            clipPathHelper(this, &tmpClip, *clip->fPath, clip->fOp, clip->fDoAA);
        } else if (clip->fRect) {
            clip->fRect->round(&ir);
            tmpClip.op(ir, clip->fOp);
        } else {
            tmpClip.setEmpty();
        }
    }

#if 0   // enable this locally for testing
    // now compare against the current rgn
    const SkRegion& rgn = this->getTotalClip();
    SkASSERT(rgn == tmpClip);
#endif
}
#endif

///////////////////////////////////////////////////////////////////////////////

void SkCanvas::computeLocalClipBoundsCompareType(EdgeType et) const {
    SkRect r;
    SkRectCompareType& rCompare = et == kAA_EdgeType ? fLocalBoundsCompareType :
            fLocalBoundsCompareTypeBW;

    if (!this->getClipBounds(&r, et)) {
        rCompare.setEmpty();
    } else {
        rCompare.set(SkScalarToCompareType(r.fLeft),
                     SkScalarToCompareType(r.fTop),
                     SkScalarToCompareType(r.fRight),
                     SkScalarToCompareType(r.fBottom));
    }
}

/*  current impl ignores edgetype, and relies on
    getLocalClipBoundsCompareType(), which always returns a value assuming
    antialiasing (worst case)
 */
bool SkCanvas::quickReject(const SkRect& rect, EdgeType et) const {

    if (!rect.isFinite())
        return true;

    if (fMCRec->fRasterClip->isEmpty()) {
        return true;
    }

    if (fMCRec->fMatrix->hasPerspective()) {
        SkRect dst;
        fMCRec->fMatrix->mapRect(&dst, rect);
        SkIRect idst;
        dst.roundOut(&idst);
        return !SkIRect::Intersects(idst, fMCRec->fRasterClip->getBounds());
    } else {
        const SkRectCompareType& clipR = this->getLocalClipBoundsCompareType(et);

        // for speed, do the most likely reject compares first
        SkScalarCompareType userT = SkScalarToCompareType(rect.fTop);
        SkScalarCompareType userB = SkScalarToCompareType(rect.fBottom);
        if (userT >= clipR.fBottom || userB <= clipR.fTop) {
            return true;
        }
        SkScalarCompareType userL = SkScalarToCompareType(rect.fLeft);
        SkScalarCompareType userR = SkScalarToCompareType(rect.fRight);
        if (userL >= clipR.fRight || userR <= clipR.fLeft) {
            return true;
        }
        return false;
    }
}

bool SkCanvas::quickReject(const SkPath& path, EdgeType et) const {
    return path.isEmpty() || this->quickReject(path.getBounds(), et);
}

static inline int pinIntForScalar(int x) {
#ifdef SK_SCALAR_IS_FIXED
    if (x < SK_MinS16) {
        x = SK_MinS16;
    } else if (x > SK_MaxS16) {
        x = SK_MaxS16;
    }
#endif
    return x;
}

bool SkCanvas::getClipBounds(SkRect* bounds, EdgeType et) const {
    SkIRect ibounds;
    if (!getClipDeviceBounds(&ibounds)) {
        return false;
    }

    SkMatrix inverse;
    // if we can't invert the CTM, we can't return local clip bounds
    if (!fMCRec->fMatrix->invert(&inverse)) {
        if (bounds) {
            bounds->setEmpty();
        }
        return false;
    }

    if (NULL != bounds) {
        SkRect r;
        // adjust it outwards if we are antialiasing
        int inset = (kAA_EdgeType == et);

        // SkRect::iset() will correctly assert if we pass a value out of range
        // (when SkScalar==fixed), so we pin to legal values. This does not
        // really returnt the correct answer, but its the best we can do given
        // that we've promised to return SkRect (even though we support devices
        // that can be larger than 32K in width or height).
        r.iset(pinIntForScalar(ibounds.fLeft - inset),
               pinIntForScalar(ibounds.fTop - inset),
               pinIntForScalar(ibounds.fRight + inset), 
               pinIntForScalar(ibounds.fBottom + inset));
        inverse.mapRect(bounds, r);
    }
    return true;
}

bool SkCanvas::getClipDeviceBounds(SkIRect* bounds) const {
    const SkRasterClip& clip = *fMCRec->fRasterClip;
    if (clip.isEmpty()) {
        if (bounds) {
            bounds->setEmpty();
        }
        return false;
    }

    if (NULL != bounds) {
        *bounds = clip.getBounds();
    }
    return true;
}

const SkMatrix& SkCanvas::getTotalMatrix() const {
    return *fMCRec->fMatrix;
}

SkCanvas::ClipType SkCanvas::getClipType() const {
    if (fMCRec->fRasterClip->isEmpty()) return kEmpty_ClipType;
    if (fMCRec->fRasterClip->isRect()) return kRect_ClipType;
    return kComplex_ClipType;
}

const SkRegion& SkCanvas::getTotalClip() const {
    return fMCRec->fRasterClip->forceGetBW();
}

const SkClipStack& SkCanvas::getTotalClipStack() const {
    return fClipStack;
}

void SkCanvas::setExternalMatrix(const SkMatrix* matrix) {
    if (NULL == matrix || matrix->isIdentity()) {
        if (fUseExternalMatrix) {
            fDeviceCMDirty = true;
        }
        fUseExternalMatrix = false;
    } else {
        fUseExternalMatrix = true;
        fDeviceCMDirty = true;  // |= (fExternalMatrix != *matrix)

        fExternalMatrix = *matrix;
        matrix->invert(&fExternalInverse);
    }
}

SkDevice* SkCanvas::createLayerDevice(SkBitmap::Config config,
                                      int width, int height,
                                      bool isOpaque) {
    SkDevice* device = this->getTopDevice();
    if (device) {
        return device->createCompatibleDeviceForSaveLayer(config, width, height,
                                                          isOpaque);
    } else {
        return NULL;
    }
}

SkDevice* SkCanvas::createCompatibleDevice(SkBitmap::Config config,
                                           int width, int height,
                                           bool isOpaque) {
    SkDevice* device = this->getDevice();
    if (device) {
        return device->createCompatibleDevice(config, width, height, isOpaque);
    } else {
        return NULL;
    }
}


//////////////////////////////////////////////////////////////////////////////
//  These are the virtual drawing methods
//////////////////////////////////////////////////////////////////////////////

void SkCanvas::clear(SkColor color) {
    SkDrawIter  iter(this);

    while (iter.next()) {
        iter.fDevice->clear(color);
    }
}

void SkCanvas::drawPaint(const SkPaint& paint) {
    this->internalDrawPaint(paint);
}

void SkCanvas::internalDrawPaint(const SkPaint& paint) {
    LOOPER_BEGIN(paint, SkDrawFilter::kPaint_Type)

    while (iter.next()) {
        iter.fDevice->drawPaint(iter, looper.paint());
    }

    LOOPER_END
}

void SkCanvas::drawPoints(PointMode mode, size_t count, const SkPoint pts[],
                          const SkPaint& paint) {
    if ((long)count <= 0) {
        return;
    }

    SkASSERT(pts != NULL);

    LOOPER_BEGIN(paint, SkDrawFilter::kPoint_Type)

    while (iter.next()) {
        iter.fDevice->drawPoints(iter, mode, count, pts, looper.paint());
    }

    LOOPER_END
}

void SkCanvas::drawRect(const SkRect& r, const SkPaint& paint) {
    if (paint.canComputeFastBounds()) {
        SkRect storage;
        if (this->quickReject(paint.computeFastBounds(r, &storage),
                              paint2EdgeType(&paint))) {
            return;
        }
    }

    LOOPER_BEGIN(paint, SkDrawFilter::kRect_Type)

    while (iter.next()) {
        iter.fDevice->drawRect(iter, r, looper.paint());
    }

    LOOPER_END
}

void SkCanvas::drawPath(const SkPath& path, const SkPaint& paint) {
    if (!path.isInverseFillType() && paint.canComputeFastBounds()) {
        SkRect storage;
        const SkRect& bounds = path.getBounds();
        if (this->quickReject(paint.computeFastBounds(bounds, &storage),
                              paint2EdgeType(&paint))) {
            return;
        }
    }
    if (path.isEmpty()) {
        if (path.isInverseFillType()) {
            this->internalDrawPaint(paint);
        }
        return;
    }

    LOOPER_BEGIN(paint, SkDrawFilter::kPath_Type)

    while (iter.next()) {
        iter.fDevice->drawPath(iter, path, looper.paint());
    }

    LOOPER_END
}

void SkCanvas::drawBitmap(const SkBitmap& bitmap, SkScalar x, SkScalar y,
                          const SkPaint* paint) {
    SkDEBUGCODE(bitmap.validate();)

    if (NULL == paint || paint->canComputeFastBounds()) {
        SkRect bounds = {
            x, y,
            x + SkIntToScalar(bitmap.width()),
            y + SkIntToScalar(bitmap.height())
        };
        if (paint) {
            (void)paint->computeFastBounds(bounds, &bounds);
        }
        if (this->quickReject(bounds, paint2EdgeType(paint))) {
            return;
        }
    }

    SkMatrix matrix;
    matrix.setTranslate(x, y);
    this->internalDrawBitmap(bitmap, NULL, matrix, paint);
}

// this one is non-virtual, so it can be called safely by other canvas apis
void SkCanvas::internalDrawBitmapRect(const SkBitmap& bitmap, const SkIRect* src,
                                      const SkRect& dst, const SkPaint* paint) {
    if (bitmap.width() == 0 || bitmap.height() == 0 || dst.isEmpty()) {
        return;
    }

    // do this now, to avoid the cost of calling extract for RLE bitmaps
    if (NULL == paint || paint->canComputeFastBounds()) {
        SkRect storage;
        const SkRect* bounds = &dst;
        if (paint) {
            bounds = &paint->computeFastBounds(dst, &storage);
        }
        if (this->quickReject(*bounds, paint2EdgeType(paint))) {
            return;
        }
    }

    const SkBitmap* bitmapPtr = &bitmap;

    SkMatrix matrix;
    SkRect tmpSrc;
    if (src) {
        tmpSrc.set(*src);
        // if the extract process clipped off the top or left of the
        // original, we adjust for that here to get the position right.
        if (tmpSrc.fLeft > 0) {
            tmpSrc.fRight -= tmpSrc.fLeft;
            tmpSrc.fLeft = 0;
        }
        if (tmpSrc.fTop > 0) {
            tmpSrc.fBottom -= tmpSrc.fTop;
            tmpSrc.fTop = 0;
        }
    } else {
        tmpSrc.set(0, 0, SkIntToScalar(bitmap.width()),
                   SkIntToScalar(bitmap.height()));
    }
    matrix.setRectToRect(tmpSrc, dst, SkMatrix::kFill_ScaleToFit);

    // ensure that src is "valid" before we pass it to our internal routines
    // and to SkDevice. i.e. sure it is contained inside the original bitmap.
    SkIRect tmpISrc;
    if (src) {
        tmpISrc.set(0, 0, bitmap.width(), bitmap.height());
        if (!tmpISrc.intersect(*src)) {
            return;
        }
        src = &tmpISrc;
    }
    this->internalDrawBitmap(*bitmapPtr, src, matrix, paint);
}

void SkCanvas::drawBitmapRect(const SkBitmap& bitmap, const SkIRect* src,
                              const SkRect& dst, const SkPaint* paint) {
    SkDEBUGCODE(bitmap.validate();)
    this->internalDrawBitmapRect(bitmap, src, dst, paint);
}

void SkCanvas::drawBitmapMatrix(const SkBitmap& bitmap, const SkMatrix& matrix,
                                const SkPaint* paint) {
    SkDEBUGCODE(bitmap.validate();)
    this->internalDrawBitmap(bitmap, NULL, matrix, paint);
}

void SkCanvas::commonDrawBitmap(const SkBitmap& bitmap, const SkIRect* srcRect,
                                const SkMatrix& matrix, const SkPaint& paint) {
    SkDEBUGCODE(bitmap.validate();)

    LOOPER_BEGIN(paint, SkDrawFilter::kBitmap_Type)

    while (iter.next()) {
        iter.fDevice->drawBitmap(iter, bitmap, srcRect, matrix, looper.paint());
    }

    LOOPER_END
}

void SkCanvas::internalDrawBitmapNine(const SkBitmap& bitmap,
                                      const SkIRect& center, const SkRect& dst,
                                      const SkPaint* paint) {
    if (NULL == paint || paint->canComputeFastBounds()) {
        SkRect storage;
        const SkRect* bounds = &dst;
        if (paint) {
            bounds = &paint->computeFastBounds(dst, &storage);
        }
        if (this->quickReject(*bounds, paint2EdgeType(paint))) {
            return;
        }
    }

    const int32_t w = bitmap.width();
    const int32_t h = bitmap.height();

    SkIRect c = center;
    // pin center to the bounds of the bitmap
    c.fLeft = SkMax32(0, center.fLeft);
    c.fTop = SkMax32(0, center.fTop);
    c.fRight = SkPin32(center.fRight, c.fLeft, w);
    c.fBottom = SkPin32(center.fBottom, c.fTop, h);

    const int32_t srcX[4] = { 0, c.fLeft, c.fRight, w };
    const int32_t srcY[4] = { 0, c.fTop, c.fBottom, h };
    SkScalar dstX[4] = {
        dst.fLeft, dst.fLeft + SkIntToScalar(c.fLeft),
        dst.fRight - SkIntToScalar(w - c.fRight), dst.fRight
    };
    SkScalar dstY[4] = {
        dst.fTop, dst.fTop + SkIntToScalar(c.fTop),
        dst.fBottom - SkIntToScalar(h - c.fBottom), dst.fBottom
    };

    if (dstX[1] > dstX[2]) {
        dstX[1] = dstX[0] + (dstX[3] - dstX[0]) * c.fLeft / (w - c.width());
        dstX[2] = dstX[1];
    }

    if (dstY[1] > dstY[2]) {
        dstY[1] = dstY[0] + (dstY[3] - dstY[0]) * c.fTop / (h - c.height());
        dstY[2] = dstY[1];
    }

    SkIRect s;
    SkRect  d;
    for (int y = 0; y < 3; y++) {
        s.fTop = srcY[y];
        s.fBottom = srcY[y+1];
        d.fTop = dstY[y];
        d.fBottom = dstY[y+1];
        for (int x = 0; x < 3; x++) {
            s.fLeft = srcX[x];
            s.fRight = srcX[x+1];
            d.fLeft = dstX[x];
            d.fRight = dstX[x+1];
            this->internalDrawBitmapRect(bitmap, &s, d, paint);
        }
    }
}

void SkCanvas::drawBitmapNine(const SkBitmap& bitmap, const SkIRect& center,
                              const SkRect& dst, const SkPaint* paint) {
    SkDEBUGCODE(bitmap.validate();)

    // Need a device entry-point, so gpu can use a mesh
    this->internalDrawBitmapNine(bitmap, center, dst, paint);
}

class SkDeviceFilteredPaint {
public:
    SkDeviceFilteredPaint(SkDevice* device, const SkPaint& paint) {
        SkDevice::TextFlags flags;
        if (device->filterTextFlags(paint, &flags)) {
            SkPaint* newPaint = fLazy.set(paint);
            newPaint->setFlags(flags.fFlags);
            newPaint->setHinting(flags.fHinting);
            fPaint = newPaint;
        } else {
            fPaint = &paint;
        }
    }

    const SkPaint& paint() const { return *fPaint; }

private:
    const SkPaint*  fPaint;
    SkLazyPaint     fLazy;
};

void SkCanvas::DrawRect(const SkDraw& draw, const SkPaint& paint,
                        const SkRect& r, SkScalar textSize) {
    if (paint.getStyle() == SkPaint::kFill_Style) {
        draw.fDevice->drawRect(draw, r, paint);
    } else {
        SkPaint p(paint);
        p.setStrokeWidth(SkScalarMul(textSize, paint.getStrokeWidth()));
        draw.fDevice->drawRect(draw, r, p);
    }
}

void SkCanvas::DrawTextDecorations(const SkDraw& draw, const SkPaint& paint,
                                   const char text[], size_t byteLength,
                                   SkScalar x, SkScalar y) {
    SkASSERT(byteLength == 0 || text != NULL);

    // nothing to draw
    if (text == NULL || byteLength == 0 ||
        draw.fClip->isEmpty() ||
        (paint.getAlpha() == 0 && paint.getXfermode() == NULL)) {
        return;
    }

    SkScalar    width = 0;
    SkPoint     start;

    start.set(0, 0);    // to avoid warning
    if (paint.getFlags() & (SkPaint::kUnderlineText_Flag |
                            SkPaint::kStrikeThruText_Flag)) {
        width = paint.measureText(text, byteLength);

        SkScalar offsetX = 0;
        if (paint.getTextAlign() == SkPaint::kCenter_Align) {
            offsetX = SkScalarHalf(width);
        } else if (paint.getTextAlign() == SkPaint::kRight_Align) {
            offsetX = width;
        }
        start.set(x - offsetX, y);
    }

    if (0 == width) {
        return;
    }

    uint32_t flags = paint.getFlags();

    if (flags & (SkPaint::kUnderlineText_Flag |
                 SkPaint::kStrikeThruText_Flag)) {
        SkScalar textSize = paint.getTextSize();
        SkScalar height = SkScalarMul(textSize, kStdUnderline_Thickness);
        SkRect   r;

        r.fLeft = start.fX;
        r.fRight = start.fX + width;

        if (flags & SkPaint::kUnderlineText_Flag) {
            SkScalar offset = SkScalarMulAdd(textSize, kStdUnderline_Offset,
                                             start.fY);
            r.fTop = offset;
            r.fBottom = offset + height;
            DrawRect(draw, paint, r, textSize);
        }
        if (flags & SkPaint::kStrikeThruText_Flag) {
            SkScalar offset = SkScalarMulAdd(textSize, kStdStrikeThru_Offset,
                                             start.fY);
            r.fTop = offset;
            r.fBottom = offset + height;
            DrawRect(draw, paint, r, textSize);
        }
    }
}

void SkCanvas::drawText(const void* text, size_t byteLength,
                        SkScalar x, SkScalar y, const SkPaint& paint) {
    LOOPER_BEGIN(paint, SkDrawFilter::kText_Type)

    while (iter.next()) {
        SkDeviceFilteredPaint dfp(iter.fDevice, looper.paint());
        iter.fDevice->drawText(iter, text, byteLength, x, y, dfp.paint());
        DrawTextDecorations(iter, dfp.paint(),
                            static_cast<const char*>(text), byteLength, x, y);
    }

    LOOPER_END
}

void SkCanvas::drawPosText(const void* text, size_t byteLength,
                           const SkPoint pos[], const SkPaint& paint) {
    LOOPER_BEGIN(paint, SkDrawFilter::kText_Type)

    while (iter.next()) {
        SkDeviceFilteredPaint dfp(iter.fDevice, looper.paint());
        iter.fDevice->drawPosText(iter, text, byteLength, &pos->fX, 0, 2,
                                  dfp.paint());
    }

    LOOPER_END
}

void SkCanvas::drawPosTextH(const void* text, size_t byteLength,
                            const SkScalar xpos[], SkScalar constY,
                            const SkPaint& paint) {
    LOOPER_BEGIN(paint, SkDrawFilter::kText_Type)

    while (iter.next()) {
        SkDeviceFilteredPaint dfp(iter.fDevice, looper.paint());
        iter.fDevice->drawPosText(iter, text, byteLength, xpos, constY, 1,
                                  dfp.paint());
    }

    LOOPER_END
}

void SkCanvas::drawTextOnPath(const void* text, size_t byteLength,
                              const SkPath& path, const SkMatrix* matrix,
                              const SkPaint& paint) {
    LOOPER_BEGIN(paint, SkDrawFilter::kText_Type)

    while (iter.next()) {
        iter.fDevice->drawTextOnPath(iter, text, byteLength, path,
                                     matrix, looper.paint());
    }

    LOOPER_END
}

#ifdef SK_BUILD_FOR_ANDROID
void SkCanvas::drawPosTextOnPath(const void* text, size_t byteLength,
                                 const SkPoint pos[], const SkPaint& paint,
                                 const SkPath& path, const SkMatrix* matrix) {
    LOOPER_BEGIN(paint, SkDrawFilter::kText_Type)

    while (iter.next()) {
        iter.fDevice->drawPosTextOnPath(iter, text, byteLength, pos,
                                        looper.paint(), path, matrix);
    }

    LOOPER_END
}
#endif

void SkCanvas::drawVertices(VertexMode vmode, int vertexCount,
                            const SkPoint verts[], const SkPoint texs[],
                            const SkColor colors[], SkXfermode* xmode,
                            const uint16_t indices[], int indexCount,
                            const SkPaint& paint) {
    LOOPER_BEGIN(paint, SkDrawFilter::kPath_Type)

    while (iter.next()) {
        iter.fDevice->drawVertices(iter, vmode, vertexCount, verts, texs,
                                   colors, xmode, indices, indexCount,
                                   looper.paint());
    }

    LOOPER_END
}

void SkCanvas::drawData(const void* data, size_t length) {
    // do nothing. Subclasses may do something with the data
}

//////////////////////////////////////////////////////////////////////////////
// These methods are NOT virtual, and therefore must call back into virtual
// methods, rather than actually drawing themselves.
//////////////////////////////////////////////////////////////////////////////

void SkCanvas::drawARGB(U8CPU a, U8CPU r, U8CPU g, U8CPU b,
                        SkXfermode::Mode mode) {
    SkPaint paint;

    paint.setARGB(a, r, g, b);
    if (SkXfermode::kSrcOver_Mode != mode) {
        paint.setXfermodeMode(mode);
    }
    this->drawPaint(paint);
}

void SkCanvas::drawColor(SkColor c, SkXfermode::Mode mode) {
    SkPaint paint;

    paint.setColor(c);
    if (SkXfermode::kSrcOver_Mode != mode) {
        paint.setXfermodeMode(mode);
    }
    this->drawPaint(paint);
}

void SkCanvas::drawPoint(SkScalar x, SkScalar y, const SkPaint& paint) {
    SkPoint pt;

    pt.set(x, y);
    this->drawPoints(kPoints_PointMode, 1, &pt, paint);
}

void SkCanvas::drawPoint(SkScalar x, SkScalar y, SkColor color) {
    SkPoint pt;
    SkPaint paint;

    pt.set(x, y);
    paint.setColor(color);
    this->drawPoints(kPoints_PointMode, 1, &pt, paint);
}

void SkCanvas::drawLine(SkScalar x0, SkScalar y0, SkScalar x1, SkScalar y1,
                        const SkPaint& paint) {
    SkPoint pts[2];

    pts[0].set(x0, y0);
    pts[1].set(x1, y1);
    this->drawPoints(kLines_PointMode, 2, pts, paint);
}

void SkCanvas::drawRectCoords(SkScalar left, SkScalar top,
                              SkScalar right, SkScalar bottom,
                              const SkPaint& paint) {
    SkRect  r;

    r.set(left, top, right, bottom);
    this->drawRect(r, paint);
}

void SkCanvas::drawCircle(SkScalar cx, SkScalar cy, SkScalar radius,
                          const SkPaint& paint) {
    if (radius < 0) {
        radius = 0;
    }

    SkRect  r;
    r.set(cx - radius, cy - radius, cx + radius, cy + radius);

    if (paint.canComputeFastBounds()) {
        SkRect storage;
        if (this->quickReject(paint.computeFastBounds(r, &storage),
                              paint2EdgeType(&paint))) {
            return;
        }
    }

    SkPath  path;
    path.addOval(r);
    this->drawPath(path, paint);
}

void SkCanvas::drawRoundRect(const SkRect& r, SkScalar rx, SkScalar ry,
                             const SkPaint& paint) {
    if (rx > 0 && ry > 0) {
        if (paint.canComputeFastBounds()) {
            SkRect storage;
            if (this->quickReject(paint.computeFastBounds(r, &storage),
                                  paint2EdgeType(&paint))) {
                return;
            }
        }

        SkPath  path;
        path.addRoundRect(r, rx, ry, SkPath::kCW_Direction);
        this->drawPath(path, paint);
    } else {
        this->drawRect(r, paint);
    }
}

void SkCanvas::drawOval(const SkRect& oval, const SkPaint& paint) {
    if (paint.canComputeFastBounds()) {
        SkRect storage;
        if (this->quickReject(paint.computeFastBounds(oval, &storage),
                              paint2EdgeType(&paint))) {
            return;
        }
    }

    SkPath  path;
    path.addOval(oval);
    this->drawPath(path, paint);
}

void SkCanvas::drawArc(const SkRect& oval, SkScalar startAngle,
                       SkScalar sweepAngle, bool useCenter,
                       const SkPaint& paint) {
    if (SkScalarAbs(sweepAngle) >= SkIntToScalar(360)) {
        this->drawOval(oval, paint);
    } else {
        SkPath  path;
        if (useCenter) {
            path.moveTo(oval.centerX(), oval.centerY());
        }
        path.arcTo(oval, startAngle, sweepAngle, !useCenter);
        if (useCenter) {
            path.close();
        }
        this->drawPath(path, paint);
    }
}

void SkCanvas::drawTextOnPathHV(const void* text, size_t byteLength,
                                const SkPath& path, SkScalar hOffset,
                                SkScalar vOffset, const SkPaint& paint) {
    SkMatrix    matrix;

    matrix.setTranslate(hOffset, vOffset);
    this->drawTextOnPath(text, byteLength, path, &matrix, paint);
}

///////////////////////////////////////////////////////////////////////////////

void SkCanvas::drawPicture(SkPicture& picture) {
    int saveCount = save();
    picture.draw(this);
    restoreToCount(saveCount);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SkCanvas::LayerIter::LayerIter(SkCanvas* canvas, bool skipEmptyClips) {
    SK_COMPILE_ASSERT(sizeof(fStorage) >= sizeof(SkDrawIter), fStorage_too_small);

    SkASSERT(canvas);

    fImpl = new (fStorage) SkDrawIter(canvas, skipEmptyClips);
    fDone = !fImpl->next();
}

SkCanvas::LayerIter::~LayerIter() {
    fImpl->~SkDrawIter();
}

void SkCanvas::LayerIter::next() {
    fDone = !fImpl->next();
}

SkDevice* SkCanvas::LayerIter::device() const {
    return fImpl->getDevice();
}

const SkMatrix& SkCanvas::LayerIter::matrix() const {
    return fImpl->getMatrix();
}

const SkPaint& SkCanvas::LayerIter::paint() const {
    const SkPaint* paint = fImpl->getPaint();
    if (NULL == paint) {
        paint = &fDefaultPaint;
    }
    return *paint;
}

const SkRegion& SkCanvas::LayerIter::clip() const { return fImpl->getClip(); }
int SkCanvas::LayerIter::x() const { return fImpl->getX(); }
int SkCanvas::LayerIter::y() const { return fImpl->getY(); }
