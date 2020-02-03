
/* renderer_d2d.cpp - Direct2D renderer interface for nvpr_svg */

// Copyright (c) NVIDIA Corporation. All rights reserved.

#include "nvpr_svg_config.h"  // configure path renderers to use

#if USE_D2D

#define _USE_MATH_DEFINES  // so <math.h> has M_PI
#include <Cg/vector/rgba.hpp>

#include "path.hpp"
#include "scene.hpp"
#include "renderer_d2d.hpp"
#include "showfps.h"
#include "scene_d2d.hpp"

#include "init_d2d_private.hpp"

#include <Cg/length.hpp>

#include <windows.h>

#if 0
#pragma comment (lib, "d2d1.lib")       // link with Direct2D lib
#pragma comment (lib, "d3d10_1.lib")    // link with Direct3D 10.1 lib
#endif

#ifndef IID_PPV_ARGS
// IID_PPV_ARGS would normally be defined by Microsoft's <objbase.h>
// See http://msdn.microsoft.com/en-us/library/ee330727%28VS.85%29.aspx

//  IID_PPV_ARGS(ppType)
//      ppType is the variable of type IType that will be filled
//
//      RESULTS in:  IID_IType, ppvType
//      will create a compiler error if wrong level of indirection is used.
//
extern "C++"
{
    template<typename T> void** IID_PPV_ARGS_Helper(T** pp)
    {
        // make sure everyone derives from IUnknown
        static_cast<IUnknown*>(*pp);

        return reinterpret_cast<void**>(pp);
    }
}

#define IID_PPV_ARGS(ppType) __uuidof(**(ppType)), IID_PPV_ARGS_Helper(ppType)
#endif

struct D2DPathSegmentProcessor : PathSegmentProcessor {
    
    ID2D1GeometrySink*  m_pSink;
    ID2D1PathGeometry*  m_pPath;
    D2D1_FILL_MODE      m_fillMode;
    bool                m_bInFig;

    D2DPathSegmentProcessor(ID2D1PathGeometry*  pPath)
        : m_pPath(pPath)
        , m_pSink(NULL)
        , m_fillMode(D2D1_FILL_MODE_WINDING)
        , m_bInFig(false)
    {}

    void beginPath(PathPtr p) { 
        assert(m_pPath && !m_pSink);

        m_fillMode = (p->style.fill_rule == PathStyle::NON_ZERO)
                     ? D2D1_FILL_MODE_WINDING
                     : D2D1_FILL_MODE_ALTERNATE;

        HRESULT hr = m_pPath->Open(&m_pSink);
    }
    void moveTo(const float2 plist[2], size_t coord_index, char cmd) {
        assert(m_pSink);

        if (m_bInFig)
        {
            m_pSink->EndFigure(D2D1_FIGURE_END_OPEN);
        }

        m_pSink->SetFillMode(m_fillMode);
        m_pSink->BeginFigure(D2D1::Point2F(plist[1].x, plist[1].y), D2D1_FIGURE_BEGIN_FILLED);
        m_bInFig = true;
    }
    void lineTo(const float2 plist[2], size_t coord_index, char cmd) {
        assert(m_pSink && m_bInFig);

        m_pSink->AddLine(D2D1::Point2F(plist[1].x, plist[1].y));
    }
    void quadraticCurveTo(const float2 plist[3], size_t coord_index, char cmd) {
        assert(m_pSink && m_bInFig);
        
        m_pSink->AddQuadraticBezier(D2D1::QuadraticBezierSegment(D2D1::Point2F(plist[1].x, plist[1].y),
                                                                 D2D1::Point2F(plist[2].x, plist[2].y)));
    }
    void cubicCurveTo(const float2 plist[4], size_t coord_index, char cmd) {
        assert(m_pSink && m_bInFig);

        m_pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(plist[1].x, plist[1].y),
                                               D2D1::Point2F(plist[2].x, plist[2].y),
                                               D2D1::Point2F(plist[3].x, plist[3].y)));
    }
    void arcTo(const EndPointArc &arc, size_t coord_index, char cmd) {
        m_pSink->AddArc(D2D1::ArcSegment(D2D1::Point2F(arc.p[1].x, arc.p[1].y),
                        D2D1::SizeF(arc.radii.x, arc.radii.y),
                        arc.x_axis_rotation,
                        arc.sweep_flag ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
                        arc.large_arc_flag ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL));
    }
    void close(char cmd) {
        m_pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
        m_bInFig = false;
    }
    void endPath(PathPtr p) {
        assert(m_pSink);

        if (m_bInFig)
        {
            m_pSink->EndFigure(D2D1_FIGURE_END_OPEN);
            m_bInFig = false;
        }

        HRESULT hr = m_pSink->Close();
    }
};

