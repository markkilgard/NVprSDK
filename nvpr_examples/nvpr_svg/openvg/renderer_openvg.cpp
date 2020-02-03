
/* renderer_openvg.cpp - OpenVG path rendering. */

// Copyright (c) NVIDIA Corporation. All rights reserved.

#include "nvpr_svg_config.h"  // configure path renderers to use

#if USE_OPENVG

#include "renderer_openvg.hpp"
#include "scene.hpp"
#include "path.hpp"

#include "scene_openvg.hpp"

#include <GL/glut.h>

struct VGPathCacheProcessor : PathSegmentProcessor {
    Path *p;
    VGPath &path;
    VGFillRule &fill_rule;
    vector<VGubyte> cmds;
    vector<VGfloat> coords;

    VGPathCacheProcessor(Path *p_, VGPath &path_, VGFillRule &fill_rule_)
        : p(p_)
        , path(path_)
        , fill_rule(fill_rule_)
        , cmds(p->cmd.size())
        , coords(p->coord.size())
    {
        cmds.clear();
        coords.clear();
    }

    void beginPath(PathPtr p) {
        switch (p->style.fill_rule) {
        case PathStyle::EVEN_ODD:
            fill_rule = VG_EVEN_ODD;
            break;
        default:
            assert(!"bogus style.fill_rule");
            break;
        case PathStyle::NON_ZERO:
            fill_rule = VG_NON_ZERO;
            break;
        }
    }
    void moveTo(const float2 plist[2], size_t coord_index, char cmd) {
        cmds.push_back(VG_MOVE_TO_ABS);
        coords.push_back(plist[1].x);
        coords.push_back(plist[1].y);
    }
    void lineTo(const float2 plist[2], size_t coord_index, char cmd) {
        cmds.push_back(VG_LINE_TO_ABS);
        coords.push_back(plist[1].x);
        coords.push_back(plist[1].y);
    }
    void quadraticCurveTo(const float2 plist[3], size_t coord_index, char cmd) {
        cmds.push_back(VG_QUAD_TO_ABS);
        coords.push_back(plist[1].x);
        coords.push_back(plist[1].y);
        coords.push_back(plist[2].x);
        coords.push_back(plist[2].y);
    }
    void cubicCurveTo(const float2 plist[4], size_t coord_index, char cmd) {
        cmds.push_back(VG_CUBIC_TO_ABS);
        coords.push_back(plist[1].x);
        coords.push_back(plist[1].y);
        coords.push_back(plist[2].x);
        coords.push_back(plist[2].y);
        coords.push_back(plist[3].x);
        coords.push_back(plist[3].y);
    }
    void arcTo(const EndPointArc &arc, size_t coord_index, char cmd) {
        if (arc.large_arc_flag) {
            if (arc.sweep_flag) {
                cmds.push_back(VG_LCCWARC_TO_ABS);
            } else {
                cmds.push_back(VG_LCWARC_TO_ABS);
            }
        } else {
            if (arc.sweep_flag) {
                cmds.push_back(VG_SCCWARC_TO_ABS);
            } else {
                cmds.push_back(VG_SCWARC_TO_ABS);
            }
        }
        coords.push_back(arc.radii.x);
        coords.push_back(arc.radii.y);
        coords.push_back(arc.x_axis_rotation);
        coords.push_back(arc.p[1].x);
        coords.push_back(arc.p[1].y);
    }
    void close(char cmd) {
        cmds.push_back(VG_CLOSE_PATH);
    }
    void endPath(PathPtr p) {
        path = vgCreatePath(VG_PATH_FORMAT_STANDARD,
                            VG_PATH_DATATYPE_F,
                            1.0f, 0.0f,  // scale & bias
                            VGint(cmds.size()),
                            VGint(coords.size()),
                            (unsigned int)VG_PATH_CAPABILITY_ALL);
        vgAppendPathData(path, VGint(cmds.size()), &cmds[0], &coords[0]);
    }
};

