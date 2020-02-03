
/*
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef SkFlattenable_DEFINED
#define SkFlattenable_DEFINED

#include "SkRefCnt.h"
#include "SkBitmap.h"
#include "SkReader32.h"
#include "SkTDArray.h"
#include "SkWriter32.h"

class SkFlattenableReadBuffer;
class SkFlattenableWriteBuffer;
class SkString;

#if SK_ALLOW_STATIC_GLOBAL_INITIALIZERS

#define SK_DEFINE_FLATTENABLE_REGISTRAR(flattenable) \
    static SkFlattenable::Registrar g##flattenable##Reg(#flattenable, \
                                                       flattenable::CreateProc);
#define SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(flattenable) \
    static SkFlattenable::Registrar g##flattenable##Reg(#flattenable, \
                                                       flattenable::CreateProc);

#define SK_DECLARE_FLATTENABLE_REGISTRAR_GROUP()
#define SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_START(flattenable)
#define SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_END

#else

#define SK_DEFINE_FLATTENABLE_REGISTRAR(flattenable)
#define SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(flattenable) \
        SkFlattenable::Registrar(#flattenable, flattenable::CreateProc);

#define SK_DECLARE_FLATTENABLE_REGISTRAR_GROUP() static void InitializeFlattenables();

#define SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_START(flattenable) \
    void flattenable::InitializeFlattenables() {

#define SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_END \
    }

#endif

#define SK_DECLARE_UNFLATTENABLE_OBJECT() \
    virtual Factory getFactory() SK_OVERRIDE { return NULL; }; \

#define SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(flattenable) \
    virtual Factory getFactory() SK_OVERRIDE { return CreateProc; }; \
    static SkFlattenable* CreateProc(SkFlattenableReadBuffer& buffer) { \
        return SkNEW_ARGS(flattenable, (buffer)); \
    }

/** \class SkFlattenable
 
 SkFlattenable is the base class for objects that need to be flattened
 into a data stream for either transport or as part of the key to the
 font cache.
 */
class SK_API SkFlattenable : public SkRefCnt {
public:
    typedef SkFlattenable* (*Factory)(SkFlattenableReadBuffer&);
    
    SkFlattenable() {}
    
    /** Implement this to return a factory function pointer that can be called
     to recreate your class given a buffer (previously written to by your
     override of flatten().
     */
    virtual Factory getFactory() = 0;
    
    static Factory NameToFactory(const char name[]);
    static const char* FactoryToName(Factory);
    static void Register(const char name[], Factory);

    class Registrar {
    public:
        Registrar(const char name[], Factory factory) {
            SkFlattenable::Register(name, factory);
        }
    };

protected:
    SkFlattenable(SkFlattenableReadBuffer&) {}
    /** Override this to write data specific to your subclass into the buffer,
     being sure to call your super-class' version first. This data will later
     be passed to your Factory function, returned by getFactory().
     */
    virtual void flatten(SkFlattenableWriteBuffer&) const;

private:
#if !SK_ALLOW_STATIC_GLOBAL_INITIALIZERS
    static void InitializeFlattenables();
#endif

    friend class SkGraphics;
    friend class SkFlattenableWriteBuffer;
};

// helpers for matrix and region

class SkMatrix;
extern void SkReadMatrix(SkReader32*, SkMatrix*);
extern void SkWriteMatrix(SkWriter32*, const SkMatrix&);

class SkRegion;
extern void SkReadRegion(SkReader32*, SkRegion*);
extern void SkWriteRegion(SkWriter32*, const SkRegion&);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class SkTypeface;

class SkFlattenableReadBuffer : public SkReader32 {
public:
    SkFlattenableReadBuffer();
    explicit SkFlattenableReadBuffer(const void* data);
    SkFlattenableReadBuffer(const void* data, size_t size);
    
    void setRefCntArray(SkRefCnt* array[], int count) {
        fRCArray = array;
        fRCCount = count;
    }
    
