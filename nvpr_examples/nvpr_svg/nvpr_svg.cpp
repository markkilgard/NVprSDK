
// nvpr_svg.c - GPU-accelerated Scalable Vector Graphics (SVG) demo.

// Copyright (c) NVIDIA Corporation. All rights reserved.

// Requires the OpenGL Utility Toolkit (GLUT) and Cg runtime (version
// 2.0 or higher).

#include "nvpr_svg_config.h"  // configure path renderers to use

#define _WIN32_WINNT 0x0500 // Windows 2000 to have access to GetFontUnicodeRanges, etc.
#define _USE_MATH_DEFINES  // so <math.h> has M_PI

#include <vector>
#include <string>
#include <string.h>

#include <boost/lexical_cast.hpp>

#include <stdio.h>    /* for printf and NULL */
#include <string.h>
#include <wchar.h>
#include <stdlib.h>   /* for exit */
#include <math.h>     /* for sin and cos */
#if __APPLE__
#include <GL/glew.h>
#include <GLUT/glut.h>
#include <OpenGL/glext.h>
#else
#include <GL/glew.h>
#include <GL/glut.h>
#endif

#include <Cg/double.hpp>
#include <Cg/vector/xyzw.hpp>
#include <Cg/vector/rgba.hpp>
#include <Cg/vector.hpp>
#include <Cg/matrix.hpp>
#include <Cg/mul.hpp>
#include <Cg/abs.hpp>
#include <Cg/max.hpp>
#include <Cg/min.hpp>
#include <Cg/clamp.hpp>
#include <Cg/iostream.hpp>
#include <Cg/transpose.hpp>

#include "request_vsync.h"
#include "showfps.h"
#include "path_parse_svg.h"
#include "path_data.h"
#include "svg_loader.hpp"
#if USE_FREETYPE2
#include "freetype2_loader.hpp"
#endif
#include "glmatrix.hpp"
#include "countof.h"
#include "svg_files.hpp"
#include "sRGB_vector.hpp"

#include "path.hpp"

#define STBI_HEADER_FILE_ONLY
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

//// Various spported renderers
// GPU-accelerated two-step "stencil, then cover" OpenGL renderer
#include "stc/scene_stc.hpp"
#if USE_SKIA
#include "skia/renderer_skia.hpp"          // Skia software renderer
#include "skia/scene_skia.hpp"
#endif
#if USE_QT
#include "qt/renderer_qt.hpp"          // Qt 4.5 software renderer
#include "qt/scene_qt.hpp"
#endif
#if USE_CAIRO
#include "cairo/renderer_cairo.hpp"    // Cairo graphics software renderer
#include "cairo/scene_cairo.hpp"
#endif
#if USE_OPENVG
#include "openvg/renderer_openvg.hpp"  // OpenVG 1.0.1 reference implementaiton renderer (sloow)
#include "openvg/scene_openvg.hpp"
#endif
#if USE_D2D
#include "d2d/init_d2d.hpp"
#include "d2d/renderer_d2d.hpp"        // Direct2D implementation
#include "d2d/scene_d2d.hpp"
#endif
#include "nvpr/renderer_nvpr.hpp"        // NV_path_rendering implementation

// Grumble, Microsoft's <windef.h> (and probably other headers) define these as macros
// We want the "Cg for C++" version of min & max
#undef min
#undef max

using namespace Cg;
using std::string;

const char *myProgramName = "GPU-accelerated path rendering";

// Command line defaults
int enable_sync = 0;

// Initial defaults set in initialize routine.
int verbose;
double4x4 view;
int ticks = 90;
int animation_mask = 0;
int show_control_points;
int show_warp_points;
int show_reference_points;
int show_clip_scissor;
bool haveDepthBuffer = false;
bool antiAliasedCurveEdges = 0;
int noSampleShading = 0;
unsigned int current_path_object;
unsigned int font_index = 0;
int current_clip_object =0;
unsigned int current_svg_filename = 0;
int angleRotateRate = 0;
bool have_dlist = false;  // when true, don't call glGet
bool use_dlist = false;
bool makingDlist = false;
bool render_top_to_bottom;
bool force_stencil_clear = false;
bool only_necessary_stencil_clears;
float zoom;
bool webExMode = false;
typedef enum {
    DRAW_PATH_DATA,
    DRAW_SVG_FILE,
#if USE_FREETYPE2
    DRAW_FREETYPE2_FONT,
#endif
} DrawMode;
DrawMode draw_mode = DRAW_PATH_DATA;
static const float4 default_clear_color = float4(0.1, 0.3, 0.6, 0.0);  /* Blue background */
static const float4 gray204_clear_color = float4(204/255.0f, 204/255.0f, 204/255.0f, 0.0);  /* Gray background */
static float4 current_clear_color = default_clear_color;
float4 clear_color;
int accumulationPasses = 0;
bool showSamplePattern = false;
float accumulationSpread;
static bool noDSA = false;  // true means force DSA emulation
static int extended_benchmark_requested = 0;
static int allowSoftwareWindow = true;
static int extended_benchmark_res  = 0;
static int extended_benchmark_file = 0;
bool report_benchmark_result = false;
int reshape_count = 0;
bool wait_to_exit = false;
bool skip_swap;
bool just_clear_and_swap;
int sw_window = -1;
int requested_samples = 16;
bool use_container = true;
int gl_container = -1;
int gl_window = -1;
int gold_window = -1;
int diff_window = -1;
int sw_window_width, sw_window_height;
int gold_window_width, gold_window_height;
int diff_window_width, diff_window_height;
int window_x, window_y;
int gl_window_width = 500, gl_window_height = 500;
static float2 wh;
bool software_window_valid = false;
bool request_software_window = false;
bool request_gold_window = false;
bool request_diff_window = false;
bool request_regress = false;
bool request_seed = false;
ColorSpace color_space = UNCORRECTED;
#if USE_CAIRO
cairo_antialias_t cairoAntialiasMode = CAIRO_ANTIALIAS_DEFAULT;
#endif
static enum SWMODE {
#if USE_SKIA
    SW_MODE_SKIA,
#endif
#if USE_QT
    SW_MODE_QT,
#endif
#if USE_CAIRO
    SW_MODE_CAIRO,
#endif
    SM_MODE_CYCLE,  // modes above this one cycle via 'R' key

#if USE_OPENVG
    SW_MODE_OPENVG,
#endif
#if USE_D2D
    SW_MODE_D2D,
    SW_MODE_D2D_WARP,
#endif
    SW_MODE_LIMIT,

    SW_MODE_GL,
    SW_MODE_NVPR,
} swMode = SWMODE(0);
#if USE_D2D
bool d2dUseWARP = false;
bool d2d_available = false;
#endif
bool start_animating = false;
int occlusion_query_mode;
int frames_to_render_before_exit = -1;
int xsteps, ysteps;
bool stipple;
float2 spread;  // how much to spread multiPass gride when xsteps*ysteps>1
unsigned int begin_current_svg_filename = ~0U;
int seeds_created;
int tests_regressed;
const char *fragment_profile_name = NULL,  // names of requested Cg profiles
           *vertex_profile_name = NULL;

float pixels_per_millimeter;  // used by svg_loader.cpp for units

FPScontext gl_fps_context;

typedef shared_ptr<struct RenderResult> RenderResultPtr;
typedef shared_ptr<struct RenderResultDifference> RenderResultDifferencePtr;
RenderResultPtr last_gold, last_software;
RenderResultDifferencePtr last_diff;
RenderResultPtr last_render;
RenderResultPtr last_gl;

NVprRendererPtr nvpr_renderer = NVprRendererPtr();
BlitRendererPtr blit_renderer;

GLenum default_stroke_cover_mode, default_fill_cover_mode;
bool doFilling, doStroking;

/* Forward declared GLUT callbacks registered by main. */
static void display(void);
static void reshape(int w, int h);
static void no_op();
static void keyboard(unsigned char c, int x, int y);
static void special(int key, int x, int y);
static void mouse(int button, int state, int x, int y);
static void motion(int x, int y);
static void initMenu();
static void initialize();
static void benchmarkReport(int v);
static void benchmarkSoftwareRendering(float &drawing_fps, float &raw_fps, bool printResults = true);
#if USE_D2D || USE_CAIRO || USE_QT || USE_OPENVG || USE_SKIA
static void benchmarkSoftwareRendering(float &raw_fps, BlitRendererPtr blit_renderer);
#endif
static void benchmarkGLRendering(float &drawing_fps, bool printResults = true);
static void extendedBenchmarkSoftwareRendering();
static void idleExtendedBenchmarkSoftwareRendering();
static void makeSoftwareWindow();
static void makeGoldWindow();
static void makeDiffWindow();

void swInit(int width, int height, BlitRendererPtr blit_renderer);
void swShutdown(BlitRendererPtr blit_renderer);

using std::vector;

GroupPtr scene;
float scene_ratio;
int non_opaque_objects;
GroupPtr clip;

struct RGB {
    GLubyte r, g, b;

    bool operator != (const RGB &other) {
        return r != other.r || g != other.g || b != other.b;
    }
    int3 convertToInt3() {
        return int3(r,g,b);
    }
};

void flipImage(RGB *img, int w, int h)
{
    RGB *bottom_row = &img[0],
        *top_row = &img[w*(h-1)];
    int rows_to_swap = h/2;
    for (int i=0; i<rows_to_swap; i++) {
        for (int j=0; j<w; j++) {
            std::swap(bottom_row[j], top_row[j]);
        }
        bottom_row += w;
        top_row -= w;
    }
}

struct ImageSignature {
    DrawMode draw_mode;

    unsigned int id;

    bool antialiased_curved_edges;
    bool multisampling;

    ImageSignature()
        : draw_mode(::draw_mode)
        , antialiased_curved_edges(true)
        , multisampling(nvpr_renderer->multisampling)
    {
        switch (draw_mode) {
        case DRAW_PATH_DATA:
            id = current_path_object;
            break;
        case DRAW_SVG_FILE:
            id = current_svg_filename;
            break;
#if USE_FREETYPE2
        case DRAW_FREETYPE2_FONT:
            id = font_index;
            break;
#endif
        default:
            assert(!"bogus DrawMode");
            break;
        }
    }
    bool matchesCurrentImageSignature() {
        if (antialiased_curved_edges != true) {  // XXXnvpr
            return false;
        }
        if (multisampling != nvpr_renderer->multisampling) {
            return false;
        }
        if (draw_mode != ::draw_mode) {
            return false;
        }
        switch (draw_mode) {
        case DRAW_PATH_DATA:
            return id == current_path_object;
        case DRAW_SVG_FILE:
            return id == current_svg_filename;
#if USE_FREETYPE2
        case DRAW_FREETYPE2_FONT:
            return id == font_index;
#endif
        default:
            assert(!"bogus DrawMode");
            return false;
        }
    }
    bool matchesSignature(const ImageSignature &other) {
        if (antialiased_curved_edges != other.antialiased_curved_edges) {
            return false;
        }
        if (multisampling != other.multisampling) {
            return false;
        }
        if (draw_mode != other.draw_mode) {
            return false;
        }
        return id == other.id;
    }
};

struct RenderResult {
    int width, height;
    RGB *img;  // with img[0] corresponding to lower-leftmost pixel
    ImageSignature state;

    RenderResult(int w, int h, RGB *img_)
        : width(w)
        , height(h)
        , img(img_)
    {}
    virtual ~RenderResult() {};

    void drawPixels() {
        glWindowPos2i(0,0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, img);
    }
    bool matchesCurrentImage() {
        if (gl_window_width != width || gl_window_height != height) {
            return false;
        }
        return state.matchesCurrentImageSignature();
    }
    bool matches(const RenderResultPtr other) {
        if (other->width != width || other->height != height) {
            return false;
        }
        return state.matchesSignature(other->state);
    }
    void flipImage() {
        RGB *bottom_row = &img[0],
            *top_row = &img[width*(height-1)];
        const int rows_to_swap = height/2;
        for (int i=0; i<rows_to_swap; i++) {
            for (int j=0; j<width; j++) {
                std::swap(bottom_row[j], top_row[j]);
            }
            bottom_row += width;
            top_row -= width;
        }
    }
    void writeImageToFile(const char *filename) {
        flipImage();
        stbi_write_tga(filename, width, height, 3, &img[0]);
        flipImage();
    }
    RGB getPixel(int col, int row) {
        assert(col >= 0);
        assert(col < width);
        assert(row >= 0);
        assert(row < height);
        return img[col*height+row];
    }
    void setPixel(int col, int row, int r, int g, int b) {
        assert(col >= 0);
        assert(col < width);
        assert(row >= 0);
        assert(row < height);
        img[col*height+row].r = r;
        img[col*height+row].g = g;
        img[col*height+row].b = b;
    }
};

int compare_exact = 0;
int diff_neighborhood = 1;
int max_difference = 50;
int adj_average = false;

struct RenderResultDifference : RenderResult {
    RenderResultDifference()
        : RenderResult(0, 0, NULL)
    {}
    virtual ~RenderResultDifference() {
        if (img) {
            delete img;
        }
    }
    enum MatchType {
        INCONSISTENT,
        DIFFERENT,
        APPROXIMATELY_SAME,
        SAME
    };
    MatchType compare(const RenderResultPtr a, const RenderResultPtr b,
                      int &different_pixels, int &close_pixels) {
        different_pixels = 0;
        close_pixels = 0;
        if (!a || !b) {
            return INCONSISTENT;
        }
        if (a->matches(b)) {
            MatchType result = SAME;
            const int w = a->width,
                      h = a->height;
            if (width != w || height != h) {
                if (img) {
                    delete img;
                }
                img = new RGB[w*h];
                width = w;
                height = h;
            }
            for (int i=0; i<w; i++) {
                for (int j=0; j<h; j++) {
                    RGB apix = a->getPixel(i,j),
                        bpix = b->getPixel(i,j);
                    if (apix != bpix) {
                        if (compare_exact) {
                            result = DIFFERENT;
                            int r = (apix.r - bpix.r + 255)/4 + 128,
                                g = (apix.g - bpix.g + 255)/4 + 128,
                                b = (apix.b - bpix.b + 255)/4 + 128;
                            setPixel(i, j, r,g,b);
                            different_pixels++;
                        } else {
                            bool different = true;
                            int min_diff = 255*3;
                            int max_diff = 0;
                            for (int y=-diff_neighborhood; y<=diff_neighborhood && different; y++) {
                                for (int x=-diff_neighborhood; x<=diff_neighborhood; x++) {
                                    int ii = clamp(i+y,0,h-1),
                                        jj = clamp(j+x,0,w-1);

                                    RGB cpix = b->getPixel(ii, jj);
                                    int diff;
                                    diff =           abs(apix.r-cpix.r);
                                    diff = max(diff, abs(apix.g-cpix.g));
                                    diff = max(diff, abs(apix.b-cpix.b));
                                    if (diff < max_difference) {
                                        different = false;
                                        break;
                                    }
                                    min_diff = min(min_diff, diff);
                                    max_diff = max(max_diff, diff);

                                    if (adj_average) {
                                        int3 iapix = apix.convertToInt3();
                                        int3 ibpix = bpix.convertToInt3();
                                        int3 icpix = cpix.convertToInt3();
                                        int3 idpix = (ibpix + icpix) / 2;
                                        int3 diff_pix = abs(iapix-idpix);

                                        diff = max(diff_pix.r, max(diff_pix.g, diff_pix.b));
                                        if (diff < max_difference) {
                                            different = false;
                                            break;
                                        }
                                        min_diff = min(min_diff, diff);
                                        max_diff = max(max_diff, diff);
                                    }
                                }
                            }
                            if (different) {
                                different_pixels++;
                                result = DIFFERENT;
                                int r = (apix.r - bpix.r + 255)/4 + 128,
                                    g = (apix.g - bpix.g + 255)/4 + 128,
                                    b = (apix.b - bpix.b + 255)/4 + 128;
                                setPixel(i, j, r,g,b);
                            } else {
                                if (result != DIFFERENT && min_diff > 0) {
                                    result = APPROXIMATELY_SAME;
                                }
                                setPixel(i, j, apix.r/3,apix.g/3,apix.b/3);
                                close_pixels++;
                            }
                        }
                    } else {
                        setPixel(i, j, apix.r/3,apix.g/3,apix.b/3);
                    }
                }
            }
            return result;
        }
        return INCONSISTENT;
    }
};

struct RenderResultFromFile : RenderResult {
    RenderResultFromFile(int w, int h, stbi_uc *img_)
        : RenderResult(w, h, reinterpret_cast<RGB*>(img_))
    {
        flipImage();
    }
    virtual ~RenderResultFromFile() {
        stbi_uc *data = reinterpret_cast<stbi_uc*>(img);
        stbi_image_free(data);
    }
};

struct RenderResultFromGLUTWindow : RenderResult {
    RenderResultFromGLUTWindow(int glut_window)
        : RenderResult(0, 0, NULL)
    {
        glutSetWindow(glut_window);
        width = glutGet(GLUT_WINDOW_WIDTH);
        height = glutGet(GLUT_WINDOW_HEIGHT);

        glWindowPos2i(0,0);
        const int components = 3;
        GLubyte *pixels = new GLubyte[width*height*components];
        glPixelStorei(GL_PACK_ALIGNMENT, 1);  // avoid GL's dumb default of 4
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        img = reinterpret_cast<RGB*>(pixels);
    }
    virtual ~RenderResultFromGLUTWindow() {
        GLubyte *pixels = reinterpret_cast<GLubyte*>(img);
        delete pixels;
    }
};

void validateStencil()
{
    glutSetWindow(gl_window);
    glClearStencil(0);
    // Transition from reverse painter's algorithm to forward requires forcing a stencil clear.
    force_stencil_clear = true;
}

const int GL_WINDOW = 1;
const int SW_WINDOW = 2;
const int GOLD_WINDOW = 4;
const int BOTH_WINDOWS = 3;
const int ALL_WINDOWS = 7;

static void do_redisplay(int mask)
{
    if (mask & GL_WINDOW) {
        if (gl_window >= 0) {
            glutPostWindowRedisplay(gl_window);
        }
    }
    if (mask & SW_WINDOW) {
        if (sw_window >= 0) {
            glutPostWindowRedisplay(sw_window);
        }
    }
    if (mask & GOLD_WINDOW) {
        if (gold_window >= 0) {
            glutPostWindowRedisplay(gold_window);
        }
    }
}



static void send_clear_color(const float4 &clear_color)
{
    if (nvpr_renderer->render_sRGB) {
        float4 lin_color = srgb2linear(clear_color);
        glClearColor(lin_color.r, lin_color.g, lin_color.b, lin_color.a);
    } else {
        glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
    }
}

void update_clear_color()
{
    glutSetWindow(gl_window);
    if (render_top_to_bottom) {
        if (non_opaque_objects > 0) {
            glClearColor(0,0,0,0);
        } else {
            send_clear_color(clear_color);
        }
    } else {
        send_clear_color(clear_color);
    }
    software_window_valid = false;
    do_redisplay(BOTH_WINDOWS);
}

