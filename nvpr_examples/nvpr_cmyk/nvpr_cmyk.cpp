
// nvpr_cmyk.cpp - CMYK rendering of PostScript tiger

// Copyright (c) NVIDIA Corporation. All rights reserved.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glew.h>
#ifdef __APPLE__
# include <GLUT/glut.h>
#else
# include <GL/glut.h>
# ifdef _WIN32
#  include <windows.h>  // for wglGetProcAddress
# else
#  include <GL/glx.h>  // for glXGetProcAddress
# endif
#endif

#include <string>
#include <vector>
#include <algorithm>

#include <Cg/double.hpp>
#include <Cg/vector/xyzw.hpp>
#include <Cg/vector/rgba.hpp>
#include <Cg/vector.hpp>
#include <Cg/matrix.hpp>
#include <Cg/mul.hpp>
#include <Cg/min.hpp>
#include <Cg/max.hpp>
#include <Cg/iostream.hpp>
#include <Cg/stdlib.hpp>

using namespace Cg;

#include "nvpr_glew_init.h"
#include "tiger.h"  // for PostScript tiger rendering
#include "countof.h"
#include "gl_debug_callback.h"
#include "showfps.h"  // for reporting "frames per second" performance
#include "request_vsync.h"
#include "sRGB_math.h"
#include "srgb_table.h"
#include "gl_framebuffer.hpp"
#include "gl_program.hpp"
#include "cg4cpp_xform.hpp"
#include "parse_fms_mode.hpp"

// Undef these because <windows.h> and other headers set these; we want the Cg::min and Cg::max functions.
#undef min
#undef max

const char *program_name = "nvpr_cmyk";
int stroking = 1,
    filling = 1;
bool animating = false;
bool sRGB_blending = true;
bool CMYK_rendering = true;
bool spot_colors = true;
bool use_supersample = false;
bool use_dlist = false;
bool dirty_dlist = true;
bool draw_bounds = false;
bool force_stencil_clear = false;
int visualize_channels = 0;
bool checkerboard_clear = true;
bool frame_sync = false;

bool cmyk12[6] = { 1,1,1,1, 1,1 };

/* Scaling and rotation state. */
float anchor_x = 0,
      anchor_y = 0;  /* Anchor for rotation and scaling. */
int scale_y = 0, 
    rotate_x = 0;  /* Prior (x,y) location for scaling (vertical) or rotation (horizontal)? */
int zooming = 0;  /* Are we zooming currently? */
int scaling = 0;  /* Are we scaling (zooming) currently? */

/* Sliding (translation) state. */
float2 slide_xy = float2(0,0);  // Prior (x,y) location for sliding.
int sliding = 0;  /* Are we sliding currently? */
float slide_scale = 1.0;
unsigned int path_count;
int canvas_width = 640, canvas_height = 480;
int black_backdrop = 1;

int initial_stencil_samples = 8;
int initial_color_samples = 1;

float tiger_bounds[4] = { 0, 0, 0, 0 };

float3x3 model, view;

FPScontext gl_fps_context;

float window_width, window_height;

GLFramebufferCapabilities capabilities;
GLFramebuffer *fb;

float aspect_ratio;
float view_width, view_height;
float3x3 win2obj;

float spin_direction = 1;

GLSLProgram prog_Constant_OneMinusCMYK_A;
GLSLProgram prog_Constant_OneMinusCMYK12_A;
GLSLProgram prog_Constant_RGBA;
GLSLProgram prog_Tex_CMYA_KA_to_RGBA;
GLSLProgram prog_Tex_CMYA_K12A_to_RGBA;
GLSLProgram prog_Tex_CMYA_K12A_to_cyan;
GLSLProgram prog_Tex_CMYA_K12A_to_magenta;
GLSLProgram prog_Tex_CMYA_K12A_to_yellow;
GLSLProgram prog_Tex_CMYA_K12A_to_black;
GLSLProgram prog_Tex_CMYA_K12A_to_spot1;
GLSLProgram prog_Tex_CMYA_K12A_to_spot2;
GLSLProgram prog_Tex_CMYA_K12A_to_CMY_as_RGB;
GLSLProgram prog_Tex_CMYA_K12A_to_K12_as_RGB;

static void fatalError(const char *message)
{
  fprintf(stderr, "%s: %s\n", program_name, message);
  exit(1);
}

static const GLfloat nice_blue_clear[3] = { 0.1f, 0.3f, 0.6f };

void initColorModel()
{
  static const GLfloat fps[3] = { 0.2f, 0.9f, 0.4f };

  if (CMYK_rendering) {
    glDisable(GL_FRAMEBUFFER_SRGB);
    colorFPS(fps[0], fps[1], fps[2]);
    if (fb) {
      fb->setSlices(2);
    }
  } else {
    if (fb) {
      fb->setSlices(1);
    }
    if (sRGB_blending) {
      colorFPS(
        convertSRGBColorComponentToLinearf(fps[0]),
        convertSRGBColorComponentToLinearf(fps[1]),
        convertSRGBColorComponentToLinearf(fps[2]));
      glEnable(GL_FRAMEBUFFER_SRGB);
    } else {
      colorFPS(fps[0], fps[1], fps[2]);
      glDisable(GL_FRAMEBUFFER_SRGB);
    }
  }
}

float3 convertCMYK2RGB(float4 cmyk)
{
  float3 inv_rgb = cmyk.xyz + cmyk.w;
  float3 rgb = 1-min(1, inv_rgb);
  return rgb;
}

// BG = black-generation function
float BG(float k)
{
  // The black-generation function computes the black component as a function of
  // the nominal k value. It can simply return its k operand unchanged, or it can
  // return a larger value for extra black, a smaller value for less black, or 0.0 for no
  // black at all.
  return k;  // implement as identity function
}

// UCR = undercolor-removal function
float UCR(float k)
{
  // The undercolor-removal function computes the amount to subtract from each of 
  // the intermediate c, m, and y values to produce the final cyan, magenta, and yellow
  // components. It can simply return its k operand unchanged, or it can return 0.0
  // (so that no color is removed), some fraction of the black amount, or even a
  // negative amount, thereby adding to the total amount of colorant.
  return k;  // implement as identity function
}

// See section 6.2.3 "Conversion from DeviceRGB to DeviceCMYK" in PDF 1.7 specification.
float4 rgb2cmyk(float3 rgb)
{
  float3 cmy = 1-rgb;
  float k = min(cmy[0], min(cmy[1], cmy[2]));
  float3 cyan_yellow_magenta = saturate(cmy - UCR(k));
  float black = BG(k);
  return float4(cyan_yellow_magenta,black);
}

const float3 white = float3(1,1,1);
const float3 pink_nose = float3(0.783538,0.318547,0.318547);

