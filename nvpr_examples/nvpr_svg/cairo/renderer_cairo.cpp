
/* renderer_cairo.cpp - Cairo path rendering back-end for nvpr_svg. */

// Copyright (c) NVIDIA Corporation. All rights reserved.

/* Requires the OpenGL Utility Toolkit (GLUT) and Cg runtime (version
   2.0 or higher). */

#include "nvpr_svg_config.h"  // configure path renderers to use

#if USE_CAIRO

#define _USE_MATH_DEFINES  // so <math.h> has M_PI

#include <Cg/vector/rgba.hpp>

#include "renderer_cairo.hpp"
#include "scene.hpp"
#include "path.hpp"

#include "scene_cairo.hpp"

//#include <Cg/lerp.hpp>  // This has problems with Visual Studio 2008
#define lerp(a,b,t) ((a) + (t)*((b)-(a)))
#include <Cg/length.hpp>

// Release builds shouldn't have verbose conditions.
#ifdef NDEBUG
#define verbose (0)
#else
extern int verbose;
#endif

struct CairoPathSegmentProcessor : PathSegmentProcessor {
    cairo_t *cr;

    CairoPathSegmentProcessor(cairo_t *cr_)
        : cr(cr_)
    {}

    void beginPath(PathPtr p) { 
    }
    void moveTo(const float2 plist[2], size_t coord_index, char cmd) {
        cairo_move_to(cr, plist[1].x, plist[1].y);
    }
    void lineTo(const float2 plist[2], size_t coord_index, char cmd) {
        cairo_line_to(cr, plist[1].x, plist[1].y);
    }
    void quadraticCurveTo(const float2 plist[3], size_t coord_index, char cmd) {
        float2 e0 = lerp(plist[0], plist[1], 2.0f/3),
               e1 = lerp(plist[2], plist[1], 2.0f/3);
        cairo_curve_to(cr,
                       e0.x, e0.y,
                       e1.x, e1.y,
                       plist[2].x, plist[2].y);
    }
    void cubicCurveTo(const float2 plist[4], size_t coord_index, char cmd) {
        cairo_curve_to(cr,
                       plist[1].x, plist[1].y,
                       plist[2].x, plist[2].y,
                       plist[3].x, plist[3].y);
    }
    void arcTo(const EndPointArc &arc, size_t coord_index, char cmd) {
        // Convert to a center point arc to be able to render the arc center.
        CenterPointArc center_point_arc(arc);
        switch (center_point_arc.form) {
        case CenterPointArc::BEHAVED:
            // Is the elliptical arc part of a circle?
            if (center_point_arc.radii.x == center_point_arc.radii.y) {
                // In the circle case, psi just rotates both angle1 and angle2.
                const double angle1 = center_point_arc.theta1+center_point_arc.psi,
                             angle2 = angle1+center_point_arc.delta_theta;
                if (center_point_arc.delta_theta >= 0) {
                    cairo_arc(cr,
                        center_point_arc.center.x, center_point_arc.center.y,
                        center_point_arc.radii.x,
                        angle1, angle2);
                } else {
                    cairo_arc_negative(cr,
                        center_point_arc.center.x, center_point_arc.center.y,
                        center_point_arc.radii.x,
                        angle1, angle2);
                }
            } else {
                cairo_matrix_t m;

                cairo_get_matrix(cr, &m);
                cairo_translate(cr, 
                    center_point_arc.center.x, center_point_arc.center.y);
                cairo_rotate(cr, /*radians*/center_point_arc.psi);
                cairo_scale(cr, center_point_arc.radii.x, center_point_arc.radii.y);
                if (center_point_arc.delta_theta >= 0) {
                    cairo_arc(cr, /*x,y*/0,0, /*radii*/1,
                              center_point_arc.theta1, center_point_arc.theta1+center_point_arc.delta_theta);
                } else {
                    cairo_arc_negative(cr, /*x,y*/0,0, /*radii*/1,
                                       center_point_arc.theta1, center_point_arc.theta1+center_point_arc.delta_theta);
                }
                cairo_set_matrix(cr, &m);
            }
            break;
        case CenterPointArc::DEGENERATE_LINE:
            cairo_line_to(cr, arc.p[1].x, arc.p[1].y);
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
        cairo_close_path(cr);
    }
};

static cairo_line_cap_t lineCapConverter(const PathStyle *style)
{
    switch (style->line_cap) {
    default:
        assert(!"bad line_cap");
    case PathStyle::BUTT_CAP:
        return CAIRO_LINE_CAP_BUTT;
    case PathStyle::ROUND_CAP:
        return CAIRO_LINE_CAP_ROUND;
    case PathStyle::SQUARE_CAP:
        return CAIRO_LINE_CAP_SQUARE;
    case PathStyle::TRIANGLE_CAP:
        printf("Cairo lacks triangle cap\n");
        return CAIRO_LINE_CAP_BUTT;
    }
}

static cairo_line_join_t lineJoinConverter(const PathStyle *style)
{
    switch (style->line_join) {
    default:
        assert(!"bad line_join");
    case PathStyle::MITER_TRUNCATE_JOIN:
        printf("Cairo doesn't support Qt-style MITER_TRUNCATE_JOIN\n");
        return CAIRO_LINE_JOIN_MITER;
    case PathStyle::MITER_REVERT_JOIN:
        return CAIRO_LINE_JOIN_MITER;
    case PathStyle::ROUND_JOIN:
        return CAIRO_LINE_JOIN_ROUND;
    case PathStyle::BEVEL_JOIN:
        return CAIRO_LINE_JOIN_BEVEL;
    case PathStyle::NONE_JOIN:
        printf("Cairo doesn't support 'none' join, treating as bevel\n");
        return CAIRO_LINE_JOIN_BEVEL;
    }
}

static void drawPathHelper(cairo_t *cr,
                           cairo_pattern_t *fill_pattern,
                           cairo_pattern_t *stroke_pattern,
                           const PathStyle *style)
{
    extern bool doFilling, doStroking;
    if (style->do_stroke && stroke_pattern && doStroking) {
        if (style->do_fill && fill_pattern && doFilling) {
            cairo_set_fill_rule(cr, style->fill_rule == PathStyle::NON_ZERO
                                    ? CAIRO_FILL_RULE_WINDING
                                    : CAIRO_FILL_RULE_EVEN_ODD);
            cairo_set_source(cr, fill_pattern);
            // preserve so following stroke uses same path
            cairo_fill_preserve(cr);
        }
        cairo_set_line_width(cr, style->stroke_width);
        cairo_set_line_cap(cr, lineCapConverter(style));
        cairo_set_line_join(cr, lineJoinConverter(style));
        cairo_set_miter_limit(cr, style->miter_limit);
        if (style->dash_array.size() > 0) {
            vector<double> dash_array;
            size_t n = style->dash_array.size();

            dash_array.reserve(n);
            for (size_t i = 0; i<n; i++) {
                dash_array.push_back(style->dash_array[i]);
            }
            cairo_set_dash (cr, &dash_array[0], int(n), style->dash_offset);
        } else {
            cairo_set_dash (cr, NULL, 0, 0);
        }
        cairo_set_source(cr, stroke_pattern);
        cairo_stroke(cr);
    } else if (style->do_fill && fill_pattern && doFilling) {
        // just filling (no stroking)
        cairo_set_fill_rule(cr, style->fill_rule == PathStyle::NON_ZERO
                                ? CAIRO_FILL_RULE_WINDING
                                : CAIRO_FILL_RULE_EVEN_ODD);
        cairo_set_source(cr, fill_pattern);
        cairo_fill(cr);
    }
}

struct CairoPathDrawProcessor : CairoPathSegmentProcessor {
    cairo_pattern_t *brush;
    cairo_pattern_t *pen;

