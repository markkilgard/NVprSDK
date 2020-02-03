
/* renderer_qt.cpp - Qt path rendering. */

// Copyright (c) NVIDIA Corporation. All rights reserved.

/* Requires the OpenGL Utility Toolkit (GLUT) and Cg runtime (version
   2.0 or higher). */

#define _USE_MATH_DEFINES  // so <math.h> has M_PI

#include "nvpr_svg_config.h"  // configure path renderers to use

#if USE_QT

# if defined(_MSC_VER)
#  pragma comment (lib, "QtCore4.lib")
# endif

#include "renderer_qt.hpp"
#include "scene.hpp"
#include "path.hpp"

#include "scene_qt.hpp"

#include <Cg/degrees.hpp>

#if _MSC_VER
# pragma comment (lib, "QtGui4.lib")         // link with Qt lib
#endif

#include <QtCore/qmath.h>
#define Q_PI M_PI

// Release builds shouldn't have verbose conditions.
#ifdef NDEBUG
#define verbose (0)
#else
extern int verbose;
#endif

// BEGIN code from qt/src/svg/qsvghandler.cpp
static void pathArcSegment(QPainterPath &path,
                           qreal xc, qreal yc,
                           qreal th0, qreal th1,
                           qreal rx, qreal ry, qreal xAxisRotation)
{
    qreal sinTh, cosTh;
    qreal a00, a01, a10, a11;
    qreal x1, y1, x2, y2, x3, y3;
    qreal t;
    qreal thHalf;

    sinTh = qSin(xAxisRotation * (Q_PI / 180.0));
    cosTh = qCos(xAxisRotation * (Q_PI / 180.0));

    a00 =  cosTh * rx;
    a01 = -sinTh * ry;
    a10 =  sinTh * rx;
    a11 =  cosTh * ry;

    thHalf = 0.5 * (th1 - th0);
    t = (8.0 / 3.0) * qSin(thHalf * 0.5) * qSin(thHalf * 0.5) / qSin(thHalf);
    x1 = xc + qCos(th0) - t * qSin(th0);
    y1 = yc + qSin(th0) + t * qCos(th0);
    x3 = xc + qCos(th1);
    y3 = yc + qSin(th1);
    x2 = x3 + t * qSin(th1);
    y2 = y3 - t * qCos(th1);

    path.cubicTo(a00 * x1 + a01 * y1, a10 * x1 + a11 * y1,
                 a00 * x2 + a01 * y2, a10 * x2 + a11 * y2,
                 a00 * x3 + a01 * y3, a10 * x3 + a11 * y3);
}