void mySendColor(GLuint color)
{
  GLubyte red = (color>> 16)&0xFF,
          green = (color >> 8)&0xFF,
          blue  = (color >> 0)&0xFF;
  if (CMYK_rendering) {
    float3 rgb = float3(SRGB_to_LinearRGB[red],
                        SRGB_to_LinearRGB[green],
                        SRGB_to_LinearRGB[blue]);
    float spot1 = 0;  // white
    float spot2 = 0;  // nose pink

    float4 cmyk = rgb2cmyk(rgb);
    const float4 cmyk_black = float4(0,0,0,1);  // CMY=0, blacK=1
    if (spot_colors) {
      // This switch table is tied to 32-bit encoded colors used in tiger_style.h
      switch (color) {
      case 0xcccccc:  // 60% white
        spot1 = 0.603827;
        cmyk = cmyk_black;
        break;
      case 0xffffff:  // pure white
        spot1 = 1;
        cmyk = cmyk_black;
        break;
      case 0xe5e5b2:  // shadowed tiger teeth cream
#if 0
        spot1 = 0.445201;
        rgb -= spot1;
#endif
        break;
      case 0xe59999:  // pink nose
        spot2 = 1;
        cmyk = cmyk_black;
        break;
      case 0xb26565:  // dark tiger pink nose
#if 0
        spot2 = 0.408531;
        rgb -= spot1;
        rgb.bg = float2(0);
#endif
        break;
      default:
        // Use process colors
        break;
      }
    }
    if ((spot1 != 0) || (spot2 != 0)) {
      prog_Constant_OneMinusCMYK12_A.use();
      prog_Constant_OneMinusCMYK12_A.setUniform4f("CMYK", cmyk[0], cmyk[1], cmyk[2], cmyk[3]);
      prog_Constant_OneMinusCMYK12_A.setUniform2f("spot12", spot1, spot2);
      prog_Constant_OneMinusCMYK12_A.setUniform1f("alpha", 1);
    } else {
      prog_Constant_OneMinusCMYK_A.use();
      prog_Constant_OneMinusCMYK_A.setUniform4f("CMYK", cmyk[0], cmyk[1], cmyk[2], cmyk[3]);
      prog_Constant_OneMinusCMYK_A.setUniform1f("alpha", 1);
    }
  } else {
    // RGB rendering
    if (sRGB_blending) {
      glColor3f(SRGB_to_LinearRGB[red], SRGB_to_LinearRGB[green], SRGB_to_LinearRGB[blue]);
    } else {
      glColor3ub(red, green, blue);
    }
  }
}

void initGraphics()
{
  /* Before rendering to a window with a stencil buffer, clear the stencil
  buffer to zero and the color buffer to blue: */
  glClearStencil(0);

  initColorModel();
  initTiger();
  setTigerSendColorFunc(mySendColor);

  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_NOTEQUAL, 0, 0x1F);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
}

void initModelAndViewMatrices(float scene_bounds[4])
{
  float2 corner1 = float2(scene_bounds[0], scene_bounds[1]);
  float2 corner2 = float2(scene_bounds[2], scene_bounds[3]);
  float2 center = (corner1+corner2)/2;
  model = mul(mul(translate(center), scale(0.9,0.9)), translate(-center));
  view = translate(0, 0);
}

// scene_bounds = (x_min,y_min,x_max,y_max)
void configureProjection(float scene_bounds[4])
{
  float3x3 inverse_projection, viewport;

  viewport = ortho(0,window_width, 0,window_height);
  // Start with initial scene bounds
  float2 corner1 = float2(scene_bounds[0], scene_bounds[1]);
  float2 corner2 = float2(scene_bounds[2], scene_bounds[3]);

  float2 scene_dims = corner2 - corner1;
  view_width = corner2.x - corner1.x;
  view_height = corner2.y - corner1.y;
  float view_ratio = view_height/view_width;
  assert(scene_dims.x != 0);
  assert(scene_dims.y != 0);
  if ((scene_dims.x > 0) ^ (scene_dims.y > 0)) {
    spin_direction = 1;
  } else {
    spin_direction = -1;
  }
  float2 center = (corner1+corner2)/2;
  // Now readjust to make the view "square"
  if (scene_dims.x > scene_dims.y) {
    scene_dims.y = scene_dims.x;
  } else {
    scene_dims.x = scene_dims.y;
  }
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  // Adjust for window aspect ratio
  float xaspect = aspect_ratio/view_ratio < 1 ? aspect_ratio/view_ratio : 1;
  float yaspect = aspect_ratio/view_ratio < 1 ? 1 : view_ratio/aspect_ratio;
  glScalef(xaspect, yaspect, 1);  // Correct for window aspect ratio
  float left = center.x - view_width/2;
  float right = center.x + view_width/2;
  float top = center.y - view_height/2;
  float bottom = center.y + view_height/2;
  glOrtho(left, right, bottom, top, -1, 1);
  inverse_projection = mul(inverse_ortho(left, right, top, bottom), scale(1/xaspect, 1/yaspect));
  win2obj = mul(inverse_projection, viewport);
}

static void reshape(int w, int h)
{
  reshapeFPScontext(&gl_fps_context, w, h);
  glViewport(0,0, w,h);
  window_width = w;
  window_height = h;
  aspect_ratio = window_height/window_width;

  if (fb) {
    fb->resize(w, h);
  } else {
    fb = new GLFramebuffer(capabilities,
      //GL_SRGB8_ALPHA8,
      GL_RGBA8,
      GL_STENCIL_INDEX8,
      w, h, CMYK_rendering ? 2 : 1,
      initial_stencil_samples, initial_color_samples,
      use_supersample ? GLFramebuffer::Supersample : GLFramebuffer::Resolve);
    fb->setAlphaHandling(GLFramebuffer::PremultipliedAlpha);
  }

  configureProjection(tiger_bounds);
  force_stencil_clear = true;
}

void drawTiger()
{
  glEnable(GL_STENCIL_TEST);
  fb->blendConfiguration();
  if (CMYK_rendering) {
    drawTigerRange(filling, stroking, 0, path_count);
    glUseProgram(0);
  } else {
    glUseProgram(0);
    drawTigerRange(filling, stroking, 0, path_count);
  }
}

void fastDrawTiger()
{
  GLuint tiger_dlist = 1;
  if (dirty_dlist) {
    glNewList(tiger_dlist, GL_COMPILE); {
      drawTiger();
    } glEndList();
    dirty_dlist = false;
  }
  glCallList(tiger_dlist);
}