    CairoPathDrawProcessor(cairo_t *cr_, cairo_pattern_t *brush_, cairo_pattern_t *pen_)
        : CairoPathSegmentProcessor(cr_)
        , brush(brush_)
        , pen(pen_)
    {}

    void endPath(PathPtr p) {
        drawPathHelper(cr, brush, pen, &p->style);
    }
};

struct CairoPathCacheProcessor : CairoPathSegmentProcessor {
    cairo_path_t* &path;

    CairoPathCacheProcessor(cairo_t *cr_, cairo_path_t* &path_)
        : CairoPathSegmentProcessor(cr_)
        , path(path_)
    {
        cairo_new_path(cr);
    }

    void endPath(PathPtr p) {
        if (path) {
            cairo_path_destroy(path);
        }
        path = cairo_copy_path(cr);
    }
};

void CairoRenderer::configureSurface(int width, int height)
{
    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cr = cairo_create(surface);
    cairo_set_antialias(cr, antialias_mode);
}

void CairoRenderer::shutdown()
{
    if (cr) {
        cairo_destroy(cr);
        cr = NULL;
    }
    if (surface) {
        cairo_surface_destroy(surface);
        surface = NULL;
    }
}

void CairoRenderer::setAntialias(cairo_antialias_t mode)
{
    antialias_mode = mode;
    if (cr) {
        cairo_set_antialias(cr, antialias_mode);
    }
}

void CairoRenderer::setFilter(cairo_filter_t mode)
{
    filter_mode = mode;
}

void CairoRenderer::clear(float3 clear_color)
{
    /* Set surface to opaque color (r, g, b) */
    cairo_set_source_rgb(cr, clear_color.r, clear_color.g, clear_color.b);
    cairo_paint(cr);
}

void CairoRenderer::setView(float4x4 view)
{
    int w = cairo_image_surface_get_width(surface),
        h = cairo_image_surface_get_height(surface);

    extern float scene_ratio;
    const float2 scale = clipToSurfaceScales(w, h, scene_ratio);

    cairo_identity_matrix (cr);
    cairo_scale(cr, w/2.0, h/2.0);
    cairo_translate(cr, 1, 1);
    cairo_scale(cr, scale.x, scale.y);

    cairo_matrix_t m;
    cairo_matrix_init(&m, view[0][0], view[1][0],
                          view[0][1], view[1][1],
                          view[0][3], view[1][3]);
    cairo_transform(cr, &m);
}

VisitorPtr CairoRenderer::makeVisitor()
{
    return VisitorPtr(new CairoVisitors::Draw(
        dynamic_pointer_cast<CairoRenderer>(shared_from_this())));
}

void CairoRenderer::copyImageToWindow()
{
    glWindowPos2f(0,0);

    const int w = cairo_image_surface_get_width(surface),
              h = cairo_image_surface_get_height(surface);

    const unsigned char *pixels = cairo_image_surface_get_data(surface);

    assert(cairo_image_surface_get_stride(surface) == w*4);
    glDrawPixels(w, h, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels);
}

const char *CairoRenderer::getWindowTitle()
{
    return "Cairo path rendering"; 
}

const char *CairoRenderer::getName()
{
    return "Cairo"; 
}

shared_ptr<RendererState<Path> > CairoRenderer::alloc(Path *owner)
{
    return CairoPathRendererStatePtr(new CairoPathRendererState(shared_from_this(), owner));
}

shared_ptr<RendererState<Shape> > CairoRenderer::alloc(Shape *owner)
{
    return CairoShapeRendererStatePtr(new CairoShapeRendererState(shared_from_this(), owner));
}

shared_ptr<RendererState<Paint> > CairoRenderer::alloc(Paint *owner)
{
    SolidColorPaint *solid_color_paint = dynamic_cast<SolidColorPaint*>(owner);
    if (solid_color_paint) {
        return CairoSolidColorPaintRendererStatePtr(new CairoSolidColorPaintRendererState(shared_from_this(), solid_color_paint));
    }

    LinearGradientPaint *linear_gradient_paint = dynamic_cast<LinearGradientPaint*>(owner);
    if (linear_gradient_paint) {
        return CairoLinearGradientPaintRendererStatePtr(new CairoLinearGradientPaintRendererState(shared_from_this(), linear_gradient_paint));
    }

    RadialGradientPaint *radial_gradient_paint = dynamic_cast<RadialGradientPaint*>(owner);
    if (radial_gradient_paint) {
        return CairoRadialGradientPaintRendererStatePtr(new CairoRadialGradientPaintRendererState(shared_from_this(), radial_gradient_paint));
    }

    ImagePaint *image_paint = dynamic_cast<ImagePaint*>(owner);
    if (image_paint) {
        return CairoImagePaintRendererStatePtr(new CairoImagePaintRendererState(shared_from_this(), image_paint));
    }

    assert(!"paint unsupported by Cairo renderer");
    return CairoPaintRendererStatePtr();
}

void CairoPathRendererState::validate()
{
    if (valid) {
        return;
    }
    CairoPathCacheProcessor processor(getRenderer()->cr, path);
    owner->processSegments(processor);
    valid = true;
}

void CairoPathRendererState::invalidate()
{
    valid = false;
}

void CairoSolidColorPaintRendererState::validate(float opacity)
{
    if (valid && opacity == this->opacity) {
        return;
    }
    SolidColorPaint *paint = dynamic_cast<SolidColorPaint*>(owner);
    assert(paint);
    if (paint) {
        if (pattern) {
            cairo_pattern_destroy(pattern);
        }
        float4 solid_color = paint->getColor();
        pattern = cairo_pattern_create_rgba(solid_color.r, solid_color.g, solid_color.b,
                                            solid_color.a * opacity);
    }
    valid = true;
}

static cairo_extend_t convert_SVG_spread_method_to_cairo_extend(SpreadMethod v)
{
    switch (v) {
    case PAD:
        return CAIRO_EXTEND_PAD;
    case REFLECT:
        return CAIRO_EXTEND_REFLECT;
    case REPEAT:
        return CAIRO_EXTEND_REPEAT;
    default:
        assert(!"bogus spread method");
    case NONE:
        return CAIRO_EXTEND_NONE;
    }
}

void CairoGradientPaintRendererState::setGradientStops(const GradientPaint *paint, float opacity)
{
    const std::vector<GradientStop>& stops = paint->getStopArray();
    for (vector<GradientStop>::const_iterator stop = stops.begin();
        stop != stops.end();
        ++stop) {
            cairo_pattern_add_color_stop_rgba(pattern, stop->offset,
                stop->color.r, stop->color.g, stop->color.b, stop->color.a * opacity);
    }

    setOpacity(opacity);
}

void CairoGradientPaintRendererState::setGenericGradientPatternParameters(const GradientPaint *paint, float opacity)
{
    setGradientStops(paint, opacity);

    const cairo_extend_t extend = convert_SVG_spread_method_to_cairo_extend(paint->getSpreadMethod());
    cairo_pattern_set_extend(pattern, extend);
}

void CairoLinearGradientPaintRendererState::validate(float opacity)
{
    if (valid && opacity == this->opacity) {
        return;
    }

    const LinearGradientPaint *paint = dynamic_cast<LinearGradientPaint*>(owner);
    assert(paint);
    if (paint) {
        if (pattern) {
            cairo_pattern_destroy(pattern);
        }
        // http://www.cairographics.org/manual/cairo-pattern.html#cairo-pattern-create-linear
        pattern = cairo_pattern_create_linear(paint->getV1().x, paint->getV1().y,
                                              paint->getV2().x, paint->getV2().y);

        setGenericGradientPatternParameters(paint, opacity);
    }
    valid = true;
}

void CairoRadialGradientPaintRendererState::validate(float opacity)
{
    if (valid) {
        return;
    }
    const RadialGradientPaint *paint = dynamic_cast<RadialGradientPaint*>(owner);
    assert(paint);
    if (pattern) {
        cairo_pattern_destroy(pattern);
    }
    // http://www.w3.org/TR/SVG/pservers.html#RadialGradients says
    // "If the point defined by fx and fy lies outside the circle
    // defined by cx, cy and r, then the user agent shall set the
    // focal point to the intersection of the line from (cx, cy)
    // to (fx, fy) with the circle defined by cx, cy and r."
    //
    // Cairo's cairo_pattern_create_radial doesn't force the focal
    // point to be within the circle as SVG says so this code does.
    float2 focal_point = paint->getFocalPoint();
    float2 vector = focal_point - paint->getCenter();
    float len = length(vector);
    if (len > paint->getRadius()) {
        // Fudge, has to be  somewhat less than 1 (0.999 is too
        // much) or cairo draws a solid color.
        const float sub_unity = 199.0f/200;
        focal_point = paint->getCenter() + sub_unity*paint->getRadius()*vector/len;
    }
    // http://www.cairographics.org/manual/cairo-pattern.html#cairo-pattern-create-radial
    pattern = cairo_pattern_create_radial(focal_point.x, focal_point.y, 0,
        paint->getCenter().x, paint->getCenter().y, paint->getRadius());
    cairo_pattern_set_filter(pattern, getRenderer()->filter_mode);
    // CAIRO_EXTEND_PAD pixels outside of the pattern copy the closest pixel from the source
    cairo_pattern_set_extend (pattern, CAIRO_EXTEND_PAD);

    setGenericGradientPatternParameters(paint, opacity);
    valid = true;
}

// CairoImagePaintRendererState

CairoImagePaintRendererState::CairoImagePaintRendererState(RendererPtr renderer, ImagePaint *paint)
    : CairoPaintRendererState(renderer, paint, false/*not gradient*/)
{
    const int width = paint->image->width,
              height = paint->image->height;

    // http://www.cairographics.org/manual/cairo-Image-Surfaces.html#cairo-image-surface-create
    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);