// the arc handling code underneath is from XSVG (BSD license)
/*
 * Copyright  2002 USC/Information Sciences Institute
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Information Sciences Institute not be used in advertising or
 * publicity pertaining to distribution of the software without
 * specific, written prior permission.  Information Sciences Institute
 * makes no representations about the suitability of this software for
 * any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * INFORMATION SCIENCES INSTITUTE DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL INFORMATION SCIENCES
 * INSTITUTE BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
static void pathArc(QPainterPath &path,
                    qreal               rx,
                    qreal               ry,
                    qreal               x_axis_rotation,
                    int         large_arc_flag,
                    int         sweep_flag,
                    qreal               x,
                    qreal               y,
                    qreal curx, qreal cury)
{
    qreal sin_th, cos_th;
    qreal a00, a01, a10, a11;
    qreal x0, y0, x1, y1, xc, yc;
    qreal d, sfactor, sfactor_sq;
    qreal th0, th1, th_arc;
    int i, n_segs;
    qreal dx, dy, dx1, dy1, Pr1, Pr2, Px, Py, check;

    rx = qAbs(rx);
    ry = qAbs(ry);

    sin_th = qSin(x_axis_rotation * (Q_PI / 180.0));
    cos_th = qCos(x_axis_rotation * (Q_PI / 180.0));

    dx = (curx - x) / 2.0;
    dy = (cury - y) / 2.0;
    dx1 =  cos_th * dx + sin_th * dy;
    dy1 = -sin_th * dx + cos_th * dy;
    Pr1 = rx * rx;
    Pr2 = ry * ry;
    Px = dx1 * dx1;
    Py = dy1 * dy1;
    /* Spec : check if radii are large enough */
    check = Px / Pr1 + Py / Pr2;
    if (check > 1) {
        rx = rx * qSqrt(check);
        ry = ry * qSqrt(check);
    }

    a00 =  cos_th / rx;
    a01 =  sin_th / rx;
    a10 = -sin_th / ry;
    a11 =  cos_th / ry;
    x0 = a00 * curx + a01 * cury;
    y0 = a10 * curx + a11 * cury;
    x1 = a00 * x + a01 * y;
    y1 = a10 * x + a11 * y;
    /* (x0, y0) is current point in transformed coordinate space.
       (x1, y1) is new point in transformed coordinate space.

       The arc fits a unit-radius circle in this space.
    */
    d = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
    sfactor_sq = 1.0 / d - 0.25;
    if (sfactor_sq < 0) sfactor_sq = 0;
    sfactor = qSqrt(sfactor_sq);
    if (sweep_flag == large_arc_flag) sfactor = -sfactor;
    xc = 0.5 * (x0 + x1) - sfactor * (y1 - y0);
    yc = 0.5 * (y0 + y1) + sfactor * (x1 - x0);
    /* (xc, yc) is center of the circle. */

    th0 = atan2(y0 - yc, x0 - xc);
    th1 = atan2(y1 - yc, x1 - xc);

    th_arc = th1 - th0;
    if (th_arc < 0 && sweep_flag)
        th_arc += 2 * Q_PI;
    else if (th_arc > 0 && !sweep_flag)
        th_arc -= 2 * Q_PI;

    n_segs = qCeil(qAbs(th_arc / (Q_PI * 0.5 + 0.001)));

    for (i = 0; i < n_segs; i++) {
        pathArcSegment(path, xc, yc,
                       th0 + i * th_arc / n_segs,
                       th0 + (i + 1) * th_arc / n_segs,
                       rx, ry, x_axis_rotation);
    }
}
// END code from qt/src/svg/qsvghandler.cpp

struct QtPathCacheProcessor : PathSegmentProcessor {
    QPainterPtr painter;
    QPainterPath &path;

    QtPathCacheProcessor(QPainterPtr p, QPainterPath &path_)
        : painter(p)
        , path(path_)
    { }

    void beginPath(PathPtr p) {
        path = QPainterPath();  // reset the path
        switch (p->style.fill_rule) {
        case PathStyle::EVEN_ODD:
            assert(path.fillRule() == Qt::OddEvenFill);
            break;
        case PathStyle::NON_ZERO:
            path.setFillRule(Qt::WindingFill);
            break;
        default:
            assert(!"bogus style.fill_rule");
            break;
        }
    }
    void moveTo(const float2 plist[2], size_t coord_index, char cmd) {
        path.moveTo(plist[1].x, plist[1].y);
    }
    void lineTo(const float2 plist[2], size_t coord_index, char cmd) {
        path.lineTo(plist[1].x, plist[1].y);
    }
    void quadraticCurveTo(const float2 plist[3], size_t coord_index, char cmd) {
        path.quadTo(plist[1].x, plist[1].y,
                    plist[2].x, plist[2].y);
    }
    void cubicCurveTo(const float2 plist[4], size_t coord_index, char cmd) {
        path.cubicTo(plist[1].x, plist[1].y,
                     plist[2].x, plist[2].y,
                     plist[3].x, plist[3].y);
    }
    void arcTo(const EndPointArc &arc, size_t coord_index, char cmd) {
        // Convert to a center point arc to be able to render the arc center.
        CenterPointArc center_point_arc(arc);
        switch (center_point_arc.form) {
        case CenterPointArc::BEHAVED:
            {
                // Qt doesn't have an SVG-style partial elliptical arcTo
                // primitive so we approximate the SVG arcTo as a
                // sequence of cubic curves.
                //
                // Qt's path.arcTo doesn't have an x_axis_rotation
                // and generates a lineTo from the current position
                // to the arc start.  See:
                // http://doc.qt.nokia.com/4.6/qpainterpath.html#arcTo
                pathArc(path,
                        arc.radii.x, arc.radii.y,
                        arc.x_axis_rotation,
                        arc.large_arc_flag, arc.sweep_flag,
                        arc.p[1].x, arc.p[1].y,
                        arc.p[0].x, arc.p[0].y);
            }
            break;
        case CenterPointArc::DEGENERATE_LINE:
            path.lineTo(arc.p[1].x, arc.p[1].y);
            break;
        case CenterPointArc::DEGENERATE_POINT:
            // Do nothing.
            break;
        default:
            assert(!"bogus CenterPointArc form");
            break;
        }
    }
    void close(char cmd) {
        path.closeSubpath();
    }
    void endPath(PathPtr p) {
    }
};