static void glinit()
{
    // XXX remember this when you try to make depth buffering work!
    glDepthMask(0);

    glLineStipple(1, 0xF0F0);
    glFrontFace(GL_CCW);  /* OpenGL's default. */
 
    if (render_top_to_bottom) {
        if (nvpr_renderer->alpha_bits > 0) {
            printf("using Reverse Painter's algorithm (top-to-bottom render order)\n");
        } else {
            render_top_to_bottom = false;
            printf("framebuffer lacks alpha so Reverse Painter's algorithm not allowed\n");
        }
    }
    validateStencil();
    update_clear_color();
}

void set_clear_color(float4 color)
{
    clear_color = color;
    if (dot(color.rgb,float3(0.3,0.5,0.2)) > 0.75) {
        colorFPS(0.3,0.5,0.9);
    } else {
        colorFPS(1,1,0);
    }
    update_clear_color();
}

#if USE_FREETYPE2
static void setFontScene(unsigned int font_index);
#endif
static void setPathScene(PathInfo &pathInfo);
static void setSVGScene(const char *svg_filename);
static void setClipObject(PathInfo &pathInfo);
static void reloadScene();
int lookupSVGPath(const char *filename);
static void idle();
static void benchmarkIdle();
static void enableAnimation(int mask);

#if !defined(_WIN32)
#include <strings.h>
#define stricmp strcasecmp
#endif

static void invalidateScene()
{
    if (scene) {
        ForEachShapeTraversal traversal;
        scene->traverse(VisitorPtr(new Invalidate), traversal);
    }
}

void configureWindowForGL()
{
    static bool not_done_yet = true;

    glinit();
    set_clear_color(current_clear_color);

    glutDisplayFunc(display);
    if (gl_container < 0) {
        glutReshapeFunc(reshape);
    }
    if (report_benchmark_result) {
        printf("benchmark mode disables user input\n");
    } else {
        glutSetWindow(gl_container);
        glutKeyboardFunc(keyboard);
        glutSpecialFunc(special);

        glutSetWindow(gl_window);
        glutKeyboardFunc(keyboard);
        glutSpecialFunc(special);
        glutMouseFunc(mouse);
        glutMotionFunc(motion);
        initMenu();
    }

    nvpr_renderer->interrogateFramebuffer();
    nvpr_renderer->configureGL();
    nvpr_renderer->setColorSpace(color_space);

    requestSynchornizedSwapBuffers(enable_sync);

    nvpr_renderer->init_cg(vertex_profile_name, fragment_profile_name);
    if (verbose || not_done_yet) {
        nvpr_renderer->report_cg_profiles();
    }
    nvpr_renderer->load_shaders();

#if USE_NVPR
    if (nvpr_renderer) {
        nvpr_renderer->setColorSpace(color_space);
        if (verbose || not_done_yet) {
            nvpr_renderer->report_cg_profiles();
        }
        nvpr_renderer->load_shaders();
    }
#endif

    not_done_yet = false;
}

void configureSamples(int count)
{
    printf("Reconfiguring GPU framebuffer for %d samples/pixel...\n", count);
    assert(use_container);
    requested_samples = count;

    glutSetWindow(gl_window);
    // Force deletion of all texture & buffer objects held by nvpr_renderer.
    // XXX should this really be needed?
    ForEachShapeTraversal traversal;
    scene->traverse(VisitorPtr(new FlushRendererState(nvpr_renderer)), traversal);

    glutSetWindow(gl_container);
    glutDestroyWindow(gl_window);

    char display_string[200];
    sprintf(display_string, "rgb alpha~8 double stencil~8 %s samples=%d acc=16",
        haveDepthBuffer ? "depth" : "",
        count);
    glutInitDisplayString(display_string);
    if (!glutGet(GLUT_DISPLAY_MODE_POSSIBLE)) {
        printf("fallback GLUT display config!\n");
        glutInitDisplayString(NULL);
        int displayMode = GLUT_RGB | GLUT_ALPHA | GLUT_DOUBLE | GLUT_STENCIL | GLUT_ACCUM;
        if (haveDepthBuffer) {
            displayMode |= GLUT_DEPTH;
        } 
        glutInitDisplayMode(displayMode);
    }

    gl_window = glutCreateSubWindow(gl_container, 0, 0, gl_window_width, gl_window_height);
    nvpr_renderer->setViewport(0, 0, gl_window_width, gl_window_height);

    configureWindowForGL();
    invalidateScene();  // because there's a new OpenGL context
}

int main(int argc, char **argv)
{
    char display_string[200];

    printf("Executable: %d bit\n", (int)(8*sizeof(int*)));

    glutInitWindowSize(gl_window_width, gl_window_height);

    glutInit(&argc, argv);

    initialize();
    for (int i=1; i<argc; i++) {
        if (!stricmp("-nosync", argv[i])) {
            printf("requesting no vertical retrace sync\n");
            enable_sync = 0;
        } else
        if (!stricmp("-nocontainer", argv[i])) {
            printf("-nocontainer so no run-time control of antilaising mode\n");
            use_container = false;
        } else
        if (!stricmp("-z", argv[i])) {
            printf("requesting no depth buffer\n");
            haveDepthBuffer = true;
        } else            
        if (!strcmp("-vsync", argv[i])) {
            printf("requesting vertical retrace sync\n");
            enable_sync = 1;
        } else
        if (!stricmp("-aliased", argv[i]) ||
            !strcmp("-1", argv[i])) {
            printf("requesting no anti-aliasing\n");
            requested_samples = 0;
        } else
        if (!strcmp("-32", argv[i])) {
            printf("requesting 32 samples\n");
            requested_samples = 32;
        } else
        if (!strcmp("-16", argv[i])) {
            printf("requesting 16 samples\n");
            requested_samples = 16;
        } else
        if (!strcmp("-8", argv[i])) {
            printf("requesting 8 samples\n");
            requested_samples = 8;
        } else
        if (!strcmp("-4", argv[i])) {
            printf("requesting 4 samples\n");
            requested_samples = 4;
        } else
        if (!strcmp("-2", argv[i])) {
            printf("requesting 2 samples\n");
            requested_samples = 2;
        } else
        if (!stricmp("-linearRGB", argv[i])) {
            printf("requesting linear RGB (linear interpolation & blending)\n");
            color_space = CORRECTED_SRGB;
        } else
        if (!strcmp("-v", argv[i])) {
            verbose = 1;
        } else
        if (!stricmp("-noDSA", argv[i])) {
            printf("will never use DSA (EXT_direct_state_access), emulates DSA instead\n");
            noDSA = true;
        } else
        if (!stricmp("-noStroking", argv[i])) {
            printf("SKIP stroking in content (use 'x' to toggle back on)\n");
            doStroking = false;
        } else
        if (!stricmp("-noFilling", argv[i])) {
            printf("SKIP filling in content (use 'X' to toggle back on)\n");
            doFilling = false;
        } else
        if (!stricmp("-grid", argv[i])) {
            i++;
            if (i >= argc) {
              printf("-grid expects NxM argument\n");
              exit(1);
            } else {
              int w, h;

              int rc = sscanf(argv[i], "%dx%d", &w, &h);
              if (rc == 2) {
                  stipple = false;
                  xsteps = w;
                  ysteps = h;
              } else {
                  printf("-grid expects NxM argument\n");
                  exit(1);
              }
            }
        } else
        if (!stricmp("-stipple", argv[i])) {
            i++;
            if (i >= argc) {
              printf("-stipple expects NxM argument\n");
              exit(1);
            } else {
              int w, h;

              int rc = sscanf(argv[i], "%dx%d", &w, &h);
              if (rc == 2) {
                  stipple = true;
                  xsteps = w;
                  ysteps = h;
              } else {
                  printf("-stipple expects NxM argument\n");
                  exit(1);
              }
            }
        } else
        if (!stricmp("-dlist", argv[i])) {
            printf("use display lists\n");
            use_dlist = true;
        } else
        if (!stricmp("-webex", argv[i])) {
            printf("WebEx mode reloads shaders on every scene change\n");
            webExMode = true;
        } else
        if (!stricmp("-frameCount", argv[i])) {
            i++;
            if (i >= argc) {
              printf("-frameCount expects integer argument\n");
              exit(1);
            } else {
              frames_to_render_before_exit = atoi(argv[i]);
            }
            printf("requested %d frames to render before exit\n", frames_to_render_before_exit);
        } else
        if (!stricmp("-benchmark", argv[i])) {
            glutIdleFunc(idle);
            angleRotateRate = 0;
            report_benchmark_result = true;
        } else
        if (!stricmp("-xbenchmark", argv[i])) {  // Musawir's extended benchmark mode
            extended_benchmark_requested = 1;
        } else
        if (!stricmp("-noSW", argv[i])) {
            allowSoftwareWindow = false;
        } else
        if (!stricmp("-black", argv[i])) {
            current_clear_color = float4(0);
        } else
        if (!stricmp("-white", argv[i])) {
            current_clear_color = float4(1,1,1,0);
        } else
        if (!stricmp("-blue", argv[i])) {
            current_clear_color = default_clear_color;
        } else
        if (!stricmp("-vprofile", argv[i])) {
            i++;
            if (i >= argc) {
                printf("-vprofile expects profile name argument, such as arbvp1\n");
                exit(1);
            } else {
                vertex_profile_name = argv[i];
                printf("Requesting Cg vertex profile %s\n", vertex_profile_name);
            }
        } else
        if (!stricmp("-fprofile", argv[i])) {
            i++;
            if (i >= argc) {
                printf("-fprofile expects profile name argument, such as arbfp1\n");
                exit(1);
            } else {
                fragment_profile_name = argv[i];
                printf("Requesting Cg fragment profile %s\n", fragment_profile_name);
            }
        } else
        if (!stricmp("-wait_to_exit", argv[i])) {
            wait_to_exit = true;
        } else
        if (!stricmp("-noSpin", argv[i])) {
            angleRotateRate = 0;
        } else
        if (!stricmp("-spin", argv[i])) {
            angleRotateRate = 1;
        } else
        if (!stricmp("-animate", argv[i]) ||
            !stricmp("-animating", argv[i]) ||
            !stricmp("-start_animating", argv[i])) {
            start_animating = true;
        } else
        if (!stricmp("-diff", argv[i])) {
            request_diff_window = true;
        } else
        if (!stricmp("-gold", argv[i])) {
            request_gold_window = true;
        } else
        if (!stricmp("-regress", argv[i])) {
            request_regress = true;
        } else
        if (!stricmp("-seed", argv[i])) {
            request_seed = true;
        } else
#if USE_CAIRO
        if (!stricmp("-cairo", argv[i])) {
            request_software_window = true;
            swMode = SW_MODE_CAIRO;
        } else
#endif
#if USE_SKIA
        if (!stricmp("-skia", argv[i])) {
            request_software_window = true;
            swMode = SW_MODE_SKIA;
        } else
#endif
#if USE_QT
        if (!stricmp("-qt", argv[i])) {
            request_software_window = true;
            swMode = SW_MODE_QT;
        } else
#endif
#if USE_OPENVG
        if (!stricmp("-openvg", argv[i])) {
            request_software_window = true;
            swMode = SW_MODE_OPENVG;
        } else
#endif
#if USE_D2D
        if (!stricmp("-d2d", argv[i])) {
            request_software_window = true;
            swMode = SW_MODE_D2D;
        } else
#endif
        if (!stricmp("-accumPasses", argv[i])) {
            i++;
            if (i >= argc) {
              printf("-accumPasses expects integer argument\n");
              exit(1);
            } else {
              accumulationPasses = atoi(argv[i]);
            }
            printf("requested %d accumulation passes\n", accumulationPasses);
        } else
        if (!stricmp("-topToBottom", argv[i]) ||
            !stricmp("-reversePainters", argv[i])) {
            render_top_to_bottom = true;
        } else
        if (!stricmp("-best", argv[i]) ||
            !stricmp("-bestquality", argv[i])) {
            printf("requesting best quality (iterate over samples if ARB_sample_shading or NV_explicit_multisample is supported)\n");
            antiAliasedCurveEdges = true;
        } else
        if (!stricmp("-noSampleShading", argv[i]) ||
            !stricmp("-noSampleShade", argv[i])) {
            printf("FORCE ignoring ARB_sample_shading\n");
            noSampleShading = true;
        } else
        if (!stricmp("-svg", argv[i])) {
            i++;
            if (i < argc) {
                draw_mode = DRAW_SVG_FILE;
                current_svg_filename = lookupSVGPath(argv[i]);
            } else {
                printf("-svg option expects a following SVG filename\n");
                exit(1);
            }
#if USE_FREETYPE2
        } else
        if (!stricmp("-font", argv[i])) {
            i++;
            if (i < argc) {
                draw_mode = DRAW_FREETYPE2_FONT;
                font_index = lookup_font(argv[i]);
            } else {
                printf("-font option expects a following font name\n");
                exit(1);
            }
#endif
        } else
        if (!stricmp("-path", argv[i])) {
            i++;
            if (i < argc) {
                draw_mode = DRAW_PATH_DATA;
                current_path_object = abs(atoi(argv[i])) % num_path_objects;
            } else {
                printf("-path option expects a following path index\n");
                exit(1);
            }
        } else {
            printf("unrecognized option: %s\n", argv[i]);
            exit(1);
        }
    }

    if (xsteps*ysteps > 1) {
        const int gridSize = xsteps*ysteps;
        const int passes = stipple ? gridSize/2 : gridSize;
        printf("Requested passCount = %d (%dx%d%s)\n", passes, xsteps, ysteps, stipple ? " stippled" : "");
    }
    colorFPS(1,1,0);

    if (use_container) {
        glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
        gl_container = glutCreateWindow(myProgramName);
        glutReshapeFunc(reshape);
        glutDisplayFunc(no_op);
        glutMouseFunc(mouse);
        glutMotionFunc(motion);
    }

    sprintf(display_string, "rgb alpha~8 double stencil~8 %s samples=%d acc=16",
        haveDepthBuffer ? "depth" : "", requested_samples);
    glutInitDisplayString(display_string);
    if (!glutGet(GLUT_DISPLAY_MODE_POSSIBLE)) {
        printf("fallback GLUT display config!\n");
        glutInitDisplayString(NULL);
        int displayMode = GLUT_RGB | GLUT_ALPHA | GLUT_DOUBLE | GLUT_STENCIL | GLUT_ACCUM;
        if (haveDepthBuffer) {
            displayMode |= GLUT_DEPTH;
        } 
        glutInitDisplayMode(displayMode);
    }

    if (use_container) {
        gl_window = glutCreateSubWindow(gl_container, 0, 0, gl_window_width, gl_window_height);
    } else {
        gl_window = glutCreateWindow(myProgramName);
    }

    int screen_width_in_pixels = glutGet(GLUT_SCREEN_WIDTH);
    int screen_width_in_mm = glutGet(GLUT_SCREEN_WIDTH_MM);
    //int screen_height_in_mm = glutGet(GLUT_SCREEN_HEIGHT_MM);
    pixels_per_millimeter = float(screen_width_in_pixels)/screen_width_in_mm;

    if (glutExtensionSupported("GL_NV_path_rendering")) {
        if (!nvpr_renderer) {
            nvpr_renderer = NVprRendererPtr(new NVprRenderer(noDSA, vertex_profile_name, fragment_profile_name));
        }
        printf("NV_path_rendering extension supported\n");
    } else {
        printf("NV_path_rendering not supported by OpenGL implementation, cannot run\n");
        exit(1);
    }

#if USE_D2D
    d2d_available = initDirect2D();
    if (d2d_available) {
        printf("Direct2D renderer is available\n");
    } else {
        printf("NO Direct2D renderer available (try running under Vista or Windows 7)\n");
    }
#endif

    nvpr_renderer->setColorSpace(color_space);

    configureWindowForGL();

    setClipObject(path_objects[current_clip_object]);
    reloadScene();

    if (start_animating) {
        enableAnimation(GL_WINDOW);
    }

    if (extended_benchmark_requested) {
        if (allowSoftwareWindow) {
            request_software_window = true;
        }
        glutIdleFunc(idleExtendedBenchmarkSoftwareRendering);
    } else if (report_benchmark_result) {
        int sixSeconds = 6000;

        printf("RUNNING BENCHMARK (will complete in %d seconds)...\n", sixSeconds/1000);
        //enableFPS();
        animation_mask = GL_WINDOW;
        glutTimerFunc(sixSeconds, benchmarkReport, 0);
    }

    initFPScontext(&gl_fps_context, FPS_USAGE_TEXTURE);
    scaleFPS(3);

    glutMainLoop();
    return 0;
}

static void resetView()
{
    view = identity4x4();
    ticks = 90;
}

static float4x4 view_to_surface;
static void updateTransform();

static const char *sw_windowTitle(BlitRendererPtr blit_renderer);

static void disableAnimation(int mask)
{
    animation_mask &= ~mask;
#if USE_D2D
    if (swMode == SW_MODE_D2D || swMode == SW_MODE_D2D_WARP) {
        if (animation_mask & SW_WINDOW) {
            // still animating, leave title alone
        } else {
            // Remove fps from title
            glutSetWindow(sw_window);
            const char *title = blit_renderer->getWindowTitle();
            glutSetWindowTitle(title);
        }
    }
#endif
    if (animation_mask == 0) {
        glutIdleFunc(NULL);
        disableFPS();
    }
    if (gl_window >= 0) {
        glutSetWindow(gl_window);
        updateTransform();
    }
    software_window_valid = false;
    do_redisplay(mask);
}

static void enableAnimation(int mask)
{
    invalidateFPS();
    animation_mask |= mask;
    enableFPS();
    glutIdleFunc(idle);
}

static void updateSceneCoverModes(NodePtr scene)
{
#if 0 // XXXnvpr
    ForEachShapeTraversal traversal;
    scene->traverse(VisitorPtr(
        new GLVisitors::SetCoverMode(nvpr_renderer, default_fill_cover_mode, default_stroke_cover_mode)),
        traversal);
#endif
}