    // Would be nice to use cairo_image_surface_create_for_data but Cairo uses 32-bit ARGB
    // packed values instead of RGBA with 8-bit components.  So convert from RasterImage::Pixel
    // values to CAIRO_FORMAT_ARGB32 values...

    // http://www.cairographics.org/manual/cairo-Image-Surfaces.html#cairo-image-surface-get-data
    unsigned char *data = cairo_image_surface_get_data(surface);

    // CAIRO_FORMAT_ARGB32 means each pixel is a 32-bit quantity, with alpha in the upper 8 bits,
    // then red, then green, then blue. The 32-bit quantities are stored native-endian.
    // Pre-multiplied alpha is used. (That is, 50% transparent red is 0x80800000, not 0x80ff0000.)
    unsigned int *argb = reinterpret_cast<unsigned int *>(data);
    const RasterImage::Pixel *pixels = paint->image->pixels;
    for (int i=0; i<height; i++) {
        for (int j=0; j<width; j++) {
            *argb = (pixels->a << 24) | (pixels->r << 16) | (pixels->g << 8) | (pixels->b << 0);
            argb++;
            pixels++;
        }
    }
    // Tell Cairo that we touched the surface's image.
    // http://www.cairographics.org/manual/cairo-cairo-surface-t.html#cairo-surface-mark-dirty
    cairo_surface_mark_dirty(surface);
}