static void drawPathHelper(D2DRendererPtr pRenderer,
                           ID2D1PathGeometry *pPath,
                           ID2D1Brush *pBrush,
                           ID2D1Brush *pPen, 
                           ID2D1StrokeStyle *pStrokeStyle,
                           PathStyle *style)
{
    extern bool doFilling, doStroking;
    if (style->do_stroke && doStroking) {
        if (pBrush && doFilling) {
            pRenderer->m_pRenderTarget->FillGeometry(pPath, pBrush);
        }
        pRenderer->m_pRenderTarget->DrawGeometry(pPath, pPen, style->stroke_width, pStrokeStyle);
    } else if (style->do_fill && doFilling) {

        ID2D1Factory* pFactory1 = NULL;
        ID2D1Factory* pFactory2 = NULL;
        pPath->GetFactory(&pFactory1);
        pBrush->GetFactory(&pFactory2);

        pRenderer->m_pRenderTarget->FillGeometry(pPath, pBrush);
    }
}

void D2DRenderer::configureSurface(int width, int height)
{
    assert(!m_pFactory && !m_pRenderTarget);
    if (width == 0 || height == 0) { 
        return;
    }

    HWND hWnd = WindowFromDC(wglGetCurrentDC());

    HRESULT hr = S_OK;
    
    // create D2D1 Factory
    D2D1_FACTORY_OPTIONS f_ops;
    f_ops.debugLevel = D2D1_DEBUG_LEVEL_ERROR;
    hr = LazyD2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, 
                           __uuidof(ID2D1Factory),
                           &f_ops,
                           (void**)&m_pFactory);

    // create D3D10 Device
    if (SUCCEEDED(hr)) {
        D3D10_DRIVER_TYPE alt_driver = D3D10_DRIVER_TYPE_WARP;
        //D3D10_DRIVER_TYPE alt_driver = D3D10_DRIVER_TYPE_SOFTWARE;
        hr = LazyD3D10CreateDevice1(NULL, m_bIsWARP ? alt_driver : D3D10_DRIVER_TYPE_HARDWARE, NULL,
                                    D3D10_CREATE_DEVICE_BGRA_SUPPORT | D3D10_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS,
                                    D3D10_FEATURE_LEVEL_10_0,
                                    D3D10_1_SDK_VERSION,
                                    &m_pD3DDevice);
    }

    // create swap chain
    if (SUCCEEDED(hr)) {
        IDXGIDevice  *dxgiDevice;
        IDXGIAdapter *dxgiAdapter;
        IDXGIFactory *dxgiFactory;
    
        m_pD3DDevice->QueryInterface(&dxgiDevice);
        dxgiDevice->GetAdapter(&dxgiAdapter);
        dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));

        DXGI_SWAP_CHAIN_DESC swapDesc;
        ::ZeroMemory(&swapDesc, sizeof(swapDesc));
        swapDesc.BufferDesc.Width = width;
        swapDesc.BufferDesc.Height = height;
        swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //DXGI_FORMAT_B8G8R8A8_UNORM;
        swapDesc.BufferDesc.RefreshRate.Numerator = 60;
        swapDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapDesc.SampleDesc.Count = 1;
        swapDesc.SampleDesc.Quality = 0;
        swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapDesc.BufferCount = 1;
        swapDesc.OutputWindow = hWnd;
        swapDesc.Windowed = TRUE;

        hr = dxgiFactory->CreateSwapChain(dxgiDevice, &swapDesc, &m_pSwapChain);
        dxgiDevice->Release();
        dxgiAdapter->Release();
        dxgiFactory->Release();
    }

    // get back buffer surface pointer
    if (SUCCEEDED(hr)) {
        hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&m_pSurface));
    }

    // create D2D1 Render Target
    if (SUCCEEDED(hr)) {
        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
                                                                           D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
        hr = m_pFactory->CreateDxgiSurfaceRenderTarget(m_pSurface, props, &m_pRenderTarget);
        /*hr = m_pFactory->CreateHwndRenderTarget(&D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT), 
                                                &D2D1::HwndRenderTargetProperties(hWnd, D2D1::SizeU(width, height), D2D1_PRESENT_OPTIONS_IMMEDIATELY),
                                                &m_pRenderTarget);*/
    }

    if (FAILED(hr)) {
        shutdown();
    }
}

