
/*
 * Copyright 2011 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef SkDrawFilter_DEFINED
#define SkDrawFilter_DEFINED

#include "SkRefCnt.h"

class SkCanvas;
class SkPaint;

/**
 *  Right before something is being draw, filter() is called with the
 *  paint. The filter may modify the paint as it wishes, which will then be
 *  used for the actual drawing. Note: this modification only lasts for the
 *  current draw, as a temporary copy of the paint is used.
 */
class SkDrawFilter : public SkRefCnt {
public:
    enum Type {
        kPaint_Type,
        kPoint_Type,
        kLine_Type,
        kBitmap_Type,
        kRect_Type,
        kPath_Type,
        kText_Type
    };

    /**
     *  Called with the paint that will be used to draw the specified type.
     *  The implementation may modify the paint as they wish.
     */
    virtual void filter(SkPaint*, Type) = 0;
};

#endif
