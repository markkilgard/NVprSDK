
/* renderer_d2d.hpp - Direct2D renderer class. */

#ifndef __renderer_d2d_hpp__
#define __renderer_d2d_hpp__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "nvpr_svg_config.h"  // configure path renderers to use

#if USE_D2D

#include "path.hpp"
#include "scene.hpp"

// Direct2D graphics library
#include <d2d1.h>     // Download the DirectX SDK from http://msdn.microsoft.com/en-us/directx/
#include <d3d10_1.h>  // Direct3D 10.1

// http://msdn.microsoft.com/en-us/library/dd370990(VS.85).aspx

#include "init_d2d.hpp"

typedef shared_ptr<struct D2DRenderer> D2DRendererPtr;

typedef shared_ptr<struct D2DPathRendererState> D2DPathRendererStatePtr;
typedef shared_ptr<struct D2DShapeRendererState> D2DShapeRendererStatePtr;



struct D2DRenderer : BlitRenderer {
    
    ID2D1Factory           *m_pFactory;
    ID2D1RenderTarget      *m_pRenderTarget;
    //ID2D1HwndRenderTarget  *m_pRenderTarget;
    IDXGISurface           *m_pSurface;
    IDXGISwapChain         *m_pSwapChain;
    ID3D10Device1          *m_pD3DDevice;
    D2D1_MATRIX_3X2_F       m_mViewMatrix;
    bool cache_paths : 1;
    bool m_bInDraw   : 1;
    bool m_bIsWARP   : 1;

    D2DRenderer(bool isWARP = false)
        : m_pFactory(NULL)
        , m_pRenderTarget(NULL)
        , m_pSurface(NULL)
        , m_pSwapChain(NULL)
        , m_pD3DDevice(NULL)
        , m_mViewMatrix(D2D1::Matrix3x2F::Identity())
        , cache_paths(true)
        , m_bInDraw(false)
        , m_bIsWARP(isWARP)
    { }

    ~D2DRenderer()
    {
        shutdown();
    }

    void configureSurface(int width, int height);
    void shutdown();
    void recreateSurface(bool isWARP);
    //void setAntialias(cairo_antialias_t cairoAntialiasMode);

    void setCachePaths(bool caching) {
        cache_paths = caching;
    }

    void clear(float3 clear_color);
    void setView(float4x4 view);
    VisitorPtr makeVisitor();
    void copyImageToWindow();
    const char *getName();
    const char *getWindowTitle();
    void reportFPS();
    void swapBuffers();

    void beginDraw();
    void endDraw();

    shared_ptr<RendererState<Shape> > alloc(Shape *owner);
    shared_ptr<RendererState<Path> > alloc(Path *owner);
    shared_ptr<RendererState<Paint> > alloc(Paint *owner);
};

template <typename T>
struct D2DRendererState : SpecificRendererState<T,D2DRenderer> {
    typedef T *OwnerPtr;  // intentionally not a SharedPtr
    D2DRendererState(RendererPtr renderer_, OwnerPtr owner_) 
        : SpecificRendererState<T,D2DRenderer>(renderer_, owner_)
    {}
};

struct D2DShapeRendererState : D2DRendererState<Shape> {
    bool valid;
    ID2D1Brush *brush;
    ID2D1Brush *pen;
    ID2D1StrokeStyle *strokeStyle;

    D2DShapeRendererState(RendererPtr renderer, Shape *shape)
        : D2DRendererState<Shape>(renderer, shape)
        , brush(NULL)
        , pen(NULL)
        , strokeStyle(NULL)
        , valid(false)
    {}

    ~D2DShapeRendererState() {
        // Don't release the brush or pen - will be released by the PaintRendererState
        if (strokeStyle) {
            strokeStyle->Release();
            strokeStyle = NULL;
        }
    }

    D2DPathRendererStatePtr getPathRendererState() {
        PathRendererStatePtr path_state = owner->getPath()->getRendererState(getRenderer());
        D2DPathRendererStatePtr d2d_path_state = dynamic_pointer_cast<D2DPathRendererState>(path_state);
        return d2d_path_state;
    }

    void draw();
    void getGeometry(vector<ID2D1Geometry*>* geometryArray);
    void validate();
    void invalidate();
};

struct D2DPathRendererState : D2DRendererState<Path> {
    bool valid;
    ID2D1PathGeometry *path;

    void validate(D2DRendererPtr renderer);
    void invalidate();

    D2DPathRendererState(RendererPtr renderer, Path *owner)
        : D2DRendererState<Path>(renderer, owner)
        , path(NULL)
        , valid(false)
    { }

    ~D2DPathRendererState() {
        if (path)
        {
            path->Release();
            path = NULL;
        }
    }
};

typedef shared_ptr<struct D2DPaintRendererState> D2DPaintRendererStatePtr;
struct D2DPaintRendererState : D2DRendererState<Paint> {
protected:
    bool valid;
    ID2D1Brush  *brush;

public:
    D2DPaintRendererState(RendererPtr renderer, Paint *paint)
        : D2DRendererState<Paint>(renderer, paint)
        , brush(NULL)
        , valid(false)
    {}

    virtual ~D2DPaintRendererState() {
        if (brush) {
            brush->Release();
            brush = NULL;
        }
    }

    virtual void validate(const Shape* shape, float opacity) = 0;
    void invalidate();

    ID2D1Brush *getPattern() {
        return brush;
    }
};

typedef shared_ptr<struct D2DSolidColorPaintRendererState> D2DSolidColorPaintRendererStatePtr;
struct D2DSolidColorPaintRendererState : D2DPaintRendererState {
    D2DSolidColorPaintRendererState(RendererPtr renderer, SolidColorPaint *paint)
        : D2DPaintRendererState(renderer, paint)
    {}

    void validate(const Shape* shape, float opacity);
};

typedef shared_ptr<struct D2DLinearGradientPaintRendererState> D2DLinearGradientPaintRendererStatePtr;
struct D2DLinearGradientPaintRendererState : D2DPaintRendererState {
    D2DLinearGradientPaintRendererState(RendererPtr renderer, LinearGradientPaint *paint)
        : D2DPaintRendererState(renderer, paint)
    {}

    void validate(const Shape* shape, float opacity);
};

typedef shared_ptr<struct D2DRadialGradientPaintRendererState> D2DRadialGradientPaintRendererStatePtr;
struct D2DRadialGradientPaintRendererState : D2DPaintRendererState {
    D2DRadialGradientPaintRendererState(RendererPtr renderer, RadialGradientPaint *paint)
        : D2DPaintRendererState(renderer, paint)
    {}

    void validate(const Shape* shape, float opacity);
};

#endif // USE_D2D

#endif // __renderer_d2d_hpp__