void drawBounds(float bounds[4])
{
  glDisable(GL_STENCIL_TEST);
  glEnable(GL_LINE_STIPPLE);
  if (fb->isSupersampled()) {
    glLineWidth(2);
    glLineStipple(8, 0x5555);
  } else {
    glLineWidth(1);
    glLineStipple(4, 0x5555);
  }
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glColor3f(1,1,0);  // yellow
  glBegin(GL_QUADS); {
    glVertex2f(bounds[0], bounds[1]);
    glVertex2f(bounds[2], bounds[1]);
    glVertex2f(bounds[2], bounds[3]);
    glVertex2f(bounds[0], bounds[3]);
  } glEnd();
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void drawScene()
{
  glMatrixPushEXT(GL_MODELVIEW); {
    float3x3 mat = mul(view, model);
    MatrixLoadToGL(mat);
    if (draw_bounds) {
      drawBounds(tiger_bounds);
    }
    if (use_dlist) {
      fastDrawTiger();
    } else {
      drawTiger();
    }
  } glMatrixPopEXT(GL_MODELVIEW);
}

static void renderFullscreenQuad()
{
  glBegin(GL_QUADS); {
    glVertex2f(-1,-1);
    glVertex2f(+1,-1);
    glVertex2f(+1,+1);
    glVertex2f(-1,+1);
  } glEnd();
}

// When rendering "conventional" geometry when there are more coverage
// samples than color samples, we want to rasterize at the color sample
// rate even if there are more coverage (stencil) samples.  This assumes
// we are not stencil or depth testing.
static void ensureColorSampleRateRendering(int cov_samples, int color_samples)
{
  if (cov_samples > color_samples) {
    if (color_samples > 1) {
      // Assume no depth or stencil testing when GL_RASTER_MULTISAMPLE_EXT
      // is enabled.
      glEnable(GL_RASTER_MULTISAMPLE_EXT);
      const GLboolean fixed_locations = GL_TRUE;
      glRasterSamplesEXT(color_samples, fixed_locations);
    } else {
      // If we just have 1 color sample, running with
      // GL_RASTER_MULTISAMPLE_EXT enabled actually acts like 2 samples.
      // This is due to a technicality where there's no such thing as "1
      // sample" multisampling so glRasterSamplesEXT(1, fixed_locations)
      // doesn't render to 1 sample like you think it might.  Instead just
      // disable multisampling.
      glDisable(GL_MULTISAMPLE);
    }
  }
}

// Undo what ensureColorSampleRateRendering might have done.
static void resumeNormalRateRendering(int cov_samples, int color_samples)
{
  if (cov_samples > color_samples) {
    if (color_samples > 1) {
      glDisable(GL_RASTER_MULTISAMPLE_EXT);
    } else {
      glEnable(GL_MULTISAMPLE);
    }
  }
}

static void drawCheckers(int w, int h, int scale, int cov_samples, int color_samples)
{
  glUseProgram(0);
  glViewport(0, 0, w, h);
  glDisable(GL_STENCIL_TEST);
  // Make sure we are rendering at color sample rate.
  // Otherwise, we could double-hit color samples on the checker diagonals.
  ensureColorSampleRateRendering(cov_samples, color_samples); {
    glDisable(GL_BLEND);
    glMatrixPushEXT(GL_PROJECTION); {
      glMatrixLoadIdentityEXT(GL_PROJECTION);
      glMatrixOrthoEXT(GL_PROJECTION, 0, w, 0, h, -1, 1);
      const int tile_w = scale*16;
      const int tile_h = scale*16;
      const float square_gray = 0.4;
      if (CMYK_rendering || !sRGB_blending) {
        float c = square_gray;
        glColor4f(c,c,c, 0);
      } else {
        float c = convertSRGBColorComponentToLinearf(square_gray);
        glColor4f(c,c,c, 0);
      }
      glBegin(GL_QUADS); {
        int row = 0;
        for (int j=0; j<h; j += tile_h) {
          row = !row;
          for (int i=row&1 ? 0 : tile_w; i<w; i += 2*tile_w) {
            glVertex2i(i,j);
            glVertex2i(i+tile_w,j);
            glVertex2i(i+tile_w,j+tile_h);
            glVertex2i(i,j+tile_h);
          }
        }
      } glEnd();
    } glMatrixPopEXT(GL_PROJECTION);
  } resumeNormalRateRendering(cov_samples, color_samples);
}

void renderFBO()
{
  // Bind to framebuffer's framebuffer object (FBO).
  fb->bind();
  // Clear color (and stencil if necessary) buffers...
  GLbitfield clear_mask = GL_COLOR_BUFFER_BIT;
  if (force_stencil_clear) {
    // Slight performance optimization: avoid stencil clear if stencil buffer isn't undefined
    // due to a initial allocation, resize, or damage.
    clear_mask |= GL_STENCIL_BUFFER_BIT;
    force_stencil_clear = false;
  }
  glClear(clear_mask);
  drawScene();
}

void pickCMYKConversionShader()
{
  if (visualize_channels > 0) {
    GLSLProgram *p = NULL;
    switch (visualize_channels) {
    case 1:
      p = &prog_Tex_CMYA_K12A_to_cyan;
      break;
    case 2:
      p = &prog_Tex_CMYA_K12A_to_magenta;
      break;
    case 3:
      p = &prog_Tex_CMYA_K12A_to_yellow;
      break;
    case 4:
      p = &prog_Tex_CMYA_K12A_to_black;
      break;
    case 5:
      p = &prog_Tex_CMYA_K12A_to_spot1;
      break;
    case 6:
      p = &prog_Tex_CMYA_K12A_to_spot2;
      break;
    case 7:
      p = &prog_Tex_CMYA_K12A_to_CMY_as_RGB;
      break;
    case 8:
      p = &prog_Tex_CMYA_K12A_to_K12_as_RGB;
      break;
    default:
      assert(!"bad visualize_channels");
    }
    p->use();
    const GLuint tex_unit_zero = 0; 
    p->bindTexture("sampler_CMYA_K12A",
      fb->DownsampleTexture(), fb->DownsampleTarget(), tex_unit_zero);
  } else {
    if (spot_colors) {
      GLSLProgram &p = prog_Tex_CMYA_K12A_to_RGBA;
      p.use();
      p.setUniform4f("CMYK_Mask", cmyk12[0],cmyk12[1],cmyk12[2],cmyk12[3]);
      if (cmyk12[4]) {
        p.setUniform3f("spot1_rgb", white.r, white.g, white.b);
      } else {
        p.setUniform3f("spot1_rgb", 0,0,0);
      }
      if (cmyk12[5]) {
        p.setUniform3f("spot2_rgb", pink_nose.r, pink_nose.g, pink_nose.b);
      } else {
        p.setUniform3f("spot2_rgb", 0,0,0);
      }
      const GLuint tex_unit_zero = 0; 
      p.bindTexture("sampler_CMYA_K12A",
        fb->DownsampleTexture(), fb->DownsampleTarget(), tex_unit_zero);
    } else {
      GLSLProgram &p = prog_Tex_CMYA_KA_to_RGBA;
      p.use();
      p.setUniform4f("CMYK_Mask", cmyk12[0],cmyk12[1],cmyk12[2],cmyk12[3]);
      const GLuint tex_unit_zero = 0; 
      p.bindTexture("sampler_CMYA_KA",
        fb->DownsampleTexture(), fb->DownsampleTarget(), tex_unit_zero);
    }
  }
}

void niceClear(GLbitfield clear_mask, int w, int h, int scale, int cov_samples, int color_samples)
{
  if (checkerboard_clear) {
    const float background_gray = 0.2;
    if (CMYK_rendering || !sRGB_blending) {
      float c = background_gray;
      glClearColor(c,c,c, 0);
    } else {
      float c = convertSRGBColorComponentToLinearf(background_gray);
      glClearColor(c,c,c, 0);
    }
    glClear(clear_mask);
    drawCheckers(w, h, scale, cov_samples, color_samples);
  } else {
    if (CMYK_rendering || !sRGB_blending) {
      glClearColor(nice_blue_clear[0], nice_blue_clear[1], nice_blue_clear[2], 0.0);
    } else {
      glClearColor(
        convertSRGBColorComponentToLinearf(nice_blue_clear[0]),
        convertSRGBColorComponentToLinearf(nice_blue_clear[1]),
        convertSRGBColorComponentToLinearf(nice_blue_clear[2]),
        0.0);
    }
    glClear(clear_mask);
  }
}

static void doCMYKConversion()
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  if (fb->isSupersampled()) {
    glViewport(0, 0, window_width, window_height);
  }
  const int one_color_sample = 1;
  const int one_cov_sample = 1;
  const int scale = 1;
  niceClear(GL_COLOR_BUFFER_BIT,
    window_width, window_height,
    scale, one_cov_sample, one_color_sample);  // No stencil clearing needed.
  pickCMYKConversionShader();
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_FRAMEBUFFER_SRGB);
  renderFullscreenQuad();
  glDisable(GL_FRAMEBUFFER_SRGB);
}