void D2DRenderer::shutdown()
{
    if (m_pSurface) {
        m_pSurface->Release();
        m_pSurface = NULL;
    }
    if (m_pSwapChain) {
        m_pSwapChain->Release();
        m_pSwapChain = NULL;
    }
    if (m_pD3DDevice) {
        m_pD3DDevice->Release();
        m_pD3DDevice = NULL;
    }
    if (m_pRenderTarget) {
        m_pRenderTarget->Release();
        m_pRenderTarget = NULL;
    }
    if (m_pFactory) {
        m_pFactory->Release();
        m_pFactory = NULL;
    }
}

void D2DRenderer::recreateSurface(bool isWARP)
{
    if (!m_pRenderTarget) {
        return;
    }

    D2D1_SIZE_F rtSize = m_pRenderTarget->GetSize();
    shutdown();
    m_bIsWARP = isWARP;
    configureSurface(int(rtSize.width), int(rtSize.height));
}

void D2DRenderer::clear(float3 clear_color)
{
    if (!m_pRenderTarget) {
        return;
    }

    m_pRenderTarget->Clear(D2D1::ColorF(clear_color.r, clear_color.g, clear_color.b));
}

void D2DRenderer::setView(float4x4 view)
{
    if (!m_pRenderTarget) {
        return;
    }
    const D2D1_SIZE_F rtSize = m_pRenderTarget->GetSize();

    extern float scene_ratio;
    const float4x4 surface_to_clip = surfaceToClip(rtSize.width,
                                                   rtSize.height,
                                                   scene_ratio);

    D2D1_MATRIX_3X2_F s = D2D1::Matrix3x2F::Scale(rtSize.width/2.0f, -rtSize.height/2.0f);
    D2D1_MATRIX_3X2_F t = D2D1::Matrix3x2F::Translation(1,-1);
    D2D1_MATRIX_3X2_F s2c = D2D1::Matrix3x2F::Scale(surface_to_clip[0][0], surface_to_clip[1][1]);
    D2D1_MATRIX_3X2_F m = D2D1::Matrix3x2F(view[0][0], view[1][0],
                                           view[0][1], view[1][1],
                                           view[0][3], view[1][3]);

    // Direct2D has an overloaded matrix multiply operator that needs to compose right-to-left
    m_mViewMatrix = m * (s2c* (t*s));
}


VisitorPtr D2DRenderer::makeVisitor()
{
    return VisitorPtr(new D2DVisitors::Draw(
        dynamic_pointer_cast<D2DRenderer>(shared_from_this())));
}