void QtRenderer::configureSurface(int width, int height)
{
    image = QImagePtr(new QImage(width, height, QImage::Format_ARGB32));
    painter = QPainterPtr(new QPainter(image.get()));
    painter->setRenderHint(QPainter::Antialiasing);  // http://doc.trolltech.com/4.5/qpainter.html#RenderHint-enum
}

void QtRenderer::shutdown()
{
    // first null-out painter so it disconnects from image, then null-out image
    painter = QPainterPtr();
    image = QImagePtr();
}

void QtRenderer::clear(float3 clear_color)
{
    QColor ccolor;
    ccolor.setRgbF(clear_color.r, clear_color.g, clear_color.b);
    QBrush clear_brush(ccolor);
    painter->setOpacity(1.0);
    painter->resetTransform();
    painter->fillRect(0, 0, image->width(), image->height(), clear_brush);
}

void QtRenderer::setView(float4x4 view)
{
    extern float scene_ratio;
    const float2 scale = clipToSurfaceScales(image->width(), image->height(), scene_ratio);

    // Establish transform
    QTransform transform;
    // Step 1: make mapping from [-1,+1] clip space to pixel space
    transform.scale(image->width()/2.0, image->height()/2.0);
    transform.translate(1, 1);
    transform.scale(scale.x, scale.y);

    // Step 2: make a mapping from surface space to clip space
    QTransform v(view[0][0], view[1][0], view[3][0],
                 view[0][1], view[1][1], view[3][1],
                 view[0][3], view[1][3], view[3][3]);
    // XXX Qt doesn't actually appear to handle projective transform correctly.
    transform = v * transform;
    painter->setTransform(transform);
}

VisitorPtr QtRenderer::makeVisitor()
{
    return VisitorPtr(new QtVisitors::Draw(
        dynamic_pointer_cast<QtRenderer>(shared_from_this())));
}

void QtRenderer::copyImageToWindow()
{
    glWindowPos2f(0,0);

    const int w = image->width(),
              h = image->height();

    const unsigned char *pixels = image->bits();

    assert(image->bytesPerLine()  == w*4);
    glDrawPixels(w, h, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels);
}

const char *QtRenderer::getWindowTitle()
{
	return "Qt path rendering";
}

const char *QtRenderer::getName()
{
    return "Qt";
}

shared_ptr<RendererState<Path> > QtRenderer::alloc(Path *owner)
{
    return QtPathRendererStatePtr(new QtPathRendererState(shared_from_this(), owner));
}

shared_ptr<RendererState<Shape> > QtRenderer::alloc(Shape *owner)
{
    return QtShapeRendererStatePtr(new QtShapeRendererState(shared_from_this(), owner));
}

void QtPathRendererState::validate()
{
    if (valid) {
        return;
    }

    QtRendererPtr renderer = getRenderer();
    QtPathCacheProcessor processor(renderer->painter, path);
    owner->processSegments(processor);
    valid = true;
}

void QtPathRendererState::invalidate()
{
    // XXX eventually the path's QPainterPath could be validated here
    valid = false;
}

static Qt::PenCapStyle lineCapConverter(const PathStyle *style)
{
    switch (style->line_cap) {
    default:
        assert(!"bad line_cap");
    case PathStyle::BUTT_CAP:
        return Qt::FlatCap;
    case PathStyle::ROUND_CAP:
        return Qt::RoundCap;
    case PathStyle::SQUARE_CAP:
        return Qt::SquareCap;
    case PathStyle::TRIANGLE_CAP:
        printf("Qt lacks triangle cap\n");
        return Qt::FlatCap;
    }
}