static void drawAnyOverlays()
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  if (fb->isSupersampled()) {
    glViewport(0,0, window_width, window_height);
    glUseProgram(0);
    handleFPS(&gl_fps_context);
    fb->viewport();
  } else {
    glUseProgram(0);
    handleFPS(&gl_fps_context);
  }
}

void display()
{
  if (CMYK_rendering) {
    glClearColor(1,1,1,0);  // clear colors to inverted "zero"
    renderFBO();
    fb->resolve();
    doCMYKConversion();
  } else {
    // Bind to framebuffer's framebuffer object (FBO).
    fb->bind();
    // Clear color (and stencil if necessary) buffers...
    GLbitfield clear_mask = GL_COLOR_BUFFER_BIT;
    if (force_stencil_clear) {
      // Slight performance optimization: avoid stencil clear if stencil buffer isn't undefined
      // due to a initial allocation, resize, or damage.
      clear_mask |= GL_STENCIL_BUFFER_BIT;
      force_stencil_clear = false;
    }
    niceClear(clear_mask,
      fb->RenderWidth(), fb->RenderHeight(),
      fb->RenderScale(), fb->CoverageSamples(), fb->ColorSamples());
    drawScene();
    fb->copy();
  }
  drawAnyOverlays();

  glutSwapBuffers();
}

void animate()
{
  glutPostRedisplay();
}

static const char *visualize_channels_str[] = {
  "normal",
  "cyan",
  "magenta",
  "yellow",
  "black (key)",
  "white (spot 1)",
  "pink nose (spot 2)",
  "CMY as RGB",
  "K12 as RGB",
};

void updateVisualizeChannels()
{
  printf("visualize_channels = %s (%d)\n", visualize_channels_str[visualize_channels], visualize_channels);
  dirty_dlist = true;
  invalidateFPS();
}

void updateAnimation()
{
  if (animating) {
    glutIdleFunc(animate);
    invalidateFPS();
    enableFPS();
  } else {
    glutIdleFunc(NULL);
    disableFPS();
  }
}

static void updateBlackBackdrop()
{
  prog_Tex_CMYA_K12A_to_cyan.set1i("black_backdrop", black_backdrop);
  prog_Tex_CMYA_K12A_to_magenta.set1i("black_backdrop", black_backdrop);
  prog_Tex_CMYA_K12A_to_yellow.set1i("black_backdrop", black_backdrop);
  prog_Tex_CMYA_K12A_to_black.set1i("black_backdrop", black_backdrop);
  prog_Tex_CMYA_K12A_to_spot1.set1i("black_backdrop", black_backdrop);
  prog_Tex_CMYA_K12A_to_spot2.set1i("black_backdrop", black_backdrop);
  invalidateFPS();
}

void keyboard(unsigned char c, int x, int y)
{
  switch (c) {
  case 27:  /* Esc quits */
    delete fb;
    exit(0);
    return;
  case 'a':
    black_backdrop = !black_backdrop;
    printf("black_backdrop = %d\n", black_backdrop);
    updateBlackBackdrop();
    break;
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
    cmyk12[c-'1'] = !cmyk12[c-'1'];
    printf("CMYK12 mask:");
    for (int i=0; i<6; i++) {
      if (cmyk12[i]) {
        printf(" %c", "CMYK12"[i]);
      } else {
        printf(" _");
      }
    }
    printf("\n");
    dirty_dlist = true;
    break;
  case 'V':
    visualize_channels--;
    if (visualize_channels < 0) {
      visualize_channels = countof(visualize_channels_str)-1;
    }
    updateVisualizeChannels();
    break;
  case 'v':
    visualize_channels = (visualize_channels+1)%countof(visualize_channels_str);
    updateVisualizeChannels();
    break;
  case 'F':
    frame_sync = !frame_sync;
    printf("frame_sync = %d\n", frame_sync);
    requestSynchornizedSwapBuffers(frame_sync);
    invalidateFPS();
    break;
  case 'r':
    visualize_channels = 0;
    updateVisualizeChannels();
    break;
  case 'c':
    CMYK_rendering = !CMYK_rendering;
    initColorModel();
    printf("Mode: %s\n", CMYK_rendering ? "CMYK" : "RGB");
    printf("  CMYK_rendering = %d\n", CMYK_rendering);
    dirty_dlist = true;
    invalidateFPS();
    break;
  case 'x':
    checkerboard_clear = !checkerboard_clear;
    printf("checkerboard_clear = %d\n", checkerboard_clear);
    break;
  case 'z':
    printf("Requesting GL_DEPTH24_STENCIL8 format...\n");
    if (fb->CoverageSamples() != fb->ColorSamples()) {
      assert(capabilities.hasFramebufferMixedSamples());
      printf("Warning: GL_DEPTH24_STENCIL8 forces same number of color & coverage samples!\n");
    }
    fb->setDepthStencilFormat(GL_DEPTH24_STENCIL8);
    force_stencil_clear = true;
    invalidateFPS();
    break;
  case 'Z':
    printf("Requesting GL_STENCIL_INDEX8 format...\n");
    fb->setDepthStencilFormat(GL_STENCIL_INDEX8);
    force_stencil_clear = true;
    invalidateFPS();
    break;
  case 'i':
    {
      float megabyte = float(1<<20);
      unsigned int fb_size = fb->framebufferSize();
      float fb_megs = fb_size / megabyte;
      unsigned int total_size = fb->totalSize();
      float total_megs = total_size / megabyte;
      printf("Mode: %s\n", CMYK_rendering ? "CMYK" : "RGB");
      if (sRGB_blending) {
        printf("sRGB-correct blending\n");
      } else {
        printf("conventional blending (non-sRGB-correct)\n");
      }
      printf("Format: %s%d:%d\n",
        fb->Supersampled() ? "4x" : "",
        fb->CoverageSamples(), fb->ColorSamples());
      printf("Slices: %d\n", fb->Slices());
      printf("bits per color sample = %u\n", fb->bitsPerColorSample());
      printf("bits per coverage sample = %u\n", fb->bitsPerCoverageSample());
      printf("framebuffer size = %u bytes\n", fb_size);
      printf("total size = %u bytes\n", total_size);
      printf("framebuffer megabytes = %.2f\n", fb_megs);
      printf("total megabytes = %.2f\n", total_megs);
    }
    return;  // no redisplay needed
  case ' ':
    animating = !animating;
    updateAnimation();
    break;
  case 'd':
    use_dlist = !use_dlist;
    printf("use_dlist = %d\n", use_dlist);
    invalidateFPS();
    break;
  case 13:  /* Enter redisplays */
    break;
  case 'b':
    sRGB_blending = !sRGB_blending;
    if (sRGB_blending) {
      printf("sRGB-correct blending\n");
    } else {
      printf("conventional blending (non-sRGB-correct)\n");
    }
    initColorModel();
    dirty_dlist = true;
    invalidateFPS();
    break;
  case 'B':
    draw_bounds = !draw_bounds;
    printf("use_dlist = %d\n", draw_bounds);
    dirty_dlist = true;
    invalidateFPS();
    break;
  case 'p':
    spot_colors = !spot_colors;
    printf("spot_colors = %d\n", spot_colors);
    dirty_dlist = true;
    invalidateFPS();
    break;
  case 'S':
    use_supersample = !use_supersample;
    if (use_supersample) {
      fb->setBufferMode(GLFramebuffer::Supersample);
    } else if (fb->ColorSamples() > 1) {
      fb->setBufferMode(GLFramebuffer::Resolve);
    } else {
      fb->setBufferMode(GLFramebuffer::Normal);
    }
    printf("supersampe = %d\n", use_supersample);
    invalidateFPS();
    force_stencil_clear = true;
    break;
  case 's':
    stroking = !stroking;
    dirty_dlist = true;
    invalidateFPS();
    break;
  case 'f':
    filling = !filling;
    dirty_dlist = true;
    invalidateFPS();
    break;
  case 'R':
    initModelAndViewMatrices(tiger_bounds);
    invalidateFPS();
    break;
  case '+':
    path_count++;
    dirty_dlist = true;
    invalidateFPS();
    break;
  case '-':
    path_count--;
    dirty_dlist = true;
    invalidateFPS();
    break;
  default:
    return;
  }
  glutPostRedisplay();
}