static void setClipObject(PathInfo &pathInfo)
{
    printf("current clip object: %s\n", pathInfo.name);
    clip = GroupPtr(new Group);

    GroupPtr combo = GroupPtr(new Group);
    
    TransformPtr transformedCombo(new Transform(combo));

    int n = pathInfo.num_path_strings;
    float4 master_bbox;
    for (int i=0; i<n; i++) {
        PathStyle style;
        style.fill_rule = pathInfo.fill_rule;
        PathPtr path(new Path(style, pathInfo.path_string[i]));
        if (i == 0) {
            master_bbox = path->getBounds();
        } else {
            const float4 path_bounds = path->getBounds();
            master_bbox.xy = min(master_bbox.xy, path_bounds.xy);
            master_bbox.zw = max(master_bbox.zw, path_bounds.zw);
        }

        ShapePtr shape(new Shape(path));
        combo->push_back(shape);
    }
    if (verbose) {
        std::cout << "master clip bounding box = " << master_bbox << std::endl;
    }
    float l = master_bbox.x,
          r = master_bbox.z,
          t = master_bbox.y,
          b = master_bbox.w;
    float4x4 ortho = float4x4(2/(r-l),0,0,-(r+l)/(r-l),
                              0,2/(t-b),0,-(t+b)/(t-b),
                              0,0,1,0,
                              0,0,0,1);
    // Scale down by 90% to better fit frame
    float4x4 scale = float4x4(0.9,0,0,0,
                              0,0.9,0,0,
                              0,0,1,0,
                              0,0,0,1);
    transformedCombo->setMatrix(mul(scale,ortho));

    clip->push_back(transformedCombo);
}

#if USE_FREETYPE2
GroupPtr glyphLoader(unsigned int font_index)
{
    float y = 0;

    char glyph = '!';  // initial character

    init_freetype2();
    GroupPtr group = GroupPtr(new Group);
    for (int j=9; j>=0; j--) {
        float line_height = 0;

        float x = 0;
        for (int i=0; i<10; i++) {
            PathPtr path = load_freetype2_glyph(font_index, glyph+i+j*10);
            if (!path) {
                return GroupPtr();
            }

            float4 fillColor = float4(1,1,1,1),  // white
                   strokeColor = float4(1,0,0,1);  // red
            if (all(clear_color.rgb == 1)) {
                fillColor = float4(0,0,0,1);  // black
            }
            SolidColorPaintPtr fill_paint(new SolidColorPaint(fillColor)),
                               stroke_paint(new SolidColorPaint(strokeColor));
            ShapePtr shape(new Shape(path, fill_paint, stroke_paint));
            float4x4 matrix = float4x4(1,0,0,x,
                                       0,1,0,y,
                                       0,0,1,0,
                                       0,0,0,1);

            TransformPtr translated_glyph(new Transform(shape, matrix));
            group->push_back(translated_glyph);
            float4 bbox = path->getBounds();
            float char_width = bbox.z-bbox.x,
                  char_height = bbox.w-bbox.y;
            x += char_width;
            if (char_height > line_height) {
                line_height = char_height;
            }
        }
        y += line_height;
    }
    return group;
}
#endif

// Collect various functions that must be called and
// variables that must be reset when the scene changes.
static void sceneChanged()
{
    // scene_ratio might have changed
    nvpr_renderer->setSceneRatio(scene_ratio);
    updateSceneCoverModes(scene);
    software_window_valid = false;
    force_stencil_clear = true;

    CountNonOpaqueObjectsPtr counter(new CountNonOpaqueObjects);
    scene->traverse(counter);
    non_opaque_objects = counter->getCount();
    update_clear_color();
    updateTransform();
    do_redisplay(ALL_WINDOWS);
    invalidateFPS();

	if (webExMode) {
		nvpr_renderer->load_shaders();
	}
}

#if USE_FREETYPE2
static void setFontScene(unsigned int font_index)
{
    glutSetWindow(gl_window);
    GroupPtr glyph_group = glyphLoader(font_index);
    if (glyph_group) {
        const float4 scene_bounds = glyph_group->getBounds();
        float l = scene_bounds.x,
              r = scene_bounds.z,
              b = scene_bounds.y,
              t = scene_bounds.w;
        scene_ratio = abs(b-t)/abs(r-l);
        // Wind "from" vertices in counter-clockwise order from lower-left
        const float2 from[4] = {float2(l,b),float2(r,b),float2(r,t),float2(l,t)};
        float xscale = 1,
              yscale = 1;
        if (abs(r-l) > abs(b-t)) {
            yscale = abs(b-t)/abs(r-l);
            if (verbose) {
                printf("wide %f\n", scene_ratio);
            }
        } else {
            xscale = abs(r-l)/abs(b-t);
            if (verbose) {
                printf("tall %f\n", scene_ratio);
            }
        }
        // Wind "to" vertices in counter-clockwise order from lower-left
        static const float2 to[4] = {float2(-1,-1),float2(1,-1),float2(1,1),float2(-1,1)};
        // Scale down by 90% to better fit frame
        float shrinkage = 0.9;
        WarpTransformPtr normalizedScene(new WarpTransform(glyph_group, to, from,
            shrinkage*float2(xscale, yscale)));
        scene = GroupPtr(new Group);
        scene->push_back(normalizedScene);

        printf("scalable font: %s\n", font_name(font_index));
        sceneChanged();
    }
}
#endif

static void setPathScene(PathInfo &pathInfo)
{
    printf("current path object: %s\n", pathInfo.name);
    glutSetWindow(gl_window);
    scene = GroupPtr(new Group);

    GroupPtr combo = GroupPtr(new Group);

    // If you want to hack multiple instances of the path...
    for (int instance=0; instance<1; instance++) {
        int n = pathInfo.num_path_strings;
        float4 master_bbox;
        for (int i=0; i<n; i++) {
            PathStyle style;
            style.fill_rule = pathInfo.fill_rule;
            PathPtr path(new Path(style, pathInfo.path_string[i]));
            if (i == 0) {
                master_bbox = path->getBounds();
            } else {
                const float4 path_bounds = path->getBounds();
                master_bbox.xy = min(master_bbox.xy, path_bounds.xy);
                master_bbox.zw = max(master_bbox.zw, path_bounds.zw);
            }
            SolidColorPaintPtr fill_paint(new SolidColorPaint(float4(pathInfo.fill_color[i], 1))),
                               stroke_paint(new SolidColorPaint(float4(1,0,0,1)));
            ShapePtr shape(new Shape(path, fill_paint, stroke_paint));
            combo->push_back(shape);
        }
        if (verbose) {
            std::cout << "master fill bounding box = " << master_bbox << std::endl;
        }
        float l = master_bbox.x,
              r = master_bbox.z,
              t = master_bbox.y,
              b = master_bbox.w;

        // Wind "from" vertices in counter-clockwise order from lower-left
        const float2 from[4] = {float2(l,b),float2(r,b),float2(r,t),float2(l,t)};
        float xscale = 1,
              yscale = 1;
        if (abs(r-l) > abs(b-t)) {
            yscale = abs(b-t)/abs(r-l);
            if (verbose) {
                printf("wide %f\n", scene_ratio);
            }
        } else {
            xscale = abs(r-l)/abs(b-t);
            if (verbose) {
                printf("tall %f\n", scene_ratio);
            }
        }
        // Wind "to" vertices in counter-clockwise order from lower-left
        static const float2 to[4] = {float2(-1,-1),float2(1,-1),float2(1,1),float2(-1,1)};
        // Scale down by 90% to better fit frame
        float shrinkage = 0.9;
        WarpTransformPtr transformedCombo(new WarpTransform(combo, to, from,
            shrinkage*float2(xscale, yscale)));

        scene->push_back(transformedCombo);
    }
    sceneChanged();
}

static void setSVGScene(const char *svg_filename)
{
    bool printStuff = !extended_benchmark_requested || verbose;

    if (printStuff) {
        printf("loading SVG file: %s...", svg_filename);
    }
    fflush(stdout);
    glutSetWindow(gl_window);
    SvgScenePtr svg_scene = svg_loader(svg_filename);
    if (svg_scene) {

        const float4 svg_scene_bounds = svg_scene->getBounds();
        float l = svg_scene_bounds.x,
              r = svg_scene_bounds.z,
              t = svg_scene_bounds.y,
              b = svg_scene_bounds.w;
        scene_ratio = abs(b-t)/abs(r-l);
        // Wind "from" vertices in counter-clockwise order from lower-left
        const float2 from[4] = {float2(l,b),float2(r,b),float2(r,t),float2(l,t)};
        float xscale = 1,
              yscale = 1;
        if (abs(r-l) > abs(b-t)) {
            yscale = abs(b-t)/abs(r-l);
            if (verbose) {
                printf("wide %f\n", scene_ratio);
            }
        } else {
            xscale = abs(r-l)/abs(b-t);
            if (verbose) {
                printf("tall %f\n", scene_ratio);
            }
        }
        // Wind "to" vertices in counter-clockwise order from lower-left
        static const float2 to[4] = {float2(-1,-1),float2(1,-1),float2(1,1),float2(-1,1)};
        // Scale down by 90% to better fit frame
        float shrinkage = 0.9;
        WarpTransformPtr normalizedScene(new WarpTransform(svg_scene, to, from,
            shrinkage*float2(xscale, yscale)));
        scene = GroupPtr(new Group);
        scene->push_back(normalizedScene);
        sceneChanged();
        if (printStuff) {
            printf(" ok\n");
        }
    } else {
        if (printStuff) {
            printf(" FAILED\n");
        }
    }
}

static void reloadScene()
{
    switch (draw_mode) {
    case DRAW_PATH_DATA:
        setPathScene(path_objects[current_path_object]);
        break;
    case DRAW_SVG_FILE:
        setSVGScene(getSVGFileName(current_svg_filename));
        break;
#if USE_FREETYPE2
    case DRAW_FREETYPE2_FONT:
        setFontScene(font_index);
        break;
#endif
    default:
        assert(!"bogus draw mode");
        break;
    }
}

static void initialize()
{
    current_svg_filename = 0;
    current_path_object = 0;
    current_clip_object = 1;
    draw_mode = DRAW_PATH_DATA;
    //draw_mode = DRAW_SVG_FILE;

    verbose = 0;
    resetView();
    show_control_points = 0;
    show_warp_points = 0;
    show_reference_points = 0;
    show_clip_scissor = 0;
    zoom = 1;
    angleRotateRate = 0;
    render_top_to_bottom = false;
    only_necessary_stencil_clears = true;

    accumulationPasses = 0;
    showSamplePattern = false;
    accumulationSpread = 1.0;
    skip_swap = false;
    just_clear_and_swap = false;
    software_window_valid = false;
    occlusion_query_mode = 0;
    xsteps = 1;
    ysteps = 1;
    stipple = true;
    spread = float2(2.0);

    default_stroke_cover_mode = GL_CONVEX_HULL_NV;
    default_fill_cover_mode = GL_CONVEX_HULL_NV;

    doFilling = true;
    doStroking = true;

    disableAnimation(BOTH_WINDOWS);
}

static void reinitialize()
{
    initialize();
    setClipObject(path_objects[current_clip_object]);
    reloadScene();
}

static void menu(int item)
{
    if (item < 0) {
        special(-item, 0, 0);
    } else {
        keyboard((unsigned char)item, 0, 0);
    }
}

static void pathMenu(int item)
{
    assert(item < num_path_objects);
    current_path_object = item;
    setPathScene(path_objects[current_path_object]);
    have_dlist = false;
    software_window_valid = false;
    draw_mode = DRAW_PATH_DATA;
    do_redisplay(ALL_WINDOWS);
}

static void svgMenu(int item)
{
    current_svg_filename = item;
    setSVGScene(getSVGFileName(current_svg_filename));
    have_dlist = false;
    software_window_valid = false;
    draw_mode = DRAW_SVG_FILE;
    do_redisplay(ALL_WINDOWS);
}

#if USE_FREETYPE2
static void fontMenu(int item)
{
    assert(item < num_fonts());
    font_index = item;
    setFontScene(font_index);
    have_dlist = false;
    software_window_valid = false;
    draw_mode = DRAW_FREETYPE2_FONT;
    do_redisplay(ALL_WINDOWS);
}
#endif

static struct JitterConfig {
    int xsteps, ysteps;
    bool stipple;
    float xspread, yspread;
    const char *name;
} jitter_config[] = {
    { 1,1, false, 2.0,2.0, "1x1 normal" },
    { 3,3, false, 2.0,2.0, "3x3 normal" },
    { 3,3, true, 2.0,2.0, "4-sample cross" },
    { 4,4, false, 2.0,2.0, "4x4 normal" },
    { 4,4, true, 2.0,2.0, "8-sample 4x4 checkerboard" },
    { 7,2, false, 14.0,2.0, "7x2 horizontal blur" },
    { 2,7, false, 2.0,14.0, "2x7 vertical blur"  },
    { 7,7, false, 14.0,14.0, "7x7 square blur"  },
};

static void forceOffRenderTopToBottom()
{
    if (render_top_to_bottom) {
        render_top_to_bottom = 0;
        printf("forcing off render_top_to_bottom = %d\n", render_top_to_bottom);
        update_clear_color();
    }
}

static void jitterMenu(int item)
{
    assert(item >= 0);
    assert(item < icountof(jitter_config));
    xsteps = jitter_config[item].xsteps;
    ysteps = jitter_config[item].ysteps;
    stipple = jitter_config[item].stipple;
    spread = float2(jitter_config[item].xspread, jitter_config[item].yspread);

    have_dlist = false;
    software_window_valid = false;
#if USE_FREETYPE2
    draw_mode = DRAW_FREETYPE2_FONT;
#endif
    do_redisplay(BOTH_WINDOWS);

    forceOffRenderTopToBottom();
}

enum {
    MENU_NEAREST = 100000,  // way above the max color map size!
    MENU_LINEAR,
    MENU_MIPMAP,
};

class InvalidatePaintRendererState : public Visitor
{
public:
    RendererPtr renderer;
    InvalidatePaintRendererState(RendererPtr renderer_)
        : renderer(renderer_)
    {
    }

    void visit(ShapePtr shape) {
        if (shape->getFillPaint()) {
            shape->getFillPaint()->invalidateRenderState(renderer);
        }
        if (shape->getStrokePaint()) {
            shape->getStrokePaint()->invalidateRenderState(renderer);
        }
    }
};

static void textureFilterMenu(int item)
{
    if (item >=1 && item <= nvpr_renderer->max_anisotropy_limit) {
        printf("Setting max anistropy = %d\n", item);
        nvpr_renderer->max_anisotropy = item;
    } else {
        if (item >= 32 && item <= 8192) {
            printf("Setting color ramp size = %d\n", item);
            nvpr_renderer->color_ramp_size = item;
        } else {
            switch (item) {
            case MENU_NEAREST:
                nvpr_renderer->min_filter = GL_NEAREST;
                nvpr_renderer->mag_filter = GL_NEAREST;
                printf("Setting nearest/nearest filtering\n");
                break;
            case MENU_LINEAR:
                nvpr_renderer->min_filter = GL_LINEAR;
                nvpr_renderer->mag_filter = GL_LINEAR;
                printf("Setting linear/linear filtering\n");
                break;
            case MENU_MIPMAP:
                nvpr_renderer->min_filter = GL_LINEAR_MIPMAP_LINEAR;
                nvpr_renderer->mag_filter = GL_LINEAR;
                printf("Setting linear/linear-mipmap-linear filtering\n");
                break;
            default:
                return;  // ignore
            }
        }
    }
    invalidateScene();
    glutPostWindowRedisplay(gl_window);
}

static int makeTextureMenu()
{
    int tex_filter_menu = glutCreateMenu(textureFilterMenu);
    glutAddMenuEntry("Nearest-Nearest", MENU_NEAREST);
    glutAddMenuEntry("Linear-Linear", MENU_LINEAR);
    glutAddMenuEntry("Linear/LinearMipmapLinear", MENU_MIPMAP);
    GLint max_aniso = max(16, nvpr_renderer->max_anisotropy_limit);
    while (max_aniso > 0) {
        string label = "Max anistropy " + lexical_cast<string,int>(max_aniso);
        glutAddMenuEntry(label.c_str(), max_aniso);
        max_aniso >>= 1;
    }
    int color_ramp_size = 8192;
    while (color_ramp_size >= 32) {
        string label = "Color ramp size " + lexical_cast<string,int>(color_ramp_size);
        glutAddMenuEntry(label.c_str(), color_ramp_size);
        color_ramp_size >>= 1;
    }
    return tex_filter_menu;
}

#if USE_CAIRO
static int makeCairoMenu();
#endif

#if USE_SKIA || USE_QT || USE_D2D || USE_CAIRO
static void swConfigWindow();

static void otherRendererMenu(int renderer)
{
    swMode = SWMODE(renderer);
    swConfigWindow();
}
#endif

static int other_renderer_menu = -1;

static void initOtherRenderersMenu()
{
#if USE_SKIA || USE_QT || USE_D2D || USE_CAIRO
    other_renderer_menu = glutCreateMenu(otherRendererMenu);
#if USE_SKIA
    glutAddMenuEntry("Skia", SW_MODE_SKIA);
#endif
#if USE_QT
    glutAddMenuEntry("Qt", SW_MODE_QT);
#endif
#if USE_CAIRO
    glutAddMenuEntry("Cairo", SW_MODE_CAIRO);
#endif
#if USE_D2D
    if (d2d_available) {
        glutAddMenuEntry("Direct2D Hardware (GPU)", SW_MODE_D2D);
        glutAddMenuEntry("Direct2D Warp (CPU)", SW_MODE_D2D_WARP);
    }
#endif
#endif
}

static void updateColorSpace()
{
    glutSetWindow(gl_window);
    bool changed = nvpr_renderer->setColorSpace(color_space);
    if (changed) {
        invalidateScene();
        update_clear_color();
    }
    if (color_space == CORRECTED_SRGB) {
        printf("Using corrected sRGB color space\n");
    } else {
        printf("Using uncorrected color space\n");
    }
}

static void colorSpaceMenu(int cs)
{
    color_space = ColorSpace(cs);
    updateColorSpace();
}

int color_space_menu = -1;
int main_menu = -1;

static void initColorSpaceMenu()
{
    if (nvpr_renderer->sRGB_capable) {
        color_space_menu = glutCreateMenu(colorSpaceMenu);
        glutAddMenuEntry("Corrected sRGB", CORRECTED_SRGB);
        glutAddMenuEntry("Uncorrected", UNCORRECTED);
    }
}

