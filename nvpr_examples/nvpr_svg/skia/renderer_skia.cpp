
/* renderer_skia.cpp - Skia path rendering back-end for nvpr_svg. */

// Copyright (c) NVIDIA Corporation. All rights reserved.

#define _USE_MATH_DEFINES  // so <math.h> has M_PI

#include "nvpr_svg_config.h"  // configure path renderers to use

#if USE_SKIA

#include "renderer_skia.hpp"
#include "scene.hpp"
#include "path.hpp"

#include "scene_skia.hpp"

#include <SkDashPathEffect.h>  // http://skia.googlecode.com/svn/trunk/docs/html/class_sk_dash_path_effect.html
#include <SkPixelRef.h>  // http://skia.googlecode.com/svn/trunk/docs/html/class_sk_pixel_ref.html

#include <Cg/degrees.hpp>

#if _MSC_VER
# pragma comment (lib, "skia.lib")         // link with Skia lib
#endif

// Release builds shouldn't have verbose conditions.
#ifdef NDEBUG
#define verbose (0)
#else
extern int verbose;
#endif

// Do our best to use Qt code "as-is" with no modifications
// This Qt arc-generation code is verbatim in render_qt.cpp as well
#define Q_PI M_PI
typedef double qreal;  // from qglobal.h
#define qSin(x) sin(x)
#define qCos(x) cos(x)
#define qSqrt(x) sqrt(x)
#define qCeil(x) int(ceil(x))
#define qAbs(x) fabs(x)
#define cubicTo(x1,y1,x2,y2,x3,y3) cubicTo(SkScalar(x1), SkScalar(y1), SkScalar(x2), SkScalar(y2), SkScalar(x3), SkScalar(y3))

// BEGIN code from qt/src/svg/qsvghandler.cpp

