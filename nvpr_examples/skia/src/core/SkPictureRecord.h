
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef SkPictureRecord_DEFINED
#define SkPictureRecord_DEFINED

#include "SkCanvas.h"
#include "SkFlattenable.h"
#include "SkPathHeap.h"
#include "SkPicture.h"
#include "SkPictureFlat.h"
#include "SkTemplates.h"
#include "SkWriter32.h"

class SkPictureRecord : public SkCanvas {
public:
    SkPictureRecord(uint32_t recordFlags);
    virtual ~SkPictureRecord();

    virtual int save(SaveFlags) SK_OVERRIDE;
    virtual int saveLayer(const SkRect* bounds, const SkPaint*, SaveFlags) SK_OVERRIDE;
    virtual void restore() SK_OVERRIDE;
    virtual bool translate(SkScalar dx, SkScalar dy) SK_OVERRIDE;
    virtual bool scale(SkScalar sx, SkScalar sy) SK_OVERRIDE;
    virtual bool rotate(SkScalar degrees) SK_OVERRIDE;
    virtual bool skew(SkScalar sx, SkScalar sy) SK_OVERRIDE;
    virtual bool concat(const SkMatrix& matrix) SK_OVERRIDE;
    virtual void setMatrix(const SkMatrix& matrix) SK_OVERRIDE;
    virtual bool clipRect(const SkRect&, SkRegion::Op, bool) SK_OVERRIDE;
    virtual bool clipPath(const SkPath&, SkRegion::Op, bool) SK_OVERRIDE;
    virtual bool clipRegion(const SkRegion& region, SkRegion::Op op) SK_OVERRIDE;
    virtual void clear(SkColor) SK_OVERRIDE;
    virtual void drawPaint(const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPoints(PointMode, size_t count, const SkPoint pts[],
                            const SkPaint&) SK_OVERRIDE;
    virtual void drawRect(const SkRect& rect, const SkPaint&) SK_OVERRIDE;
    virtual void drawPath(const SkPath& path, const SkPaint&) SK_OVERRIDE;
    virtual void drawBitmap(const SkBitmap&, SkScalar left, SkScalar top,
                            const SkPaint*) SK_OVERRIDE;
    virtual void drawBitmapRect(const SkBitmap&, const SkIRect* src,
                                const SkRect& dst, const SkPaint*) SK_OVERRIDE;
    virtual void drawBitmapMatrix(const SkBitmap&, const SkMatrix&,
                                  const SkPaint*) SK_OVERRIDE;
    virtual void drawBitmapNine(const SkBitmap& bitmap, const SkIRect& center,
                                const SkRect& dst, const SkPaint*) SK_OVERRIDE;
    virtual void drawSprite(const SkBitmap&, int left, int top,
                            const SkPaint*) SK_OVERRIDE;
    virtual void drawText(const void* text, size_t byteLength, SkScalar x,
                          SkScalar y, const SkPaint&) SK_OVERRIDE;
    virtual void drawPosText(const void* text, size_t byteLength,
                             const SkPoint pos[], const SkPaint&) SK_OVERRIDE;
    virtual void drawPosTextH(const void* text, size_t byteLength,
                      const SkScalar xpos[], SkScalar constY, const SkPaint&) SK_OVERRIDE;
    virtual void drawTextOnPath(const void* text, size_t byteLength,
                            const SkPath& path, const SkMatrix* matrix,
                                const SkPaint&) SK_OVERRIDE;
    virtual void drawPicture(SkPicture& picture) SK_OVERRIDE;
    virtual void drawVertices(VertexMode, int vertexCount,
                          const SkPoint vertices[], const SkPoint texs[],
                          const SkColor colors[], SkXfermode*,
                          const uint16_t indices[], int indexCount,
                              const SkPaint&) SK_OVERRIDE;
    virtual void drawData(const void*, size_t) SK_OVERRIDE;
    virtual bool isDrawingToLayer() const SK_OVERRIDE;

    void addFontMetricsTopBottom(const SkPaint& paint, SkScalar minY, SkScalar maxY);

    const SkTDArray<const SkFlatBitmap* >& getBitmaps() const {
        return fBitmaps;
    }
    const SkTDArray<const SkFlatMatrix* >& getMatrices() const {
        return fMatrices;
    }
    const SkTDArray<const SkFlatPaint* >& getPaints() const {
        return fPaints;
    }
    const SkTDArray<SkPicture* >& getPictureRefs() const {
        return fPictureRefs;
    }
    const SkTDArray<const SkFlatRegion* >& getRegions() const {
        return fRegions;
    }

    void reset();

    const SkWriter32& writeStream() const {
        return fWriter;
    }

private:
    SkTDArray<uint32_t> fRestoreOffsetStack;
    int fFirstSavedLayerIndex;
    enum {
        kNoSavedLayerIndex = -1
    };

    void addDraw(DrawType drawType) {
#ifdef SK_DEBUG_TRACE
        SkDebugf("add %s\n", DrawTypeToString(drawType));
#endif
        fWriter.writeInt(drawType);
    }
    void addInt(int value) {
        fWriter.writeInt(value);
    }
    void addScalar(SkScalar scalar) {
        fWriter.writeScalar(scalar);
    }

    void addBitmap(const SkBitmap& bitmap);
    void addMatrix(const SkMatrix& matrix);
    void addMatrixPtr(const SkMatrix* matrix);
    void addPaint(const SkPaint& paint);
    void addPaintPtr(const SkPaint* paint);
    void addPath(const SkPath& path);
    void addPicture(SkPicture& picture);
    void addPoint(const SkPoint& point);
    void addPoints(const SkPoint pts[], int count);
    void addRect(const SkRect& rect);
    void addRectPtr(const SkRect* rect);
    void addIRect(const SkIRect& rect);
    void addIRectPtr(const SkIRect* rect);
    void addRegion(const SkRegion& region);
    void addText(const void* text, size_t byteLength);

    int find(SkTDArray<const SkFlatBitmap* >& bitmaps,
                   const SkBitmap& bitmap);
    int find(SkTDArray<const SkFlatMatrix* >& matrices,
                   const SkMatrix* matrix);
    int find(SkTDArray<const SkFlatPaint* >& paints, const SkPaint* paint);
    int find(SkTDArray<const SkFlatRegion* >& regions, const SkRegion& region);

#ifdef SK_DEBUG_DUMP
public:
    void dumpMatrices();
    void dumpPaints();
#endif

#ifdef SK_DEBUG_SIZE
public:
    size_t size() const;
    int bitmaps(size_t* size) const;
    int matrices(size_t* size) const;
    int paints(size_t* size) const;
    int paths(size_t* size) const;
    int regions(size_t* size) const;
    size_t streamlen() const;

    size_t fPointBytes, fRectBytes, fTextBytes;
    int fPointWrites, fRectWrites, fTextWrites;
#endif

#ifdef SK_DEBUG_VALIDATE
public:
    void validate() const;
private:
    void validateBitmaps() const;
    void validateMatrices() const;
    void validatePaints() const;
    void validatePaths() const;
    void validateRegions() const;
#else
public:
    void validate() const {}
#endif

private:
    SkChunkAlloc fHeap;
    int fBitmapIndex;
    SkTDArray<const SkFlatBitmap* > fBitmaps;
    int fMatrixIndex;
    SkTDArray<const SkFlatMatrix* > fMatrices;
    int fPaintIndex;
    SkTDArray<const SkFlatPaint* > fPaints;
    int fRegionIndex;
    SkTDArray<const SkFlatRegion* > fRegions;
    SkPathHeap* fPathHeap;  // reference counted
    SkWriter32 fWriter;

    // we ref each item in these arrays
    SkTDArray<SkPicture*> fPictureRefs;

    SkRefCntSet fRCSet;
    SkRefCntSet fTFSet;

    uint32_t fRecordFlags;

    // helper function to handle save/restore culling offsets
    void recordOffsetForRestore(SkRegion::Op op);

    friend class SkPicturePlayback;
    friend class SkPictureTester; // for unit testing

    typedef SkCanvas INHERITED;
};

#endif
