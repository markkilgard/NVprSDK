
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef SampleCode_DEFINED
#define SampleCode_DEFINED

#include "SkColor.h"
#include "SkEvent.h"
#include "SkKey.h"
#include "SkView.h"
class SkOSMenu;
class GrContext;

class SampleCode {
public:
    static bool KeyQ(const SkEvent&, SkKey* outKey);
    static bool CharQ(const SkEvent&, SkUnichar* outUni);

    static bool TitleQ(const SkEvent&);
    static void TitleR(SkEvent*, const char title[]);
    static bool RequestTitle(SkView* view, SkString* title);
    
    static bool PrefSizeQ(const SkEvent&);
    static void PrefSizeR(SkEvent*, SkScalar width, SkScalar height);

    static bool FastTextQ(const SkEvent&);

    static SkMSec GetAnimTime();
    static SkMSec GetAnimTimeDelta();
    static SkScalar GetAnimSecondsDelta();
    static SkScalar GetAnimScalar(SkScalar speedPerSec, SkScalar period = 0);
    // gives a sinusoidal value between 0 and amplitude
    static SkScalar GetAnimSinScalar(SkScalar amplitude,
                                     SkScalar periodInSec,
                                     SkScalar phaseInSec = 0);

    static GrContext* GetGr();
};

//////////////////////////////////////////////////////////////////////////////

// interface that constructs SkViews
class SkViewFactory : public SkRefCnt {
public:
    virtual SkView* operator() () const = 0;
};

typedef SkView* (*SkViewCreateFunc)();

// wraps SkViewCreateFunc in SkViewFactory interface
class SkFuncViewFactory : public SkViewFactory {
public:
    SkFuncViewFactory(SkViewCreateFunc func);
    virtual SkView* operator() () const SK_OVERRIDE;
    
private:
    SkViewCreateFunc fCreateFunc;
};

namespace skiagm {
class GM;
}

// factory function that creates a skiagm::GM
typedef skiagm::GM* (*GMFactoryFunc)(void*);

// Takes a GM factory function and implements the SkViewFactory interface 
// by making the GM and wrapping it in a GMSampleView. GMSampleView bridges
// the SampleView interface to skiagm::GM.
class SkGMSampleViewFactory : public SkViewFactory {
public:
    SkGMSampleViewFactory(GMFactoryFunc func);
    virtual SkView* operator() () const SK_OVERRIDE;
private:
    GMFactoryFunc fFunc;
};

class SkViewRegister : public SkRefCnt {
public:
    explicit SkViewRegister(SkViewFactory*);
    explicit SkViewRegister(SkViewCreateFunc);
    explicit SkViewRegister(GMFactoryFunc);

    ~SkViewRegister() {
        fFact->unref();
    }
    
    static const SkViewRegister* Head() { return gHead; }
    
    SkViewRegister* next() const { return fChain; }
    const SkViewFactory*   factory() const { return fFact; }
    
private:
    SkViewFactory*  fFact;
    SkViewRegister* fChain;
    
    static SkViewRegister* gHead;
};

///////////////////////////////////////////////////////////////////////////////

class SampleView : public SkView {
public:
    SampleView() : fBGColor(SK_ColorWHITE), fRepeatCount(1) {
        fUsePipe = false;
    }

    void setBGColor(SkColor color) { fBGColor = color; }

    static bool IsSampleView(SkView*);
    static bool SetRepeatDraw(SkView*, int count);
    static bool SetUsePipe(SkView*, bool);
    
    /**
     *  Call this to request menu items from a SampleView.
     *  Subclassing notes: A subclass of SampleView can overwrite this method 
     *  to add new items of various types to the menu and change its title.
     *  The events attached to any new menu items must be handled in its onEvent
     *  method. See SkOSMenu.h for helper functions.   
     */
    virtual void requestMenu(SkOSMenu* menu) {}

protected:
    virtual void onDrawBackground(SkCanvas*);
    virtual void onDrawContent(SkCanvas*) = 0;
    
    // overrides
    virtual bool onEvent(const SkEvent& evt);
    virtual bool onQuery(SkEvent* evt);
    virtual void draw(SkCanvas*);
    virtual void onDraw(SkCanvas*);

    bool fUsePipe;
    SkColor fBGColor;
    
private:
    int fRepeatCount;

    typedef SkView INHERITED;
};

#endif