static Qt::PenJoinStyle lineJoinConverter(const PathStyle *style)
{
    switch (style->line_join) {
    default:
        assert(!"bad line_join");
    case PathStyle::MITER_TRUNCATE_JOIN:
        return Qt::MiterJoin;  // exceeding miter limit "truncates" miter edges to stroke_width*miter_limit
    case PathStyle::MITER_REVERT_JOIN:
        return Qt::SvgMiterJoin;  // exceeding miter limit "snaps" to bevel
    case PathStyle::ROUND_JOIN:
        return Qt::RoundJoin;
    case PathStyle::BEVEL_JOIN:
        return Qt::BevelJoin;
    case PathStyle::NONE_JOIN:
        printf("Qt doesn't support 'none' join, treating as bevel\n");
        return Qt::BevelJoin;
    }
}

void QtShapeRendererState::getBrushes(QBrush &fill_brush, QBrush &stroke_brush)
{
    fill_brush = QBrush(Qt::NoBrush);
    stroke_brush = QBrush(Qt::NoBrush);

    assert(owner);

    const PaintPtr fill_paint = owner->getFillPaint();
    if (fill_paint) {
        PaintRendererStatePtr renderer_state = fill_paint->getRendererState(getRenderer());
        QtPaintRendererStatePtr paint_renderer_state = dynamic_pointer_cast<QtPaintRendererState>(renderer_state);
        assert(paint_renderer_state);
        if (paint_renderer_state) {
            fill_brush = paint_renderer_state->getBrush(owner->net_fill_opacity);

            if (fill_brush.style() == Qt::TexturePattern) {
                float4 bounds = owner->getBounds();  // returns two (x,y) pairs

                float w = fill_brush.textureImage().width(),
                      h = fill_brush.textureImage().height();

                float2 p1 = bounds.xy,
                       p2 = bounds.zw,
                       diff = p2-p1;

                // Inverse orthographic matrix
                QMatrix inv_ortho(diff.x/w,0,
                                  0,diff.y/h,
                                  0,0);

                fill_brush.setMatrix(inv_ortho);
            }
        }
    }

    const PaintPtr stroke_paint = owner->getStrokePaint();
    if (stroke_paint) {
        PaintRendererStatePtr renderer_state = stroke_paint->getRendererState(getRenderer());
        QtPaintRendererStatePtr paint_renderer_state = dynamic_pointer_cast<QtPaintRendererState>(renderer_state);
        assert(paint_renderer_state);
        if (paint_renderer_state) {
            stroke_brush = paint_renderer_state->getBrush(owner->net_stroke_opacity);
        }
    }
}

void QtShapeRendererState::validate()
{
    if (valid) {
        return;
    }

    // validate brushes
    QBrush fill_brush, stroke_brush;
    getBrushes(fill_brush, stroke_brush);

    brush = fill_brush;

    pen.setBrush(stroke_brush);
    const PathStyle *style = &owner->getPath()->style;
    pen.setWidthF(style->stroke_width);
    pen.setCapStyle(lineCapConverter(style));
    pen.setJoinStyle(lineJoinConverter(style));
    pen.setMiterLimit(style->miter_limit);
    if (style->dash_array.size() > 0) {
        QVector<qreal> pattern;
        int n = int(style->dash_array.size());
        pattern.reserve(n);
        // Qt's dash pattern is specified in units of the pens width,
        // e.g. a dash of length 5 in width 10 is 50 pixels long.
        for (int i = 0; i<n; i++) {
            pattern.push_back(style->dash_array[i] / style->stroke_width);
        }
        // "If an odd number of values is provided,
        if (n & 1) {
            // then the list of values is repeated to yield an even number
            // of values. Thus, stroke-dasharray: 5,3,2 is equivalent to
            // stroke-dasharray: 5,3,2,5,3,2."
            for (int i = 0; i<n; i++) {
                pattern.push_back(style->dash_array[i] / style->stroke_width);
            }
        }
        pen.setDashPattern(pattern);
        // "The offset is measured in terms of the units used to
        // specify the dash pattern." (so divide by stroke width).
        pen.setDashOffset(style->dash_offset / style->stroke_width);
    } else {
        pen.setDashPattern(QVector<qreal>());
    }

    valid = true;
}

void QtShapeRendererState::invalidate()
{
    valid = false;
}