void D2DRenderer::copyImageToWindow()
{
    if (m_pSwapChain)
    {
        m_pSwapChain->Present(0,0);
    } else {
        // XXX need to blit here for warp renderer!
    }
}

const char *D2DRenderer::getWindowTitle()
{
    if (m_bIsWARP) {
        return "Direct2D WARP path rendering";
    } else {
        return "Direct2D GPU path rendering";
    }
}

const char *D2DRenderer::getName()
{
    if (m_bIsWARP) {
        return "WARP";
    } else {
        return "D2D";
    }
}

void D2DRenderer::reportFPS()
{
    double thisFPS = just_handleFPS();
    char buf[200];
    sprintf(buf, "%s - %.1f fps",
        getWindowTitle(), thisFPS);
    glutSetWindowTitle(buf);
}

void D2DRenderer::beginDraw()
{ 
    if (m_pRenderTarget) {
        m_pRenderTarget->BeginDraw();
    }
}

void D2DRenderer::endDraw()
{
    if (m_pRenderTarget) {
        m_pRenderTarget->EndDraw();
    }
}

void D2DRenderer::swapBuffers()
{
}

shared_ptr<RendererState<Path> > D2DRenderer::alloc(Path *owner)
{
    return D2DPathRendererStatePtr(new D2DPathRendererState(shared_from_this(), owner));
}

shared_ptr<RendererState<Paint> > D2DRenderer::alloc(Paint *owner)
{
    SolidColorPaint *solid_color_paint = dynamic_cast<SolidColorPaint*>(owner);
    if (solid_color_paint) {
        return D2DPaintRendererStatePtr(new D2DSolidColorPaintRendererState(shared_from_this(), solid_color_paint));
    }

    LinearGradientPaint *linear_gradient_paint = dynamic_cast<LinearGradientPaint*>(owner);
    if (linear_gradient_paint) {
        return D2DPaintRendererStatePtr(new D2DLinearGradientPaintRendererState(shared_from_this(), linear_gradient_paint));
    }

    RadialGradientPaint *radial_gradient_paint = dynamic_cast<RadialGradientPaint*>(owner);
    if (radial_gradient_paint) {
        return D2DPaintRendererStatePtr(new D2DRadialGradientPaintRendererState(shared_from_this(), radial_gradient_paint));
    }

    assert(!"paint unsupported by D2D renderer");
    return D2DPaintRendererStatePtr();
}

struct SolidShape;
shared_ptr<RendererState<Shape> > D2DRenderer::alloc(Shape *owner)
{
    return D2DShapeRendererStatePtr(new D2DShapeRendererState(shared_from_this(), owner));
}

void D2DPathRendererState::validate(D2DRendererPtr renderer)
{
    if (valid) {

        ID2D1Factory *pFactory = NULL;
        path->GetFactory(&pFactory);
        if (pFactory == renderer->m_pFactory) {
            return;
        }
    }

    if (path) {
        path->Release();
        path = NULL;
    }
    HRESULT hr = renderer->m_pFactory->CreatePathGeometry(&path);

    owner->processSegments(D2DPathSegmentProcessor(path));
    valid = true;

    // gather tessellation info
    /*{
        ID2D1Mesh *pMesh = NULL;
        HRESULT hr = renderer->m_pRenderTarget->CreateMesh(&pMesh);
        if (SUCCEEDED(hr))
        {
            ID2D1TessellationSink *pSink = NULL;
            hr = pMesh->Open(&pSink);
            if (SUCCEEDED(hr))
            {
                hr = path->Tessellate(NULL, pSink);
                if (SUCCEEDED(hr))
                {
                    hr = pSink->Close();
                }
            }
        }
    }*/

}

void D2DPathRendererState::invalidate()
{
    valid = false;
}