VGRenderer::VGRenderer()
    : eglsurface(0)
    , eglcontext(0)
    , width(0)
    , height(0)
{
    static const EGLint config_attribs[] =
    {
        EGL_RED_SIZE,       8,
        EGL_GREEN_SIZE,     8,
        EGL_BLUE_SIZE,      8,
        EGL_ALPHA_SIZE,     8,
        EGL_LUMINANCE_SIZE, EGL_DONT_CARE,          //EGL_DONT_CARE
        EGL_SURFACE_TYPE,   EGL_WINDOW_BIT,
        EGL_NONE
    };
    EGLint numconfigs;

    //initialize EGL
    egldisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(egldisplay, NULL, NULL);
    assert(eglGetError() == EGL_SUCCESS);
    eglBindAPI(EGL_OPENVG_API);

    eglconfig = NULL;
    eglChooseConfig(egldisplay, config_attribs, &eglconfig, 1, &numconfigs);
    assert(eglconfig != NULL);
    assert(eglGetError() == EGL_SUCCESS);
    assert(numconfigs == 1);
}

void VGRenderer::configureSurface(int w, int h)
{
    int glut_window_id = glutGetWindow();

#if 0
    int winWidth = glutGet(GLUT_WINDOW_WIDTH),
        winHeight = glutGet(GLUT_WINDOW_HEIGHT);

    assert(winWidth == w);
    assert(winHeight == h);
#endif

    width = w;
    height = h;
    
    eglsurface = eglCreateWindowSurface(egldisplay, eglconfig, (void*)(size_t)glut_window_id, NULL);
    assert(eglGetError() == EGL_SUCCESS);
    eglcontext = eglCreateContext(egldisplay, eglconfig, NULL, NULL);
    assert(eglGetError() == EGL_SUCCESS);
    eglMakeCurrent(egldisplay, eglsurface, eglsurface, eglcontext);
    assert(eglGetError() == EGL_SUCCESS);

    vgSeti(VG_BLEND_MODE, VG_BLEND_SRC_OVER);
}

void VGRenderer::shutdown()
{
    EGLBoolean success;

    success = eglMakeCurrent(egldisplay, 0, 0, 0);
    assert(success);
    if (eglcontext) {
        success = eglDestroyContext(egldisplay, eglcontext);
        assert(success);
    }
    eglcontext = 0;
    if (eglsurface) {
        success = eglDestroySurface(egldisplay, eglsurface);
        assert(success);
    }
    eglsurface = 0;
    assert(eglGetError() == EGL_SUCCESS);
}

void VGRenderer::beginDraw()
{
    printf("OpenVG rendering...");
    fflush(stdout);
    start_time = glutGet(GLUT_ELAPSED_TIME);
}

void VGRenderer::clear(float3 clear_color)
{
    VGfloat color[4] = { clear_color.r, clear_color.g, clear_color.b, 1.0 };

    vgSetfv(VG_CLEAR_COLOR, 4, color);
    vgClear(0, 0, width, height);
}

void VGRenderer::setView(float4x4 view)
{
    extern float scene_ratio;
    const float2 scale = clipToSurfaceScales(width, height, scene_ratio);

    vgLoadIdentity();
    // Step 1: make mapping from [-1,+1] clip space to pixel space
    vgScale(width/2, height/2);
    vgTranslate(1,1);
    // Step 2: make mapping from clip space to surface space
    vgScale(scale.x, scale.y);
    // Step 3: make a mapping from surface space to surface space
    VGfloat m[9];  // Column-major
    m[0] = view[0][0];
    m[1] = view[1][0];
    m[2] = view[3][0];
    m[3] = view[0][1];
    m[4] = view[1][1];
    m[5] = view[3][1];
    // XXX OpenVG actually ignores this projective row!
    m[6] = view[0][3];
    m[7] = view[1][3];
    m[8] = view[3][3];
    vgMultMatrix(m);
}


VisitorPtr VGRenderer::makeVisitor()
{
    return VisitorPtr(new VGVisitors::Draw(
        dynamic_pointer_cast<VGRenderer>(shared_from_this())));
}

void VGRenderer::endDraw()
{
    float seconds = (glutGet(GLUT_ELAPSED_TIME) - start_time) / 1000.0;
    printf("done in %f seconds.\n", seconds);
}

void VGRenderer::copyImageToWindow()
{
    glWindowPos2f(0,0);
    try {
        unsigned int* tmp = new unsigned int[width*height]; //throws bad_alloc
        //NOTE: we assume here that the display is always sRGBA
        vgReadPixels(tmp, width*sizeof(unsigned int), VG_sRGBA_8888, 0, 0, width, height);
        glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, tmp);
        delete [] tmp;
    }
    catch(std::bad_alloc)
    {
        assert(!"copyImageToGL tmp buffer allocation failed");
    }
}

const char *VGRenderer::getWindowTitle()
{
    return "OpenVG path rendering";
}