void
mouse(int button, int state, int mouse_space_x, int mouse_space_y)
{
  const int modifiers = glutGetModifiers() & (GLUT_ACTIVE_CTRL|GLUT_ACTIVE_SHIFT);

  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {

      float2 win = float2(mouse_space_x, mouse_space_y);
      float3 tmp = mul(win2obj, float3(win,1));
      anchor_x = tmp.x/tmp.z;
      anchor_y = tmp.y/tmp.z;

      rotate_x = mouse_space_x;
      scale_y = mouse_space_y;
      if (modifiers & GLUT_ACTIVE_CTRL) {
        scaling = 0;
      } else {
        scaling = 1;
      }
      if (modifiers & GLUT_ACTIVE_SHIFT) {
        zooming = 0;
      } else {
        zooming = 1;
      }
    } else {
      zooming = 0;
      scaling = 0;
    }
  }
  if (button == GLUT_MIDDLE_BUTTON) {
    if (state == GLUT_DOWN) {
      float2 win = float2(mouse_space_x, mouse_space_y);
      float3 slide_xyw = mul(win2obj, float3(win,1));
      slide_xy = slide_xyw.xy/slide_xyw[2];  // divide by "w"

      if (modifiers & GLUT_ACTIVE_CTRL) {
        slide_scale = 1.0/8.0;
      } else {
        slide_scale = 1.0;
      }
      sliding = 1;
    } else {
      sliding = 0;
    }
  }
}

void
motion(int mouse_space_x, int mouse_space_y)
{
  if (zooming || scaling) {
    float angle = 0;
    float zoom = 1;
    if (scaling) {
      angle = 0.3 * (mouse_space_x - rotate_x);
    }
    if (zooming) {
      zoom = pow(1.003f, (mouse_space_y - scale_y));
    }
    angle *= spin_direction;

    float3x3 t = translate(anchor_x, anchor_y);
    float3x3 r = rotate(angle);
    const float3x3 s = scale(zoom, zoom);

    r = mul(r, s);
    float3x3 m = mul(t, r);
    t = translate(-anchor_x, -anchor_y);
    m = mul(m, t);
    view = mul(m, view);
    rotate_x = mouse_space_x;
    scale_y = mouse_space_y;
    glutPostRedisplay();
  }
  if (sliding) {
    const float2 win = float2(mouse_space_x, mouse_space_y);
    const float3 new_xyw = mul(win2obj, float3(win,1));
    const float2 new_xy = new_xyw.xy/new_xyw[2];  // divide by "w"
    const float2 offset = (new_xy - slide_xy) * slide_scale;
    const float3x3 m = translate(offset);
    view = mul(m, view);
    slide_xy = new_xy;
    glutPostRedisplay();
  }
}

static void menu(int choice)
{
  keyboard(choice, 0, 0);
}

class GLQualitySetting {
private:
  int coverage_samples;
  int color_samples;
  bool supersampled;

  int menu_value;
  std::string name;
  int effective_coverage_samples;
  int effective_color_samples;

public:
  GLQualitySetting(int cov_samples, int col_samples, bool ss) 
    : coverage_samples(cov_samples)
    , color_samples(col_samples)
    , supersampled(ss)
    , effective_coverage_samples(ss ? 4*coverage_samples : coverage_samples)
    , effective_color_samples(ss ? 4*color_samples : color_samples)
  {
    char buffer[100];
    sprintf(buffer, "%s%d:%d (samples=%d)", supersampled ? "4x" : "",
      coverage_samples, color_samples, effective_coverage_samples);
    name = std::string(buffer);
  }

  inline const char* getName() const {
    return name.c_str();
  }
  inline int getColorSamples() const {
    return color_samples;
  }
  inline int getCoverageSamples() const {
    return coverage_samples;
  }
  inline bool getSupersampled() const {
    return supersampled;
  }

  bool operator<( const GLQualitySetting& other) const { 
    if (effective_coverage_samples != other.effective_coverage_samples) {
      return effective_coverage_samples < other.effective_coverage_samples;
    }
    if (effective_color_samples != other.effective_color_samples) {
      return effective_color_samples < other.effective_color_samples;
    }
    if (coverage_samples != other.coverage_samples) {
      return coverage_samples < other.coverage_samples;
    }
    if (color_samples != other.color_samples) {
      return color_samples < other.color_samples;
    }
    return true;
  }
};

typedef std::vector<GLQualitySetting> GLQualitySettingArray;
GLQualitySettingArray quality_settings;

static void selectQuality(int ndx)
{
  const GLQualitySetting &setting = quality_settings[ndx];\
  GLFramebuffer::BufferMode mode = GLFramebuffer::Normal;
  if (setting.getSupersampled()) {
    mode = GLFramebuffer::Supersample;
  } else if (setting.getColorSamples() > 1) {
    mode = GLFramebuffer::Resolve;
  }
  fb->setQuality(setting.getCoverageSamples(),
    setting.getColorSamples(),
    mode);
  force_stencil_clear = true;
  invalidateFPS();
  glutPostRedisplay();
}

static void createMenu(bool hasFramebufferMixedSamples)
{
  // Create "Quality..." submenu
  int quality_menu = glutCreateMenu(selectQuality);
  for (int s=0; s<2; s++) {
    for (int i=1; i<=8; i *= 2) {
      for (int j=i; j<=16; j *= 2) {
        if (!hasFramebufferMixedSamples) {
          if (i != j) {
            break;  // Skip
          }
        }
        quality_settings.push_back(GLQualitySetting(j, i, !!s));
      }
    }
  }
  // Sort by relative quality (lowest quality first)
  std::sort(quality_settings.begin(), quality_settings.end());
  // Populate the submenu with sorted quality settings
  for (size_t i=0; i<quality_settings.size(); i++) {
    const GLQualitySetting &setting = quality_settings[i];
    glutAddMenuEntry(setting.getName(), int(i)); 
  }

  // Create "CMYK mask..." submenu
  int cmyk_mask_menu = glutCreateMenu(menu);
  glutAddMenuEntry("[1] Toggle Cyan mask", '1');
  glutAddMenuEntry("[2] Toggle Magenta mask", '2');
  glutAddMenuEntry("[3] Toggle Yellow mask", '3');
  glutAddMenuEntry("[4] Toggle Black mask", '4');
  glutAddMenuEntry("[5] Toggle Spot 1 (white) mask", '5');
  glutAddMenuEntry("[6] Toggle Spot 2 (pink nose) mask", '6');

  // Create main menu
  glutCreateMenu(menu);
  glutAddSubMenu("Quality...", quality_menu);
  glutAddMenuEntry("[ ] Toggle continuous rendering", ' ');
  glutAddMenuEntry("[c] Toggle CMYK vs RGB", 'c');
  glutAddMenuEntry("[p] Toggle use of 2 spot colors", 'p');
  glutAddMenuEntry("[v] Cycle CMYK visualization", 'v');
  glutAddMenuEntry("[a] Toggle black drop for single CMYK inks", 'a');
  glutAddMenuEntry("[V] Reverse CMYK visualization", 'V');
  glutAddMenuEntry("[f] Toggle filling", 'f');
  glutAddMenuEntry("[s] Toggle stroking", 's');
  glutAddMenuEntry("[b] Toggle sRGB-correct blending (RGB only)", 'b');
  glutAddMenuEntry("[B] Toggle scene bounds", 'B');
  glutAddMenuEntry("[x] Toggle blue vs. checkerboard clear", 'x');
  glutAddMenuEntry("[r] Reset CMYK view", 'r');
  glutAddSubMenu("CMYK mask...", cmyk_mask_menu);
  glutAddMenuEntry("[Esc] Quit", 27);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
}

