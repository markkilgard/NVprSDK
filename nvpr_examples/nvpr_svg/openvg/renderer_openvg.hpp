
/* renderer_openvg.hpp - OpenVG renderer class. */

#ifndef __renderer_openvg_hpp__
#define __renderer_openvg_hpp__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "nvpr_svg_config.h"  // configure path renderers to use

#include "path.hpp"
#include "scene.hpp"

#if USE_OPENVG

#define EGL_STATIC_LIBRARY
#include <EGL/egl.h>
#define OPENVG_STATIC_LIBRARY
#include <VG/openvg.h>
#include <VG/vgu.h>

typedef shared_ptr<struct VGPathRendererState> VGPathRendererStatePtr;
typedef shared_ptr<struct VGRenderer> VGRendererPtr;
typedef shared_ptr<struct VGShapeRendererState> VGShapeRendererStatePtr;

struct VGRenderer : GLBlitRenderer {

    EGLDisplay egldisplay;
    EGLConfig eglconfig;
    EGLSurface eglsurface;
    EGLContext eglcontext;
    int width, height;
    int start_time;

    VGRenderer();

    void configureSurface(int width, int height);
    void shutdown();

    void beginDraw();
    void clear(float3 clear_color);
    void setView(float4x4 view);
    VisitorPtr makeVisitor();
    void endDraw();
    void copyImageToWindow();
    const char *getWindowTitle();
    const char *getName();

    shared_ptr<RendererState<Shape> > alloc(Shape *owner);
    shared_ptr<RendererState<Path> > alloc(Path *owner);
    shared_ptr<RendererState<Paint> > alloc(Paint *owner);
};

template <typename T>
struct VGRendererState : SpecificRendererState<T,VGRenderer> {
    typedef T *OwnerPtr;  // intentionally not a SharedPtr
    VGRendererState(RendererPtr renderer_, OwnerPtr owner_) 
        : SpecificRendererState<T,VGRenderer>(renderer_, owner_)
    {}
};

struct VGShapeRendererState : VGRendererState<Shape> {
    bool valid;
    VGfloat *fill_transform, *stroke_transform;

    VGShapeRendererState(RendererPtr renderer, Shape *shape)
        : VGRendererState<Shape>(renderer, shape)
        , valid(false)
        , fill_transform(NULL)
        , stroke_transform(NULL)
    {}

    ~VGShapeRendererState()
    {}

    VGPathRendererStatePtr getPathRendererState() {
        PathRendererStatePtr path_state = owner->getPath()->getRendererState(getRenderer());
        VGPathRendererStatePtr vg_path_state = dynamic_pointer_cast<VGPathRendererState>(path_state);
        return vg_path_state;
    }

    void draw();
    void validate();
    void invalidate();
    void getPaints(VGPaint &fill_paint, VGPaint &stroke_paint);
    void setStrokeParameters(const PathStyle &style);
};
typedef shared_ptr<struct VGShapeRendererState> VGShapeRendererStatePtr;

struct VGPathRendererState : RendererState<Path> {
    bool valid;
    VGPath path;
    VGFillRule fill_rule;

    void validate();
    void invalidate();

    VGPathRendererState(RendererPtr renderer, Path *owner)
        : RendererState<Path>(renderer, owner)
        , valid(false)
        , path(0)
    { }

    VGRendererPtr getRenderer() {
        RendererPtr locked_renderer = renderer.lock();
        assert(locked_renderer);
        VGRendererPtr vg = dynamic_pointer_cast<VGRenderer>(locked_renderer);
        assert(vg);
        return vg;
    }
};
typedef shared_ptr<struct VGPathRendererState> VGPathRendererStatePtr;

struct VGPaintRendererState : VGRendererState<Paint> {
protected:
    bool valid;
    VGPaint paint;

public:
    VGPaintRendererState(RendererPtr renderer, Paint *paint)
        : VGRendererState<Paint>(renderer, paint)
        , valid(false)
        , paint(0)
    {}

    virtual ~VGPaintRendererState() {
        if (paint) {
            vgDestroyPaint(paint);
        }
        paint = 0;
    }

    virtual void validate() = 0;
    void invalidate();

    VGPaint getPaint() {
        return paint;
    }
};
typedef shared_ptr<struct VGPaintRendererState> VGPaintRendererStatePtr;

struct VGSolidColorPaintRendererState : VGPaintRendererState {
    VGSolidColorPaintRendererState(RendererPtr renderer, SolidColorPaint *paint)
        : VGPaintRendererState(renderer, paint)
    {}

    void validate();
};
typedef shared_ptr<struct VGSolidColorPaintRendererState> VGSolidColorPaintRendererStatePtr;

struct VGGradientPaintRendererState : VGPaintRendererState {
    VGGradientPaintRendererState(RendererPtr renderer, GradientPaint *paint)
        : VGPaintRendererState(renderer, paint)
    {}

    VGfloat transform[9];

    void setGenericGradientPaintParameters(const GradientPaint *p);
};
typedef shared_ptr<struct VGGradientPaintRendererState> VGGradientPaintRendererStatePtr;

struct VGLinearGradientPaintRendererState : VGGradientPaintRendererState {
    VGLinearGradientPaintRendererState(RendererPtr renderer, LinearGradientPaint *paint)
        : VGGradientPaintRendererState(renderer, paint)
    {}

    void validate();
};
typedef shared_ptr<struct VGLinearGradientPaintRendererState> VGLinearGradientPaintRendererStatePtr;

struct VGRadialGradientPaintRendererState : VGGradientPaintRendererState {
    VGRadialGradientPaintRendererState(RendererPtr renderer, RadialGradientPaint *paint)
        : VGGradientPaintRendererState(renderer, paint)
    {}

    void validate();
};
typedef shared_ptr<struct VGRadialGradientPaintRendererState> VGRadialGradientPaintRendererStatePtr;

#endif // USE_OPENVG

#endif // __renderer_openvg_hpp__