static void initMenu()
{
    if (main_menu >= 0) {
        glutSetMenu(main_menu);
        glutAttachMenu(GLUT_RIGHT_BUTTON);
        return;
    }
    int path_object_menu = glutCreateMenu(pathMenu);
    for (int i=0; i<num_path_objects; i++) {
        glutAddMenuEntry(path_objects[i].name, i);
    }

    int svg_file_menu = initSVGMenus(svgMenu);

    initOtherRenderersMenu();
    initColorSpaceMenu();

#if USE_FREETYPE2
    int font_file_menu = glutCreateMenu(fontMenu);
    const int n = num_fonts();
    for (int i=0; i<n; i++) {
        glutAddMenuEntry(font_name(i), i);
    }
#endif

    int backgroundColorMenu = glutCreateMenu(menu);
    glutAddMenuEntry("[3] Black background", '3');
    glutAddMenuEntry("[4] White background", '4');
    glutAddMenuEntry("[5] Blue (default)", '5');
    glutAddMenuEntry("[6] Gray", '6');

    int samples_per_pixel_menu = -1;
    if (gl_container >= 0) {
        samples_per_pixel_menu = glutCreateMenu(configureSamples);
        glutAddMenuEntry("1 sample/pixel (aliased)", 1);
        glutAddMenuEntry("2 samples/pixel ", 2);
        glutAddMenuEntry("4 samples/pixel ", 4);
        glutAddMenuEntry("8 samples/pixel ", 8);
        glutAddMenuEntry("16 samples/pixel ", 16);
        glutAddMenuEntry("32 samples/pixel ", 32);
    }

    int jitter_config_menu = glutCreateMenu(jitterMenu);
    const int jitter_configs = icountof(jitter_config);
    for (int i=0; i<jitter_configs; i++) {
        glutAddMenuEntry(jitter_config[i].name, i);
    }

    int tex_menu = makeTextureMenu();
#if USE_CAIRO
    int cairo_menu = makeCairoMenu();
#endif

    main_menu = glutCreateMenu(menu);
    glutAddSubMenu("SVG files...", svg_file_menu);
    glutAddSubMenu("Path objects...", path_object_menu);
    if (other_renderer_menu >= 0) {
        glutAddSubMenu("Renderer...", other_renderer_menu);
    }
    if (color_space_menu >= 0) {
        glutAddSubMenu("Color space...", color_space_menu);
    }
#if USE_FREETYPE2
    glutAddSubMenu("Scalable fonts...", font_file_menu);
#endif
    if (gl_container >= 0) {
        glutAddSubMenu("Antialiasing...", samples_per_pixel_menu);
    }
    glutAddSubMenu("Background color...", backgroundColorMenu);
    glutAddSubMenu("Jitter configurations...", jitter_config_menu);
    glutAddSubMenu("Texture filtering...", tex_menu);
#if USE_CAIRO
    glutAddSubMenu("Cairo...", cairo_menu);
#endif
    glutAddMenuEntry("[`] Invalidate scene", '`');
    glutAddMenuEntry("[~] Reload shaders", '~');
    glutAddMenuEntry("[1] Reload scene", '1');
    glutAddMenuEntry("[!] Reload initial state", '!');
    glutAddMenuEntry("[c] Toggle control points", 'c');
    glutAddMenuEntry("[C] Toggle warp points", 'C');
    glutAddMenuEntry("[l] Toggle clip path scissor box", 'l');
    glutAddMenuEntry("[7] Cycle through 1, 4, 9, and 16 passes", '7');
    glutAddMenuEntry("[8] Toggle skipping glutSwapBuffers", '8');
    glutAddMenuEntry("[9] Toggle reverse Painter's algorithm", '9');
    glutAddMenuEntry("[0] Toggle using display lists", '0');
    glutAddMenuEntry("[(] Toggle convex hull & bouding box for fill covering", '(');
    glutAddMenuEntry("[)] Toggle convex hull & bouding box for stroke covering", ')');
    glutAddMenuEntry("[g] Forward font", 'g');
    glutAddMenuEntry("[G] Backward font", 'g');
    glutAddMenuEntry("[f] Forward SVG", 'f');
    glutAddMenuEntry("[f] Backward SVG", 'F');
    glutAddMenuEntry("[w] Forward path object", 'w');
    glutAddMenuEntry("[W] Backward path object", 'W');
    glutAddMenuEntry("[;] Increase accumulation passes", ';');
    glutAddMenuEntry("[:] Decrease accumulation passes", ':');
    glutAddMenuEntry("[h] Increase accumulation spread", 'h');
    glutAddMenuEntry("[H] Decrease accumulation spread", 'H');
    glutAddMenuEntry("[P] Toggle showing accumulation sample pattern", 'P');
    glutAddMenuEntry("[i] Dump scene statistics", 'i');
    glutAddMenuEntry("[Space] Benchmark", ' ');
    glutAddMenuEntry("[n] Toggle software-rendered window", 'n');
    glutAddMenuEntry("[j] Toggle gold image window", 'j');
    glutAddMenuEntry("[J] Toggle difference image window", 'J');
    glutAddMenuEntry("[N] Run software rendering benchmarks", 'N');
    glutAddMenuEntry("[R] Toggle Cairo/Qt software rendering", 'R');
    glutAddMenuEntry("[=] Toggle spinning & scaling benchmark animation", '=');
    glutAddMenuEntry("[v] Toggle buffer swap synchornization", 'v');
    glutAddMenuEntry("[Ctrl F] Flush renderer states", 6);
    glutAddMenuEntry("[Ctrl G] Run regressions", 7);
    glutAddMenuEntry("[Ctrl H] Seed regressions", 8);
    glutAddMenuEntry("[Ctrl I] Invalidate renderer states", 9);
    glutAddMenuEntry("[Home] Reset view to initial view", -GLUT_KEY_HOME);
    glutAddMenuEntry("[Insert] Toggle sRGB color correction", -GLUT_KEY_INSERT);
    glutAddMenuEntry("[Esc] Quit", 27);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

#if 0 // useful for debugging
static void visualizeStencil()
{
    static const GLubyte bit_color[8][3] = {
        { 0,0,128 },  // 0 bit = blue
        { 0,32,0 },   // 1 bit = dark green
        { 0,64,0 },   // 2 bit = medium green
        { 0,128,0 },  // 3 bit = bright green
        { 0,0,64 },   // 4 bit = dim blue
        { 128,0,0 },  // 5 bit = bright red
        { 64,0,0 },   // 6 bit = medium red
        { 32,0,0 }    // 7 bit = dark red
    };

    glMatrixPushEXT(GL_MODELVIEW);
    glMatrixLoadIdentityEXT(GL_MODELVIEW);
    glStencilMask(0);
    glColorMask(1,1,1,1);
    glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
    glDisable(GL_STENCIL_TEST);
    glColor3f(0,0,0);
    glRectf(-1,-1,1,1);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE,GL_ONE);
    for (int i=0; i<8; i++) {
        glStencilFunc(GL_EQUAL, 1<<i, 1<<i);
        glColor3ubv(bit_color[i]);
        glRectf(-1,-1,1,1);
    }
    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glMatrixPopEXT(GL_MODELVIEW);
}
#endif

static void clear(bool clearDepth = false)
{
    GLbitfield clearBuffers = GL_COLOR_BUFFER_BIT;

    glColorMask(1,1,1,1);
    if (clearDepth) {
        glDepthMask(1);
        clearBuffers |= GL_DEPTH_BUFFER_BIT;
    }
    if (force_stencil_clear ||
        render_top_to_bottom ||
        !only_necessary_stencil_clears ||
        glutLayerGet(GLUT_NORMAL_DAMAGED)) {

        glStencilMask(~0);
        clearBuffers |= GL_STENCIL_BUFFER_BIT;
        force_stencil_clear = false;
    }
    glClear(clearBuffers);
}