static void pathArcSegment(SkPath &path,
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
static void pathArc(SkPath &path,
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

struct SkiaPathCacheProcessor : PathSegmentProcessor {
    SkPath &path;

    SkiaPathCacheProcessor(SkPath &path_)
        : path(path_)
    { }

    void beginPath(PathPtr p) {
        path = SkPath();  // reset the path
        switch (p->style.fill_rule) {
        case PathStyle::EVEN_ODD:
            path.setFillType(SkPath::kEvenOdd_FillType);
            break;
        case PathStyle::NON_ZERO:
            assert(path.getFillType() == SkPath::kWinding_FillType);
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
                // Skia doesn't have an SVG-style partial elliptical arcTo
                // primitive so we approximate the SVG arcTo as a
                // sequence of cubic curves.
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
        path.close();
    }
    void endPath(PathPtr p) {
    }
};

void SkiaRenderer::configureSurface(int width, int height)
{
    bitmap.setConfig(SkBitmap::kARGB_8888_Config, width, height);
    bitmap.allocPixels();  // have the bitmap allocate memory for the pixels offscreen
    canvas.setBitmapDevice(bitmap);
}

void SkiaRenderer::shutdown()
{
}

void SkiaRenderer::clear(float3 clear_color)
{
    bitmap.eraseRGB(U8CPU(clear_color.r * 255),
                    U8CPU(clear_color.g * 255),
                    U8CPU(clear_color.b * 255));
}

void SkiaRenderer::setView(float4x4 view)
{
    extern float scene_ratio;
    const float2 scale = clipToSurfaceScales(bitmap.width(), bitmap.height(), scene_ratio);

    canvas.resetMatrix();
    canvas.scale(bitmap.width()/2.0f, bitmap.height()/2.0f);
    canvas.translate(1, 1);
    canvas.scale(scale.x, scale.y);
    SkMatrix m;
    m[0] = view[0][0];
    m[1] = view[0][1];
    m[2] = view[0][3];
    m[3] = view[1][0];
    m[4] = view[1][1];
    m[5] = view[1][3];
    // XXX skia mostly handles projection correctly unless eye plane clipping required
    // XXX at least it looks like eye plane clipping is when the issues appear to start
    m[6] = view[3][0];
    m[7] = view[3][1];
    m[8] = view[3][3];
    canvas.concat(m);
}

VisitorPtr SkiaRenderer::makeVisitor()
{
    return VisitorPtr(new SkiaVisitors::Draw(
        dynamic_pointer_cast<SkiaRenderer>(shared_from_this())));
}

void SkiaRenderer::copyImageToWindow()
{
    glWindowPos2f(0,0);

    //bitmap.lockPixels();

    const int w = bitmap.width(),
              h = bitmap.height();

    const void *pixels = bitmap.getPixels();

    assert(bitmap.rowBytes() == w*4);
    // Skia's RGBA component layout philosophy is "default to OpenGL order (in memory: r,g,b,a)".
    // See comment and definitions in "SkColorPrive.h".
    glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    //bitmap.unlockPixels();
}

const char *SkiaRenderer::getWindowTitle()
{
    return "Skia path rendering";
}

const char *SkiaRenderer::getName()
{
    return "Skia";
}


shared_ptr<RendererState<Path> > SkiaRenderer::alloc(Path *owner)
{
    return SkiaPathRendererStatePtr(new SkiaPathRendererState(shared_from_this(), owner));
}

shared_ptr<RendererState<Shape> > SkiaRenderer::alloc(Shape *owner)
{
    return SkiaShapeRendererStatePtr(new SkiaShapeRendererState(shared_from_this(), owner));
}

void SkiaPathRendererState::validate()
{
    if (valid) {
        return;
    }

    SkiaRendererPtr renderer = getRenderer();
    SkiaPathCacheProcessor processor(path);
    owner->processSegments(processor);
    valid = true;
}

void SkiaPathRendererState::invalidate()
{
    valid = false;
}

void SkiaShapeRendererState::validate()
{
    const Shape *shape = owner;

    PaintPtr fill_paint = shape->getFillPaint();
    if (fill_paint) {
        PaintRendererStatePtr renderer_state = fill_paint->getRendererState(getRenderer());
        SkiaPaintRendererStatePtr paint_renderer_state = dynamic_pointer_cast<SkiaPaintRendererState>(renderer_state);
        paint_renderer_state->validate(shape->net_fill_opacity);
    }

    const PaintPtr stroke_paint = shape->getStrokePaint();
    if (stroke_paint) {
        PaintRendererStatePtr renderer_state = stroke_paint->getRendererState(getRenderer());
        SkiaPaintRendererStatePtr paint_renderer_state = dynamic_pointer_cast<SkiaPaintRendererState>(renderer_state);
        paint_renderer_state->validate(shape->net_stroke_opacity);
    }

    if (valid) {
        return;
    }
    // Nothing about the shape to actually validate.
    valid = true;
}

SkPaint& SkiaShapeRendererState::getPaint(PaintPtr paint, float opacity)
{
    PaintRendererStatePtr renderer_state = paint->getRendererState(getRenderer());
    SkiaPaintRendererStatePtr paint_renderer_state = dynamic_pointer_cast<SkiaPaintRendererState>(renderer_state);
    assert (paint_renderer_state);

    paint_renderer_state->validate(opacity);

    SkPaint& sk_paint = paint_renderer_state->getPaint();

    if (paint_renderer_state->needsGradientMatrix()) {
        GradientPaintPtr gradient = dynamic_pointer_cast<GradientPaint>(paint);
        SkiaGradientPaintRendererStatePtr gradient_state = dynamic_pointer_cast<SkiaGradientPaintRendererState>(paint_renderer_state);

        SkMatrix matrix;
        const float3x3 &gradient_transform = gradient->getGradientTransform();

        matrix[0] = gradient_transform[0][0];
        matrix[1] = gradient_transform[0][1];
        matrix[2] = gradient_transform[0][2];

        matrix[3] = gradient_transform[1][0];
        matrix[4] = gradient_transform[1][1];
        matrix[5] = gradient_transform[1][2];

        matrix[6] = gradient_transform[2][0];
        matrix[7] = gradient_transform[2][1];
        matrix[8] = gradient_transform[2][2];
        if (gradient->getGradientUnits() == OBJECT_BOUNDING_BOX) {
            float4 bounds = owner->getBounds();
            float2 p1 = bounds.xy,
                   p2 = bounds.zw,
                   diff = p2-p1,
                   sum = p2+p1;

            SkMatrix b;
            b[0] = 1/diff.x;
            b[1] = 0;
            b[2] = -0.5f*sum.x/diff.x+0.5f;
            b[3] = 0;
            b[4] = 1/diff.y;
            b[5] = -0.5f*sum.y/diff.y+0.5f;
            b[6] = 0;
            b[7] = 0;
            b[8] = 1;
            matrix.preConcat(b);
            matrix.invert(&matrix);
        }
        gradient_state->gradient->setLocalMatrix(matrix);
    } else {
        ImagePaintPtr image = dynamic_pointer_cast<ImagePaint>(paint);
        if (image) {
            SkiaImagePaintRendererStatePtr image_state =
                dynamic_pointer_cast<SkiaImagePaintRendererState>(paint_renderer_state);

            float w = image->image->width,
                  h = image->image->height;

            SkMatrix matrix;
            matrix.setScale(w, h);
            {
                float4 bounds = owner->getBounds();
                float2 p1 = bounds.xy,
                       p2 = bounds.zw,
                       diff = p2-p1,
                       sum = p2+p1;

                SkMatrix b;
                b[0] = 1/diff.x;
                b[1] = 0;
                b[2] = -0.5f*sum.x/diff.x+0.5f;
                b[3] = 0;
                b[4] = 1/diff.y;
                b[5] = -0.5f*sum.y/diff.y+0.5f;
                b[6] = 0;
                b[7] = 0;
                b[8] = 1;
                matrix.preConcat(b);
                matrix.invert(&matrix);
            }   
            image_state->bitmap->setLocalMatrix(matrix);
        }
    }

    return sk_paint;
}

void SkiaShapeRendererState::invalidate()
{
    valid = false;
}

static SkPaint::Cap lineCapConverter(PathStyle::LineCap cap)
{
    switch (cap) {
    default:
        assert(!"bad line_cap");
    case PathStyle::BUTT_CAP:
        return SkPaint::kButt_Cap;
    case PathStyle::ROUND_CAP:
        return SkPaint::kRound_Cap;
    case PathStyle::SQUARE_CAP:
        return SkPaint::kSquare_Cap;
    case PathStyle::TRIANGLE_CAP:
        printf("Cairo lacks triangle cap\n");
        return SkPaint::kButt_Cap;
    }
}

static SkPaint::Join lineJoinConverter(PathStyle::LineJoin join)
{
    switch (join) {
    default:
        assert(!"bad line_join");
    case PathStyle::MITER_TRUNCATE_JOIN:
        return SkPaint::kMiter_Join;
    case PathStyle::MITER_REVERT_JOIN:
        return SkPaint::kMiter_Join;
    case PathStyle::ROUND_JOIN:
        return SkPaint::kRound_Join;
    case PathStyle::BEVEL_JOIN:
        return SkPaint::kBevel_Join;
    case PathStyle::NONE_JOIN:
        printf("Cairo doesn't support 'none' join, treating as bevel\n");
        return SkPaint::kBevel_Join;
    }
}

void SkiaShapeRendererState::draw()
{
    SkiaRendererPtr renderer = getRenderer();

    // validate the path
    SkiaPathRendererStatePtr path_state = getPathRendererState();
    path_state->validate();

    extern bool doFilling, doStroking;
    const PathStyle &style = owner->getPath()->style;
    if (style.do_fill && doFilling) {
	      PaintPtr fill = owner->getFillPaint();
        SkPaint &sk_paint = getPaint(fill, owner->net_fill_opacity);
        // For the SkPaint style to be filled in case the paint was
        // also used for stroking.
        sk_paint.setStyle(SkPaint::kFill_Style);
        renderer->canvas.drawPath(path_state->path, sk_paint);
    }
    if (style.do_stroke && doStroking) {
	    PaintPtr stroke = owner->getStrokePaint();
        // Skia associates stroking properties with the SkPaint rather
        // than the path.  This is an odd conceptual choice because
        // "paint" colloquially determines color but not shape.
        //
        // Indeed, Skia even associate the rendering mode (stroking or
        // filling) with the paint.
        //
        // SVG more appropriately keeps the path properties (to determine
        // the styling shape of the path) separate from the filling and
        // stroking paint.
        //
        // To match the Skia model, we have to force the sk_paint object
        // to adopt the path's shaping properties everytime since there's
        // no assurance the paint might not have been used by another path.
        // Potentially this could be optimized for solid color paints
        // that won't be shared among paths.
        //
        // Fortunately setting these properties in Skia is "cheap".
        SkPaint &sk_paint = getPaint(stroke, owner->net_stroke_opacity);

        sk_paint.setStyle(SkPaint::kStroke_Style);

        const PathStyle &style = owner->getPath()->style;

        // Set Skia 
        sk_paint.setStrokeWidth(style.stroke_width);
        sk_paint.setStrokeMiter(style.miter_limit);
        sk_paint.setStrokeCap(lineCapConverter(style.line_cap));
        sk_paint.setStrokeJoin(lineJoinConverter(style.line_join));

        // Is the path dashed?
        if (style.dash_array.size() > 0) {
            // Yes, specify a Skia dash path effect for the paint...
            size_t count = style.dash_array.size();
            // Odd dash array sizes requires us to double the dash array to make it even.
            size_t sk_count = count & 1 ? count*2 : count;
            vector<SkScalar> dash_array(sk_count);
            for (size_t i=0; i<count; i++) {
                dash_array[i] = style.dash_array[i];
            }
            // Does dash array have an odd count?
            if (count & 1) {
                // Yes, repeat the dash array again for Skia.
                for (size_t i=0; i<count; i++) {
                    dash_array[i+count] = style.dash_array[i];
                }
            }
            assert(dash_array.size() == sk_count);
            bool scale_to_fit = false;
            // Note the Skia idiom of the unref after the new & set.
            sk_paint.setPathEffect(new SkDashPathEffect(&dash_array[0], int(sk_count),
                                                        scale_to_fit))->unref();
        } else {
            // No, disable the Skia path effect in case prior use had it dashed.
            sk_paint.setPathEffect(0);
        }

        renderer->canvas.drawPath(path_state->path, sk_paint);
    }
}

void SkiaShapeRendererState::getPaths(vector<SkPath*>& paths)
{
    SkiaPathRendererStatePtr path_state = getPathRendererState();
    path_state->validate();
    paths.push_back(&path_state->path);
}

shared_ptr<RendererState<Paint> > SkiaRenderer::alloc(Paint *owner)
{
    SolidColorPaint *solid_color_paint = dynamic_cast<SolidColorPaint*>(owner);
    if (solid_color_paint) {
        return SkiaSolidColorPaintRendererStatePtr(new SkiaSolidColorPaintRendererState(shared_from_this(), solid_color_paint));
    }

    LinearGradientPaint *linear_gradient_paint = dynamic_cast<LinearGradientPaint*>(owner);
    if (linear_gradient_paint) {
        return SkiaLinearGradientPaintRendererStatePtr(new SkiaLinearGradientPaintRendererState(shared_from_this(), linear_gradient_paint));
    }

    RadialGradientPaint *radial_gradient_paint = dynamic_cast<RadialGradientPaint*>(owner);
    if (radial_gradient_paint) {
        return SkiaRadialGradientPaintRendererStatePtr(new SkiaRadialGradientPaintRendererState(shared_from_this(), radial_gradient_paint));
    }

    ImagePaint *image_paint = dynamic_cast<ImagePaint*>(owner);
    if (image_paint) {
        return SkiaImagePaintRendererStatePtr(new SkiaImagePaintRendererState(shared_from_this(), image_paint));
    }

    assert(!"paint unsupported by Skia renderer");
    return SkiaPaintRendererStatePtr();
}

// SkiaPaintRendererState

SkiaPaintRendererState::SkiaPaintRendererState(RendererPtr renderer, Paint *paint, bool is_gradient)
    : SkiaRendererState<Paint>(renderer, paint)
    , valid(false)
    , sk_paint()
    , opacity(1.0)
    , needs_gradient_matrix(is_gradient)
{
    sk_paint.setFlags(SkPaint::kAntiAlias_Flag);
}

void SkiaPaintRendererState::invalidate()
{
    valid = false;
}

// SkiaSolidColorPaintRendererState

SkiaSolidColorPaintRendererState::SkiaSolidColorPaintRendererState(RendererPtr renderer,
                                                                   SolidColorPaint *paint)
    : SkiaPaintRendererState(renderer, paint, false)
{
}

void SkiaSolidColorPaintRendererState::validate(float opacity)
{
    if (valid && opacity == this->opacity) {
        return;
    }
    SolidColorPaint *p = dynamic_cast<SolidColorPaint*>(owner);
    assert(p);
    if (p) {
        float4 color = p->getColor();
        color.a *= opacity;
        color *= 255;
        sk_paint.setARGB(U8CPU(color.a),
                         U8CPU(color.r),
                         U8CPU(color.g),
                         U8CPU(color.b));
    }
    valid = true;
}

// SkiaGradientPaintRendererState

SkiaGradientPaintRendererState::SkiaGradientPaintRendererState(RendererPtr renderer, GradientPaint *paint)
    : SkiaPaintRendererState(renderer, paint, true)
    , gradient(0)
{
    setOpacity(opacity);
}

SkiaGradientPaintRendererState::~SkiaGradientPaintRendererState() {
    if (gradient) {
        gradient->unref();
    }
}

static SkShader::TileMode spread_method_to_skia_tile_mode(SpreadMethod spread_method)
{
    switch (spread_method) {
    case PAD:
        return SkShader::kClamp_TileMode;
    case REPEAT:
        return SkShader::kRepeat_TileMode;
    case REFLECT:
        return SkShader::kMirror_TileMode;
    default:
        assert(!"bogus spread_method");
        // Fall through...
    case NONE:  // NONE not really supported
        return SkShader::kClamp_TileMode;
    }
}

int SkiaGradientPaintRendererState::allocRampData(const GradientPaint *paint,
                                                  SkColor* &colors,
                                                  SkScalar* &pos,
                                                  SkShader::TileMode &mode) {
    const vector<GradientStop> &stop_array = paint->getStopArray();
    int count = int(stop_array.size());
    colors = new SkColor [count];
    pos = new SkScalar [count];
    if (colors && pos) {
        for (int i=0; i<count; i++) {
            float4 color = stop_array[i].color;
            color.a *= opacity;
            color *= 255;
            colors[i] = SkColorSetARGB(U8CPU(color.a),
                U8CPU(color.r),
                U8CPU(color.g),
                U8CPU(color.b));
            pos[i] = stop_array[i].offset;
        }
        mode = spread_method_to_skia_tile_mode(paint->getSpreadMethod());
        return count;
    } else {
        return 0;
    }
}

void SkiaGradientPaintRendererState::freeRampData(SkColor* &colors, SkScalar* &pos) {
    delete colors;
    delete pos;
}

// SkiaLinearGradientPaintRendererState

SkiaLinearGradientPaintRendererState::SkiaLinearGradientPaintRendererState(RendererPtr renderer, LinearGradientPaint *paint)
    : SkiaGradientPaintRendererState(renderer, paint)
{
    createShader(paint);
}

void SkiaLinearGradientPaintRendererState::createShader(const LinearGradientPaint *paint)
{
    SkColor *colors = NULL;
    SkScalar *pos = NULL;
    SkShader::TileMode mode;
    int count = allocRampData(paint, colors, pos, mode);
    SkUnitMapper* mapper = 0;
    SkPoint p[2] = { { paint->getV1().x, paint->getV1().y },
                     { paint->getV2().x, paint->getV2().y } };
    gradient = SkGradientShader::CreateLinear(p, colors, pos, count, mode, mapper);
    freeRampData(colors, pos);
    sk_paint.setShader(gradient);
}

void SkiaLinearGradientPaintRendererState::validate(float opacity)
{
    if (valid && opacity == this->opacity) {
        return;
    }
    const LinearGradientPaint *paint = dynamic_cast<LinearGradientPaint*>(owner);
    assert(paint);
    if (paint) {
        setOpacity(opacity);
        gradient->unref();
        // Skia has no way to update just the color ramp so we must update the entire linear gradient shader. :-(
        createShader(paint);
    }
    valid = true;
}

// SkiaGradientPaintRendererState

SkiaRadialGradientPaintRendererState::SkiaRadialGradientPaintRendererState(RendererPtr renderer, RadialGradientPaint *paint)
    : SkiaGradientPaintRendererState(renderer, paint)
{
    createShader(paint);
}

void SkiaRadialGradientPaintRendererState::createShader(const RadialGradientPaint *paint) {
    SkColor *colors = NULL;
    SkScalar *pos = NULL;
    SkShader::TileMode mode;
    int count = allocRampData(paint, colors, pos, mode);
    SkUnitMapper* mapper = 0;
    SkPoint center = { paint->getCenter().x, paint->getCenter().y },
            focal_point = { paint->getFocalPoint().x, paint->getFocalPoint().y };

    // http://www.w3.org/TR/SVG/pservers.html#RadialGradients says:
    // "If the point defined by ‘fx’ and ‘fy’ lies outside the circle defined
    // by ‘cx’, ‘cy’ and ‘r’, then the user agent shall set the focal point
    // to the intersection of the line from (‘cx’, ‘cy’) to (‘fx’, ‘fy’) with
    // the circle defined by ‘cx’, ‘cy’ and ‘r’."
    SkPoint vector = focal_point - center;
    float len = vector.length();
    float radius = paint->getRadius();
    if (len > radius) {
        // Fudge, has to be  somewhat less than 1 (0.999 is too
        // much) or Skia draws wrong.
        const float sub_unity = 199.0f/200;
        vector.scale(sub_unity*paint->getRadius()/len);
        focal_point = center + vector;
    }

    // It is an error for colorCount to be < 2, for startRadius or
    // endRadius to be < 0, or for startRadius to be equal to endRadius. 
    assert(count >= 2);
    assert(radius > 0);
    gradient = SkGradientShader::CreateTwoPointRadial(focal_point, 0, center, radius,
      colors, pos, count, mode, mapper);
    freeRampData(colors, pos);
    sk_paint.setShader(gradient);
}

void SkiaRadialGradientPaintRendererState::validate(float opacity)
{
    if (valid && opacity == this->opacity) {
        return;
    }
    const RadialGradientPaint *paint = dynamic_cast<RadialGradientPaint*>(owner);
    assert(paint);
    if (paint) {
        setOpacity(opacity);
        gradient->unref();
        // Skia has no way to update just the color ramp so we must update the entire radial gradient shader. :-(
        createShader(paint);
    }
    valid = true;
}

// SkiaImagePaintRendererState

SkiaImagePaintRendererState::SkiaImagePaintRendererState(RendererPtr renderer, ImagePaint *paint)
    : SkiaPaintRendererState(renderer, paint, false)
{
    SkBitmap bitmap_image;
    bitmap_image.setConfig(SkBitmap::kARGB_8888_Config, paint->image->width, paint->image->height);
    bitmap_image.allocPixels();
    SkPixelRef *pixels = bitmap_image.pixelRef();
    pixels->lockPixels();

    size_t bytes = bitmap_image.getSize();
    assert(bytes == 4U*paint->image->width*paint->image->height);
    void *data = pixels->pixels();
    assert(data);
    memcpy(data, paint->image->pixels, bytes);

    pixels->unlockPixels();
    bitmap_image.buildMipMap();
    bitmap = SkShader::CreateBitmapShader(bitmap_image,
                                          SkShader::kClamp_TileMode,
                                          SkShader::kClamp_TileMode);

    sk_paint.setShader(bitmap);
    bool yes_filter = true;
    sk_paint.setFilterBitmap(yes_filter);
}

void SkiaImagePaintRendererState::validate(float opacity)
{
    if (valid && opacity == this->opacity) {
        return;
    }
    const ImagePaint *paint = dynamic_cast<ImagePaint*>(owner);
    assert(paint);
    if (paint) {
        setOpacity(opacity);
        // XXX should multiply image by opacity here??
    }
    valid = true;
}

#endif // USE_SKIA