static const char *glslf_OneMinusCMYK_A = 
"#version 330\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"uniform vec4 CMYK;\n"
"uniform float alpha;\n"
"layout(location=0) out vec4 outCMYA;\n"
"layout(location=1) out vec4 outKA;\n"
"void main() {\n"
"  outCMYA = vec4(1.0-CMYK.xyz,   alpha);\n"
"  outKA   = vec4(1.0-CMYK.w,1,1, alpha);\n"
"}\n";

static const char *glslf_OneMinusCMYK12_A = 
"#version 330\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"uniform vec4 CMYK;\n"
"uniform vec2 spot12;\n"  // two spot colors
"uniform float alpha;\n"
"layout(location=0) out vec4 outCMYA;\n"
"layout(location=1) out vec4 outK12A;\n"
"void main() {\n"
"  outCMYA = vec4(1.0-CMYK.xyz,          alpha);\n"
"  outK12A = vec4(1.0-CMYK.w,1.0-spot12, alpha);\n"
"}\n";

static const char *glslf_RGBA = 
"#version 330\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"uniform vec4 RGBA;\n"
"layout(location=0) out vec4 outRGBA;\n"
"void main() {\n"
"  outRGBA = RGBA;\n"
"}\n";

// Framebuffer resolve shaders

static const char *glslv_Tc = 
"#version 330\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"layout(location=0) in vec2 P;\n"
"layout(location=0) out vec2 TcOut;\n"
"out gl_PerVertex {\n"
"  vec4  gl_Position;\n"
"};\n"
"void main() {\n"
"  TcOut = P*0.5+0.5;\n"
"  gl_Position = vec4(P, 0.0, 1.0);\n"
"}\n";

#define CMYK2RGB \
"vec3 OneMinusCMYK2RGB(in vec4 cmyk) {\n"\
"  vec3 inv_rgb = cmyk.rgb + cmyk.a;\n"\
"  vec3 rgb = 1-min(vec3(1),inv_rgb);\n"\
"  return rgb;\n"\
"}\n"

// Read two slices of inverted CMYKA rendering and convert to RGBA
static const char *glslf_Tex_CMYA_KA_to_RGBA = 
"#version 330\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"uniform vec4 CMYK_Mask;\n"
"uniform sampler2DArray sampler_CMYA_KA;\n"
"layout(location=0) in vec2 Tc;\n"
"layout(location=0) out vec4 outColor;\n"
CMYK2RGB
"void main() {\n"
"  vec4 cmyk_inv = vec4(0);\n"
"  const int cmy_slice = 0;\n"
"  vec4 cmy_inv_a = texture(sampler_CMYA_KA, vec3(Tc,cmy_slice));\n"
"  cmyk_inv.xyz = cmy_inv_a.xyz;\n"
"  float alpha = cmy_inv_a.a;\n"
"  const int k12_slice = 1;\n"
"  vec4 k12_inv_a = texture(sampler_CMYA_KA, vec3(Tc,k12_slice));\n"
"  cmyk_inv.w = k12_inv_a.x;\n"
"  vec4 cmyk = 1-cmyk_inv;\n"
"  vec3 rgb = OneMinusCMYK2RGB(CMYK_Mask * cmyk);\n"
"  outColor = vec4(alpha*rgb, alpha);\n"  // pre-multiplied RGBA out
"}\n";

// Like glslf_Tex_CMYA_KA_to_RGBA, but supporting two spots colors
static const char *glslf_Tex_CMYA_K12A_to_RGBA = 
"#version 330\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"uniform vec4 CMYK_Mask;\n"
"uniform vec3 spot1_rgb;\n"
"uniform vec3 spot2_rgb;\n"
"uniform sampler2DArray sampler_CMYA_K12A;\n"
"layout(location=0) in vec2 Tc;\n"
"layout(location=0) out vec4 outColor;\n"
CMYK2RGB
"void main() {\n"
"  vec4 cmyk_inv = vec4(0);\n"
"  const int cmy_slice = 0;\n"
"  vec4 cmy_inv_a = texture(sampler_CMYA_K12A, vec3(Tc,cmy_slice));\n"
"  cmyk_inv.xyz = cmy_inv_a.xyz;\n"
"  float alpha = cmy_inv_a.a;\n"
"  const int k12_slice = 1;\n"
"  vec4 k12_inv_a = texture(sampler_CMYA_K12A, vec3(Tc,k12_slice));\n"
"  cmyk_inv.w = k12_inv_a.x;\n"
"  vec4 cmyk = 1-cmyk_inv;\n"
"  vec3 rgb = OneMinusCMYK2RGB(CMYK_Mask * cmyk);\n"
"  float spot1 = 1-k12_inv_a.y;\n"
"  float spot2 = 1-k12_inv_a.z;\n"
"  rgb += spot1_rgb*spot1;\n"
"  rgb += spot2_rgb*spot2;\n"
"  outColor = vec4(alpha*rgb, alpha);\n"  // pre-multiplied RGBA out
"}\n";

static const char *glslf_Tex_CMYA_K12A_to_cyan = 
"#version 330\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"uniform sampler2DArray sampler_CMYA_K12A;\n"
"uniform int black_backdrop;\n"
"layout(location=0) in vec2 Tc;\n"
"layout(location=0) out vec4 outColor;\n"
"void main() {\n"
"  const int cmy_slice = 0;\n"
"  vec4 cmy_inv_a = texture(sampler_CMYA_K12A, vec3(Tc,cmy_slice));\n"
"  float c = 1-cmy_inv_a.x;\n"
"  vec3 cyan = vec3(0,1,1);\n"
"  vec3 rgb = c*cyan;\n"
"  float alpha = (black_backdrop != 0) ? cmy_inv_a.a : c;\n"
"  outColor = vec4(alpha*rgb, alpha);\n"  // pre-multiplied RGBA out
"}\n";

static const char *glslf_Tex_CMYA_K12A_to_magenta = 
"#version 330\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"uniform sampler2DArray sampler_CMYA_K12A;\n"
"uniform int black_backdrop;\n"
"layout(location=0) in vec2 Tc;\n"
"layout(location=0) out vec4 outColor;\n"
"void main() {\n"
"  const int cmy_slice = 0;\n"
"  vec4 cmy_inv_a = texture(sampler_CMYA_K12A, vec3(Tc,cmy_slice));\n"
"  float m = 1-cmy_inv_a.y;\n"
"  vec3 magenta = vec3(1,0,1);\n"
"  vec3 rgb = m*magenta;\n"
"  float alpha = (black_backdrop != 0) ? cmy_inv_a.a : m;\n"
"  outColor = vec4(alpha*rgb, alpha);\n"  // pre-multiplied RGBA out
"}\n";