static void drawScene()
{
    if (render_top_to_bottom) {
        glDisable(GL_BLEND);
        ReverseTraversal traversal;
        scene->traverse(
            VisitorPtr(new StCVisitors::ReversePainter(nvpr_renderer, 0x80, view_to_surface)),
            traversal);
        if (non_opaque_objects>0 && any(clear_color != float4(0))) {
            // We need to "under" blend the actual background color into the scene
            // since top-to-bottom order (Reverse Painter's algorithm) requires the
            // color buffer to be cleared to (0,0,0,0) for the "under" pre-multiplied alpha
            // compositing operator to work correctly.
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
            // Only update pixels not finalized.
            glStencilFunc(GL_EQUAL, 0, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glMatrixPushEXT(GL_MODELVIEW); {
                glMatrixLoadIdentityEXT(GL_MODELVIEW);
                glMatrixPushEXT(GL_PROJECTION); {
                    glMatrixLoadIdentityEXT(GL_PROJECTION);
                    glBegin(GL_TRIANGLE_FAN); {
                        glColor4fv(reinterpret_cast<GLfloat*>(&clear_color));
                        glVertex3f(-1,-1,1);
                        glVertex3f(+1,-1,1);
                        glVertex3f(+1,+1,1);
                        glVertex3f(-1,+1,1);
                    } glEnd();
                } glMatrixPopEXT(GL_PROJECTION);
            } glMatrixPopEXT(GL_MODELVIEW);
        }
        glDisable(GL_BLEND);
    } else {
        if (xsteps > 1 || ysteps > 1) {
            glMatrixPushEXT(GL_MODELVIEW); {
                scene->traverse(VisitorPtr(new StCVisitors::DrawDilated(
                    nvpr_renderer, 0x80, view_to_surface, xsteps, ysteps, 
                    stipple, spread)));
            } glMatrixPopEXT(GL_MODELVIEW);
        } else {
            float4x4 m = makingDlist ? identity4x4() : view_to_surface;
            scene->traverse(VisitorPtr(new StCVisitors::Draw(nvpr_renderer, 0x80, m)));
        }
    }
}

static void renderScene()
{
    if (use_dlist) {
        if (!have_dlist) {
            glNewList(1,GL_COMPILE);
            makingDlist = true;  // so we don't do glGet's
            drawScene();
            makingDlist = false;
            glEndList();
            have_dlist = true;
        }
        glCallList(1);
    } else {
        drawScene();
    }

    glDisable(GL_STENCIL_TEST);
}

static void renderOverlays()
{
    if (show_clip_scissor) {
        scene->traverse(VisitorPtr(new StCVisitors::VisualizeClipScissor(nvpr_renderer, 0x80, view_to_surface)));
    }

    if (show_reference_points) {
        nvpr_renderer->disableFragmentShading();
        ForEachShapeTraversal traversal;
        scene->traverse(VisitorPtr(
            new StCVisitors::DrawReferencePoints(nvpr_renderer, view_to_surface)),
            traversal);
    }

    if (show_control_points) {
        nvpr_renderer->disableFragmentShading();
        ForEachShapeTraversal traversal;
        scene->traverse(VisitorPtr(
            new StCVisitors::DrawControlPoints(nvpr_renderer, view_to_surface)),
            traversal);
    }
    if (show_warp_points) {
        ForEachTransformTraversal transform_traversal;
        scene->traverse(VisitorPtr(
            new StCVisitors::DrawWarpPoints(nvpr_renderer, view_to_surface)),
            transform_traversal);
    }
}

vector<float2> sampleOffsetList;

static void showSampleLocations()
{
    float2 pixelGap = 1.0f/float2(gl_window_width, gl_window_height);

    glMatrixPushEXT(GL_PROJECTION); {
        glMatrixLoadIdentityEXT(GL_PROJECTION);
        glMatrixOrthoEXT(GL_PROJECTION,
            -1.5*pixelGap.x, 1.5*pixelGap.x,
            -1.5*pixelGap.y, 1.5*pixelGap.y,
            -1, 1);
        glMatrixPushEXT(GL_MODELVIEW); {
            glMatrixLoadIdentityEXT(GL_MODELVIEW);

            glColor3f(0,0,1);
            glBegin(GL_LINES); {
                glVertex2f(0.5*pixelGap.x, 1.5*pixelGap.y);
                glVertex2f(0.5*pixelGap.x, -1.5*pixelGap.y);
                glVertex2f(-0.5*pixelGap.x, 1.5*pixelGap.y);
                glVertex2f(-0.5*pixelGap.x, -1.5*pixelGap.y);
                glVertex2f(-1.5*pixelGap.x, 0.5*pixelGap.y);
                glVertex2f(1.5*pixelGap.x,  0.5*pixelGap.y);
                glVertex2f(-1.5*pixelGap.x, -0.5*pixelGap.y);
                glVertex2f(1.5*pixelGap.x,  -0.5*pixelGap.y);
            } glEnd();
            glPointSize(7);
            if (all(clear_color.rgb == 1)) {
                glColor3f(0,0.6,0.6);
            } else {
                glColor3f(1,1,0);
            }
            assert(nvpr_renderer->sample_positions.size() == size_t(nvpr_renderer->num_samples));
            for (int j=0; j<nvpr_renderer->num_samples; j++) {
                float2 centeredSampleLocation = nvpr_renderer->sample_positions[j] - 0.5f;
                centeredSampleLocation *= pixelGap;
                glBegin(GL_POINTS); {
                    for (size_t i=0; i<sampleOffsetList.size(); i++) {
                        float2 loc = sampleOffsetList[i] + centeredSampleLocation;
                        glVertex2f(loc.x, loc.y);
                    }
                } glEnd();
            }
        } glMatrixPopEXT(GL_MODELVIEW);
    } glMatrixPopEXT(GL_PROJECTION);
}

inline double closed_interval_rand(double2 x)
{
    return x.x + (x.y - x.x) * rand() / ((double) RAND_MAX);
}

float myrandom(float v)
{
    return closed_interval_rand(double2(0,v));
}

static void renderScenePasses()
{
    if (accumulationPasses > 0) {
        static bool beenhere = false;
        if (nvpr_renderer->num_samples > 1 && !beenhere) {
            fprintf(stderr, "WARNING: accumulation mode for multisampled renders has samples that spread beyond the pixel square; use 'P' to visualize the sample pattern\n");
            beenhere = true;
        }

        srand(32323);
        sampleOffsetList.clear();
        int sq = int(sqrt(float(accumulationPasses)));
        int sq2 = sq*sq;
        GLfloat accumWeight = 1.0f/accumulationPasses;
        float2 pixelGap = 1.0f/float2(gl_window_width, gl_window_height);
        float2 sampleSpace = pixelGap/sq;
        float spanWidth = min(max((accumulationPasses-2)*0.025,0),0.5);
        double2 span = double2(0.5-spanWidth, 0.5+spanWidth);

        for (int i=0; i<accumulationPasses; i++) {
            float2 sampleOffset;
            if (i < sq2) {
                float2 xy = int2(i % sq, i / sq) + float2(closed_interval_rand(span),
                                                          closed_interval_rand(span));

                sampleOffset = xy*sampleSpace;
                sampleOffset -= pixelGap/2;
                if (verbose) {
                    std::cout << "regular sample(" << i << ") = " << sampleOffset*float2(gl_window_width, gl_window_height) << std::endl;
                }
            } else {
                float2 xy = float2(myrandom(sq), myrandom(sq));
                sampleOffset = xy*sampleSpace;
                sampleOffset -= pixelGap/2;
                if (verbose) {
                    std::cout << "irregular sample(" << i << ") = " << sampleOffset*float2(gl_window_width, gl_window_height) << std::endl;
                }
            }
            sampleOffset *= accumulationSpread;

            clear();
            sampleOffsetList.push_back(sampleOffset);
            glMatrixPushEXT(GL_PROJECTION); {
                glMatrixTranslatefEXT(GL_PROJECTION, sampleOffset.x, sampleOffset.y, 0);
                renderScene();
            } glMatrixPopEXT(GL_PROJECTION);
            if (i==0) {
                glAccum(GL_LOAD, accumWeight);
            } else {
                glAccum(GL_ACCUM, accumWeight);
            }
        }
        glAccum(GL_RETURN, 1.0);
        if (showSamplePattern) {  // 'P' toggles
            showSampleLocations();
        }
    } else {
        clear();
        renderScene();
    }
}

double maxFPS = -1;

void exitIfDone()
{
    if (verbose) {
        printf("frames_to_render_before_exit = %d\n", frames_to_render_before_exit);
    }
    if (frames_to_render_before_exit > 0) {
        frames_to_render_before_exit--;
        if (frames_to_render_before_exit == 0) {
            printf("exiting because frames to render count expired\n");
            exit(0);
        }
    }
}

bool makeImageFilenameString(string &result, bool gold);
void possiblyDisableSceneIteration();

// Draw the scene with all its "meta scene" information like overlays and accumulated scenes.
static void drawMetaScene()
{
    if (occlusion_query_mode) {
        glBeginQuery(GL_SAMPLES_PASSED, 1); {
            renderScenePasses();
        } glEndQuery(GL_SAMPLES_PASSED);
        GLint result = 0;
        glGetQueryObjectiv(1, GL_QUERY_RESULT, &result);
        if (occlusion_query_mode == 1) {
            printf("result = %d\n", result);
        }
    } else {
        renderScenePasses();
    }
    if (xsteps*ysteps > 1) {
        glDisable(GL_BLEND);
    }
    if (diff_window >= 0 || request_seed) {
        if (animation_mask & GL_WINDOW) {
            // don't collect last_render image while animating to avoid slowing benchmark
        } else {
            string result;
            bool needs_seed_write = false;
            if (request_seed) {
                makeImageFilenameString(result, false);
                FILE *file = fopen(result.c_str(), "r");
                if (file) {
                    fclose(file);
                } else {
                    needs_seed_write = true;
                }
            }
            if (diff_window >= 0 || needs_seed_write) {
                last_render = RenderResultPtr(new RenderResultFromGLUTWindow(glutGetWindow()));
            }
            if (needs_seed_write) {
                assert(request_seed);
                if (last_render) {
                    printf("Seeding %s...", result.c_str());
                    fflush(stdout);
                    last_render->writeImageToFile(result.c_str());
                    printf("done.\n");
                    seeds_created++;
                }
            }
            if (diff_window >= 0) {
                glutPostWindowRedisplay(diff_window);
            }
        }
    }
    renderOverlays();
}

static void display()
{
    if (just_clear_and_swap) {
        clear();
        if (animation_mask & GL_WINDOW) {
            double thisFPS = handleFPS(&gl_fps_context);
            thisFPS = thisFPS;  // force used
        }
        glutSwapBuffers();
    } else {
        static int lastTime = 0;
        double thisFPS = 0;
        glMatrixPushEXT(GL_MODELVIEW); {
            drawMetaScene();
        } glMatrixPopEXT(GL_MODELVIEW);
        if (animation_mask & GL_WINDOW) {
            thisFPS = handleFPS(&gl_fps_context);
            maxFPS = max(maxFPS, thisFPS);
        }
        if (skip_swap) {
            glFinish();
            if (animation_mask & GL_WINDOW) {
                int thisTime = glutGet(GLUT_ELAPSED_TIME);
                if (abs(lastTime - thisTime) > 2000) {
                    printf("thisFPS = %f\n", thisFPS);
                    lastTime = thisTime;
                }
            }
        } else {
            glutSwapBuffers();
        }
    }
    exitIfDone();
}

void benchmarkReport(int v)
{
    glutSetWindow(gl_window);
    if (reshape_count != 1) {
        printf("WARNING: reshape_count expect to be 1 but is %d\n", reshape_count);
    }
    printf("window size: %dx%d pixels, %d samples/pixel\n",
        gl_window_width, gl_window_height, nvpr_renderer->num_samples);
    printf("maximum FPS in first six seconds: %.1f\n", maxFPS);
    if (request_software_window) {
        float drawing_fps, raw_fps;
        benchmarkSoftwareRendering(drawing_fps, raw_fps);
        printf("NV_path speed-up compared to raw Cairo = %.2fx\n", maxFPS/raw_fps);
        printf("NV_path speed-up compared to Cairo with glDrawPixels = %.2fx\n", maxFPS/drawing_fps);
    }
    if (wait_to_exit) {
        printf("Hit return to exit nvpr_svg...\n");
        getchar();
    }
    exit(0);
}

void startSceneRegressions();
void startSceneSeeding();

static void reshape(int w, int h)
{
    reshapeFPScontext(&gl_fps_context, w, h);
    force_stencil_clear = true;
    window_x = glutGet(GLUT_WINDOW_X);
    window_y = glutGet(GLUT_WINDOW_Y);
    if (gl_container == glutGetWindow()) {
        nvpr_renderer->setViewport(0, 0, w, h);
        glutSetWindow(gl_window);
        glutReshapeWindow(w, h);
    }
    nvpr_renderer->setViewport(0, 0, w, h);
    gl_window_width = w;
    gl_window_height = h;
    wh = float2(gl_window_width, gl_window_height);
    reshape_count++;

    // Defer making the SW window until we know where the GPU window is
    if (request_software_window) {
        makeSoftwareWindow();
        request_software_window = false;
    }
    if (request_gold_window) {
        makeGoldWindow();
        request_gold_window = false;
    }
    if (request_diff_window) {
        makeDiffWindow();
        request_diff_window = false;
    }
    if (request_regress) {
        startSceneRegressions();
    }
    if (request_seed) {
        startSceneSeeding();
    }

    // need to regenerate reset scissor
    // XXX should figure out another way to reset scissors
    have_dlist = false;
}

static void updateTransform()
{
    // Update GL window's basic transform
    glutSetWindow(gl_window);

    view_to_surface = view;  // should update view_to_surface directly!
    glMatrixLoadTransposefEXT(GL_MODELVIEW, &view_to_surface[0][0]);

    // Force cairo redraw
    software_window_valid = false;

    // Multi-pass mode will computed different dilated convex hulls.
    if (xsteps*ysteps > 1) {
        have_dlist = false;
    }
}

// Rotate scene around its center by "angle" degrees.
static void rotateScene(float angle)
{
    float a = angle * M_PI / 180.0;  // 1 degree clockwise in radians
    float c = cos(a),
          s = sin(a);
    float4x4 r = float4x4(c,-s, 0, 0,
                          s, c, 0, 0,
                          0, 0, 1, 0,
                          0, 0, 0, 1);
    view = mul(view, r);
    updateTransform();
}

static void idle()
{
    if (angleRotateRate) {
        ticks++;
        int t = (ticks % 181) - 90;
        double scale = pow(1.0003, t);
        double4x4 s = double4x4(scale,0,0,0,
                                0,scale,0,0,
                                0,0,1,0,
                                0,0,0,1);
        view = mul(view, s);
        rotateScene(angleRotateRate);
    } else {
        // Force SW redraw
        software_window_valid = false;
    }
    do_redisplay(animation_mask);
}

#if USE_SKIA
static SkiaRendererPtr skia_renderer;
#endif

#if USE_CAIRO
static CairoRendererPtr cairo_renderer;

enum {
    CAIRO_MODE_ANTIALIAS_DEFAULT,
    CAIRO_MODE_ANTIALIAS_NONE,
    CAIRO_MODE_ANTIALIAS_GRAY,
    CAIRO_MODE_ANTIALIAS_SUBPIXEL,

    CAIRO_MODE_FILTER_FAST,
    CAIRO_MODE_FILTER_GOOD,
    CAIRO_MODE_FILTER_BEST,
    CAIRO_MODE_FILTER_NEAREST,
    CAIRO_MODE_FILTER_BILINEAR,
};

static void cairoMenu(int item)
{
    bool invalidate_patterns = false;

    switch (item) {
    case CAIRO_MODE_ANTIALIAS_DEFAULT:
        cairo_renderer->setAntialias(CAIRO_ANTIALIAS_DEFAULT);
        printf("Setting Cairo antialias DEFAULT\n");
        break;
    case CAIRO_MODE_ANTIALIAS_NONE:
        cairo_renderer->setAntialias(CAIRO_ANTIALIAS_NONE);
        printf("Setting Cairo antialias NONE\n");
        break;
    case CAIRO_MODE_ANTIALIAS_GRAY:
        cairo_renderer->setAntialias(CAIRO_ANTIALIAS_GRAY);
        printf("Setting Cairo antialias GRAY\n");
        break;
    case CAIRO_MODE_ANTIALIAS_SUBPIXEL:
        cairo_renderer->setAntialias(CAIRO_ANTIALIAS_SUBPIXEL);
        printf("Setting Cairo antialias SUBPIXEL\n");
        break;

    case CAIRO_MODE_FILTER_FAST:
        cairo_renderer->setFilter(CAIRO_FILTER_FAST);
        printf("Setting Cairo filter FAST\n");
        invalidate_patterns = true;
        break;
    case CAIRO_MODE_FILTER_GOOD:
        cairo_renderer->setFilter(CAIRO_FILTER_GOOD);
        printf("Setting Cairo filter GOOD\n");
        invalidate_patterns = true;
        break;
    case CAIRO_MODE_FILTER_BEST:
        cairo_renderer->setFilter(CAIRO_FILTER_BEST);
        printf("Setting Cairo filter BEST\n");
        invalidate_patterns = true;
        break;
    case CAIRO_MODE_FILTER_NEAREST:
        cairo_renderer->setFilter(CAIRO_FILTER_NEAREST);
        printf("Setting Cairo filter NEAREST\n");
        invalidate_patterns = true;
        break;
    case CAIRO_MODE_FILTER_BILINEAR:
        cairo_renderer->setFilter(CAIRO_FILTER_BILINEAR);
        printf("Setting Cairo filter BILINEAR\n");
        invalidate_patterns = true;
        break;
    }

    software_window_valid = false;
    if (invalidate_patterns) {
        ForEachShapeTraversal traversal;
        scene->traverse(VisitorPtr(new InvalidatePaintRendererState(cairo_renderer)), traversal);
    }
    if (sw_window >= 0) {
        glutPostWindowRedisplay(sw_window);
    }
}

static int makeCairoMenu()
{
    int cairo_menu = glutCreateMenu(cairoMenu);
    glutAddMenuEntry("Antialias DEFAULT", CAIRO_MODE_ANTIALIAS_DEFAULT);
    glutAddMenuEntry("Antialias NONE", CAIRO_MODE_ANTIALIAS_NONE);
    glutAddMenuEntry("Antialias GRAY", CAIRO_MODE_ANTIALIAS_GRAY);
    glutAddMenuEntry("Antialias SUBPIXEL", CAIRO_MODE_ANTIALIAS_SUBPIXEL);

    glutAddMenuEntry("Filter FAST", CAIRO_MODE_FILTER_FAST);
    glutAddMenuEntry("Filter GOOD", CAIRO_MODE_FILTER_GOOD);
    glutAddMenuEntry("Filter BEST", CAIRO_MODE_FILTER_BEST);
    glutAddMenuEntry("Filter NEAREST", CAIRO_MODE_FILTER_NEAREST);
    glutAddMenuEntry("Filter BILINEAR", CAIRO_MODE_FILTER_BILINEAR);
    return cairo_menu;
}
#endif

#if USE_QT
static QtRendererPtr qt_renderer;
#endif

#if USE_OPENVG
static VGRendererPtr vg_renderer;
#endif

#if USE_D2D
static D2DRendererPtr d2d_renderer;
static D2DRendererPtr d2dWARP_renderer;
#endif

void swRender(BlitRendererPtr renderer)
{
    renderer->beginDraw();
    renderer->clear(clear_color.rgb);
    renderer->setView(view);
    scene->traverse(renderer->makeVisitor());
    renderer->endDraw();

    software_window_valid = true;
}

void swPresent(BlitRendererPtr blit_renderer)
{
    blit_renderer->copyImageToWindow();
}

static void swDisplay()
{
    if (!software_window_valid) {
        swRender(blit_renderer);
    }

    swPresent(blit_renderer);
    if (diff_window >= 0) {
        if (animation_mask & SW_WINDOW) {
            // don't collect last_render image while animating to avoid slowing benchmark
        } else {
            last_software = RenderResultPtr(new RenderResultFromGLUTWindow(glutGetWindow()));
            glutPostWindowRedisplay(diff_window);
        }
    }
    if (show_control_points || show_warp_points) {
        const float4x4 surface_to_clip = surfaceToClip(sw_window_width,
                                                       sw_window_height,
                                                       scene_ratio);

        glMatrixLoadTransposefEXT(GL_PROJECTION, &surface_to_clip[0][0]);
        glMatrixPushEXT(GL_MODELVIEW); {
            glMatrixMultTransposefEXT(GL_MODELVIEW, &view_to_surface[0][0]);
            if (show_control_points) {
                ForEachShapeTraversal traversal;
                scene->traverse(VisitorPtr(
                    new StCVisitors::DrawControlPoints(nvpr_renderer, view_to_surface)),
                    traversal);
            }

            if (show_warp_points) {
                ForEachTransformTraversal transform_traversal;
                scene->traverse(VisitorPtr(
                    new StCVisitors::DrawWarpPoints(nvpr_renderer, view_to_surface)),
                    transform_traversal);
            }
        } glMatrixPopEXT(GL_MODELVIEW);
    }
    if (animation_mask & SW_WINDOW) {
        blit_renderer->reportFPS();
    }

    blit_renderer->swapBuffers();
}

void swShutdown(BlitRendererPtr blit_renderer)
{
    if (blit_renderer) {
        ForEachShapeTraversal traversal;
        scene->traverse(VisitorPtr(new FlushRendererState(blit_renderer)), traversal);
        blit_renderer->shutdown();
    }
    last_software = RenderResultPtr();
    software_window_valid = false;
}

static const char *sw_windowTitle(BlitRendererPtr blit_renderer)
{
    if (blit_renderer) {
        return blit_renderer->getWindowTitle();
    } else {
        return "unknown path rendering";
    }
}

void swInit(int width, int height, BlitRendererPtr blit_renderer)
{
    swShutdown(blit_renderer);

    if (blit_renderer) {
        const char *title = blit_renderer->getWindowTitle();
        glutSetWindowTitle(title);
        blit_renderer->configureSurface(width, height);
    }
}

void swReshape(int width, int height)
{
    sw_window_width = width;
    sw_window_height = height;
    glViewport(0, 0, width, height);
    swInit(width, height, blit_renderer);
}

static void initBlitRenderer(SWMODE mode)
{
    switch (mode) {
#if USE_SKIA
    case SW_MODE_SKIA:
        {
            if (!skia_renderer) {
                skia_renderer = SkiaRendererPtr(new SkiaRenderer);
            }
            blit_renderer = skia_renderer;
#if 0  // revisit
            skia_renderer->setAntialias(cairoAntialiasMode);
#endif
        }
        break;
#endif
#if USE_CAIRO
    case SW_MODE_CAIRO:
        {
            if (!cairo_renderer) {
                cairo_renderer = CairoRendererPtr(new CairoRenderer);
            }
            blit_renderer = cairo_renderer;
            cairo_renderer->setAntialias(cairoAntialiasMode);
        }
        break;
#endif
#if USE_QT
    case SW_MODE_QT:
        {
            if (!qt_renderer) {
                qt_renderer = QtRendererPtr(new QtRenderer);
            }
            blit_renderer = qt_renderer;
        }
        break;
#endif
#if USE_OPENVG
    case SW_MODE_OPENVG:
        {
            if (!vg_renderer) {
                vg_renderer = VGRendererPtr(new VGRenderer);
            }
            blit_renderer = vg_renderer;
            ForEachShapeTraversal traversal;
            scene->traverse(VisitorPtr(new InvalidateRendererState(vg_renderer)), traversal);
        }
        break;
#endif
#if USE_D2D
    case SW_MODE_D2D:
        {
            // From MSDN:
            // "DXGI surface render targets don't support the ID2D1RenderTarget::Resize method.
            // To resize a DXGI surface render target, the application must release and recreate it."
            if (!d2d_renderer && d2d_available) {
                d2d_renderer = D2DRendererPtr(new D2DRenderer(false));
                d2d_renderer->recreateSurface(false);
            }
            blit_renderer = d2d_renderer;
            //d2d_renderer->setAntialias(cairoAntialiasMode);
        }
        break;
    case SW_MODE_D2D_WARP:
        {
            if (!d2dWARP_renderer && d2d_available) {
                d2dWARP_renderer = D2DRendererPtr(new D2DRenderer(true));
                d2dWARP_renderer->recreateSurface(true);
            }
            blit_renderer = d2dWARP_renderer;
            //d2dWARP_renderer->setAntialias(cairoAntialiasMode);
        }
        break;
#endif
    default:
        assert(!"bogus SW mode");
    }
}

static void makeSoftwareWindow()
{
    initBlitRenderer(swMode);
    if (sw_window < 0) { 
#ifdef linux
        // not sure why glutg3 crashes when called with NULL pointer
        glutInitDisplayString("rgb double");
#else
        glutInitDisplayString(NULL);
#endif
        glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
        glutInitWindowPosition(window_x + gl_window_width + 20, window_y);
        sw_window_width = gl_window_width;
        sw_window_height = gl_window_height;
        glutInitWindowSize(sw_window_width, sw_window_height);
        sw_window = glutCreateWindow(sw_windowTitle(blit_renderer));
        software_window_valid = false;
        glutKeyboardFunc(keyboard);
        glutMouseFunc(mouse);
        glutMotionFunc(motion);
        glutSpecialFunc(special);
        glutDisplayFunc(swDisplay);
        glutReshapeFunc(swReshape);
        requestSynchornizedSwapBuffers(enable_sync);
        glLineStipple(1, 0xF0F0);

        // Add pop-up menu to select renderer
        glutSetMenu(other_renderer_menu);
        glutAttachMenu(GLUT_RIGHT_BUTTON);
    } else {
        glutSetWindow(sw_window);
    }
}

static void unmakeSoftwareWindow()
{
    if (sw_window >= 0) { 
        disableAnimation(SW_WINDOW);
        glutDestroyWindow(sw_window);
        swShutdown(blit_renderer);
        sw_window = -1;
    }
}

static void swDisplay();

static int start_testing_time, end_testing_time;

struct BenchmarkConfig {
    RendererPtr renderer;
    SWMODE mode;
    int window;  // GLUT window ID
    float fps;

    BenchmarkConfig(RendererPtr renderer_,
                    SWMODE mode_,
                    int window_)
        : renderer(renderer_)
        , mode(mode_)
        , window(window_)
        , fps(0)
    {}
};

vector<BenchmarkConfig> config;

static void initBenchmarkConfigArray()
{
    if (config.size() > 0) {
        // Assume already initialized
        return;
    }

    // Make sure the GL renderer is first since it gets compared with
    config.push_back(BenchmarkConfig(nvpr_renderer, SW_MODE_GL, gl_window));
    if (allowSoftwareWindow) {
#if USE_CAIRO
        initBlitRenderer(SW_MODE_CAIRO);
        config.push_back(BenchmarkConfig(cairo_renderer, SW_MODE_CAIRO, sw_window));
#endif
#if USE_QT
        initBlitRenderer(SW_MODE_QT);
        config.push_back(BenchmarkConfig(qt_renderer, SW_MODE_QT, sw_window));
#endif
#if USE_SKIA
        initBlitRenderer(SW_MODE_SKIA);
        config.push_back(BenchmarkConfig(skia_renderer, SW_MODE_SKIA, sw_window));
#endif
#if USE_OPENVG && 0 // too slow to benchmark
        initBlitRenderer(SW_MODE_OPENVG);
        config.push_back(BenchmarkConfig(vg_renderer, SW_MODE_OPENVG, sw_window));
#endif
#if USE_D2D
        initBlitRenderer(SW_MODE_D2D);
        config.push_back(BenchmarkConfig(d2d_renderer, SW_MODE_D2D, sw_window));
        // XXXmjk Warp seems to be the same speed as the Hardware renderer, not sure why
        initBlitRenderer(SW_MODE_D2D_WARP);
        config.push_back(BenchmarkConfig(d2dWARP_renderer, SW_MODE_D2D_WARP, sw_window));
#endif
    }
}

static void benchmarkIdle()
{
    // SVG files to benchmark
    static const char* benchmarkFiles[] = {
        "svg/complex/tiger.svg",  // complex, 239 paths, WITH stroking
        "svg/complex/Welsh_dragon.svg",  // 150 paths, no stroking
        "svg/complex/Celtic_round_dogs.svg",  // complex, but just 1 path, no sroking
        "svg/basic/butterfly.svg",  // just 3 paths, no stroking
        "svg/simple/spikes.svg",  // all line segments, no stroking
        "svg/complex/Coat_of_Arms_of_American_Samoa.svg", // 953 paths, some stroking
        "svg/complex/cowboy.svg",  // complex, 1366 paths, no stroking
        "svg/complex/Buonaparte.svg",  // complex, 151 paths, WITH stroking
        "svg/ravg/jonata_Embrace_the_World.svg",  // complex, 225 paths, WITH gradients no stroking
        "svg/basic/Yokozawa_Hiragana.svg", // all wide strokes, 104 paths
        "svg/complex/Cougar_.svg", // all wide strokes, 104 paths
        "svg/complex/tiger_clipped_by_heart.svg", // tiger clipped by heart
    };
    static const int numBenchmarkFiles = sizeof(benchmarkFiles)/sizeof(benchmarkFiles[0]);

    // Screen resolutions to benchmark
    static const int benchmarkRes[] = {
        100, 100,
        200, 200,
        300, 300,
        400, 400,
        500, 500,
        600, 600,
        700, 700,
        800, 800,
        900, 900,
        1000, 1000,
        1100, 1100
    };
    static const int numBenchmarkRes = sizeof(benchmarkRes)/(sizeof(benchmarkRes[0])*2);

    // While more benchmark files and resolutions
    if ((extended_benchmark_res < numBenchmarkRes) && (extended_benchmark_file < numBenchmarkFiles)) {

        int width  = benchmarkRes[extended_benchmark_res*2];
        int height = benchmarkRes[extended_benchmark_res*2+1];

        // check if window has been resized to requested resolution
        bool got_reshape = (width == gl_window_width) && (height == gl_window_height);
        if (allowSoftwareWindow) {
            bool got_sw_reshape = (width == sw_window_width) && (height == sw_window_height);
            got_reshape = got_reshape && got_sw_reshape;
        }
        if (got_reshape) {

            static bool load_svg = true;
            if (load_svg) {
                
                setSVGScene(benchmarkFiles[extended_benchmark_file]);
                have_dlist = false;
                software_window_valid = false;
                load_svg = false;
            }

            for (size_t i=0; i<config.size(); i++) {
                if (config[i].renderer) {
                    ForEachShapeTraversal traversal;
                    scene->traverse(VisitorPtr(new FlushRendererStates), traversal);

#if USE_D2D || USE_CAIRO || USE_QT || USE_OPENVG || USE_SKIA
                    BlitRendererPtr blit_renderer = dynamic_pointer_cast<BlitRenderer>(config[i].renderer);
                    if (blit_renderer) {
                        BlitRendererPtr blit_renderer = dynamic_pointer_cast<BlitRenderer>(config[i].renderer);

                        swMode = SWMODE(config[i].mode);
                        glFinish();
                        benchmarkSoftwareRendering(config[i].fps, blit_renderer);
                    } else
#endif
                    {
                        benchmarkGLRendering(config[i].fps, false);
                    }
                }
            }

            const char *s = benchmarkFiles[extended_benchmark_file];
            const char *last_slash = strrchr(s, '/');
            if (last_slash) {
                s = last_slash+1;
            }
            const char *space = &"  "[int(width>999) + int(height>999)];

            printf("%-30s, %s%dx%d ", s, space, width, height);
            for (size_t i=0; i<config.size(); i++) {
                if (config[i].renderer) {
                    printf(", %9.2f", config[i].fps);
                }
            }
            for (size_t i=1; i<config.size(); i++) {
                if (config[i].renderer) {
                    printf(", %9.2f", config[0].fps / config[i].fps);
                }
            }
            printf("\n");
            fflush(stdout);

            extended_benchmark_res++;
            if (extended_benchmark_res == numBenchmarkRes) {

                extended_benchmark_res = 0;
                extended_benchmark_file++;
                load_svg = true;
            }
        } else {
            // request window resize
            if (gl_container >= 0) {
                glutSetWindow(gl_container);
            } else {
                glutSetWindow(gl_window);
            }
            int x = glutGet(GLUT_WINDOW_X),
                y = glutGet(GLUT_WINDOW_Y);
            if (allowSoftwareWindow) {
                glutSetWindow(sw_window);
                glutPositionWindow(x + width + 20, y);
                glutReshapeWindow(width, height);
            }
            if (gl_container >= 0) {
                glutSetWindow(gl_container);
                glutReshapeWindow(width, height);
            }
            glutSetWindow(gl_window);
            glutReshapeWindow(width, height);

            glutPopWindow();

            for (size_t i=0; i<config.size(); i++) {
                BlitRendererPtr blit_renderer = dynamic_pointer_cast<BlitRenderer>(config[i].renderer);

                if (blit_renderer) {
                    glutSetWindow(sw_window);
                    swInit(width, height, blit_renderer);
                } else {
                    // GL renderer
                }
            }

            return;
        }
    } else {
        end_testing_time = glutGet(GLUT_ELAPSED_TIME);

        float total_testing_time = (end_testing_time - start_testing_time)/1000.0;
        printf("Testing time = %f minutes (%f seconds)\n",
            total_testing_time/60, total_testing_time);
        fflush(stdout);

        // done benchmarking, restore state
        glutIdleFunc(NULL);
        if (allowSoftwareWindow) {
            glutSetWindow(sw_window);
            glutDisplayFunc(swDisplay);
            glutReshapeWindow(500,500);
        }
        glutSetWindow(gl_window);
        glutReshapeWindow(500,500);
        glutDisplayFunc(display);

        for (size_t i=0; i<config.size(); i++) {
            if (config[i].window == sw_window) {
                BlitRendererPtr blit_renderer = dynamic_pointer_cast<BlitRenderer>(config[i].renderer);

                swShutdown(blit_renderer);
            }
        }

        if (extended_benchmark_requested) {
            exit(0);
        }
    }
}

static void toggleGoldWindow();

bool makeImageFilenameString(string &result, bool gold)
{
    string filename;

    switch (draw_mode) {
    case DRAW_PATH_DATA:
        filename = string("path_data/") + path_objects[current_path_object].name;
        break;
    case DRAW_SVG_FILE:
        filename = getSVGFileName(current_svg_filename);
        break;
#if USE_FREETYPE2
    case DRAW_FREETYPE2_FONT:
        filename = string("fonts/") + string(font_name(font_index));
        break;
#endif
    default:
        assert(!"bogus DrawMode");
        result = "bogus.tga";
        return false;
    }

    string::size_type pos = filename.rfind('.');
    if (pos != ~0U) {
        filename.erase(pos);
    }

    char buf[4000];
    sprintf(buf, "_%dx%dx%d%s%s",
        gl_window_width, gl_window_height,
        nvpr_renderer->num_samples,
        nvpr_renderer->render_sRGB ? "_sRGB" : "",
        gold ? ".gold" : "");
    filename.append(buf);
    filename.append(".tga");

    result = filename;
    return true;
}

RenderResultPtr getLatestGoldRenderResult()
{
    return RenderResultPtr();
}

void
output(int x, int y, const char *string)
{
  void *font = GLUT_BITMAP_TIMES_ROMAN_24;
  int len, i;

  glWindowPos2i(x, y);
  len = (int) strlen(string);
  for (i = 0; i < len; i++) {
    glutBitmapCharacter(font, string[i]);
  }
}

void getGold()
{
    if (!last_gold || !last_gold->matchesCurrentImage()) {
        string filename;
        bool ok = makeImageFilenameString(filename, 0);

        if (ok) {
            int image_width, image_height, components;
            stbi_uc *data = stbi_load(filename.c_str(),
                &image_width, &image_height,
                &components, 3);
            if (data) {
                assert(components == 3);
                last_gold = RenderResultPtr(new RenderResultFromFile(image_width, image_height, data));
            } else {
                printf("could not open gold image for %s\n", filename.c_str());
                last_gold = RenderResultPtr();
            }
        } else {
            printf("unable to form filename\n");
        }
    }
}

void goldDisplay()
{
    glClear(GL_COLOR_BUFFER_BIT);

    getGold();

    if (last_gold) {
        last_gold->drawPixels();
    } else {
        glColor3f(1,1,0);
        output(10, gold_window_height/2, "no gold");
    }
    glutSwapBuffers();
}

int compareTo = 0; // initially gold

int regressionStartTime;
int seedStartTime;

void possiblyDisableSceneIteration()
{
    if (!request_regress && !request_seed) {
        glutIdleFunc(NULL);
    }
}

void forceDisableSceneIteration()
{
    if (request_regress || request_seed) {
        printf("Forcing disabling of scene iteration...\n");
        request_regress = false;
        request_seed = false;
        glutIdleFunc(NULL);
    }
}

void possiblyTerminateRegressScenes(const char *msg)
{
    if (request_regress) {
        tests_regressed++;
        if (msg) {
            printf("after %d regression tests...\n", tests_regressed);
            printf("%s\n", msg);
        } else {
            if (current_svg_filename != begin_current_svg_filename) {
                return;
            }
            printf("Full regression completed (%d tests) without mismatches, congrats!\n",
                tests_regressed);
        }
        begin_current_svg_filename = ~0U;
        int regressionEndTime = glutGet(GLUT_ELAPSED_TIME);
        float seconds = (regressionEndTime - regressionStartTime)/1000.0;
        printf("took %.1f seconds\n", seconds);
        request_regress = false;
        possiblyDisableSceneIteration();
    }
}

void diffDisplay()
{
    glClear(GL_COLOR_BUFFER_BIT);
    getGold();
    if (request_regress) {
        glutSetWindow(gl_window);
        display();
        glutSetWindow(diff_window);
    }

    assert(last_diff);
    RenderResultPtr diff_with = compareTo ? last_software : last_gold;

    int different_pixels = 0;
    int close_pixels = 0;
    RenderResultDifference::MatchType match = last_diff->compare(last_render, 
        diff_with, different_pixels, close_pixels);

    if (last_render && last_gold) {
        last_diff->drawPixels();
    }

    possiblyTerminateRegressScenes(NULL);

    switch (match) {
    case RenderResultDifference::DIFFERENT:
        glColor3f(1,0,0);
        {
            string msg = "DIFFS: " + 
                         lexical_cast<string,int>(different_pixels) +
                         " wrong pixels, " +
                         lexical_cast<string,int>(close_pixels) +
                         " close pixels";
            output(10, 5, msg.c_str());
        }
        possiblyTerminateRegressScenes("found DIFFERENT image");
        break;
    case RenderResultDifference::APPROXIMATELY_SAME:
        glColor3f(1,1,1);
        {
            string msg = "close, " +
                         lexical_cast<string,int>(different_pixels) +
                         " wrong pixels, " +
                         lexical_cast<string,int>(close_pixels) +
                         " close pixels";
            output(10, 5, msg.c_str());
        }
        possiblyTerminateRegressScenes("found DIFFERENT image");
        break;
    case RenderResultDifference::SAME:
        glColor3f(0,1,0);
        output(10, 5, "exact same");
        break;
    case RenderResultDifference::INCONSISTENT:
        glColor3f(1,1,0);
        output(10, 5, "inconsistent");
        possiblyTerminateRegressScenes("found inconsistent image");
        break;
    }

    glutSwapBuffers();
}

void goldReshape(int width, int height)
{
    gold_window_width = width;
    gold_window_height = height;
    glViewport(0, 0, width, height);
}

void diffReshape(int width, int height)
{
    diff_window_width = width;
    diff_window_height = height;
    glViewport(0, 0, width, height);
}

static void makeGoldWindow()
{
    if (gold_window < 0) { 
        glutInitDisplayString(NULL);
        glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
        glutInitWindowPosition(window_x, window_y + gl_window_height + 40);
        glutInitWindowSize(gl_window_width, gl_window_height);
        gold_window = glutCreateWindow("gold window");
        glutKeyboardFunc(keyboard);
        glutSpecialFunc(special);
        glutDisplayFunc(goldDisplay);
        glutReshapeFunc(goldReshape);
        requestSynchornizedSwapBuffers(enable_sync);
    }
}

void updateDiffWindowTitle()
{
    string title;
    switch (compareTo) {
    case 0:
        title = "GPU vs. gold";
        break;
    case 1:
        switch (swMode) {
#if USE_OPENVG
        case SW_MODE_OPENVG:
            title = "GPU vs. OpenVG";
            break;
#endif
#if USE_QT
        case SW_MODE_QT:
            title = "GPU vs. Qt";
            break;
#endif
#if USE_CAIRO
        case SW_MODE_CAIRO:
            title = "GPU vs. Cairo";
            break;
#endif
#if USE_SKIA
        case SW_MODE_SKIA:
            title = "GPU vs. Skia";
            break;
#endif
#if USE_D2D
        case SW_MODE_D2D:
            title = "GPU vs. Direct2D";
            break;
        case SW_MODE_D2D_WARP:
            title = "GPU vs. Direct2D Warp";
            break;
#endif
        default:
            title = "GPU vs. ???";
            break;
        }
        break;
    }
    switch (diff_neighborhood) {
    case 0:
        title += " using 1x1 neighborhood";
        break;
    case 1:
        title += " using 3x3 neighborhood";
        break;
    case 2:
        title += " using 5x5 neighborhood";
        break;
    }
    if (adj_average) {
        title += " with adjacent averages";
    }
    glutSetWindowTitle(title.c_str());
}

void compare_option_menu(int item)
{
    forceDisableSceneIteration();
    switch (item) {
    case 0:
    case 1:
        compareTo = item;
        glutPostWindowRedisplay(diff_window);
        break;
    default:
        break;
    }
    updateDiffWindowTitle();
}

void max_diff_menu(int item)
{
    forceDisableSceneIteration();
    max_difference = item;
    glutPostWindowRedisplay(diff_window);
    updateDiffWindowTitle();
}

void diff_menu(int item)
{
    forceDisableSceneIteration();
    switch (item) {
    case 27:
        exit(0);
        break;
    case 0:
    case 1:
    case 2:
        diff_neighborhood = item;
        adj_average = false;
        glutPostRedisplay();
        break;
    case 3:
    case 4:
        diff_neighborhood = item-2;
        adj_average = true;
        glutPostRedisplay();
        break;
    default:
        break;
    }
    updateDiffWindowTitle();
}

static void makeDiffWindow()
{
    if (diff_window < 0) { 
        last_diff = RenderResultDifferencePtr(new RenderResultDifference);
        glutInitDisplayString(NULL);
        glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
        glutInitWindowPosition(window_x + gl_window_width + 20, window_y + gl_window_height + 40);
        glutInitWindowSize(gl_window_width, gl_window_height);
        diff_window = glutCreateWindow("diff window");
        glutKeyboardFunc(keyboard);
        glutSpecialFunc(special);
        glutDisplayFunc(diffDisplay);
        glutReshapeFunc(diffReshape);
        requestSynchornizedSwapBuffers(enable_sync);

        int compare_options = glutCreateMenu(compare_option_menu);
        glutAddMenuEntry("gold", 0);
        glutAddMenuEntry("software", 1);

        int max_diff_options = glutCreateMenu(max_diff_menu);
        glutAddMenuEntry("1", 1);
        glutAddMenuEntry("5", 5);
        glutAddMenuEntry("10", 10);
        glutAddMenuEntry("20", 20);
        glutAddMenuEntry("30", 30);
        glutAddMenuEntry("40", 40);
        glutAddMenuEntry("50", 50);
        glutAddMenuEntry("60", 60);
        glutAddMenuEntry("70", 70);
        glutAddMenuEntry("80", 80);
        glutAddMenuEntry("90", 90);
        glutAddMenuEntry("100", 100);

        glutCreateMenu(diff_menu);
        glutAddSubMenu("Compare to...", compare_options);
        glutAddSubMenu("Maximum difference...", max_diff_options);
        glutAddMenuEntry("1x1 diff neighborhood", 0);
        glutAddMenuEntry("3x3 diff neighborhood", 1);
        glutAddMenuEntry("5x5 diff neighborhood", 2);

        glutAddMenuEntry("3x3 diff neighborhood + adj avg", 3);
        glutAddMenuEntry("5x5 diff neighborhood + adj avg", 4);

        glutAddMenuEntry("[Esc] Quit", 27);
        glutAttachMenu(GLUT_RIGHT_BUTTON);

        updateDiffWindowTitle();
    }
}

static void goldShutdown()
{
    printf("shutting down gold window...\n");
}

static void diffShutdown()
{
    printf("shutting down diff window...\n");
}

static void unmakeGoldWindow()
{
    if (gold_window >= 0) { 
        glutDestroyWindow(gold_window);
        goldShutdown();
        gold_window = -1;
    }
}

static void unmakeDiffWindow()
{
    if (diff_window >= 0) { 
        glutDestroyWindow(diff_window);
        diffShutdown();
        diff_window = -1;
    }
}

static void toggleGoldWindow()
{
    if (gold_window < 0) {
        makeGoldWindow();
    } else {
        unmakeGoldWindow();
    }
}

static void toggleDiffWindow()
{
    if (diff_window < 0) {
        makeDiffWindow();
    } else {
        unmakeDiffWindow();
    }
}

static void compareToGold()
{
}

#if USE_D2D || USE_CAIRO || USE_QT || USE_OPENVG || USE_SKIA
static void benchmarkSoftwareRendering(float &raw_fps, BlitRendererPtr blit_renderer)
{
    float seconds_cap = 1.0f;
    int iterations = 300;
    double startTime, stopTime;
    double seconds;

    glutSetWindow(sw_window);

#if 0  // might want this but often just adds time
    swRender(blit_renderer);  // primer
#endif

    startTime = getElapsedTime();
    swRender(blit_renderer);
    stopTime = getElapsedTime();
    seconds = stopTime-startTime;
    swPresent(blit_renderer);

    if (seconds * iterations > seconds_cap) {
        iterations = int(ceil(seconds_cap / seconds));
    }

    if (iterations < 1) {
        iterations = 1;
    }
    glFinish();

    startTime = getElapsedTime();
    for (int i=0; i<iterations; i++) {
        swRender(blit_renderer);
    }
    stopTime = getElapsedTime();
#if 0
    swPresent(blit_renderer);
    blit_renderer->swapBuffers();
    glFinish();
#endif

    seconds = stopTime-startTime;
    raw_fps = iterations/seconds;

    if (verbose) {
        printf("%s: seconds = %f, iterations = %d\n", blit_renderer->getWindowTitle(), seconds, iterations);
    }
}
#endif

static void benchmarkSoftwareRendering(float &drawing_fps, float &raw_fps, bool printResults)
{
    float seconds_cap = 1.0f;
    int iterations = 120;
    int startTime, stopTime;
    float seconds;

    glutSetWindow(sw_window);
    swInit(sw_window_width, sw_window_height, blit_renderer);

    // Prime caches and finish OpenGL
    startTime = glutGet(GLUT_ELAPSED_TIME);
    swRender(blit_renderer);
    swPresent(blit_renderer);
    glFinish();
    stopTime = glutGet(GLUT_ELAPSED_TIME);
    seconds = (stopTime-startTime)/1000.0;

    if (seconds * iterations > seconds_cap) {
        iterations = int(ceil(seconds_cap / seconds));
    }

    if (iterations < 1) {
        iterations = 1;
    }

    if (printResults) {
        printf("running %s benchmark...", sw_windowTitle(blit_renderer));
    }
    startTime = glutGet(GLUT_ELAPSED_TIME);
    for (int i=0; i<iterations; i++) {
        swRender(blit_renderer);
        swPresent(blit_renderer);
        glFlush();
    }
    glFinish();
    stopTime = glutGet(GLUT_ELAPSED_TIME);

    seconds = (stopTime-startTime)/1000.0;
    drawing_fps = iterations/seconds;
    if (printResults) {
        printf("\nResult: %.1f frames/second (%d iterations for %.2f secs)\n",
            drawing_fps, iterations, seconds);
        printf("\nResult with glDrawPixels: %.1f frames/second (%d iterations for %.2f secs)\n",
            drawing_fps, iterations, seconds);
    }

    // Prime caches and finish OpenGL
    swRender(blit_renderer);

    if (printResults) {
        printf("running %s benchmark...", sw_windowTitle(blit_renderer));
    }
    startTime = glutGet(GLUT_ELAPSED_TIME);
    for (int i=0; i<iterations; i++) {
        swRender(blit_renderer);
    }
    stopTime = glutGet(GLUT_ELAPSED_TIME);

    seconds = (stopTime-startTime)/1000.0;
    raw_fps = iterations/seconds;
    if (printResults) {
        printf("\nResult WITHOUT glDrawPixels: %.1f frames/second (%d iterations for %.2f sec)\n",
            raw_fps, iterations, seconds);
    }
}

static void benchmarkGLRendering(float &drawing_fps, bool printResults)
{
    float seconds_cap = 1.0f;
    int iterations = 300;
    int startTime, stopTime;
    float seconds;

    have_dlist = false;
    glutSetWindow(gl_window);
    display();

    // Prime caches and finish OpenGL
    startTime = glutGet(GLUT_ELAPSED_TIME);
    display();
    display();
    glFinish();
    stopTime = glutGet(GLUT_ELAPSED_TIME);
    seconds = (stopTime-startTime)/1000.0;
    seconds /= 2;

    if (seconds * iterations > seconds_cap) {
       iterations = int(ceil(seconds_cap / seconds));
    }

    if (iterations < 1) {
        iterations = 1;
    }

    if (printResults) {
        printf("running GL benchmark...");
    }

    // Make sure the benchmark runs for at least 1/3 of a second
    bool done = false;
    do {
        startTime = glutGet(GLUT_ELAPSED_TIME);
        for (int i=0; i<iterations; i++) {
            display();
        }
        glFinish();
        stopTime = glutGet(GLUT_ELAPSED_TIME);

        seconds = (stopTime-startTime)/1000.0;
        if (seconds > 0.333) {
            done = true;
        } else {
            iterations *= 2;
        }
    } while (!done);

    assert(seconds != 0);
    drawing_fps = iterations/seconds;
    if (printResults) {
        printf("\nResult: %.1f frames/second (%d iterations for %.2f secs)\n",
            drawing_fps, iterations, seconds);
    }
}

static void no_op()
{
    // no drawing
    if (verbose) {
        printf("no_op display callback called fo %d!\n", glutGetWindow());
    }
}

static void extendedBenchmarkSoftwareRendering()
{
    if (allowSoftwareWindow) {
        if (sw_window < 0) {
            makeSoftwareWindow();
        }

        glutSetWindow(sw_window);
        glutDisplayFunc(no_op);
        glutSetWindow(gl_window);
        glutDisplayFunc(no_op);
    }

    extended_benchmark_res  = 0;
    extended_benchmark_file = 0;

    initBenchmarkConfigArray();

    printf("%-30s, %-9s", "SVG File Name", "Resolution");
    for (size_t i=0; i<config.size(); i++) {
        if (config[i].renderer) {
            printf(", %5s FPS", config[i].renderer->getName());
        }
    }
    for (size_t i=1; i<config.size(); i++) {
        if (config[i].renderer) {
            char buffer[100];

            sprintf(buffer, "GL/%s", config[i].renderer->getName());
            printf(", %9s", buffer);
        }
    }
    printf("\n");

    start_testing_time = glutGet(GLUT_ELAPSED_TIME);
    glutIdleFunc(benchmarkIdle);
}

void idleExtendedBenchmarkSoftwareRendering()
{
    if (allowSoftwareWindow && sw_window < 0) {
        // A request is outstanding for a software window to be created so keep waiting
        assert(request_software_window);
        return;
    } else {
        extendedBenchmarkSoftwareRendering();
    }
}

void advanceScene()
{
    current_svg_filename = advanceSVGFile(current_svg_filename);
    if (request_seed && !request_regress) {
        string result;
        makeImageFilenameString(result, false);
        FILE *file = fopen(result.c_str(), "r");
        if (file) {
            fclose(file);
            // When file exists, check if we are done.
            if (current_svg_filename == begin_current_svg_filename) {

                int seedEndTime = glutGet(GLUT_ELAPSED_TIME);
                float seconds = (seedEndTime - seedStartTime)/1000.0;

                printf("Seeding complete for all SVG scenes!, created %d seeds\n", seeds_created);
                printf("took %.1f seconds\n", seconds);
                request_seed = false;
                possiblyDisableSceneIteration();
            }
            return;  // without setSVGScene to skip!
        } else {
            // Needs seed to be written
        }
    }
    setSVGScene(getSVGFileName(current_svg_filename));
}

void startSceneRegressions()
{
    printf("Start scene regressions...\n");
    tests_regressed = 0;
    draw_mode = DRAW_SVG_FILE;
    begin_current_svg_filename = current_svg_filename;
    makeGoldWindow();
    makeDiffWindow();
    disableAnimation(BOTH_WINDOWS);
    glutIdleFunc(advanceScene);
    regressionStartTime = glutGet(GLUT_ELAPSED_TIME);
    request_regress = true;
}

void startSceneSeeding()
{
    printf("Start scene seeding...\n");
    seeds_created = 0;
    draw_mode = DRAW_SVG_FILE;
    begin_current_svg_filename = current_svg_filename;
    disableAnimation(BOTH_WINDOWS);
    glutIdleFunc(advanceScene);
    seedStartTime = glutGet(GLUT_ELAPSED_TIME);
    request_seed = true;
    advanceScene();
}

#if USE_SKIA || USE_QT || USE_D2D || USE_CAIRO
static void swConfigWindow()
{
    makeSoftwareWindow();
    swInit(sw_window_width, sw_window_height, blit_renderer);
    software_window_valid = false;
    do_redisplay(SW_WINDOW);
    invalidateFPS();
}
#endif

static void keyboard(unsigned char c, int x, int y)
{
    if (glutGetWindow() == gl_container) {
        glutSetWindow(gl_window);
    }
    forceDisableSceneIteration();
    switch (c) {
    case '\017': // Ctrl O
        occlusion_query_mode = (occlusion_query_mode+1) % 3;
        printf("occlusion_query_mode = %d\n", occlusion_query_mode);
        break;
    case 6: // Ctrl F
        printf("Flushing renderer states...\n");
        {
        ForEachShapeTraversal traversal;
        scene->traverse(VisitorPtr(new FlushRendererStates), traversal);
        }
        break;
    case 7: // Ctrl G
        startSceneRegressions();
        return;
    case 8: // Ctrl H
        startSceneSeeding();
        return;
    case 9: // Ctrl I
        printf("Invalidating renderer states...\n");
        {
        ForEachShapeTraversal traversal;
        scene->traverse(VisitorPtr(new InvalidateRendererStates), traversal);
        }
        break;
    case 27:  /* Esc key */
        /* Demonstrate proper deallocation of Cg runtime data structures.
        Not strictly necessary if we are simply going to exit. */
        nvpr_renderer->shutdown_shaders();
        exit(0);
        break;
    case 'q':
        requested_samples <<= 1;
        if (requested_samples > 32) {
            requested_samples = 32;
        }
        configureSamples(requested_samples);
        break;
    case 'Q':
        requested_samples >>= 1;
        if (requested_samples < 1) {
            requested_samples = 1;
        }
        configureSamples(requested_samples);
        break;
#if USE_FREETYPE2
    case 'g':
        if (draw_mode == DRAW_FREETYPE2_FONT) {
            font_index = (font_index+1) % num_fonts();
        } else {
            draw_mode = DRAW_FREETYPE2_FONT;
        }
        setFontScene(font_index);
        break;
    case 'G':
        if (draw_mode == DRAW_FREETYPE2_FONT) {
            if (font_index > 0) {
                font_index--;
            } else {
                font_index = num_fonts()-1;
            }
        } else {
            draw_mode = DRAW_FREETYPE2_FONT;
        }
        setFontScene(font_index);
        break;
#endif
    case 'f':
        if (draw_mode == DRAW_SVG_FILE) {
            current_svg_filename = advanceSVGFile(current_svg_filename);
        } else {
            draw_mode = DRAW_SVG_FILE;
        }
        setSVGScene(getSVGFileName(current_svg_filename));
        break;
    case 'F':
        if (draw_mode == DRAW_SVG_FILE) {
            current_svg_filename = reverseSVGFile(current_svg_filename);
        } else {
            draw_mode = DRAW_SVG_FILE;
        }
        setSVGScene(getSVGFileName(current_svg_filename));
        break;
    case 'w':
        if (draw_mode == DRAW_PATH_DATA) {
            current_path_object = (current_path_object+1) % num_path_objects;
        } else {
            draw_mode = DRAW_PATH_DATA;
        }
        setPathScene(path_objects[current_path_object]);
        break;
    case 'W':
        if (draw_mode == DRAW_PATH_DATA) {
            if (current_path_object > 0) {
                current_path_object = (current_path_object-1);
            } else {
                current_path_object = num_path_objects-1;
            }
        } else {
            draw_mode = DRAW_PATH_DATA;
        }
        setPathScene(path_objects[current_path_object]);
        break;
    case 'h':
        accumulationSpread *= 1.05;
        printf("accumulationSpread = %f\n", accumulationSpread);
        break;
    case 'H':
        accumulationSpread /= 1.05;
        printf("accumulationSpread = %f\n", accumulationSpread);
        break;
#if USE_OPENVG
    case 18:  // Ctrl R
        swMode = SW_MODE_OPENVG;
        swConfigWindow();
        return;
#endif
#if USE_SKIA || USE_QT || USE_CAIRO
    case 'R':  // toggles Qt or Cairo (or does Cairo if using OpenVG)

        if (swMode >= SM_MODE_CYCLE) {
            swMode = SWMODE(0);
        } else {
            swMode = SWMODE((swMode + 1) % SM_MODE_CYCLE);
        }
        swConfigWindow();
        return;
#endif
#if USE_D2D
    case 5:  // Ctrl E
        swMode = SW_MODE_D2D;
        swConfigWindow();
        return;
#endif
    case 'e':
        current_clip_object = (current_clip_object+1) % num_path_objects;
        setClipObject(path_objects[current_clip_object]);
        break;
    case 'E':
        current_clip_object = (current_clip_object-1);
        if (current_clip_object < 0) {
            current_clip_object = num_path_objects-1;
        }
        setClipObject(path_objects[current_clip_object]);
        break;
    case 'l':
        show_clip_scissor = !show_clip_scissor;
        break;
    case '3':
        current_clear_color = float4(0,0,0,0);
        set_clear_color(current_clear_color);
#if USE_FREETYPE2
        // Hack font's solid color based on clear color
        if (draw_mode == DRAW_FREETYPE2_FONT) {
            setFontScene(font_index);
        }
#endif
        return;
    case '4':
        current_clear_color = float4(1,1,1,0);
        set_clear_color(current_clear_color);
#if USE_FREETYPE2
        // Hack font's solid color based on clear color
        if (draw_mode == DRAW_FREETYPE2_FONT) {
            setFontScene(font_index);
        }
#endif
        return;
    case '5':
        current_clear_color = default_clear_color;
        set_clear_color(current_clear_color);
#if USE_FREETYPE2
        // Hack font's solid color based on clear color
        if (draw_mode == DRAW_FREETYPE2_FONT) {
            setFontScene(font_index);
        }
#endif
        return;
    case '6':
        current_clear_color = gray204_clear_color;
        set_clear_color(current_clear_color);
#if USE_FREETYPE2
        // Hack font's solid color based on clear color
        if (draw_mode == DRAW_FREETYPE2_FONT) {
            setFontScene(font_index);
        }
#endif
        return;
    case '7':
        if (nvpr_renderer->alpha_bits > 0) {
            xsteps++;
            if (xsteps > 18) {
                xsteps = 1;
            }
            ysteps++;
            if (ysteps > 18) {
                ysteps = 1;
            }
            const int gridSize = xsteps*ysteps;
            const int passes = stipple ? gridSize/2 : gridSize;
            printf("passCount = %d (%dx%d%s)\n", passes, xsteps, ysteps, stipple ? " stippled" : "");
            forceOffRenderTopToBottom();
        } else {
            printf("%s: need alpha bits to use multiPassMode\n", myProgramName);
        }
        break;
    case '&':
        if (nvpr_renderer->alpha_bits > 0) {
            xsteps--;
            if (xsteps < 1) {
                xsteps = 1;
            }
            ysteps--;
            if (ysteps < 1) {
                ysteps = 1;
            }
            printf("passCount = %d (%dx%d)\n", xsteps*ysteps, xsteps, ysteps);
            forceOffRenderTopToBottom();
        } else {
            printf("%s: need alpha bits to use multiPassMode\n", myProgramName);
        }
        break;
    case '8':
        skip_swap = !skip_swap;
        printf("skip_swap = %d\n", skip_swap);
        return;
    case '*':
        just_clear_and_swap = !just_clear_and_swap;
        printf("just_clear_and_swap = %d\n", just_clear_and_swap);
        return;
    case '9':
        if (nvpr_renderer->alpha_bits > 0) {
            render_top_to_bottom = !render_top_to_bottom;
            printf("render_top_to_bottom = %d\n", render_top_to_bottom);
            validateStencil();
            update_clear_color();
        } else {
            assert(render_top_to_bottom == 0);
            printf("framebuffer lacks alpha so render_to_to_bottom not allowed\n");
        }
        break;
    case '0':
        use_dlist = !use_dlist;
        printf("use display lists = %d\n", use_dlist);
        break;
    case '(':
        switch (default_fill_cover_mode) {
        default:
            assert(!"invalid fill cover mode");
            // Fallthrough...
        case GL_CONVEX_HULL_NV:
            default_fill_cover_mode = GL_BOUNDING_BOX_NV;
            printf("fill GL_BOUNDING_BOX_NV\n");
            break;
        case GL_BOUNDING_BOX_NV:
            default_fill_cover_mode = GL_CONVEX_HULL_NV;
            printf("fill GL_CONVEX_HULL_NV\n");
            break;
        }
        updateSceneCoverModes(scene);
        break;
    case ')':
        switch (default_stroke_cover_mode) {
        case GL_CONVEX_HULL_NV:
            default_stroke_cover_mode = GL_BOUNDING_BOX_NV;
            printf("stroke GL_BOUNDING_BOX_NV BOX\n");
            break;
        case GL_BOUNDING_BOX_NV:
            default_stroke_cover_mode = GL_CONVEX_HULL_NV;
            printf("stroke GL_CONVEX_HULL_NV\n");
            break;
        }
        updateSceneCoverModes(scene);
        break;
    case 'i':
        {
            PathStats total, max;

            ForEachShapeTraversal traversal;
            scene->traverse(VisitorPtr(new StCVisitors::GatherStats(nvpr_renderer, total, max)), traversal);
            printf("  num_paths = %d\n", int(total.num_paths));
            printf("  num_cmds = %d\n", int(total.num_cmds));
            printf("  num_coords = %d\n", int(total.num_coords));

            printf("Window resolution: %dx%d pixels, %d samples/pixel\n",
                gl_window_width, gl_window_height, nvpr_renderer->num_samples);
            if (xsteps*ysteps > 1) {
                const int gridSize = xsteps*ysteps;
                const int passes = stipple ? gridSize/2 : gridSize;
                printf("Multi-pass rendering: %dx%d%s pass grid (%d total passes, %d total samples)\n",
                    xsteps, ysteps, stipple ? " stippled" : "", passes, passes*nvpr_renderer->num_samples);
            }
        }
        return;
    case 'c':
        show_control_points = !show_control_points;
        do_redisplay(BOTH_WINDOWS);
        return;
    case 'C':
        show_warp_points = !show_warp_points;
        do_redisplay(BOTH_WINDOWS);
        return;
    case 'p':
        show_reference_points = !show_reference_points;
        do_redisplay(GL_WINDOW);
        return;
    case 13:
        printf("redraw\n");
        if (glutGetWindow() != gl_window) {
            software_window_valid = false;
        }
        glutPostRedisplay();
        return;
    case 'V':
        verbose = !verbose;
        break;
    case '!':
        reinitialize();
        break;
    case '1':
        reloadScene();
        break;
    case '`':
        printf("invalidating scene...\n");
        invalidateScene();
        break;
    case '~':
        printf("reloading shaders...\n");
        nvpr_renderer->load_shaders();
        break;
    case 'S':
#define GL_FORCE_SOFTWARE_NV 0x6007
        glutSetWindow(gl_window);
        if (glIsEnabled(GL_FORCE_SOFTWARE_NV)) {
            glDisable(GL_FORCE_SOFTWARE_NV);
        } else {
            glEnable(GL_FORCE_SOFTWARE_NV);
        }
        break;
    case 'P':
        showSamplePattern = !showSamplePattern;
        do_redisplay(GL_WINDOW);
        return;
    case ',':
        rotateScene(1);  // rotate 1 degree counterclockwise
        do_redisplay(BOTH_WINDOWS);
        return;
    case '.':
        rotateScene(-1);  // rotate 1 degree clockwise
        do_redisplay(BOTH_WINDOWS);
        return;
    case ' ':
        if (glutGetWindow() == gl_window) {
            disableAnimation(SW_WINDOW);
            if (animation_mask & GL_WINDOW) {
                disableAnimation(GL_WINDOW);
            } else {
                enableAnimation(GL_WINDOW);
            }
        } else {
            disableAnimation(GL_WINDOW);
            if (animation_mask & SW_WINDOW) {
                disableAnimation(SW_WINDOW);
            } else {
                enableAnimation(SW_WINDOW);
            }
        }
        return;
    case '=':
        angleRotateRate = !angleRotateRate;
        return;
    case ';':
        accumulationPasses++;
        printf("accumulationPasses = %d, %d samples/pixel\n", accumulationPasses, nvpr_renderer->num_samples*accumulationPasses);
        break;
    case ':':
        if (accumulationPasses > 0) {
            accumulationPasses--;
        }
        printf("accumulationPasses = %d, %d samples/pixel\n", accumulationPasses, nvpr_renderer->num_samples*accumulationPasses);
        break;
    case 'v':
        enable_sync = !enable_sync;
        glutSetWindow(gl_window);
        requestSynchornizedSwapBuffers(enable_sync);
        if (sw_window >= 0) {
            glutSetWindow(sw_window);
            requestSynchornizedSwapBuffers(enable_sync);
        }
        if (gold_window >= 0) {
            glutSetWindow(gold_window);
            requestSynchornizedSwapBuffers(enable_sync);
        }
        if (diff_window >= 0) {
            glutSetWindow(diff_window);
            requestSynchornizedSwapBuffers(enable_sync);
        }
        printf("enable_sync = %d\n", enable_sync);
        return;
    case 'd':
        {
            static const char filename[] = "test.svg";
            float w = wh.x,
                  h = wh.y;
            float4x4 clip_to_svg = float4x4(w/2,0,0,w/2,
                                            0,-h/2,0,h/2, // compute "h-y" to get to GL's lower-left origin
                                            0,0,1,0,
                                            0,0,0,1);
            FILE *file = fopen(filename, "w");
            if (file) {
                scene->dumpSVG(file, clip_to_svg);
                fclose(file);
                printf("wrote %s\n", filename);
            } else {
                printf("could not open %s\n", filename);
            }
        }
        return;
    case 'x':
        doStroking = !doStroking;
        printf(doStroking ? "doing STROKING\n" : "NOT stroking\n");
        break;
    case 'X':
        doFilling = !doFilling;
        printf(doFilling ? "doing FILLING\n" : "NOT filling\n");
        break;
    case 'z':
        compareToGold();
        break;
    case 'Z':
        only_necessary_stencil_clears = !only_necessary_stencil_clears;
        printf("only_necessary_stencil_clears = %d\n", only_necessary_stencil_clears);
        return;
    case 'n':
        if (sw_window < 0) {
            makeSoftwareWindow();
        } else {
            unmakeSoftwareWindow();
        }
        return;
    case 'j':
        toggleGoldWindow();
        break;
    case 'J':
        toggleDiffWindow();
        break;
    case 'N':
        makeSoftwareWindow();
        {
            float drawing_fps, raw_fps;
            benchmarkSoftwareRendering(drawing_fps, raw_fps);
        }
        break;
#if USE_D2D
    case 4: // Ctrl-D
        if (swMode == SW_MODE_D2D) {
            glutSetWindow(sw_window);
            d2dUseWARP = !d2dUseWARP;
            if (d2d_renderer) {
                d2d_renderer->recreateSurface(d2dUseWARP);
                if (d2dUseWARP)
                    printf("D2D SW renderer enabled.\n");
                else
                    printf("D2D HW renderer enabled.\n");
            }
        }
        break;
#endif
#if USE_CAIRO
    case 14:  // Ctrl-N
        switch (cairoAntialiasMode) {
        case CAIRO_ANTIALIAS_DEFAULT:
            cairoAntialiasMode = CAIRO_ANTIALIAS_NONE;
            printf("cairoAntialiasMode = CAIRO_ANTIALIAS_NONE\n");
            break;
        case CAIRO_ANTIALIAS_NONE:
            cairoAntialiasMode = CAIRO_ANTIALIAS_GRAY;
            printf("cairoAntialiasMode = CAIRO_ANTIALIAS_GRAY\n");
            break;
        case CAIRO_ANTIALIAS_GRAY:
            cairoAntialiasMode = CAIRO_ANTIALIAS_SUBPIXEL;
            printf("cairoAntialiasMode = CAIRO_ANTIALIAS_SUBPIXEL\n");
            break;
        case CAIRO_ANTIALIAS_SUBPIXEL:
            cairoAntialiasMode = CAIRO_ANTIALIAS_DEFAULT;
            printf("cairoAntialiasMode = CAIRO_ANTIALIAS_DEFAULT\n");
            break;
        default:
            assert(!"bad cairoAntialiasMode");
            break;
        }
        if (cairo_renderer) {
            software_window_valid = false;
            cairo_renderer->setAntialias(cairoAntialiasMode);
        }
        break;
#endif
    case 'm':
        nvpr_renderer->multisampling = !nvpr_renderer->multisampling;
        printf("gl_renderer->multisampling = %d\n", nvpr_renderer->multisampling);
        glutSetWindow(gl_window);
        if (nvpr_renderer->multisampling) {
            glEnable(GL_MULTISAMPLE);
        } else {
            glDisable(GL_MULTISAMPLE);
        }
        return;
    case 't':
        draw_mode = DRAW_SVG_FILE;
        setSVGScene("svg/complex/tiger.svg");
        break;
    case 'T':
        extendedBenchmarkSoftwareRendering();
        break;
    default:
        // ignore other keys
        printf("ignoring key press = %d\n", c);
        return;
    }
    have_dlist = false;
    software_window_valid = false;
    do_redisplay(BOTH_WINDOWS);
}

void writeImage(const string filename, int w, int h)
{
    glWindowPos2i(0,0);
    vector<RGB> img(w*h);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);  // avoid GL's dumb default of 4
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, &img[0]);
    flipImage(&img[0], w, h);
    stbi_write_tga(filename.c_str(), w, h, 3, &img[0]);
}