inline D2D1_LINE_JOIN SVGToD2DLineJoin(PathStyle::LineJoin join)
{
    switch (join) {
    default: 
        assert(!"Bogus line join style");
        // Fallthrough...
    case PathStyle::MITER_REVERT_JOIN:
    case PathStyle::NONE_JOIN:
        return D2D1_LINE_JOIN_MITER;
    case PathStyle::ROUND_JOIN:
        return D2D1_LINE_JOIN_ROUND;
    case PathStyle::BEVEL_JOIN:
        return D2D1_LINE_JOIN_BEVEL;
    case PathStyle::MITER_TRUNCATE_JOIN:
        return D2D1_LINE_JOIN_MITER_OR_BEVEL;
    }
}

D2D1_CAP_STYLE SVGToD2DCapStyle(PathStyle::LineCap cap)
{
    switch (cap) {
    default:
        assert(!"Bogus cap style");
        // Fallthrough...
    case PathStyle::SQUARE_CAP:
        return D2D1_CAP_STYLE_SQUARE;
    case PathStyle::BUTT_CAP:
        return D2D1_CAP_STYLE_SQUARE;
    case PathStyle::ROUND_CAP:
        return D2D1_CAP_STYLE_ROUND;
    case PathStyle::TRIANGLE_CAP:
        return D2D1_CAP_STYLE_TRIANGLE;
    }
}

void SVGToD2DStrokeProperties(D2D1_STROKE_STYLE_PROPERTIES &properties, const PathStyle* style)
{
    properties.startCap = SVGToD2DCapStyle(style->line_cap);
    properties.endCap = SVGToD2DCapStyle(style->line_cap);
    properties.dashCap = SVGToD2DCapStyle(style->line_cap);
    properties.lineJoin = SVGToD2DLineJoin(style->line_join);
    properties.miterLimit = style->miter_limit / style->stroke_width;
    if (style->dash_array.size() > 0) {
        properties.dashStyle = D2D1_DASH_STYLE_CUSTOM;
    } else {
        properties.dashStyle = D2D1_DASH_STYLE_SOLID;
    }
    properties.dashOffset = style->dash_offset / style->stroke_width;
}

void D2DShapeRendererState::validate()
{
    const Shape *shape = owner;

    PaintPtr fill_paint = shape->getFillPaint();
    if (fill_paint) {
        PaintRendererStatePtr renderer_state = fill_paint->getRendererState(getRenderer());
        D2DPaintRendererStatePtr paint_renderer_state = dynamic_pointer_cast<D2DPaintRendererState>(renderer_state);
        paint_renderer_state->validate(shape, owner->net_fill_opacity);
        brush = paint_renderer_state->getPattern();
    }

    const PaintPtr stroke_paint = shape->getStrokePaint();
    if (stroke_paint) {
        PaintRendererStatePtr renderer_state = stroke_paint->getRendererState(getRenderer());
        D2DPaintRendererStatePtr paint_renderer_state = dynamic_pointer_cast<D2DPaintRendererState>(renderer_state);
        paint_renderer_state->validate(shape, owner->net_stroke_opacity);
        pen = paint_renderer_state->getPattern();
    }

    if (strokeStyle == NULL) {
        PathStyle* style = &owner->getPath()->style;
        std::vector<float> dashes;
        
        if (style->dash_array.size()) {
            dashes.reserve(style->dash_array.size());
            for (size_t i = 0; i < style->dash_array.size(); i++) {
                dashes.push_back(style->dash_array[i] / style->stroke_width);
            }
        }

        if (style->do_stroke) {
            D2D1_STROKE_STYLE_PROPERTIES prop;
            SVGToD2DStrokeProperties(prop, style);
            HRESULT hr = getRenderer()->m_pFactory->CreateStrokeStyle(
                prop,
                dashes.size() ? &dashes[0] : NULL, 
                UINT(dashes.size()),
                &strokeStyle);
            assert(SUCCEEDED(hr));
        }
    }

    if (valid) {
        return;
    }
    // Nothing about the shape to actually validate.
    valid = true;
}

