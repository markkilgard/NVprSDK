
/* renderer_cairo.hpp - Cairo renderer class. */

#ifndef __renderer_cairo_hpp__
#define __renderer_cairo_hpp__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "nvpr_svg_config.h"  // configure path renderers to use

#if USE_CAIRO

#include "path.hpp"
#include "scene.hpp"
#include "renderer.hpp"

// Cairo graphics library
#include <cairo.h>

// http://www.cairographics.org/manual/

typedef shared_ptr<struct CairoRenderer> CairoRendererPtr;

typedef shared_ptr<struct CairoPathRendererState> CairoPathRendererStatePtr;
typedef shared_ptr<struct CairoShapeRendererState> CairoShapeRendererStatePtr;

typedef shared_ptr<struct CairoSolidColorPaintRendererState> CairoSolidColorPaintRendererStatePtr;
typedef shared_ptr<struct CairoLinearGradientPaintRendererState> CairoLinearGradientPaintRendererStatePtr;
typedef shared_ptr<struct CairoRadialGradientPaintRendererState> CairoRadialGradientPaintRendererStatePtr;
typedef shared_ptr<struct CairoImagePaintRendererState> CairoImagePaintRendererStatePtr;

struct CairoRenderer : GLBlitRenderer{

    cairo_t *cr;
    cairo_surface_t *surface;
    bool cache_paths : 1;

    cairo_filter_t filter_mode;
    cairo_antialias_t antialias_mode;

    CairoRenderer()
        : cr(NULL)
        , surface(NULL)
        , cache_paths(true)
        , filter_mode(CAIRO_FILTER_GOOD)
        , antialias_mode(CAIRO_ANTIALIAS_DEFAULT)
    { }

    void configureSurface(int width, int height);
    void shutdown();
    void setAntialias(cairo_antialias_t mode);
    void setFilter(cairo_filter_t mode);

    void setCachePaths(bool caching) {
        cache_paths = caching;
    }

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
struct CairoRendererState : SpecificRendererState<T,CairoRenderer> {
    typedef T *OwnerPtr;  // intentionally not a SharedPtr
    CairoRendererState(RendererPtr renderer_, OwnerPtr owner_) 
        : SpecificRendererState<T,CairoRenderer>(renderer_, owner_)
    {}
};

struct CairoShapeRendererState : CairoRendererState<Shape> {
    bool valid;

    CairoShapeRendererState(RendererPtr renderer, Shape *shape)
        : CairoRendererState<Shape>(renderer, shape)
        , valid(false)
    {}

    ~CairoShapeRendererState() {
    }

    CairoPathRendererStatePtr getPathRendererState() {
        PathRendererStatePtr path_state = owner->getPath()->getRendererState(getRenderer());
        CairoPathRendererStatePtr cairo_path_state = dynamic_pointer_cast<CairoPathRendererState>(path_state);
        return cairo_path_state;
    }

    void draw();
    void getPaths(vector<cairo_path_t*>& paths);
    void validate();
    void invalidate();
    cairo_pattern_t *getPattern(PaintPtr paint);
    void getPatterns(cairo_pattern_t *&brush, cairo_pattern_t *&pen);
};

typedef shared_ptr<struct CairoPaintRendererState> CairoPaintRendererStatePtr;
struct CairoPaintRendererState : CairoRendererState<Paint> {
protected:
    cairo_pattern_t *pattern;
    bool valid;
    float opacity;
    const bool needs_gradient_matrix;

public:
    CairoPaintRendererState(RendererPtr renderer, Paint *paint, bool is_gradient)
        : CairoRendererState<Paint>(renderer, paint)
        , pattern(NULL)
        , valid(false)
        , opacity(1.0)
        , needs_gradient_matrix(is_gradient)
    {}

    virtual ~CairoPaintRendererState() {
        cairo_pattern_destroy(pattern);
    }

    virtual void validate(float opacity) = 0;
    void invalidate();

    cairo_pattern_t *getPattern() {
        return pattern;
    }
    void setOpacity(float new_opacity)
    {
        if (opacity != new_opacity) {
            opacity = new_opacity;
            valid = false;
        }
    }
    inline bool needsGradientMatrix() {
        return needs_gradient_matrix;
    }
};

struct CairoSolidColorPaintRendererState : CairoPaintRendererState {
    CairoSolidColorPaintRendererState(RendererPtr renderer, SolidColorPaint *paint)
        : CairoPaintRendererState(renderer, paint, false/*not gradient*/)
    {}

    void validate(float opacity);
};

typedef shared_ptr<struct CairoGradientPaintRendererState> CairoGradientPaintRendererStatePtr;
struct CairoGradientPaintRendererState : CairoPaintRendererState {
protected:
    CairoGradientPaintRendererState(RendererPtr renderer, GradientPaint *paint)
        : CairoPaintRendererState(renderer, paint, /*is gradient*/true)
    {}

    void setGradientStops(const GradientPaint *paint, float opacity);
    void setGenericGradientPatternParameters(const GradientPaint *paint, float opacity);
};

struct CairoLinearGradientPaintRendererState : CairoGradientPaintRendererState {
    CairoLinearGradientPaintRendererState(RendererPtr renderer, LinearGradientPaint *paint)
        : CairoGradientPaintRendererState(renderer, paint)
    {}

    void validate(float opacity);
};

struct CairoRadialGradientPaintRendererState : CairoGradientPaintRendererState {
    CairoRadialGradientPaintRendererState(RendererPtr renderer, RadialGradientPaint *paint)
        : CairoGradientPaintRendererState(renderer, paint)
    {}

    void validate(float opacity);
};

struct CairoImagePaintRendererState : CairoPaintRendererState {
    cairo_surface_t *surface;  // cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);

    CairoImagePaintRendererState(RendererPtr renderer, ImagePaint *paint);
    void validate(float opacity);
    ~CairoImagePaintRendererState() {
        cairo_surface_destroy(surface);
    }
};

struct CairoPathRendererState : CairoRendererState<Path> {
    bool valid;
    cairo_path_t *path;

    void validate();
    void invalidate();

    CairoPathRendererState(RendererPtr renderer, Path *owner)
        : CairoRendererState<Path>(renderer, owner)
        , valid(false)
        , path(NULL)
    { }

    ~CairoPathRendererState() {
        cairo_path_destroy(path);
    }
};

#endif // USE_CAIRO

#endif // __renderer_cairo_hpp__