void CairoImagePaintRendererState::validate(float opacity)
{
    if (valid && opacity == this->opacity) {
        return;
    }
    const ImagePaint *paint = dynamic_cast<ImagePaint*>(owner);
    assert(paint);
    if (paint) {
        setOpacity(opacity);
        // XXX should multiply image by opacity here??
        if (pattern) {
            cairo_pattern_destroy(pattern);
        }
        pattern = cairo_pattern_create_for_surface(surface);
        cairo_pattern_set_filter(pattern, getRenderer()->filter_mode);
    }
    valid = true;
}

void CairoShapeRendererState::validate()
{
    const Shape *shape = owner;

    PaintPtr fill_paint = shape->getFillPaint();
    if (fill_paint) {
        PaintRendererStatePtr renderer_state = fill_paint->getRendererState(getRenderer());
        CairoPaintRendererStatePtr paint_renderer_state = dynamic_pointer_cast<CairoPaintRendererState>(renderer_state);
        paint_renderer_state->validate(shape->net_fill_opacity);
    }

    const PaintPtr stroke_paint = shape->getStrokePaint();
    if (stroke_paint) {
        PaintRendererStatePtr renderer_state = stroke_paint->getRendererState(getRenderer());
        CairoPaintRendererStatePtr paint_renderer_state = dynamic_pointer_cast<CairoPaintRendererState>(renderer_state);
        paint_renderer_state->validate(shape->net_stroke_opacity);
    }

    if (valid) {
        return;
    }
    // Nothing about the shape to actually validate.
    valid = true;
}

