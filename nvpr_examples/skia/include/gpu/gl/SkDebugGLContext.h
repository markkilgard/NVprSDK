
/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef SkDebugGLContext_DEFINED
#define SkDebugGLContext_DEFINED

#include "SkGLContext.h"

class SkDebugGLContext : public SkGLContext {

public:
    SkDebugGLContext() {};

    virtual void makeCurrent() const SK_OVERRIDE {};

protected:
    virtual const GrGLInterface* createGLContext() SK_OVERRIDE;

    virtual void destroyGLContext() SK_OVERRIDE {};
};

#endif