static const char *glslf_Tex_CMYA_K12A_to_yellow = 
"#version 330\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"uniform sampler2DArray sampler_CMYA_K12A;\n"
"uniform int black_backdrop;\n"
"layout(location=0) in vec2 Tc;\n"
"layout(location=0) out vec4 outColor;\n"
"void main() {\n"
"  const int cmy_slice = 0;\n"
"  vec4 cmy_inv_a = texture(sampler_CMYA_K12A, vec3(Tc,cmy_slice));\n"
"  float y = 1-cmy_inv_a.z;\n"
"  vec3 yellow = vec3(1,1,0);\n"
"  vec3 rgb = y*yellow;\n"
"  float alpha = (black_backdrop != 0) ? cmy_inv_a.a : y;\n"
"  outColor = vec4(alpha*rgb, alpha);\n"  // pre-multiplied RGBA out
"}\n";

static const char *glslf_Tex_CMYA_K12A_to_black = 
"#version 330\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"uniform sampler2DArray sampler_CMYA_K12A;\n"
"uniform int black_backdrop;\n"
"layout(location=0) in vec2 Tc;\n"
"layout(location=0) out vec4 outColor;\n"
"void main() {\n"
"  const int k12_slice = 1;\n"
"  vec4 k12_inv_a = texture(sampler_CMYA_K12A, vec3(Tc,k12_slice));\n"
"  float k = 1-k12_inv_a.x;\n"
"  vec3 black = vec3(0,0,0);\n"
"  vec3 white = vec3(1,1,1);\n"
"  vec3 rgb = k*black+(1-k)*white;\n"
"  float alpha = (black_backdrop != 0) ? k12_inv_a.a : k;\n"
"  outColor = vec4(alpha*rgb, alpha);\n"  // pre-multiplied RGBA out
"}\n";

static const char *glslf_Tex_CMYA_K12A_to_spot1 = 
"#version 330\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"uniform sampler2DArray sampler_CMYA_K12A;\n"
"uniform vec3 spot1_rgb;\n"
"uniform vec3 spot2_rgb;\n"
"uniform int black_backdrop;\n"
"layout(location=0) in vec2 Tc;\n"
"layout(location=0) out vec4 outColor;\n"
"void main() {\n"
"  const int k12_slice = 1;\n"
"  vec4 k12_inv_a = texture(sampler_CMYA_K12A, vec3(Tc,k12_slice));\n"
"  float spot1 = 1-k12_inv_a.y;\n"
"  vec3 rgb = spot1*spot1_rgb;\n"
"  float alpha = (black_backdrop != 0) ? k12_inv_a.a : spot1;\n"
"  outColor = vec4(alpha*rgb, alpha);\n"  // pre-multiplied RGBA out
"}\n";

static const char *glslf_Tex_CMYA_K12A_to_spot2 = 
"#version 330\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"uniform sampler2DArray sampler_CMYA_K12A;\n"
"uniform vec3 spot2_rgb;\n"
"uniform int black_backdrop;\n"
"layout(location=0) in vec2 Tc;\n"
"layout(location=0) out vec4 outColor;\n"
"void main() {\n"
"  const int k12_slice = 1;\n"
"  vec4 k12_inv_a = texture(sampler_CMYA_K12A, vec3(Tc,k12_slice));\n"
"  float spot2 = 1-k12_inv_a.z;\n"
"  vec3 rgb = spot2*spot2_rgb;\n"
"  float alpha = (black_backdrop != 0) ? k12_inv_a.a : spot2;\n"
"  outColor = vec4(alpha*rgb, alpha);\n"  // pre-multiplied RGBA out
"}\n";

static const char *glslf_Tex_CMYA_K12A_to_CMY_as_RGB = 
"#version 330\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"uniform sampler2DArray sampler_CMYA_K12A;\n"
"layout(location=0) in vec2 Tc;\n"
"layout(location=0) out vec4 outColor;\n"
"void main() {\n"
"  const int cmy_slice = 0;\n"
"  vec4 cmy_inv_a = texture(sampler_CMYA_K12A, vec3(Tc,cmy_slice));\n"
"  vec3 rgb = cmy_inv_a.rgb;\n"
"  float alpha = cmy_inv_a.a;\n"
"  outColor = vec4(alpha*rgb, alpha);\n"  // pre-multiplied RGBA out
"}\n";

static const char *glslf_Tex_CMYA_K12A_to_K12_as_RGB = 
"#version 330\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"uniform sampler2DArray sampler_CMYA_K12A;\n"
"layout(location=0) in vec2 Tc;\n"
"layout(location=0) out vec4 outColor;\n"
"void main() {\n"
"  const int k12_slice = 1;\n"
"  vec4 k12_inv_a = texture(sampler_CMYA_K12A, vec3(Tc,k12_slice));\n"
"  vec3 rgb = k12_inv_a.rgb;\n"
"  float alpha = k12_inv_a.a;\n"
"  outColor = vec4(alpha*rgb, alpha);\n"  // pre-multiplied RGBA out
"}\n";

void initShaders()
{
  GLuint id;

  // "Cover" shaders to convert constant CMYK colors to two slices of ivnerted CMYK
  id = prog_Constant_OneMinusCMYK_A.compileFragmentShaderOnly(glslf_OneMinusCMYK_A);
  if (!id) {
    fatalError("failed to compile glslf_OneMinusCMYK_A");
  }
  id = prog_Constant_OneMinusCMYK12_A.compileFragmentShaderOnly(glslf_OneMinusCMYK12_A);
  if (!id) {
    fatalError("failed to compile glslf_OneMinusCMYK12_A");
  }
  id = prog_Constant_RGBA.compileFragmentShaderOnly(glslf_RGBA);
  if (!id) {
    fatalError("failed to compile glslf_RGBA");
  }
  // Resolve two slices of inverted CMYK to RGBA
  id = prog_Tex_CMYA_KA_to_RGBA.compileVertexFragmentShaderPair(glslv_Tc, glslf_Tex_CMYA_KA_to_RGBA);
  if (!id) {
    fatalError("failed to compile glslf_Tex_CMYA_KA_to_RGBA");
  }
  id = prog_Tex_CMYA_K12A_to_RGBA.compileVertexFragmentShaderPair(glslv_Tc, glslf_Tex_CMYA_K12A_to_RGBA);
  if (!id) {
    fatalError("failed to compile glslf_Tex_CMYA_K12A_to_RGBA");
  }
  id = prog_Tex_CMYA_K12A_to_cyan.compileVertexFragmentShaderPair(glslv_Tc, glslf_Tex_CMYA_K12A_to_cyan);
  if (!id) {
    fatalError("failed to compile glslf_Tex_CMYA_K12A_to_cyan");
  }
  id = prog_Tex_CMYA_K12A_to_magenta.compileVertexFragmentShaderPair(glslv_Tc, glslf_Tex_CMYA_K12A_to_magenta);
  if (!id) {
    fatalError("failed to compile glslf_Tex_CMYA_K12A_to_magenta");
  }
  id = prog_Tex_CMYA_K12A_to_yellow.compileVertexFragmentShaderPair(glslv_Tc, glslf_Tex_CMYA_K12A_to_yellow);
  if (!id) {
    fatalError("failed to compile glslf_Tex_CMYA_K12A_to_yellow");
  }
  id = prog_Tex_CMYA_K12A_to_black.compileVertexFragmentShaderPair(glslv_Tc, glslf_Tex_CMYA_K12A_to_black);
  if (!id) {
    fatalError("failed to compile glslf_Tex_CMYA_K12A_to_black");
  }
  id = prog_Tex_CMYA_K12A_to_spot1.compileVertexFragmentShaderPair(glslv_Tc, glslf_Tex_CMYA_K12A_to_spot1);
  if (!id) {
    fatalError("failed to compile glslf_Tex_CMYA_K12A_to_spot1");
  }
  prog_Tex_CMYA_K12A_to_spot1.use();
  prog_Tex_CMYA_K12A_to_spot1.setUniform3fv("spot1_rgb", &white[0]);
  id = prog_Tex_CMYA_K12A_to_spot2.compileVertexFragmentShaderPair(glslv_Tc, glslf_Tex_CMYA_K12A_to_spot2);
  if (!id) {
    fatalError("failed to compile glslf_Tex_CMYA_K12A_to_spot2");
  }
  prog_Tex_CMYA_K12A_to_spot2.use();
  prog_Tex_CMYA_K12A_to_spot2.setUniform3fv("spot2_rgb", &pink_nose[0]);

  id = prog_Tex_CMYA_K12A_to_CMY_as_RGB.compileVertexFragmentShaderPair(glslv_Tc, glslf_Tex_CMYA_K12A_to_CMY_as_RGB);
  if (!id) {
    fatalError("failed to compile glslf_Tex_CMYA_K12A_to_CMY_as_RGB");
  }
  id = prog_Tex_CMYA_K12A_to_K12_as_RGB.compileVertexFragmentShaderPair(glslv_Tc, glslf_Tex_CMYA_K12A_to_K12_as_RGB);
  if (!id) {
    fatalError("failed to compile glslf_Tex_CMYA_K12A_to_K12_as_RGB");
  }
}