const char *VGRenderer::getName()
{
    return "OpenVG";
}

shared_ptr<RendererState<Path> > VGRenderer::alloc(Path *owner)
{
    return VGPathRendererStatePtr(new VGPathRendererState(shared_from_this(), owner));
}

shared_ptr<RendererState<Shape> > VGRenderer::alloc(Shape *owner)
{
    assert(owner);
    return VGShapeRendererStatePtr(new VGShapeRendererState(shared_from_this(), owner));
}

void VGPathRendererState::validate()
{
    if (valid) {
        return;
    }

    VGPathCacheProcessor processor(owner, path, fill_rule);
    owner->processSegments(processor);
    valid = true;
}

void VGPathRendererState::invalidate()
{
    valid = false;
}

static VGCapStyle lineCapConverter(const PathStyle &style)
{
    switch (style.line_cap) {
    default:
        assert(!"bad line_cap");
    case PathStyle::BUTT_CAP:
        return VG_CAP_BUTT;
    case PathStyle::ROUND_CAP:
        return VG_CAP_ROUND;
    case PathStyle::SQUARE_CAP:
        return VG_CAP_SQUARE;
    case PathStyle::TRIANGLE_CAP:
        printf("OpenVG lacks triangle cap\n");
        return VG_CAP_BUTT;
    }
}

static VGJoinStyle lineJoinConverter(const PathStyle &style)
{
    switch (style.line_join) {
    default:
        assert(!"bad line_join");
    case PathStyle::MITER_TRUNCATE_JOIN:
        printf("OpenVG doesn't support Qt-style MITER_TRUNCATE_JOIN\n");
        return VG_JOIN_MITER;
    case PathStyle::MITER_REVERT_JOIN:
        return VG_JOIN_MITER;
    case PathStyle::ROUND_JOIN:
        return VG_JOIN_ROUND;
    case PathStyle::BEVEL_JOIN:
        return VG_JOIN_BEVEL;
    case PathStyle::NONE_JOIN:
        printf("OpenVG doesn't support 'none' join, treating as bevel\n");
        return VG_JOIN_BEVEL;
    }
}

void VGSolidColorPaintRendererState::validate()
{
    if (valid) {
        return;
    }
    SolidColorPaint *p = dynamic_cast<SolidColorPaint*>(owner);
    assert(p);
    if (p) {
        if (paint) {
            vgDestroyPaint(paint);
        }
        paint = vgCreatePaint();
        vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
        const float4 solid_color = p->getColor();
        VGfloat color[] = { solid_color.r, solid_color.g, solid_color.b, solid_color.a };
        vgSetParameterfv(paint, VG_PAINT_COLOR, 4, color);
    }
    valid = true;
}

static VGColorRampSpreadMode convert_SVG_spread_method_to_VG_spread_mode(SpreadMethod v)
{
    switch (v) {
    case PAD:
        return VG_COLOR_RAMP_SPREAD_PAD;
    case REFLECT:
        return VG_COLOR_RAMP_SPREAD_REFLECT;
    case REPEAT:
        return VG_COLOR_RAMP_SPREAD_REPEAT;
    default:
        assert(!"bogus spread method");
    case NONE:
        if (verbose) {
            printf("NONE has no OpenVG equivalent\n");
        }
        return VG_COLOR_RAMP_SPREAD_PAD;
    }
}

void VGGradientPaintRendererState::setGenericGradientPaintParameters(const GradientPaint *p)
{
    const vector<GradientStop> &stop_array = p->getStopArray();
    const size_t count = stop_array.size();
    vector<VGfloat> farray(count*5);
    for (size_t i=0; i<count; i++) {
        farray.push_back(stop_array[i].offset);
        farray.push_back(stop_array[i].color.r);
        farray.push_back(stop_array[i].color.g);
        farray.push_back(stop_array[i].color.b);
        farray.push_back(stop_array[i].color.a);
    }
    vgSetParameterfv(paint, VG_PAINT_COLOR_RAMP_STOPS, VGint(5*count), &farray[0]);

    // The VG_MATRIX_FILL_PAINT_TO_USER and VG_MATRIX_STROKE_PAINT_TO_USER
    // is content (not brush) state.
    const float3x3 &gradient_transform = p->getGradientTransform();
    transform[0] = gradient_transform[0][0];
    transform[1] = gradient_transform[1][0];
    transform[2] = gradient_transform[2][0];
    transform[3] = gradient_transform[0][1];
    transform[4] = gradient_transform[1][1];
    transform[5] = gradient_transform[2][1];
    transform[6] = gradient_transform[0][2];
    transform[7] = gradient_transform[1][2];
    transform[8] = gradient_transform[2][2];

    VGint spread_mode = VGint(convert_SVG_spread_method_to_VG_spread_mode(p->getSpreadMethod()));
    vgSetParameteriv(paint, VG_PAINT_COLOR_RAMP_SPREAD_MODE, 1, &spread_mode);
}

