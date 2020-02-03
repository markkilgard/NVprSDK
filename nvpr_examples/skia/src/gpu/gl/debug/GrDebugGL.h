
/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GrDebugGL_DEFINED
#define GrDebugGL_DEFINED

#include "SkTArray.h"
#include "gl/GrGLInterface.h"

class GrFakeRefObj;
class GrTextureUnitObj;
class GrBufferObj;
class GrTextureObj;
class GrFrameBufferObj;
class GrRenderBufferObj;
class GrProgramObj;

////////////////////////////////////////////////////////////////////////////////
// This is the main debugging object. It is a singleton and keeps track of
// all the other debug objects.
class GrDebugGL {
public:
    enum GrObjTypes {
        kTexture_ObjTypes = 0,
        kBuffer_ObjTypes,
        kRenderBuffer_ObjTypes,
        kFrameBuffer_ObjTypes,
        kShader_ObjTypes,
        kProgram_ObjTypes,
        kTextureUnit_ObjTypes,
        kObjTypeCount
    };

    GrFakeRefObj *createObj(GrObjTypes type) {
        GrFakeRefObj *temp = (*gFactoryFunc[type])();

        fObjects.push_back(temp);

        return temp;
    }

    GrFakeRefObj *findObject(GrGLuint ID, GrObjTypes type);

    GrGLuint getMaxTextureUnits() const { return kDefaultMaxTextureUnits; }

    void setCurTextureUnit(GrGLuint curTextureUnit) { fCurTextureUnit = curTextureUnit; }
    GrGLuint getCurTextureUnit() const { return fCurTextureUnit; }

    GrTextureUnitObj *getTextureUnit(int iUnit) {
        GrAlwaysAssert(0 <= iUnit && kDefaultMaxTextureUnits > iUnit);

        return fTextureUnits[iUnit];
    }

    void setArrayBuffer(GrBufferObj *arrayBuffer);
    GrBufferObj *getArrayBuffer()                   { return fArrayBuffer; }

    void setElementArrayBuffer(GrBufferObj *elementArrayBuffer);
    GrBufferObj *getElementArrayBuffer()                            { return fElementArrayBuffer; }

    void setTexture(GrTextureObj *texture);

    void setFrameBuffer(GrFrameBufferObj *frameBuffer);
    GrFrameBufferObj *getFrameBuffer()                  { return fFrameBuffer; }

    void setRenderBuffer(GrRenderBufferObj *renderBuffer);
    GrRenderBufferObj *getRenderBuffer()                  { return fRenderBuffer; }

    void useProgram(GrProgramObj *program);

    static GrDebugGL *getInstance() {
//        static GrDebugGL Obj;

        return &Obj;
    }

    void report() const;

protected:

private:
    // the OpenGLES 2.0 spec says this must be >= 2
    static const GrGLint kDefaultMaxTextureUnits = 8;

    GrGLuint        fMaxTextureUnits;
    GrGLuint        fCurTextureUnit;
    GrBufferObj *   fArrayBuffer;
    GrBufferObj *   fElementArrayBuffer;
    GrFrameBufferObj *fFrameBuffer;
    GrRenderBufferObj *fRenderBuffer;
    GrProgramObj *  fProgram;
    GrTextureObj *  fTexture;
    GrTextureUnitObj *fTextureUnits[kDefaultMaxTextureUnits];

    typedef GrFakeRefObj *(*Create)();

    static Create gFactoryFunc[kObjTypeCount];

    static GrDebugGL Obj;

    // global store of all objects
    SkTArray<GrFakeRefObj *> fObjects;

    GrDebugGL();
    ~GrDebugGL();
};

////////////////////////////////////////////////////////////////////////////////
// Helper macro to make creating an object (where you need to get back a derived
// type) easier
#define GR_CREATE(className, classEnum)                     \
    reinterpret_cast<className *>(GrDebugGL::getInstance()->createObj(classEnum))

////////////////////////////////////////////////////////////////////////////////
// Helper macro to make finding objects less painful
#define GR_FIND(id, className, classEnum)                   \
    reinterpret_cast<className *>(GrDebugGL::getInstance()->findObject(id, classEnum))

#endif // GrDebugGL_DEFINED