void writeShader(const char *filename, const char *source_code)
{
  FILE *fp = fopen(filename, "w");
  if (fp) {
    fwrite(source_code, sizeof(char), strlen(source_code), fp);
    fclose(fp);
  } else {
    fprintf(stderr, "%s: could not open <%s> for writing\n", program_name, filename);
  }
}

void writeShaders()
{
  writeShader("Constant_OneMinusCMYK_A.glslf", glslf_OneMinusCMYK_A);
  writeShader("Constant_OneMinusCMYK12_A.glslf", glslf_OneMinusCMYK12_A);
  writeShader("Constant_RGBA.glslf", glslf_RGBA);
  writeShader("Resolve.glslv", glslv_Tc);
  writeShader("Tex_CMYA_KA_to_RGBA.glslf", glslf_Tex_CMYA_KA_to_RGBA);
  writeShader("Tex_CMYA_K12A_to_RGBA.glslf", glslf_Tex_CMYA_K12A_to_RGBA);
}

int main(int argc, char **argv)
{
  GLenum status;
  int i;

  glutInitWindowSize(canvas_width, canvas_height);
  glutInit(&argc, argv);
  for (i=1; i<argc; i++) {
    if (!strcmp(argv[i], "-fillonly")) {
      stroking = false;
      continue;
    }
    if (!strcmp(argv[i], "-vsync")) {
      frame_sync = true;
      continue;
    }
    if (!strcmp(argv[i], "-animate")) {
      animating = 1;
      continue;
    }
    if (!strcmp(argv[i], "-writeshaders")) {
      writeShaders();
      continue;
    }
    if (!strcmp(argv[i], "-mode")) {
      i++;
      if (i < argc) {
        bool ok = parseMixedSamplesModeString(argv[i],
          initial_stencil_samples, initial_color_samples, use_supersample);
        if (ok) {
          continue;
        }
      }
      modeOptionHelp(program_name);
      exit(1);
    }
    if (!strcmp(argv[i], "-rgb")) {
      CMYK_rendering = false;
      continue;
    }
    if (!strcmp(argv[i], "-cmyk")) {
      CMYK_rendering = true;
      continue;
    }
    fprintf(stderr, "usage: %s [-fillonly] [-vsync] [-rgb] [-cmyk]\n",
      program_name);
    exit(1);
  }

  // No stencil, depth, or alpha needed since using GLFramebuffer for real framebuffer.
  glutInitDisplayString("rgb double");

  glutCreateWindow("GPU-accelerated CMYK rendering");
  printf("vendor: %s\n", glGetString(GL_VENDOR));
  printf("version: %s\n", glGetString(GL_VERSION));
  printf("renderer: %s\n", glGetString(GL_RENDERER));
  printf("Executable: %d bit\n", (int)(8*sizeof(int*)));
  printf("\n");
  printf("Use left mouse button to scale/zoom (vertical, up/down) and rotate (right=clockwise, left=ccw)\n");
  printf("Rotate and zooming is centered where you first left mouse click\n");
  printf("Hold down Ctrl at left mouse click to JUST SCALE\n");
  printf("Hold down Shift at left mouse click to JUST ROTATE\n");
  printf("\n");
  printf("Use middle mouse button to slide (translate)\n");

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);

  status = glewInit();
  if (status != GLEW_OK) {
    fatalError("OpenGL Extension Wrangler (GLEW) failed to initialize");
  }

  bool hasNVpr = !!glutExtensionSupported("GL_NV_path_rendering");
  if (!hasNVpr) {
    fatalError("This example requires NV_path_rendering OpenGL extension\n");
  }

  initialize_NVPR_GLEW_emulation(stdout, program_name, 0);

  // Use glutExtensionSupported because glewIsSupported is unreliable for DSA.
  bool hasDSA = !!glutExtensionSupported("GL_EXT_direct_state_access");
  if (!hasDSA) {
    fatalError("OpenGL implementation doesn't support GL_EXT_direct_state_access (you should be using NVIDIA GPUs...)");
  }

  bool hasFramebufferMixedSamples = capabilities.hasFramebufferMixedSamples();
  if (hasFramebufferMixedSamples) {
    printf("Support for NV_framebuffer_mixed_samples found\n");
    printf("You have more quality/performance choices\n");
  } else {
    printf("NO support for NV_framebuffer_mixed_samples found\n");
    printf("Your quality/performance choices are limited\n");
    if (initial_stencil_samples != initial_color_samples) {
      if (initial_stencil_samples > 8) {
        initial_stencil_samples = 8;
      }
      printf("Forcing %d:%d to %dx\n", initial_stencil_samples, initial_color_samples, initial_stencil_samples);
      initial_color_samples = initial_stencil_samples;
    }
  }
  printf("\nStencil samples = %d\n"
         "Color samples = %d\n"
         "Supersampling = %s\n",
    initial_stencil_samples, initial_color_samples, use_supersample ? "2x2" : "none");
  GLRegisterDebugCallback();

  initGraphics();
  updateAnimation();
  requestSynchornizedSwapBuffers(frame_sync);

  path_count = getTigerPathCount();
  createMenu(hasFramebufferMixedSamples);

  getTigerBounds(tiger_bounds, 1, 1);
  initModelAndViewMatrices(tiger_bounds);

  initShaders();

  initFPScontext(&gl_fps_context, FPS_USAGE_TEXTURE);
  scaleFPS(3);
  updateBlackBackdrop();

  glutMainLoop();
  return 0;
}

#ifndef _WIN32
// Linux run-time linker workaround!
#include <pthread.h>
void junk() {
  int i = pthread_getconcurrency();  // Reference a pthread function explicitly.
  i = i;
};
#endif
