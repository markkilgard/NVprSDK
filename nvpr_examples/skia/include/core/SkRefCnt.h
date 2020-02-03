
/*
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef SkRefCnt_DEFINED
#define SkRefCnt_DEFINED

#include "SkThread.h"

/** \class SkRefCnt

    SkRefCnt is the base class for objects that may be shared by multiple
    objects. When a new owner wants a reference, it calls ref(). When an owner
    wants to release its reference, it calls unref(). When the shared object's
    reference count goes to zero as the result of an unref() call, its (virtual)
    destructor is called. It is an error for the destructor to be called
    explicitly (or via the object going out of scope on the stack or calling
    delete) if getRefCnt() > 1.
*/
class SK_API SkRefCnt : SkNoncopyable {
public:
    /** Default construct, initializing the reference count to 1.
    */
    SkRefCnt() : fRefCnt(1) {}

    /**  Destruct, asserting that the reference count is 1.
    */
    virtual ~SkRefCnt() {
#ifdef SK_DEBUG
        SkASSERT(fRefCnt == 1);
        fRefCnt = 0;    // illegal value, to catch us if we reuse after delete
#endif
    }

    /** Return the reference count.
    */
    int32_t getRefCnt() const { return fRefCnt; }

    /** Increment the reference count. Must be balanced by a call to unref().
    */
    void ref() const {
        SkASSERT(fRefCnt > 0);
        sk_atomic_inc(&fRefCnt);
    }

    /** Decrement the reference count. If the reference count is 1 before the
        decrement, then call delete on the object. Note that if this is the
        case, then the object needs to have been allocated via new, and not on
        the stack.
    */
    void unref() const {
        SkASSERT(fRefCnt > 0);
        if (sk_atomic_dec(&fRefCnt) == 1) {
            fRefCnt = 1;    // so our destructor won't complain
            SkDELETE(this);
        }
    }

    void validate() const {
        SkASSERT(fRefCnt > 0);
    }

private:
    mutable int32_t fRefCnt;
};

///////////////////////////////////////////////////////////////////////////////

/** Helper macro to safely assign one SkRefCnt[TS]* to another, checking for
    null in on each side of the assignment, and ensuring that ref() is called
    before unref(), in case the two pointers point to the same object.
 */
#define SkRefCnt_SafeAssign(dst, src)   \
    do {                                \
        if (src) src->ref();            \
        if (dst) dst->unref();          \
        dst = src;                      \
    } while (0)


/** Check if the argument is non-null, and if so, call obj->ref()
 */
template <typename T> static inline void SkSafeRef(T* obj) {
    if (obj) {
        obj->ref();
    }
}

/** Check if the argument is non-null, and if so, call obj->unref()
 */
template <typename T> static inline void SkSafeUnref(T* obj) {
    if (obj) {
        obj->unref();
    }
}

///////////////////////////////////////////////////////////////////////////////

/**
 *  Utility class that simply unref's its argument in the destructor.
 */
template <typename T> class SkAutoTUnref : SkNoncopyable {
public:
    explicit SkAutoTUnref(T* obj = NULL) : fObj(obj) {}
    ~SkAutoTUnref() { SkSafeUnref(fObj); }

    T* get() const { return fObj; }

    void reset(T* obj) {
        SkSafeUnref(fObj);
        fObj = obj;
    }

    /**
     *  Return the hosted object (which may be null), transferring ownership.
     *  The reference count is not modified, and the internal ptr is set to NULL
     *  so unref() will not be called in our destructor. A subsequent call to
     *  detach() will do nothing and return null.
     */
    T* detach() {
        T* obj = fObj;
        fObj = NULL;
        return obj;
    }

private:
    T*  fObj;
};

class SkAutoUnref : public SkAutoTUnref<SkRefCnt> {
public:
    SkAutoUnref(SkRefCnt* obj) : SkAutoTUnref<SkRefCnt>(obj) {}
};

class SkAutoRef : SkNoncopyable {
public:
    SkAutoRef(SkRefCnt* obj) : fObj(obj) { SkSafeRef(obj); }
    ~SkAutoRef() { SkSafeUnref(fObj); }
private:
    SkRefCnt* fObj;
};

/** Wrapper class for SkRefCnt pointers. This manages ref/unref of a pointer to
    a SkRefCnt (or subclass) object.
 */
template <typename T> class SkRefPtr {
public:
    SkRefPtr() : fObj(NULL) {}
    SkRefPtr(T* obj) : fObj(obj) { SkSafeRef(fObj); }
    SkRefPtr(const SkRefPtr& o) : fObj(o.fObj) { SkSafeRef(fObj); }
    ~SkRefPtr() { SkSafeUnref(fObj); }

    SkRefPtr& operator=(const SkRefPtr& rp) {
        SkRefCnt_SafeAssign(fObj, rp.fObj);
        return *this;
    }
    SkRefPtr& operator=(T* obj) {
        SkRefCnt_SafeAssign(fObj, obj);
        return *this;
    }

    T* get() const { return fObj; }
    T& operator*() const { return *fObj; }
    T* operator->() const { return fObj; }

    typedef T* SkRefPtr::*unspecified_bool_type;
    operator unspecified_bool_type() const {
        return fObj ? &SkRefPtr::fObj : NULL;
    }

private:
    T* fObj;
};

#endif