void D2DShapeRendererState::invalidate()
{
    valid = false;

    const Shape *shape = owner;

    PaintPtr fill_paint = shape->getFillPaint();
    if (fill_paint) {
        PaintRendererStatePtr renderer_state = fill_paint->getRendererState(getRenderer());
        D2DPaintRendererStatePtr paint_renderer_state = dynamic_pointer_cast<D2DPaintRendererState>(renderer_state);
        paint_renderer_state->invalidate();
    }

    const PaintPtr stroke_paint = shape->getStrokePaint();
    if (stroke_paint) {
        PaintRendererStatePtr renderer_state = stroke_paint->getRendererState(getRenderer());
        D2DPaintRendererStatePtr paint_renderer_state = dynamic_pointer_cast<D2DPaintRendererState>(renderer_state);
        paint_renderer_state->invalidate();
    }

    if (strokeStyle) {
        strokeStyle->Release();
        strokeStyle = NULL;
    }
}

void D2DShapeRendererState::draw()
{
    D2DRendererPtr renderer = getRenderer();
    if (!renderer->m_pRenderTarget) {
        return;
    }

    // validate the brush and pen
    validate();

    D2DPathRendererStatePtr d2d_prs = getPathRendererState();

    if (!renderer->cache_paths) {
        // invalidate the path
        d2d_prs->invalidate();
    }
    
    d2d_prs->validate(renderer);
    drawPathHelper(renderer, d2d_prs->path, brush, pen, strokeStyle, &owner->getPath()->style);
}

void D2DShapeRendererState::getGeometry(vector<ID2D1Geometry*>* geometryArray)
{
    D2DRendererPtr renderer = getRenderer();
    D2DPathRendererStatePtr d2d_prs = getPathRendererState();

    if (!renderer->cache_paths) {
        // invalidate the path
        d2d_prs->invalidate();
    }
    
    d2d_prs->validate(renderer);
    geometryArray->push_back(d2d_prs->path);
    // TODO: Add geometry for the stroke if there is one
}

static D2D1_EXTEND_MODE SVGSpreadToD2DExtend(SpreadMethod spread)
{
    switch (spread) {
    default:
        assert(!"bogus spread method");
        // Fallthrough...
    case NONE:
        return D2D1_EXTEND_MODE_CLAMP;
    case PAD:
        return D2D1_EXTEND_MODE_CLAMP;
    case REFLECT:
        return D2D1_EXTEND_MODE_MIRROR;
    case REPEAT:
        return D2D1_EXTEND_MODE_WRAP;
    }
}

inline float3x3 NormalToUser(const Shape* shape)
{
    float4 bounds = shape->getPath()->getActualFillBounds();
    float2 p1 = bounds.xy,
           p2 = bounds.zw,
           diff = p2-p1;

    return float3x3(diff.x, 0, p1.x,
                    0, diff.y, p1.y,
                    0, 0, 1);
}

inline D2D1_MATRIX_3X2_F D2DGradientTransform(const Shape* shape, GradientPaint* paint)
{
    float3x3 d2dTransform;
    if (paint->getGradientUnits() == OBJECT_BOUNDING_BOX) {
        d2dTransform = mul(paint->getGradientTransform(), NormalToUser(shape));
    } else {
        assert(paint->getGradientUnits() == USER_SPACE_ON_USE);
        d2dTransform = paint->getGradientTransform();
    }
    D2D1_MATRIX_3X2_F transform = {
        d2dTransform[0][0], d2dTransform[1][0], 
        d2dTransform[0][1], d2dTransform[1][1], 
        d2dTransform[0][2], d2dTransform[1][2],
    };

    return transform;
}

