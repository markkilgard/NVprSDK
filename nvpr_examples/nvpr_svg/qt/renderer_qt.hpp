
/* renderer_qt.hpp - Qt renderer class. */

#ifndef __renderer_qt_hpp__
#define __renderer_qt_hpp__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "nvpr_svg_config.h"  // configure path renderers to use

#if USE_QT

#include "path.hpp"
#include "scene.hpp"

// Qt 4.5 headers
#include <QBrush>  // http://doc.trolltech.com/4.5/qbrush.html
#include <QPen>  // http://doc.trolltech.com/4.5/qbrush.html
#include <QImage>  // http://doc.trolltech.com/4.5/qimage.html
#include <QTransform>  // http://doc.trolltech.com/4.5/qtransform.html
#include <QPainter>  // http://doc.trolltech.com/4.5/qpainter.html
#include <QPainterPath>  // http://doc.trolltech.com/4.5/qpainterpath.html

typedef shared_ptr<struct QtRenderer> QtRendererPtr;

typedef shared_ptr<struct QtPathRendererState> QtPathRendererStatePtr;
typedef shared_ptr<struct QtShapeRendererState> QtShapeRendererStatePtr;

typedef shared_ptr<struct QtSolidColorPaintRendererState> QtSolidColorPaintRendererStatePtr;
typedef shared_ptr<struct QtLinearGradientPaintRendererState> QtLinearGradientPaintRendererStatePtr;
typedef shared_ptr<struct QtRadialGradientPaintRendererState> QtRadialGradientPaintRendererStatePtr;
typedef shared_ptr<struct QtImagePaintRendererState> QtImagePaintRendererStatePtr;

typedef shared_ptr<QImage> QImagePtr;
typedef shared_ptr<QPainter> QPainterPtr;

struct QtRenderer : GLBlitRenderer {

    QImagePtr image;
    QPainterPtr painter;

    QtRenderer() { }

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
struct QtRendererState : SpecificRendererState<T,QtRenderer> {
    typedef T *OwnerPtr;  // intentionally not a SharedPtr
    QtRendererState(RendererPtr renderer_, OwnerPtr owner_) 
        : SpecificRendererState<T,QtRenderer>(renderer_, owner_)
    {}
};

struct QtShapeRendererState : QtRendererState<Shape> {
    bool valid;
    QBrush brush;
    QPen pen;

    QtShapeRendererState(RendererPtr renderer, Shape *shape)
        : QtRendererState<Shape>(renderer, shape)
        , valid(false)
    {}

    ~QtShapeRendererState()
    {}

    QtPathRendererStatePtr getPathRendererState() {
        PathRendererStatePtr path_state = owner->getPath()->getRendererState(getRenderer());
        QtPathRendererStatePtr qt_path_state = dynamic_pointer_cast<QtPathRendererState>(path_state);
        return qt_path_state;
    }

    void draw();
    void getPaths(vector<QPainterPath*>& paths);
    void validate();
    void invalidate();
    void getBrushes(QBrush &fill_brush, QBrush &stroke_brush);
};

struct QtPathRendererState : RendererState<Path> {
    bool valid;
    QPainterPath path;

    void validate();
    void invalidate();

    QtPathRendererState(RendererPtr renderer, Path *owner)
        : RendererState<Path>(renderer, owner)
        , valid(false)
    { }

    QtRendererPtr getRenderer() {
        RendererPtr locked_renderer = renderer.lock();
        assert(locked_renderer);
        QtRendererPtr qt = dynamic_pointer_cast<QtRenderer>(locked_renderer);
        assert(qt);
        return qt;
    }
};

typedef shared_ptr<struct QtPaintRendererState> QtPaintRendererStatePtr;
struct QtPaintRendererState : QtRendererState<Paint> {
protected:
    QBrush brush;
    float opacity;
    bool valid;

public:
    QtPaintRendererState(RendererPtr renderer, Paint *paint)
        : QtRendererState<Paint>(renderer, paint)
        , brush(Qt::NoBrush)
        , opacity(1.0)
        , valid(false)
    {}

    virtual ~QtPaintRendererState()
    {}

    virtual void validate(float opacity) = 0;
    void invalidate();

    void setOpacity(float new_opacity) {
        if (new_opacity != opacity) {
            opacity = new_opacity;
            valid = false;
        }
    }

    QBrush getBrush(float opacity) {
        validate(opacity);
        return brush;
    }
};

struct QtSolidColorPaintRendererState : QtPaintRendererState {
    QtSolidColorPaintRendererState(RendererPtr renderer, SolidColorPaint *paint);
    void validate(float opacity);
};

typedef shared_ptr<struct QtGradientPaintRendererState> QtGradientPaintRendererStatePtr;
struct QtGradientPaintRendererState : QtPaintRendererState {
protected:
    QtGradientPaintRendererState(RendererPtr renderer, GradientPaint *paint);
    void setGradientStops(QGradient &gradient, const GradientPaint *paint, float opacity);
    void setGenericGradientParameters(QGradient &, const GradientPaint *paint, float opacity);
};

struct QtLinearGradientPaintRendererState : QtGradientPaintRendererState {
    QtLinearGradientPaintRendererState(RendererPtr renderer, LinearGradientPaint *paint);
    void validate(float opacity);
};

struct QtRadialGradientPaintRendererState : QtGradientPaintRendererState {
    QtRadialGradientPaintRendererState(RendererPtr renderer, RadialGradientPaint *paint);
    void validate(float opacity);
};

struct QtImagePaintRendererState : QtPaintRendererState {
    QImage image;  // QImage(pixels, width, height, QImage::Format_ARGB32) 

    QtImagePaintRendererState(RendererPtr renderer, ImagePaint *paint);
    void validate(float opacity);
};

#endif // USE_QT

#endif // __renderer_qt_hpp__
