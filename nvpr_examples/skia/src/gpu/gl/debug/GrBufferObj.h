
/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GrBufferObj_DEFINED
#define GrBufferObj_DEFINED

#include "GrFakeRefObj.h"

////////////////////////////////////////////////////////////////////////////////
class GrBufferObj : public GrFakeRefObj {
    GR_DEFINE_CREATOR(GrBufferObj);

public:
    GrBufferObj() 
        : GrFakeRefObj()
        , fDataPtr(NULL)
        , fMapped(false)
        , fBound(false)
        , fSize(0)
        , fUsage(GR_GL_STATIC_DRAW) {
    }
    virtual ~GrBufferObj() {
        delete[] fDataPtr;
    }

    void access() {
        // cannot access the buffer if it is currently mapped
        GrAlwaysAssert(!fMapped);
    }

    void setMapped()        { fMapped = true; }
    void resetMapped()      { fMapped = false; }
    bool getMapped() const  { return fMapped; }

    void setBound()         { fBound = true; }
    void resetBound()       { fBound = false; }
    bool getBound() const   { return fBound; }

    void allocate(GrGLint size, const GrGLchar *dataPtr) {
        GrAlwaysAssert(size >= 0);

        // delete pre-existing data
        delete[] fDataPtr;

        fSize = size;
        fDataPtr = new GrGLchar[size];
        if (dataPtr) {
            memcpy(fDataPtr, dataPtr, fSize);
        }
        // TODO: w/ no dataPtr the data is unitialized - this could be tracked
    }
    GrGLint getSize() const { return fSize; }
    GrGLchar *getDataPtr()  { return fDataPtr; }

    GrGLint getUsage() const { return fUsage; }
    void setUsage(GrGLint usage) { fUsage = usage; }

    virtual void deleteAction() SK_OVERRIDE {

        // buffers are automatically unmapped when deleted
        this->resetMapped();

        this->INHERITED::deleteAction();
    }


protected:
private:

    GrGLchar*   fDataPtr;
    bool        fMapped;       // is the buffer object mapped via "glMapBuffer"?
    bool        fBound;        // is the buffer object bound via "glBindBuffer"?
    GrGLint     fSize;         // size in bytes
    GrGLint     fUsage;        // one of: GL_STREAM_DRAW, GL_STATIC_DRAW, GL_DYNAMIC_DRAW

    typedef GrFakeRefObj INHERITED;
};

#endif // GrBufferObj_DEFINED