cairo_pattern_t *CairoShapeRendererState::getPattern(PaintPtr paint)
{
    if (paint) {
        PaintRendererStatePtr renderer_state = paint->getRendererState(getRenderer());
        CairoPaintRendererStatePtr paint_renderer_state = dynamic_pointer_cast<CairoPaintRendererState>(renderer_state);
        if (paint_renderer_state) {
            cairo_pattern_t *pattern = paint_renderer_state->getPattern();
            assert(pattern);

            if (paint_renderer_state->needsGradientMatrix()) {
                GradientPaintPtr gradient = dynamic_pointer_cast<GradientPaint>(paint);

                cairo_matrix_t m;
                // The gradient transform is "an optional additional 
                // transformation from the gradient coordinate system
                // onto the target coordinate system".  So the transform
                // takes "gradient space" coordinates and transform them
                // to "path space".  Cairo' pattern matrix actually expects
                // the reverse transformation so the inverse is used.
                const float3x3 &inverse_gradient_transform = gradient->getInverseGradientTransform();
                m.xx = inverse_gradient_transform[0][0];
                m.xy = inverse_gradient_transform[0][1];
                m.x0 = inverse_gradient_transform[0][2];
                m.yx = inverse_gradient_transform[1][0];
                m.yy = inverse_gradient_transform[1][1];
                m.y0 = inverse_gradient_transform[1][2];
                if (verbose) {
                    std::cout << "gradient_transform = " << gradient->getGradientTransform() << std::endl;
                    std::cout << "inverse_gradient_transform = " << gradient->getInverseGradientTransform() << std::endl;
                }
                if (gradient->getGradientUnits() == OBJECT_BOUNDING_BOX) {
                    float4 bounds = owner->getBounds();
                    float2 p1 = bounds.xy,
                           p2 = bounds.zw,
                           diff = p2-p1,
                           sum = p2+p1;

                    // Construct 2D orthographic matrix
                    cairo_matrix_t b, combo;
                    b.xx = 1/diff.x;
                    b.xy = 0;
                    b.x0 = -0.5*sum.x/diff.x+0.5;
                    b.yx = 0;
                    b.yy = 1/diff.y;
                    b.y0 = -0.5*sum.y/diff.y+0.5;

                    cairo_matrix_multiply(&combo, &b, &m);
                    // http://cairographics.org/manual/cairo-pattern.html#cairo-pattern-set-matrix
                    cairo_pattern_set_matrix(pattern, &combo);
                } else {
                    assert(gradient->getGradientUnits() == USER_SPACE_ON_USE);
                    cairo_pattern_set_matrix(pattern, &m);
                }
            } else {
                ImagePaintPtr image = dynamic_pointer_cast<ImagePaint>(paint);
                if (image) {
                    CairoImagePaintRendererStatePtr image_state =
                        dynamic_pointer_cast<CairoImagePaintRendererState>(paint_renderer_state);

                    float w = image->image->width,
                          h = image->image->height;

                    float4 bounds = owner->getBounds();
                    float2 p1 = bounds.xy,
                           p2 = bounds.zw,
                           diff = p2-p1,
                           sum = p2+p1;

                    cairo_matrix_t b;
                    b.xx = w/diff.x;
                    b.xy = 0;
                    b.x0 = -0.5*sum.x/diff.x+0.5;
                    b.yx = 0;
                    b.yy = h/diff.y;
                    b.y0 = -0.5*sum.y/diff.y+0.5;
                    cairo_pattern_set_matrix(pattern, &b);
                }
            }
            return pattern;
        }
    }
    return NULL;
}