// F11 creates snapshot
void snapshot(int window)
{
    glutSetWindow(window);
    string filename;
    bool ok = makeImageFilenameString(filename, 0);

    if (ok) {
        printf("writing image %s...", filename.c_str());
        fflush(stdout);
        writeImage(filename, gl_window_width, gl_window_height);
        printf("done\n");

        if (gold_window >= 0) {
            glutPostWindowRedisplay(gold_window);
        }
        last_gold = RenderResultPtr();  // force a reload of last gold from disk
    }
}

static void special(int key, int x, int y)
{
    if (glutGetWindow() == gl_container) {
        glutSetWindow(gl_window);
    }
    switch (key) {
    case GLUT_KEY_F5:
        if (nvpr_renderer->alpha_bits > 0) {
            xsteps--;
            if (xsteps < 1) {
                xsteps = 1;
            }
            printf("passCount = %d (%dx%d)\n", xsteps*ysteps, xsteps, ysteps);
            forceOffRenderTopToBottom();
        } else {
            printf("%s: need alpha bits to use multiPassMode\n", myProgramName);
        }
        break;
    case GLUT_KEY_F6:
        if (nvpr_renderer->alpha_bits > 0) {
            ysteps--;
            if (ysteps < 1) {
                ysteps = 1;
            }
            printf("passCount = %d (%dx%d)\n", xsteps*ysteps, xsteps, ysteps);
            forceOffRenderTopToBottom();
        } else {
            printf("%s: need alpha bits to use multiPassMode\n", myProgramName);
        }
        break;
    case GLUT_KEY_F7:
        if (nvpr_renderer->alpha_bits > 0) {
            xsteps++;
            if (xsteps > 18) {
                xsteps = 1;
            }
            printf("passCount = %d (%dx%d)\n", xsteps*ysteps, xsteps, ysteps);
            forceOffRenderTopToBottom();
        } else {
            printf("%s: need alpha bits to use multiPassMode\n", myProgramName);
        }
        break;
    case GLUT_KEY_F8:
        if (nvpr_renderer->alpha_bits > 0) {
            ysteps++;
            if (ysteps > 18) {
                ysteps = 1;
            }
            printf("passCount = %d (%dx%d)\n", xsteps*ysteps, xsteps, ysteps);
            forceOffRenderTopToBottom();
        } else {
            printf("%s: need alpha bits to use multiPassMode\n", myProgramName);
        }
        break;
    case GLUT_KEY_F1:
        spread.x *= 0.75;
        printf("spread = %f,%f\n", float(spread.x), float(spread.y));
        break;
    case GLUT_KEY_F2:
        spread.y *= 0.75;
        printf("spread = %f,%f\n", float(spread.x), float(spread.y));
        break;
    case GLUT_KEY_F3:
        spread.x *= 1.333;
        printf("spread = %f,%f\n", float(spread.x), float(spread.y));
        break;
    case GLUT_KEY_F4:
        spread.y *= 1.333;
        printf("spread = %f,%f\n", float(spread.x), float(spread.y));
        break;
    case GLUT_KEY_F9:
        stipple = !stipple;
        printf("%dx%d %s\n", xsteps, ysteps, stipple ? "stippled" : "non-stippled");
        break;
    case GLUT_KEY_F11:
        snapshot(gl_window);
        break;
    case GLUT_KEY_HOME:
        resetView();
        updateTransform();
        do_redisplay(GL_WINDOW);
        break;
    case GLUT_KEY_INSERT:
        {
            // Toggle color space
            if (color_space == CORRECTED_SRGB) {
                color_space = UNCORRECTED;
            } else {
                color_space = CORRECTED_SRGB;
            }
            updateColorSpace();
        }
        break;
    case GLUT_KEY_UP:
        view[3][1] += 0.1;
        updateTransform();
        do_redisplay(BOTH_WINDOWS);
        return;
    case GLUT_KEY_DOWN:
        view[3][1] -= 0.1;
        updateTransform();
        do_redisplay(BOTH_WINDOWS);
        return;
    case GLUT_KEY_LEFT:
        view[3][0] -= 0.1;
        updateTransform();
        do_redisplay(BOTH_WINDOWS);
        return;
    case GLUT_KEY_RIGHT:
        view[3][0] += 0.1;
        updateTransform();
        do_redisplay(BOTH_WINDOWS);
        return;
    case GLUT_KEY_PAGE_UP:
        zoom *= 1.01;
        updateTransform();
        software_window_valid = false;
        do_redisplay(BOTH_WINDOWS);
        return;
    case GLUT_KEY_PAGE_DOWN:
        zoom /= 1.01;
        updateTransform();
        software_window_valid = false;
        do_redisplay(BOTH_WINDOWS);
        return;
    default:
        // ignore other keys
        return;
    }
    have_dlist = false;
    software_window_valid = false;
    do_redisplay(BOTH_WINDOWS);
}