void QtShapeRendererState::draw()
{
    QtRendererPtr renderer = getRenderer();

    // validate brushes
    validate();

    // validate the path
    QtPathRendererStatePtr path_state = getPathRendererState();
    path_state->validate();

    extern bool doFilling, doStroking;
    const PathStyle &style = owner->getPath()->style;
    if (style.do_fill && doFilling) {
        renderer->painter->fillPath(path_state->path, brush);
    }
    if (style.do_stroke && doStroking) {
        // From http://qt.nokia.com/doc/4.6/qpen.html#setWidth
        // "A line width of zero indicates a cosmetic pen. This
        // means that the pen width is always drawn one pixel wide,
        // independent of the transformation set on the painter."
        //
        // Skip zero width stroking to avoid Qt's cosmetic pen.
        if (style.stroke_width > 0) {
            renderer->painter->strokePath(path_state->path, pen);
        }
    }
}

void QtShapeRendererState::getPaths(vector<QPainterPath*>& paths)
{
    QtRendererPtr renderer = getRenderer();

    // validate the path
    QtPathRendererStatePtr path_state = getPathRendererState();
    path_state->validate();

    paths.push_back(&path_state->path);
}

shared_ptr<RendererState<Paint> > QtRenderer::alloc(Paint *owner)
{
    SolidColorPaint *solid_color_paint = dynamic_cast<SolidColorPaint*>(owner);
    if (solid_color_paint) {
        return QtSolidColorPaintRendererStatePtr(new QtSolidColorPaintRendererState(shared_from_this(), solid_color_paint));
    }

    LinearGradientPaint *linear_gradient_paint = dynamic_cast<LinearGradientPaint*>(owner);
    if (linear_gradient_paint) {
        return QtLinearGradientPaintRendererStatePtr(new QtLinearGradientPaintRendererState(shared_from_this(), linear_gradient_paint));
    }

    RadialGradientPaint *radial_gradient_paint = dynamic_cast<RadialGradientPaint*>(owner);
    if (radial_gradient_paint) {
        return QtRadialGradientPaintRendererStatePtr(new QtRadialGradientPaintRendererState(shared_from_this(), radial_gradient_paint));
    }

    ImagePaint *image_paint = dynamic_cast<ImagePaint*>(owner);
    if (image_paint) {
        return QtImagePaintRendererStatePtr(new QtImagePaintRendererState(shared_from_this(), image_paint));
    }

    assert(!"paint unsupported by Qt renderer");
    return QtPaintRendererStatePtr();
}

void QtPaintRendererState::invalidate()
{
    valid = false;
}

QtSolidColorPaintRendererState::QtSolidColorPaintRendererState(RendererPtr renderer, SolidColorPaint *paint)
    : QtPaintRendererState(renderer, paint)
{}

void QtSolidColorPaintRendererState::validate(float opacity)
{
    if (valid && opacity == this->opacity) {
        return;
    }
    SolidColorPaint *paint = dynamic_cast<SolidColorPaint*>(owner);
    assert(paint);
    if (paint) {
        QColor color;

        const float4 solid_color = paint->getColor();
        color.setRgbF(solid_color.r, solid_color.g, solid_color.b);
        color.setAlphaF(solid_color.a * opacity);
        brush.setColor(color);
        brush.setStyle(Qt::SolidPattern);
    }
    valid = true;
}

static QGradient::Spread convert_SVG_spread_method_to_QGradientSpread(SpreadMethod v)
{
    switch (v) {
    case PAD:
        return QGradient::PadSpread;
    case REFLECT:
        return QGradient::ReflectSpread;
    case REPEAT:
        return QGradient::RepeatSpread;
    default:
        assert(!"bogus spread method");
    case NONE:
        assert(!"none spread not supported by Qt");
        return QGradient::PadSpread;
    }
}

QtGradientPaintRendererState::QtGradientPaintRendererState(RendererPtr renderer, GradientPaint *paint)
    : QtPaintRendererState(renderer, paint)
{}

void QtGradientPaintRendererState::setGradientStops(QGradient &gradient, const GradientPaint *paint, float opacity)
{
    QVector<QGradientStop> stops;

    const vector<GradientStop> &stop_array = paint->getStopArray();
    for (vector<GradientStop>::const_iterator stop = stop_array.begin();
        stop != stop_array.end();
        ++stop) {
        QColor color;
        color.setRgbF(stop->color.r, stop->color.g, stop->color.b);
        color.setAlphaF(stop->color.a * opacity);
        stops.push_back(QGradientStop(stop->offset, color));
    }
    gradient.setStops(stops);
}