void VGLinearGradientPaintRendererState::validate()
{
    if (valid) {
        return;
    }
    LinearGradientPaint *p = dynamic_cast<LinearGradientPaint*>(owner);
    assert(p);
    if (p) {
        if (paint) {
            vgDestroyPaint(paint);
        }
        paint = vgCreatePaint();
        vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_LINEAR_GRADIENT);
        VGfloat linear_gradient[4] = { p->getV1().x, p->getV1().y, p->getV2().x, p->getV2().y };
        vgSetParameterfv(paint, VG_PAINT_LINEAR_GRADIENT, 4, linear_gradient);
        setGenericGradientPaintParameters(p);
    }
    valid = true;
}

void VGRadialGradientPaintRendererState::validate()
{
    if (valid) {
        return;
    }
    RadialGradientPaint *p = dynamic_cast<RadialGradientPaint*>(owner);
    assert(p);
    if (p) {
        if (paint) {
            vgDestroyPaint(paint);
        }
        paint = vgCreatePaint();
        vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_RADIAL_GRADIENT);
        VGfloat radial_gradient[5] = { p->getCenter().x, p->getCenter().y,
                                       p->getFocalPoint().x, p->getFocalPoint().y,
                                       p->getRadius() };
        vgSetParameterfv(paint, VG_PAINT_RADIAL_GRADIENT, 5, radial_gradient);
        setGenericGradientPaintParameters(p);
    }
    valid = true;
}

void VGShapeRendererState::validate()
{
    if (valid) {
        return;  // Already valid.
    }
    Shape *shape = dynamic_cast<Shape*>(owner);
    assert(shape);
    if (shape) {
        // Validate fill paint.
        const PaintPtr fill_paint = shape->getFillPaint();
        if (fill_paint) {
            PaintRendererStatePtr renderer_state = fill_paint->getRendererState(getRenderer());
            VGPaintRendererStatePtr paint_renderer_state = dynamic_pointer_cast<VGPaintRendererState>(renderer_state);
            paint_renderer_state->validate();
        }
        // Validate stroke paint.
        const PaintPtr stroke_paint = shape->getStrokePaint();
        if (stroke_paint) {
            PaintRendererStatePtr renderer_state = stroke_paint->getRendererState(getRenderer());
            VGPaintRendererStatePtr paint_renderer_state = dynamic_pointer_cast<VGPaintRendererState>(renderer_state);
            paint_renderer_state->validate();
        }
    }
    valid = true;
}

void VGShapeRendererState::invalidate()
{
    valid = false;
}

void VGShapeRendererState::setStrokeParameters(const PathStyle &style)
{
    vgSetf(VG_STROKE_LINE_WIDTH, style.stroke_width);
    vgSeti(VG_STROKE_CAP_STYLE, lineCapConverter(style));
    vgSeti(VG_STROKE_JOIN_STYLE, lineJoinConverter(style));
    vgSetf(VG_STROKE_MITER_LIMIT, style.miter_limit);
    const size_t dash_count = style.dash_array.size();
    if (dash_count > 0) {
        // Grrr, OpenVG 1.1 specification sez: "If the dash pattern has an odd
        // number of elements, the final element is ignored. Note that this
        // behavior is different from that defined by SVG; the SVG behavior
        // may be implemented by duplicating the odd-length dash pattern to 
        // obtain one with even length."
        if (dash_count & 1) {
            vector<VGfloat> doubled_dash_array(dash_count*2);
            for (size_t i=0; i<dash_count; i++) {
                doubled_dash_array[i           ] = // same as...
                doubled_dash_array[i+dash_count] = style.dash_array[i];
            }
            vgSetfv(VG_STROKE_DASH_PATTERN, VGint(dash_count*2), &doubled_dash_array[0]);
        } else {
            vgSetfv(VG_STROKE_DASH_PATTERN, VGint(style.dash_array.size()), &style.dash_array[0]);
        }
    } else {
        vgSetfv(VG_STROKE_DASH_PATTERN, 0, NULL);
    }
    vgSetf(VG_STROKE_DASH_PHASE, style.dash_offset);
    VGint dash_phase = style.dash_phase == PathStyle::MOVETO_RESETS;
    vgSetf(VG_STROKE_DASH_PHASE_RESET, dash_phase);

    if (stroke_transform) {
        vgSeti(VG_MATRIX_MODE, VG_MATRIX_STROKE_PAINT_TO_USER);
        assert(!"not really right");
        vgLoadMatrix(stroke_transform);
    }
}