ID2D1GradientStopCollection* CreateGradientStops(GradientPaint* paint, ID2D1RenderTarget* renderer)
{
    const std::vector<GradientStop>& stops = paint->getStopArray();
    D2D1_GRADIENT_STOP* d2dStops = new D2D1_GRADIENT_STOP[stops.size()];
    for (size_t i = 0; i < stops.size(); i++) {
        d2dStops[i].color = D2D1::ColorF(stops[i].color.r, stops[i].color.g, 
                                         stops[i].color.b, stops[i].color.a);
        d2dStops[i].position = stops[i].offset;
    }

    ID2D1GradientStopCollection *pGradientStops = NULL;
    renderer->CreateGradientStopCollection(d2dStops, UINT(stops.size()),
        D2D1_GAMMA_2_2, SVGSpreadToD2DExtend(paint->getSpreadMethod()), 
        &pGradientStops);

    delete[] d2dStops;

    return pGradientStops;
}

void D2DPaintRendererState::invalidate()
{
    if (brush) {
        brush->Release();
        brush = NULL;
    }
}

void D2DSolidColorPaintRendererState::validate(const Shape* shape, float opacity)
{
    if (!brush) {
        SolidColorPaint *solid_paint = dynamic_cast<SolidColorPaint*>(owner);
        assert(solid_paint);
        float4 color = solid_paint->getColor();
        getRenderer()->m_pRenderTarget->CreateSolidColorBrush(
            D2D1::ColorF(color.r, color.g,
                         color.b, color.a),
            D2D1::BrushProperties(opacity),
            (ID2D1SolidColorBrush**)&brush);
    }
}

void D2DLinearGradientPaintRendererState::validate(const Shape* shape, float opacity)
{
    if (!brush) {
        LinearGradientPaint *linear_paint = dynamic_cast<LinearGradientPaint*>(owner);
        assert(linear_paint);

        ID2D1GradientStopCollection *pGradientStops = CreateGradientStops(linear_paint, getRenderer()->m_pRenderTarget);
        if (pGradientStops) {
            getRenderer()->m_pRenderTarget->CreateLinearGradientBrush(
                D2D1::LinearGradientBrushProperties(
                    D2D1::Point2F(linear_paint->getV1().x, linear_paint->getV1().y),
                    D2D1::Point2F(linear_paint->getV2().x, linear_paint->getV2().y)),
                D2D1::BrushProperties(opacity, D2DGradientTransform(shape, linear_paint)),
                pGradientStops,
                (ID2D1LinearGradientBrush**)&brush);

            pGradientStops->Release();
        }
    }
}

void D2DRadialGradientPaintRendererState::validate(const Shape* shape, float opacity)
{
    if (!brush) {
        RadialGradientPaint *radial_paint = dynamic_cast<RadialGradientPaint*>(owner);
        assert(radial_paint);

        ID2D1GradientStopCollection *pGradientStops = CreateGradientStops(radial_paint, getRenderer()->m_pRenderTarget);
        if (pGradientStops) {
            float2 originOffset = radial_paint->getFocalPoint() - radial_paint->getCenter();
            float offsetLen = length(originOffset);
            // Fudge, has to be  somewhat less than 1 (0.999 is too
            // much) or Direct2D draws a weird triangle.
            const float sub_unity = 199.0f/200;
            float radius = radial_paint->getRadius();
            if (offsetLen > sub_unity*radius) {
                // Shrink the originOffset to be slightly inside the radius.
                originOffset = sub_unity*radius*originOffset/offsetLen;
            }
            getRenderer()->m_pRenderTarget->CreateRadialGradientBrush(
                D2D1::RadialGradientBrushProperties(
                    D2D1::Point2F(radial_paint->getCenter().x, radial_paint->getCenter().y), 
                    D2D1::Point2F(originOffset.x, originOffset.y),
                    radius, radius),
                D2D1::BrushProperties(opacity, D2DGradientTransform(shape, radial_paint)),
                pGradientStops,
                (ID2D1RadialGradientBrush**)&brush);

            pGradientStops->Release();
        }
    }
}

#endif // USE_D2D