void QtGradientPaintRendererState::setGenericGradientParameters(QGradient &gradient, const GradientPaint *paint, float opacity)
{
    setGradientStops(gradient, paint, opacity);

    if (paint->getGradientUnits() == USER_SPACE_ON_USE) {
        gradient.setCoordinateMode(QGradient::LogicalMode);
    } else {
        assert(paint->getGradientUnits() == OBJECT_BOUNDING_BOX);
        gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    }

    gradient.setSpread(convert_SVG_spread_method_to_QGradientSpread(paint->getSpreadMethod()));
}

QtLinearGradientPaintRendererState::QtLinearGradientPaintRendererState(RendererPtr renderer, LinearGradientPaint *paint)
    : QtGradientPaintRendererState(renderer, paint)
{}

void QtLinearGradientPaintRendererState::validate(float opacity)
{
    if (valid && opacity == this->opacity) {
        return;
    }
    const LinearGradientPaint *paint = dynamic_cast<LinearGradientPaint*>(owner);
    assert(paint);
    if (paint) {
        QLinearGradient linear_gradient(paint->getV1().x, paint->getV1().y,
                                        paint->getV2().x, paint->getV2().y);

        setGenericGradientParameters(linear_gradient, paint, opacity);

        brush = QBrush(linear_gradient);
        const float3x3 &gradient_transform = paint->getGradientTransform();
        brush.setMatrix(QMatrix(gradient_transform[0][0],gradient_transform[1][0],
                        gradient_transform[0][1],gradient_transform[1][1],
                        gradient_transform[0][2],gradient_transform[1][2]));
        brush.setStyle(Qt::LinearGradientPattern);
    }
    valid = true;
}

QtRadialGradientPaintRendererState::QtRadialGradientPaintRendererState(RendererPtr renderer, RadialGradientPaint *paint)
    : QtGradientPaintRendererState(renderer, paint)
{}

void QtRadialGradientPaintRendererState::validate(float opacity)
{
    if (valid && opacity == this->opacity) {
        return;
    }
    const RadialGradientPaint *paint = dynamic_cast<RadialGradientPaint*>(owner);
    assert(paint);
    if (paint) {
        QRadialGradient radial_gradient(paint->getCenter().x, paint->getCenter().y,
                                        paint->getRadius(),
                                        paint->getFocalPoint().x, paint->getFocalPoint().y);

        setGenericGradientParameters(radial_gradient, paint, opacity);

        brush = QBrush(radial_gradient);
        const float3x3 &gradient_transform = paint->getGradientTransform();
        brush.setMatrix(QMatrix(gradient_transform[0][0],gradient_transform[1][0],
                                gradient_transform[0][1],gradient_transform[1][1],
                                gradient_transform[0][2],gradient_transform[1][2]));
        brush.setStyle(Qt::RadialGradientPattern);
    }
    valid = true;
}

// QtImagePaintRendererState

QtImagePaintRendererState::QtImagePaintRendererState(RendererPtr renderer, ImagePaint *paint)
    : QtPaintRendererState(renderer, paint)
{
    const int width = paint->image->width,
              height = paint->image->height;

    image = QImage(width, height, QImage::Format_ARGB32_Premultiplied);

    unsigned int *argb = reinterpret_cast<unsigned int *>(image.bits());
    const RasterImage::Pixel *pixels = paint->image->pixels;
    for (int i=0; i<height; i++) {
        for (int j=0; j<width; j++) {
            *argb = (pixels->a << 24) | (pixels->r << 16) | (pixels->g << 8) | (pixels->b << 0);
            argb++;
            pixels++;
        }
    }
}

void QtImagePaintRendererState::validate(float opacity)
{
    if (valid && opacity == this->opacity) {
        return;
    }
    const ImagePaint *paint = dynamic_cast<ImagePaint*>(owner);
    assert(paint);
    if (paint) {
        setOpacity(opacity);
        // XXX should multiply image by opacity here??
        brush.setTextureImage(image);
    }
    valid = true;
}

#endif // USE_QT