void VGShapeRendererState::getPaints(VGPaint &return_fill_paint, VGPaint &return_stroke_paint)
{
    return_fill_paint = 0;
    return_stroke_paint = 0;

    validate();
    assert(owner);

    const PaintPtr fill_paint = owner->getFillPaint();
    if (fill_paint) {
        PaintRendererStatePtr renderer_state = fill_paint->getRendererState(getRenderer());
        VGPaintRendererStatePtr paint_renderer_state = dynamic_pointer_cast<VGPaintRendererState>(renderer_state);
        return_fill_paint = paint_renderer_state->getPaint();
    }

    const PaintPtr stroke_paint = owner->getStrokePaint();
    if (stroke_paint) {
        PaintRendererStatePtr renderer_state = stroke_paint->getRendererState(getRenderer());
        VGPaintRendererStatePtr paint_renderer_state = dynamic_pointer_cast<VGPaintRendererState>(renderer_state);
        return_stroke_paint = paint_renderer_state->getPaint();
    }
}

void VGShapeRendererState::draw()
{
    VGRendererPtr renderer = getRenderer();

    // validate brushes
    validate();

    VGPaint fill_paint, stroke_paint;
    getPaints(fill_paint, stroke_paint);

    // validate the path
    VGPathRendererStatePtr path_state = getPathRendererState();
    path_state->validate();

    extern bool doFilling, doStroking;
    PathPtr p = owner->getPath();
    VGPath path = path_state->path;
    if (p->style.do_fill && doFilling) {
        // configure filling
        VGPathRendererStatePtr prs = getPathRendererState();
        vgSeti(VG_FILL_RULE, prs->fill_rule);
        vgSetPaint(fill_paint, VG_FILL_PATH);
        VGbitfield paint_modes = VG_FILL_PATH;
        if (fill_transform) {
            vgSeti(VG_MATRIX_MODE, VG_MATRIX_FILL_PAINT_TO_USER);
            assert(!"not really right");
            vgLoadMatrix(fill_transform);
        }
        if (p->style.do_stroke && doStroking) {
            // configure stroking too
            setStrokeParameters(p->style);
            vgSetPaint(stroke_paint, VG_STROKE_PATH);
            paint_modes |= VG_STROKE_PATH;
        }
        vgDrawPath(path, paint_modes);
    } else {
        if (p->style.do_stroke && doStroking) {
            // just stroke
            setStrokeParameters(p->style);
            vgSetPaint(stroke_paint, VG_STROKE_PATH);
            vgDrawPath(path, VG_STROKE_PATH);
        }
    }
}

shared_ptr<RendererState<Paint> > VGRenderer::alloc(Paint *owner)
{
    SolidColorPaint *solid_color_paint = dynamic_cast<SolidColorPaint*>(owner);
    if (solid_color_paint) {
        return VGSolidColorPaintRendererStatePtr(new VGSolidColorPaintRendererState(shared_from_this(), solid_color_paint));
    }

    LinearGradientPaint *linear_gradient_paint = dynamic_cast<LinearGradientPaint*>(owner);
    if (linear_gradient_paint) {
        return VGLinearGradientPaintRendererStatePtr(new VGLinearGradientPaintRendererState(shared_from_this(), linear_gradient_paint));
    }

    RadialGradientPaint *radial_gradient_paint = dynamic_cast<RadialGradientPaint*>(owner);
    if (radial_gradient_paint) {
        return VGRadialGradientPaintRendererStatePtr(new VGRadialGradientPaintRendererState(shared_from_this(), radial_gradient_paint));
    }

    assert(!"paint unsupported by VG renderer");
    return VGPaintRendererStatePtr();
}

void VGPaintRendererState::invalidate()
{
    valid = false;
}

#endif // USE_OPENVG