void CairoShapeRendererState::getPatterns(cairo_pattern_t *&fill_pattern, cairo_pattern_t *&stroke_pattern)
{
    assert(owner);
    validate();

    fill_pattern = getPattern(owner->getFillPaint());
    stroke_pattern = getPattern(owner->getStrokePaint());
}

void CairoShapeRendererState::invalidate()
{
    valid = false;
}

void CairoPaintRendererState::invalidate()
{
    valid = false;
}

void CairoShapeRendererState::draw()
{
    CairoRendererPtr renderer = getRenderer();

    cairo_pattern_t *fill_pattern, *stroke_pattern;
    getPatterns(fill_pattern, stroke_pattern);

    if (renderer->cache_paths) {
        // validate the path
        CairoPathRendererStatePtr cairo_prs = getPathRendererState();
        cairo_prs->validate();

        cairo_new_path(renderer->cr);
        cairo_append_path(renderer->cr, cairo_prs->path);
        drawPathHelper(renderer->cr, fill_pattern, stroke_pattern, &owner->getPath()->style);
    } else {
        CairoPathDrawProcessor processor(renderer->cr, fill_pattern, stroke_pattern);
        owner->getPath()->processSegments(processor);
    }
}

void CairoShapeRendererState::getPaths(vector<cairo_path_t*>& paths)
{
    CairoRendererPtr renderer = getRenderer();

    CairoPathRendererStatePtr cairo_prs = getPathRendererState();
    cairo_prs->validate();

    paths.push_back(cairo_prs->path);
}

#endif // USE_CAIRO
