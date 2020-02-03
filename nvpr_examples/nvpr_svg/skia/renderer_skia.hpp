
/* renderer_skia.hpp - Skia renderer class. */

#ifndef __renderer_skia_hpp__
#define __renderer_skia_hpp__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "nvpr_svg_config.h"  // configure path renderers to use

#if USE_SKIA

#include "path.hpp"
#include "scene.hpp"

#ifdef _WIN32
#define GR_WIN32_BUILD 1
#define GR_GL_FUNCTION_TYPE __stdcall
#define SK_SCALAR_IS_FLOAT
#define SK_CAN_USE_FLOAT
#define SK_BUILD_FOR_WIN32
#define SK_IGNORE_STDINT_DOT_H
#else
#define GR_GL_FUNCTION_TYPE /* default calling convention */
#endif

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable : 4267)
#endif

#include <SkBitmap.h>  // http://skia.googlecode.com/svn/trunk/docs/html/class_sk_bitmap.html
#include <SkCanvas.h>  // http://skia.googlecode.com/svn/trunk/docs/html/class_sk_canvas.html
#include <SkPath.h>    // http://skia.googlecode.com/svn/trunk/docs/html/class_sk_path.html
#include <SkShader.h>  // http://skia.googlecode.com/svn/trunk/docs/html/class_sk_shader.html
#include <SkGradientShader.h>  // http://skia.googlecode.com/svn/trunk/docs/html/class_sk_gradient_shader.html

#ifdef _MSC_VER
# pragma warning(pop)
#endif

typedef shared_ptr<struct SkiaRenderer> SkiaRendererPtr;

typedef shared_ptr<struct SkiaPathRendererState> SkiaPathRendererStatePtr;
typedef shared_ptr<struct SkiaShapeRendererState> SkiaShapeRendererStatePtr;

typedef shared_ptr<struct SkiaSolidColorPaintRendererState> SkiaSolidColorPaintRendererStatePtr;
typedef shared_ptr<struct SkiaLinearGradientPaintRendererState> SkiaLinearGradientPaintRendererStatePtr;
typedef shared_ptr<struct SkiaRadialGradientPaintRendererState> SkiaRadialGradientPaintRendererStatePtr;
typedef shared_ptr<struct SkiaImagePaintRendererState> SkiaImagePaintRendererStatePtr;

struct SkiaRenderer : GLBlitRenderer {

    SkBitmap bitmap;
    SkCanvas canvas;
    bool cache_paths : 1;

    SkiaRenderer()
        : cache_paths(true)
    { }

    ~SkiaRenderer() {
        //bitmap.freePixels();
        canvas.setDevice(NULL);
    }

    void configureSurface(int width, int height);
    void shutdown();

    void clear(float3 clear_color);
    void setView(float4x4 view);
    VisitorPtr makeVisitor();
    void copyImageToWindow();
    const char *getWindowTitle();
    const char *getName();

    shared_ptr<RendererState<Shape> > alloc(Shape *owner);
    shared_ptr<RendererState<Path> > alloc(Path *owner);
    shared_ptr<RendererState<Paint> > alloc(Paint *owner);
};

template <typename T>
struct SkiaRendererState : SpecificRendererState<T,SkiaRenderer> {
    typedef T *OwnerPtr;  // intentionally not a SharedPtr
    SkiaRendererState(RendererPtr renderer_, OwnerPtr owner_) 
        : SpecificRendererState<T,SkiaRenderer>(renderer_, owner_)
    {}
};

struct SkiaShapeRendererState : SkiaRendererState<Shape> {
    bool valid;

    SkiaShapeRendererState(RendererPtr renderer, Shape *shape)
        : SkiaRendererState<Shape>(renderer, shape)
        , valid(false)
    {}

    ~SkiaShapeRendererState()
    {}

    SkiaPathRendererStatePtr getPathRendererState() {
        PathRendererStatePtr path_state = owner->getPath()->getRendererState(getRenderer());
        SkiaPathRendererStatePtr skia_path_state = dynamic_pointer_cast<SkiaPathRendererState>(path_state);
        return skia_path_state;
    }

    void draw();
    void getPaths(vector<SkPath*>& paths);
    void validate();
    void invalidate();
    SkPaint &getPaint(PaintPtr paint, float opacity);
};

struct SkiaPathRendererState : RendererState<Path> {
    bool valid;
    SkPath path;

    void validate();
    void invalidate();

    SkiaPathRendererState(RendererPtr renderer, Path *owner)
        : RendererState<Path>(renderer, owner)
        , valid(false)
    { }

    SkiaRendererPtr getRenderer() {
        RendererPtr locked_renderer = renderer.lock();
        assert(locked_renderer);
        SkiaRendererPtr skia = dynamic_pointer_cast<SkiaRenderer>(locked_renderer);
        assert(skia);
        return skia;
    }
};

typedef shared_ptr<struct SkiaPaintRendererState> SkiaPaintRendererStatePtr;
struct SkiaPaintRendererState : SkiaRendererState<Paint> {
protected:
    bool valid;
    SkPaint sk_paint;
    float opacity;
    const bool needs_gradient_matrix;

public:
    SkiaPaintRendererState(RendererPtr renderer, Paint *paint, bool is_gradient);
    virtual ~SkiaPaintRendererState() {}
    virtual void validate(float opacity) = 0;
    void invalidate();

    inline void setOpacity(float new_opacity) {
        if (new_opacity != opacity) {
            opacity = new_opacity;
            valid = false;
        }
    }

    inline SkPaint &getPaint() {
        return sk_paint;
    }

    inline bool needsGradientMatrix() {
        return needs_gradient_matrix;
    }
};

struct SkiaSolidColorPaintRendererState : SkiaPaintRendererState {
    SkiaSolidColorPaintRendererState(RendererPtr renderer, SolidColorPaint *paint);
    void validate(float opacity);
};

// Base functionality used by both linear and radial gradeints
typedef shared_ptr<struct SkiaGradientPaintRendererState> SkiaGradientPaintRendererStatePtr;
struct SkiaGradientPaintRendererState : SkiaPaintRendererState {
    SkShader *gradient;

protected:
    SkiaGradientPaintRendererState(RendererPtr renderer, GradientPaint *paint);
    ~SkiaGradientPaintRendererState();
    int allocRampData(const GradientPaint *paint,
                      SkColor* &colors,
                      SkScalar* &pos,
                      SkShader::TileMode &mode);
    void freeRampData(SkColor* &colors, SkScalar* &pos);
};

struct SkiaLinearGradientPaintRendererState : SkiaGradientPaintRendererState {
    SkiaLinearGradientPaintRendererState(RendererPtr renderer, LinearGradientPaint *paint);
    void createShader(const LinearGradientPaint *paint);
    void validate(float opacity);
};

struct SkiaRadialGradientPaintRendererState : SkiaGradientPaintRendererState {
    SkiaRadialGradientPaintRendererState(RendererPtr renderer, RadialGradientPaint *paint);
    void createShader(const RadialGradientPaint *paint);
    void validate(float opacity);
};

struct SkiaImagePaintRendererState : SkiaPaintRendererState {
    SkShader *bitmap;  // SkShader::CreateBitmapShader 

    SkiaImagePaintRendererState(RendererPtr renderer, ImagePaint *paint);
    void validate(float opacity);
};

#endif // USE_SKIA

#endif // __renderer_skia_hpp__