    void setTypefaceArray(SkTypeface* array[], int count) {
        fTFArray = array;
        fTFCount = count;
    }

    /**
     *  Call this with a pre-loaded array of Factories, in the same order as
     *  were created/written by the writer. SkPicture uses this.
     */
    void setFactoryPlayback(SkFlattenable::Factory array[], int count) {
        fFactoryTDArray = NULL;
        fFactoryArray = array;
        fFactoryCount = count;
    }

    /**
     *  Call this with an initially empty array, so the reader can cache each
     *  factory it sees by name. Used by the pipe code in conjunction with
     *  the writer's kInlineFactoryNames_Flag.
     */
    void setFactoryArray(SkTDArray<SkFlattenable::Factory>* array) {
        fFactoryTDArray = array;
        fFactoryArray = NULL;
        fFactoryCount = 0;
    }
    
    SkTypeface* readTypeface();
    SkRefCnt* readRefCnt();
    void* readFunctionPtr();
    SkFlattenable* readFlattenable();
    
private:
    SkRefCnt** fRCArray;
    int        fRCCount;
    
    SkTypeface** fTFArray;
    int        fTFCount;
    
    SkTDArray<SkFlattenable::Factory>* fFactoryTDArray;
    SkFlattenable::Factory* fFactoryArray;
    int                     fFactoryCount;
    
    typedef SkReader32 INHERITED;
};

///////////////////////////////////////////////////////////////////////////////

#include "SkPtrRecorder.h"

/**
 *  Subclass of SkTPtrSet specialed to call ref() and unref() when the
 *  base class's incPtr() and decPtr() are called. This makes it a valid owner
 *  of each ptr, which is released when the set is reset or destroyed.
 */
class SkRefCntSet : public SkTPtrSet<SkRefCnt*> {
public:
    virtual ~SkRefCntSet();
    
protected:
    // overrides
    virtual void incPtr(void*);
    virtual void decPtr(void*);
};

class SkFactorySet : public SkTPtrSet<SkFlattenable::Factory> {};

class SkFlattenableWriteBuffer : public SkWriter32 {
public:
    SkFlattenableWriteBuffer(size_t minSize);
    virtual ~SkFlattenableWriteBuffer();

    void writeTypeface(SkTypeface*);
    void writeRefCnt(SkRefCnt*);
    void writeFunctionPtr(void*);
    void writeFlattenable(SkFlattenable* flattenable);
    
    SkRefCntSet* getTypefaceRecorder() const { return fTFSet; }
    SkRefCntSet* setTypefaceRecorder(SkRefCntSet*);
    
    SkRefCntSet* getRefCntRecorder() const { return fRCSet; }
    SkRefCntSet* setRefCntRecorder(SkRefCntSet*);
    
    SkFactorySet* getFactoryRecorder() const { return fFactorySet; }
    SkFactorySet* setFactoryRecorder(SkFactorySet*);

    enum Flags {
        kCrossProcess_Flag       = 0x01,
        /**
         *  Instructs the writer to inline Factory names as there are seen the
         *  first time (after that we store an index). The pipe code uses this.
         */
        kInlineFactoryNames_Flag = 0x02,
    };
    Flags getFlags() const { return (Flags)fFlags; }
    void setFlags(Flags flags) { fFlags = flags; }
    
    bool isCrossProcess() const {
        return SkToBool(fFlags & kCrossProcess_Flag);
    }
    bool inlineFactoryNames() const {
        return SkToBool(fFlags & kInlineFactoryNames_Flag);
    }

    bool persistBitmapPixels() const {
        return (fFlags & kCrossProcess_Flag) != 0;
    }
    
    bool persistTypeface() const { return (fFlags & kCrossProcess_Flag) != 0; }

private:
    uint32_t        fFlags;
    SkRefCntSet*    fTFSet;
    SkRefCntSet*    fRCSet;
    SkFactorySet*   fFactorySet;
    
    typedef SkWriter32 INHERITED;
};

#endif