ActiveControlPoint active_control_point;
class GetActiveControlPoint : public MatrixSaveVisitor
{
public:
    GetActiveControlPoint(ActiveControlPoint &hit_)
        : hit(hit_)
    {
        matrix_stack.pop();
        matrix_stack.push(hit.current_transform);
    }
    void visit(ShapePtr shape) {
        shape->getPath()->findNearerControlPoint(hit);
    }
    void apply(TransformPtr transform) {
        MatrixSaveVisitor::apply(transform);
        hit.current_transform = matrix_stack.top();
    }
    void unapply(TransformPtr transform) {
        MatrixSaveVisitor::unapply(transform);
        hit.current_transform = matrix_stack.top();
    }

protected:
    ActiveControlPoint &hit;
};

ActiveWarpPoint active_warp_point;
class GetActiveWarpPoint : public MatrixSaveVisitor
{
public:
    GetActiveWarpPoint(ActiveWarpPoint &hit_)
        : hit(hit_)
    {
        matrix_stack.pop();
        matrix_stack.push(hit.current_transform);
        if (verbose) {
            std::cout << "top = " << hit.current_transform << std::endl;
        }
    }
    void visit(ShapePtr shape) { }  // ignore shapes, interested in transform
    void apply(TransformPtr transform) {
        WarpTransformPtr warp_transform = dynamic_pointer_cast<WarpTransform>(transform);
        // Is this transform a warp transform?
        if (warp_transform) {
            hit.current_transform = warp_transform->scaledTransform(matrix_stack.top());
            warp_transform->findNearerControlPoint(hit);
        }

        MatrixSaveVisitor::apply(transform);
    }
    void unapply(TransformPtr transform) {
        MatrixSaveVisitor::unapply(transform);
    }

protected:
    ActiveWarpPoint &hit;
};

bool zooming = false,
     rotating = false,
     sliding = false;

/* Scaling and rotation state. */
float anchor_x = 0,
      anchor_y = 0;  /* Anchor for rotation and scaling. */
float scale_y = 0, 
      rotate_x = 0;  /* Prior (x,y) location for scaling (vertical) or rotation (horizontal)? */

/* Sliding (translation) state. */
float slide_x = 0,
      slide_y = 0,  /* Prior (x,y) location for sliding. */
      slide_scale = 1;

void
mouse(int button, int state, int mouse_space_x, int mouse_space_y)
{
    if (glutGetWindow() == gl_container) {
        glutSetWindow(gl_window);
    }
    const bool isSWwindow = glutGetWindow() == sw_window;

    if (isSWwindow) {
        if (animation_mask & GL_WINDOW) {
            disableAnimation(GL_WINDOW);
            enableAnimation(SW_WINDOW);
        }
    } else {
        if (animation_mask & SW_WINDOW) {
            disableAnimation(SW_WINDOW);
            enableAnimation(GL_WINDOW);
        }
    }

    const float2 wh = isSWwindow
                    ? float2(sw_window_width,sw_window_height)
                    : float2(gl_window_width,gl_window_height);
    const int update_mask = isSWwindow ? SW_WINDOW : GL_WINDOW;
    const float x = mouse_space_x,
                y = wh.y - mouse_space_y;
    const int modifiers = glutGetModifiers() & (GLUT_ACTIVE_CTRL|GLUT_ACTIVE_SHIFT);
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            if (show_warp_points) {
                float w = wh.x,
                      h = wh.y;
                float2 mousespace_xy = float2(mouse_space_x, mouse_space_y);
                float4x4 clip_to_mouse = float4x4(w/2,0,0,w/2,
                    0,-h/2,0,h/2, // compute "h-y" to get to GL's lower-left origin
                    0,0,1,0,
                    0,0,0,1);

                int done_update_mask = glutGetModifiers() & GLUT_ACTIVE_ALT
                                     ? BOTH_WINDOWS
                                     : update_mask;
                float4x4 surface_to_clip = surfaceToClip(wh.x, wh.y, scene_ratio);                

                active_warp_point = ActiveWarpPoint(mousespace_xy,
                                                    mul(clip_to_mouse, mul(surface_to_clip, view_to_surface)),
                                                    done_update_mask, 10);
                ForEachTransformTraversal transform_traversal;
                scene->traverse(
                    VisitorPtr(new GetActiveWarpPoint(active_warp_point)),
                    transform_traversal);
                if (active_warp_point.transform) {
                    return;
                }
            }
            if (!show_control_points || modifiers) {
                anchor_x = x;
                anchor_y = y;
                rotate_x = x;
                scale_y = y;
                if (verbose) {
                    printf("anchor = %f,%f\n", x, y);
                }
                zooming = true;
                rotating = true;
                // Only SHIFT means just rotate (no zooming).
                if (modifiers == GLUT_ACTIVE_SHIFT) {
                    zooming = 0;
                }
                // Only CTRL means just zoom (no rotating).
                if (modifiers == GLUT_ACTIVE_CTRL) {
                    rotating = 0;
                }
            } else {
                float w = wh.x,
                      h = wh.y;
                float2 mousespace_xy = float2(mouse_space_x, mouse_space_y);
                float4x4 clip_to_mouse = float4x4(w/2,0,0,w/2,
                    0,-h/2,0,h/2, // compute "h-y" to get to GL's lower-left origin
                    0,0,1,0,
                    0,0,0,1);

                int done_update_mask = glutGetModifiers() & GLUT_ACTIVE_ALT
                                     ? BOTH_WINDOWS
                                     : update_mask;
                float4x4 surface_to_clip = surfaceToClip(wh.x, wh.y, scene_ratio);

                active_control_point = ActiveControlPoint(mousespace_xy,
                                                          mul(clip_to_mouse, mul(surface_to_clip, view_to_surface)),
                                                          done_update_mask, 10);
                ForEachShapeTraversal traversal;
                scene->traverse(
                    VisitorPtr(new GetActiveControlPoint(active_control_point)),
                    traversal);
                if (active_control_point.path) {
                    printf("mouse point=%d (%d,%d)",
                        int(active_control_point.coord_index), mouse_space_x, mouse_space_y);
                    if (active_control_point.coord_usage == ActiveControlPoint::X_AND_Y) {
                        const float *p = &active_control_point.path->coord[active_control_point.coord_index];
                        const float2 xy = float2(p[0],p[1]);
                        std::cout << " xy = " << xy;
                    }
                    printf("\n");
                } else {
                    printf("no hit\n");
                }
            }
        } else {
            if (active_control_point.path) {
                do_redisplay(~active_control_point.update_mask);
                active_control_point.path.reset();
                active_control_point.coord_index = ~0;
            }
            if (active_warp_point.transform) {
                do_redisplay(~active_warp_point.update_mask);
                active_warp_point.transform.reset();
                active_warp_point.ndx = ~0;
            }
            if (zooming || rotating) {
                do_redisplay(~update_mask);
            }
            zooming = false;
            rotating = false;
        }
    }
    if (button == GLUT_MIDDLE_BUTTON) {
        if (state == GLUT_DOWN) {
            if ((modifiers & GLUT_ACTIVE_ALT) &&
                (glutGetWindow() == gl_window)) {
                GLuint stencil;
                GLint x = mouse_space_x,
                      y = gl_window_height - mouse_space_y;

                glReadPixels(x, y, 1,1, GL_STENCIL_INDEX, GL_UNSIGNED_INT, &stencil);
                printf("stencil @ (%d,%d) = 0x%x (%d)\n", x, y, stencil, stencil);
                return;
            }
            if (modifiers & GLUT_ACTIVE_CTRL) {
                slide_scale = 1.0/8.0;
            } else {
                slide_scale = 1.0;
            }
            slide_y = mouse_space_y;
            slide_x = mouse_space_x;
            sliding = true;
        } else {
            sliding = false;
            do_redisplay(~update_mask);
        }
        if (verbose) {
            printf("sliding: %d for %f,%f\n", sliding, slide_x, slide_y);
        }
    }
}

void
motion(int mouse_space_x, int mouse_space_y)
{
    if (glutGetWindow() == gl_container) {
        glutSetWindow(gl_window);
    }
    const bool isSWwindow = glutGetWindow() == sw_window;
    const float2 wh = isSWwindow
                    ? float2(sw_window_width,sw_window_height)
                    : float2(gl_window_width,gl_window_height);
    const int update_mask = isSWwindow ? SW_WINDOW : GL_WINDOW;
    const float x = mouse_space_x,
                y = wh.y - mouse_space_y;
    if (active_control_point.path) {
        float2 mousespace_xy = float2(mouse_space_x, mouse_space_y);
        active_control_point.set(mousespace_xy);
        have_dlist = false;
        software_window_valid = false;
        do_redisplay(active_control_point.update_mask);
    }
    if (active_warp_point.transform) {
        float2 mousespace_xy = float2(mouse_space_x, mouse_space_y);
        active_warp_point.set(mousespace_xy);
        have_dlist = false;
        software_window_valid = false;
        do_redisplay(active_warp_point.update_mask);
    }
    if (zooming || rotating) {
        float angle = 0;
        float zoom = 1;
        if (rotating) {
            angle = 0.003 * (x - rotate_x);
        }
        if (zooming) {
            zoom = pow(2, (scale_y - y) * 2.0/wh.y);
        }
        float xx = anchor_x*2.0/wh.x - 1.0,
              yy = anchor_y*2.0/wh.y - 1.0;

        const float2 scale = clipToSurfaceScales(wh.x, wh.y, scene_ratio);
        xx /= scale.x;
        yy /= scale.y;

        float4x4 t, r, s, m;
        t = float4x4(1,0,0,xx,
                     0,1,0,yy,
                     0,0,1,0,
                     0,0,0,1);
        r = float4x4(cos(angle), -sin(angle), 0, 0,
                     sin(angle), cos(angle), 0, 0,
                     0,0,1,0,
                     0,0,0,1);
        s = float4x4(zoom,0,0,0,
                     0,zoom,0,0,
                     0,0,1,0,
                     0,0,0,1);
        r = mul(r,s);
        m = mul(t,r);
        t = float4x4(1,0,0,-xx,
                     0,1,0,-yy,
                     0,0,1,0,
                     0,0,0,1);
        m = mul(m,t);
        view = mul(m,view);
        if (verbose) {
            printf("zoom = %f\n", zoom);
            printf("angle = %f\n", angle);
            std::cout << "view = " << view << std::endl;
        }
        rotate_x = x;
        scale_y = y;
        updateTransform();
        software_window_valid = false;
        do_redisplay(update_mask);
    }
    if (sliding) {
        float x_offset = (mouse_space_x - slide_x) * 2.0/wh.x;
        float y_offset = (slide_y - mouse_space_y) * 2.0/wh.y;
        x_offset *= slide_scale;
        y_offset *= slide_scale;
        const float2 scale = clipToSurfaceScales(wh.x, wh.y, scene_ratio);
        x_offset /= scale.x;
        y_offset /= scale.y;
        float4x4 m = float4x4(1,0,0,x_offset,
                              0,1,0,y_offset,
                              0,0,1,0,
                              0,0,0,1);

        view = mul(m, view);
        slide_y = mouse_space_y;
        slide_x = mouse_space_x;
        if (verbose) {
            std::cout << "view = " << view << std::endl;
        }
        updateTransform();
        software_window_valid = false;
        do_redisplay(update_mask);
    }
}
