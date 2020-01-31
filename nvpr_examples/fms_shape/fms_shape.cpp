
/* fms_shape.c - single-pass NV_framebuffer_mixed_samples shape rendering */

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
#include <Cg/vector.hpp>
#include <Cg/matrix.hpp>
#include <Cg/mul.hpp>
#include <Cg/iostream.hpp>
#include <Cg/stdlib.hpp>

using namespace Cg;

#include "nvpr_glew_init.h"
#include "countof.h"
#include "gl_debug_callback.h"
#include "showfps.h"
#include "request_vsync.h"
#include "sRGB_math.h"
#include "srgb_table.h"
#include "parse_fms_mode.hpp"
#include "gl_framebuffer.hpp"
#include "cg4cpp_xform.hpp"
#include "gl_program.hpp"

const char *program_name = "fms_shape";
bool animating = false;
bool sRGB_blending = false;
bool use_supersample = false;
bool draw_bounds = false;
bool scissor_test = false;
bool frame_sync = false;
bool draw_all = true;
bool general_rect = false;
bool avoid_stencil = false;  // stencil needed to draw StC paths
bool use_program = true;
static bool draw_path = false;

enum ShapeMode {
  RECTANGLE,
  CIRCLE,
  ROUNDED_RECTANGLE,
  NUM_SHAPES
};

ShapeMode shape_mode = RECTANGLE;

int allRectsPathBase = 5;

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
//int canvas_width = 640, canvas_height = 480;
int canvas_width = 200, canvas_height = 200;

int initial_stencil_samples = 8;
int initial_color_samples = 1;

float rect_bounds[4] = { 0, 0, 400, 400 };

float3x3 model, view;

FPScontext gl_fps_context;

GLSLProgram prog_RectangleShape[4];
GLSLProgram prog_CircleShape[4];
GLSLProgram prog_RoundedRectangleShape[4];

static void fatalError(const char *message)
{
  fprintf(stderr, "%s: %s\n", program_name, message);
  exit(1);
}


static int mostSignificantBit(int myInt){
    int i = 0;
    while (myInt != 0) {
        ++i;
        myInt >>= 1;
    }
    return i;
}

void initColorModel()
{
  //static const GLfloat rgb[3] = { 0.1, 0.3, 0.6 };
  static const GLfloat rgb[3] = { 0,0,0 };
  static const GLfloat fps[3] = { 0.2f, 0.9f, 0.4f };
  if (sRGB_blending) {
    glClearColor(
      convertSRGBColorComponentToLinearf(rgb[0]),
      convertSRGBColorComponentToLinearf(rgb[1]),
      convertSRGBColorComponentToLinearf(rgb[2]),
      0.0);
  } else {
    glClearColor(rgb[0], rgb[1], rgb[2], 0.0);
  }
  colorFPS(fps[0], fps[1], fps[2]);
}

void initGraphics()
{
  /* Before rendering to a window with a stencil buffer, clear the stencil
     buffer to zero and the color buffer to blue: */
  glClearStencil(0);

  initColorModel();

  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_NOTEQUAL, 0, 0x1F);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

  if (avoid_stencil) {
    glEnable(GL_RASTER_MULTISAMPLE_EXT);
    glRasterSamplesEXT(8, GL_FALSE); 
  }
}

void initModelAndViewMatrices(float scene_bounds[4])
{
  float2 corner1 = float2(scene_bounds[0], scene_bounds[1]);
  float2 corner2 = float2(scene_bounds[2], scene_bounds[3]);
  float2 center = (corner1+corner2)/2;
  model = mul(mul(translate(center), scale(0.9,0.9)), translate(-center));
  view = translate(0, 0);
}

float window_width, window_height;

GLFramebufferCapabilities capabilities;
GLFramebuffer *fb;

float aspect_ratio;
float view_width, view_height;
float3x3 win2obj;

float spin_direction = 1;

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

static void updateProg()
{
  int cov_samples = fb->CoverageSamples();
  if (cov_samples > 1) {
    int ndx = mostSignificantBit(cov_samples)-2;
#if 0
    prog_RectangleShape[ndx].set1i("num_samples", cov_samples);
    prog_RectangleShape[ndx].set1i("all_sample_mask", (1<<cov_samples)-1);
#endif
    prog_RoundedRectangleShape[ndx].set2f("inset", 1.0f, 0.6f);
  }
}

static void updateScissor()
{
  // For when the scissor is enabled.
  const int border = 9; // pixels
  const int scale = fb->RenderScale();
  glScissor(border*scale, border*scale,
      (fb->Width()-2*border)*scale, (fb->Height()-2*border)*scale);
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
      GL_SRGB8_ALPHA8, avoid_stencil ? GL_NONE : GL_STENCIL_INDEX8,
      w, h, 1,
      initial_stencil_samples, initial_color_samples,
      use_supersample ? GLFramebuffer::Supersample : GLFramebuffer::Normal);
    fb->setAlphaHandling(GLFramebuffer::PremultipliedAlpha);
    updateProg();
  }

  configureProjection(rect_bounds);

  updateScissor();
}

static void color(float r, float g, float b, float a)
{
    glColor4f(a*r, a*g, a*b, a);
}

static int quad = 0;

static void rect(float x, float y, float w, float h)
{
  if (quad != 1) {
    glTexCoord2f(-1,-1);
    glVertex2f(x, y);
    glTexCoord2f(1,-1);
    glVertex2f(x+w, y);
    glTexCoord2f(1,1);
    glVertex2f(x+w, y+h);
  }

  if (quad != 2) {
    glTexCoord2f(-1,-1);
    glVertex2f(x, y);
    glTexCoord2f(1,1);
    glVertex2f(x+w, y+h);
      glTexCoord2f(-1,1);
      glVertex2f(x, y+h);
  }
}

static void rrect(float x, float y, float w, float h, float inset_w, float inset_h)
{
  if (quad != 1) {
    glTexCoord2f(-1-inset_w,-1-inset_h);
    glVertex2f(x, y);
    glTexCoord2f(1+inset_w,-1-inset_h);
    glVertex2f(x+w, y);
    glTexCoord2f(1+inset_w,1+inset_h);
    glVertex2f(x+w, y+h);
  }

  if (quad != 2) {
    glTexCoord2f(-1-inset_w,-1-inset_h);
    glVertex2f(x, y);
    glTexCoord2f(1+inset_w,1+inset_h);
    glVertex2f(x+w, y+h);
      glTexCoord2f(-1-inset_w,1+inset_h);
      glVertex2f(x+inset_w, y+h);
  }
}


static void generalRect(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3)
{
  if (quad != 1) {
    glTexCoord2f(-1,-1);
    glVertex2f(x0, y0);
    glTexCoord2f(1,-1);
    glVertex2f(x1, y1);
    glTexCoord2f(1,1);
    glVertex2f(x2, y2);
  }

  if (quad != 2) {
    glTexCoord2f(-1,-1);
    glVertex2f(x0, y0);
    glTexCoord2f(1,1);
    glVertex2f(x2, y2);
      glTexCoord2f(-1,1);
      glVertex2f(x3, y3);
  }
}

static void rectPath(GLuint path, float x, float y, float w, float h)
{
  GLubyte commands[] = {
    GL_MOVE_TO_NV,
    GL_RELATIVE_HORIZONTAL_LINE_TO_NV,
    GL_RELATIVE_VERTICAL_LINE_TO_NV,
    GL_RELATIVE_HORIZONTAL_LINE_TO_NV,
    GL_CLOSE_PATH_NV
  };
  GLfloat coords[] = {
    x, y,
    w,
    h,
    -w
  };
  glPathCommandsNV(path, countof(commands), commands, countof(coords), GL_FLOAT, coords);
}

static void circlePath(GLuint path, float x, float y, float w, float h)
{
  GLubyte commands[] = {
    GL_ROUNDED_RECT2_NV,
    GL_CLOSE_PATH_NV
  };
  GLfloat coords[] = {
    x, y,
    w,
    h,
    w/2,
    h/2
  };
  glPathCommandsNV(path, countof(commands), commands, countof(coords), GL_FLOAT, coords);
}

enum DrawMode {
  DRAW_IMMEDIATE,
  MAKE_PATH,
  DRAW_PATH
};

static void drawPaths(DrawMode mode, int pathBase)
{
  for (int i=0; i<50; i++) {
    // Abuse trig functions for random numbers number generator.
    float s = ::sin(float(i)) * 0.5 + 0.5;
    float c = ::cos(float(i)) * 0.5 + 0.5;

    if (i & 1) {
      color(1,0,1, 0.2);
    } else {
      color(0,1,0, 0.3);
    }
    switch (mode) {
    case MAKE_PATH:
      rectPath(pathBase+i, s*200, c*200, 140, 200);
      break;
    case DRAW_PATH:
      glStencilThenCoverFillPathNV(pathBase+i, GL_COUNT_UP_NV, 0xFF, GL_BOUNDING_BOX_NV);
      break;
    case DRAW_IMMEDIATE:
      rect(s*200, c*200, 140, 200);
      break;
    }
  }
}

GLuint rect_path_obj = 1;
GLuint circle_path_obj = 2;
GLuint rrect_path_obj = 3;

static void drawRects()
{
  if (draw_path) {
    color(1,1,1, 0.5);
    glUseProgram(0);
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
    glStencilFunc(GL_NOTEQUAL, 0, ~0);
    if (draw_all) {
      drawPaths(DRAW_PATH, allRectsPathBase);
    } else {
      switch (shape_mode) {
      case RECTANGLE:
        glStencilThenCoverFillPathNV(rect_path_obj, GL_COUNT_UP_NV, 0xFF, GL_BOUNDING_BOX_NV);
        break;
      case CIRCLE:
        glStencilThenCoverFillPathNV(circle_path_obj, GL_COUNT_UP_NV, 0xFF, GL_BOUNDING_BOX_NV);
        break;
      default:
        assert(!"bogus mode");
        // Fallthrough...
      case ROUNDED_RECTANGLE:
        glStencilThenCoverFillPathNV(rrect_path_obj, GL_COUNT_UP_NV, 0xFF, GL_BOUNDING_BOX_NV);
        break;
      }
    }
    glDisable(GL_STENCIL_TEST);
  } else {
    int cov_samples = fb->CoverageSamples();
    int ndx = mostSignificantBit(cov_samples);
    if (use_program && cov_samples > 1) {
      switch (shape_mode) {
      case RECTANGLE:
        prog_RectangleShape[ndx-2].use();
        break;
      case CIRCLE:
        prog_CircleShape[ndx-2].use();
        break;
      default:
        assert(!"bogus mode");
        // Fallthrough...
      case ROUNDED_RECTANGLE:
        prog_RoundedRectangleShape[ndx-2].use();
        break;
      }
    } else {
      glUseProgram(0);
    }
    glDisable(GL_STENCIL_TEST);
    glCoverageModulationNV(GL_RGBA);
    glBegin(GL_TRIANGLES); {
#if 0
      glColor4f(0.5,0.5,0, 0.5);
      rect(50,50,120,100);

      color(1,0,0.5, 0.7);
      rect(170, 190, 30, 50);

      color(1,0,0.5, 0.7);
      rect(300, 90, 350, 10);

      glColor4f(0.8,0,0.8,0.8);
      rect(250, 150, 60, 80);

      color(0.6, 0.3, 0.2, 0.4);
      rect(10, 30, 280, 170);
#endif
#if 1
      if (draw_all) {
        drawPaths(DRAW_IMMEDIATE, 0);
      } else {
        color(1,1,1, 0.5);
        switch (shape_mode) {
        case RECTANGLE:
        case CIRCLE:
          if (general_rect) {
            generalRect(150, 150, 340, 180, 310, 20, -140, -30);
          } else {
            rect(150, 150, 80, 100);
          }
          break;
        default:
          assert(!"bogus mode");
          // Fallthrough...
        case ROUNDED_RECTANGLE:
          rrect(150, 150, 80, 100, 1.5, 1.5);
          break;
        }
      }
#endif
    } glEnd();
  }

    glUseProgram(0);
}

void drawBounds(float bounds[4])
{
  glDisable(GL_STENCIL_TEST);
  glEnable(GL_LINE_STIPPLE);
  const int scale = fb->RenderScale();
  glLineWidth(1.0*scale);
  glLineStipple(4*scale, 0x5555);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glColor3f(1,1,0);  // yellow
  glBegin(GL_QUADS); {
    glVertex2f(bounds[0], bounds[1]);
    glVertex2f(bounds[2], bounds[1]);
    glVertex2f(bounds[2], bounds[3]);
    glVertex2f(bounds[0], bounds[3]);
  } glEnd();
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_STENCIL_TEST);
}

void display()
{
  fb->bind();
  fb->blendConfiguration();
  const GLenum color_encoding = (sRGB_blending ? GL_SRGB : GL_LINEAR);
  fb->colorEncodingConfiguration(color_encoding);
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glEnable(GL_STENCIL_TEST);
  if (scissor_test) {
    glEnable(GL_SCISSOR_TEST);
  }
  glMatrixPushEXT(GL_MODELVIEW); {
    float3x3 mat = mul(view, model);
    MatrixLoadToGL(mat);
    if (draw_bounds) {
      drawBounds(rect_bounds);
    }
    drawRects();
  } glMatrixPopEXT(GL_MODELVIEW);
  if (scissor_test) {
    glDisable(GL_SCISSOR_TEST);
  }

  fb->copy();
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  if (fb->isSupersampled()) {
    glViewport(0,0, window_width, window_height);
    handleFPS(&gl_fps_context);
    fb->viewport();
  } else {
    handleFPS(&gl_fps_context);
  }
  glutSwapBuffers();
}

void animate()
{
  glutPostRedisplay();
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

void keyboard(unsigned char c, int x, int y)
{
  switch (c) {
  case 27:  /* Esc quits */
    delete fb;
    exit(0);
    return;
  case 'c':
    shape_mode = ShapeMode(int(shape_mode+1) % (unsigned int)NUM_SHAPES);
    printf("shape_mode = %d\n", shape_mode);
    invalidateFPS();
    break;
  case 'a':
    draw_all = !draw_all;
    printf("draw_all = %d\n", draw_all);
    invalidateFPS();
    break;
  case 'g':
    general_rect = !general_rect;
    printf("general_rect = %d\n", general_rect);
    break;
  case 'n':
    draw_path = !draw_path;
    printf("draw_path = %d\n", draw_path);
    invalidateFPS();
    break;
  case 'q':
    quad = (quad+1)%3;
    break;
  case 'p':
    use_program = !use_program;
    printf("use_program = %d\n", use_program);
    invalidateFPS();
    break;
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
    {
      int samples = 1<<(c-'1');
      if (samples > 8 && !capabilities.hasFramebufferMixedSamples()) {
        printf("Warning: native %d samples requires NV_framebuffer_mixed_samples\n", samples);
        return;
      }
      fb->setQuality(samples,
        samples,
        fb->Supersampled() ? GLFramebuffer::Supersample : GLFramebuffer::Normal);
      printf("         now %s%d:%d\n", fb->Supersampled() ? "2x2 " : "", samples, samples);
      invalidateFPS();
    }
    break;
  case '@':  // Grr, Shifted 2 (@) isn't in sequential order with Shifted 1 though 5 in ASCII.
    c = '!'+1;  // Force to the ASCII value that works.
  case '!':
  case '#':
  case '$':
  case '%':
    if (c == '!'|| capabilities.hasFramebufferMixedSamples()) {  // allow 1:1 always
      int cov_samples = 1<<(c-'!');
      int col_samples = 1;
      fb->setQuality(cov_samples,
        col_samples,
        fb->Supersampled() ? GLFramebuffer::Supersample : GLFramebuffer::Normal);
      printf("         now %s%d:%d\n", fb->Supersampled() ? "2x2 " : "", cov_samples, col_samples);
      updateProg();
    } else {
      printf("Needs NV_framebuffer_mixed_samples support for %d:1 mode\n", 1<<(c-'!'));
    }
    invalidateFPS();
    break;
  case 'z':
    printf("Requesting GL_DEPTH24_STENCIL8 format...\n");
    {
      int cov_samples = fb->CoverageSamples();
      int col_samples = fb->ColorSamples();
      if (cov_samples != col_samples) {
        bool supersampled = fb->Supersampled();
        assert(capabilities.hasFramebufferMixedSamples());
        printf("Warning: GL_DEPTH24_STENCIL8 forces color sample count to match coverage sample count!\n");
        printf("         was %s%d:%d\n", supersampled ? "2x2 " : "", cov_samples, col_samples);
        if (cov_samples > 8) {
          cov_samples = 4;
          col_samples = 4;
          supersampled = true;
        } else {
          col_samples = cov_samples;
        }
        fb->setQuality(cov_samples,
          cov_samples,
          supersampled ? GLFramebuffer::Supersample : GLFramebuffer::Normal);
        printf("         now %s%d:%d\n", supersampled ? "2x2 " : "", cov_samples, col_samples);
      }
    }
    fb->setDepthStencilFormat(GL_DEPTH24_STENCIL8);
    invalidateFPS();
    break;
  case 'Z':
    printf("Requesting GL_STENCIL_INDEX8 format...\n");
    fb->setDepthStencilFormat(GL_STENCIL_INDEX8);
    invalidateFPS();
    break;
  case 'i':
    {
      float megabyte = float(1<<20);
      unsigned int fb_size = fb->framebufferSize();
      float fb_megs = fb_size / megabyte;
      unsigned int total_size = fb->totalSize();
      float total_megs = total_size / megabyte;
      printf("Window size: %dx%d\n", fb->Width(), fb->Height());
      printf("Color mode: %s\n", sRGB_blending ? "sRGB-encoded" : "linear (uncorrected)");
      printf("Format:      %s%d:%d\n",
        fb->Supersampled() ? "4x" : "",
        fb->CoverageSamples(), fb->ColorSamples());
      printf("  bits per color sample = %u\n", fb->bitsPerColorSample());
      printf("  bits per coverage sample = %u\n", fb->bitsPerCoverageSample());
      printf("  framebuffer size = %u bytes\n", fb_size);
      printf("  total size = %u bytes\n", total_size);
      printf("framebuffer megabytes = %.2f\n", fb_megs);
      printf("total megabytes = %.2f (includes downsample buffer if needed)\n", total_megs);
    }
    return;  // no redisplay needed
  case ' ':
    animating = !animating;
    updateAnimation();
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
    break;
  case 'B':
    draw_bounds = !draw_bounds;
    printf("draw_bounds = %d\n", draw_bounds);
    break;
  case 't':
    scissor_test = !scissor_test;
    printf("scissor_test = %d\n", scissor_test);
    break;
  case 'S':
    use_supersample = !use_supersample;
    fb->setSupersampled(use_supersample);
    updateScissor();
    invalidateFPS();
    printf("supersampe = %d\n", use_supersample);
    break;
  case 'F':
    frame_sync = !frame_sync;
    printf("frame_sync = %d\n", frame_sync);
    requestSynchornizedSwapBuffers(frame_sync);
    invalidateFPS();
    break;
  case 'r':
    initModelAndViewMatrices(rect_bounds);
    break;
  case '+':
    path_count++;
    break;
  case '-':
    path_count--;
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
  const GLQualitySetting &setting = quality_settings[ndx];
  fb->setQuality(setting.getCoverageSamples(),
    setting.getColorSamples(),
    setting.getSupersampled() ? GLFramebuffer::Supersample : GLFramebuffer::Normal);
  glutPostRedisplay();
}

static void createMenu(bool hasFramebufferMixedSamples)
{
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
  std::sort(quality_settings.begin(), quality_settings.end());

  for (size_t i=0; i<quality_settings.size(); i++) {
    const GLQualitySetting &setting = quality_settings[i];
    glutAddMenuEntry(setting.getName(), int(i)); 
  }

  glutCreateMenu(menu);
  glutAddSubMenu("Quality...", quality_menu);
  glutAddMenuEntry("[ ] Toggle animation", ' ');
  glutAddMenuEntry("[p] Toggle FMS program or raw primitives (shows crack)", 'p');
  glutAddMenuEntry("[a] All or one", 'a');
  glutAddMenuEntry("[c] Cycle shape mode", 'c');
  glutAddMenuEntry("[q] Cycle quad triangles", 'q');
  glutAddMenuEntry("[b] Toggle sRGB-correct blending", 'b');
  glutAddMenuEntry("[g] Toggle rectangle or general quadrilateral", 'g');
  glutAddMenuEntry("[S] Toggle 2x2 supersampling", 'S');
  glutAddMenuEntry("[B] Toggle showing bounds", 'B');
  glutAddMenuEntry("[t] Toggle scissor test", 't');
  glutAddMenuEntry("[n] Toggle drawing with StC paths", 'n');
  glutAddMenuEntry("[F] Toggle vsync", 'F');
  glutAddMenuEntry("[r] Reset view", 'r');
  glutAddMenuEntry("[Esc] Quit", 27);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
}

#define BUG_1699332_WORKAROUND

static const char *glslv_Tc = 
//"#version 150 compatibility\n"
#ifdef BUG_1699332_WORKAROUND
"varying vec2 texCoord[1];\n"
"#define gl_TexCoord texCoord\n"
#endif
"void main() {\n"
"  gl_FrontColor = gl_Color;\n"
#ifdef BUG_1699332_WORKAROUND
"  gl_TexCoord[0] = gl_MultiTexCoord0.xy;\n"
#else
"  gl_TexCoord[0] = gl_MultiTexCoord0;\n"
#endif
"  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
"}\n";

static const char *glslf_color =
//"#version 150 compatibility\n"
"void main() {\n"
"  gl_FragColor = gl_Color;\n"
"}\n";

// This shader recomputes recorrect per-pixel coverage for a rectangle
// to avoid double blending.
//
// Strategy:
// 1) short-cut: if fragment fully covers pixel, accept the full coverage
// 2) interpolate at sample 0
// 3) if sample 0 is "above" rectangle's diagonal, recompute coverage
// 4) if sample 0 is "below" rectangle's diagonal, 
static const char *glsl_fms_rectangle =
"#version 400 compatibility\n"
"#extension GL_NV_sample_mask_override_coverage : require\n"
"layout(override_coverage) out int gl_SampleMask[];\n"
#ifdef BUG_1699332_WORKAROUND
"in vec2 texCoord[1];\n"
"#define gl_TexCoord texCoord\n"
#endif
#if 0
"uniform int num_samples;\n"
"uniform int all_sample_mask;\n"
#else
"const int num_samples = %d;\n"
"const int all_sample_mask = 0x%x;\n"
#endif
"void main() {\n"
"  gl_FragColor = gl_Color;\n"
#if 1
"  if (gl_SampleMaskIn[0] == all_sample_mask) {\n"
//"    gl_FragColor = vec4(1,1,0,1);\n"
"    gl_SampleMask[0] = all_sample_mask;\n"
"  } else {\n"
#else
"  {\n"
#endif
"    int mask = 0;\n"
"    for (int i=0; i<num_samples; i++) {\n"
"      vec2 st = interpolateAtSample(gl_TexCoord[0], i).xy;\n"
"      if (all(lessThan(abs(st),vec2(1)))) {\n"
"        mask |= (1 << i);\n"
"      }\n"
"    }\n"
"    int otherMask = mask & ~gl_SampleMaskIn[0];\n"
"    if (otherMask > gl_SampleMaskIn[0]) {\n"
"      gl_SampleMask[0] = 0;\n"
"    } else {\n"
"      gl_SampleMask[0] = mask;\n"
"    }\n"
"  }\n"
"}\n";

static const char *glslf_fms_circle =
"#version 400 compatibility\n"
"#extension GL_NV_sample_mask_override_coverage : require\n"
"layout(override_coverage) out int gl_SampleMask[];\n"
#ifdef BUG_1699332_WORKAROUND
"in vec2 texCoord[1];\n"
"#define gl_TexCoord texCoord\n"
#endif
#if 0
"uniform int num_samples;\n"
"uniform int all_sample_mask;\n"
#else
"const int num_samples = %d;\n"
"const int all_sample_mask = 0x%x;\n"
#endif
"float sqdist(vec2 xy) { return dot(xy,xy); }\n"
"void main() {\n"
"  gl_FragColor = gl_Color;\n"
#if 1
"  vec2 dSTdx = dFdx(gl_TexCoord[0].xy);\n"
"  vec2 dSTdy = dFdy(gl_TexCoord[0].xy);\n"
"  vec2 bounded_st = abs(gl_TexCoord[0].xy) + abs(dSTdx) + abs(dSTdy);\n"
"  float r = sqdist(bounded_st);\n"
"  if (r < 1 && gl_SampleMaskIn[0] == all_sample_mask) {\n"
//"    gl_FragColor = vec4(1,1,0,1);\n"
"    gl_SampleMask[0] = all_sample_mask;\n"
"  } else {\n"
#else
"  {\n"
#endif
"    int mask = 0;\n"
"    for (int i=0; i<num_samples; i++) {\n"
"      vec2 st = interpolateAtSample(gl_TexCoord[0], i).xy;\n"
"      if (sqdist(st) <= 1) {\n"
"        mask |= (1 << i);\n"
"      }\n"
"    }\n"
"    int otherMask = mask & ~gl_SampleMaskIn[0];\n"
"    if (otherMask > gl_SampleMaskIn[0]) {\n"
"      gl_SampleMask[0] = 0;\n"
"    } else {\n"
"      gl_SampleMask[0] = mask;\n"
"    }\n"
"  }\n"
"}\n";

static const char *glslf_fms_rrect =
"#version 400 compatibility\n"
"#extension GL_NV_sample_mask_override_coverage : require\n"
"layout(override_coverage) out int gl_SampleMask[];\n"
#ifdef BUG_1699332_WORKAROUND
"in vec2 texCoord[1];\n"
"#define gl_TexCoord texCoord\n"
#endif
#if 1
"uniform vec2 inset;\n"
#else
"const vec2 inset = vec2(1.5,1.5);\n"
#endif
#if 0
"uniform int num_samples;\n"
"uniform int all_sample_mask;\n"
#else
"const int num_samples = %d;\n"
"const int all_sample_mask = 0x%x;\n"
#endif
"float sqdist(vec2 xy) { return dot(xy,xy); }\n"
"void main() {\n"
"  gl_FragColor = gl_Color;\n"
"  vec2 dSTdx = dFdx(gl_TexCoord[0].xy);\n"
"  vec2 dSTdy = dFdy(gl_TexCoord[0].xy);\n"
"  vec2 st = gl_TexCoord[0].xy;\n"
"  vec2 bounded_st = max(abs(st) + abs(dSTdx) + abs(dSTdy) - inset,0.0);\n"
"  float r = sqdist(bounded_st);\n"
"  if (r < 1 && gl_SampleMaskIn[0] == all_sample_mask) {\n"
"    gl_SampleMask[0] = all_sample_mask;\n"
"  } else {\n"
"    int mask = 0;\n"
"    for (int i=0; i<num_samples; i++) {\n"
"      vec2 st = max(abs(interpolateAtSample(gl_TexCoord[0], i).xy) - inset, 0.0);\n"
"      if (sqdist(st) <= 1) {\n"
"        mask |= (1 << i);\n"
"      }\n"
"    }\n"
"    int otherMask = mask & ~gl_SampleMaskIn[0];\n"
"    if (otherMask > gl_SampleMaskIn[0]) {\n"
"      gl_SampleMask[0] = 0;\n"
"    } else {\n"
"      gl_SampleMask[0] = mask;\n"
"    }\n"
"  }\n"
"}\n";

#if 0  // Example of Skia shader
#version 430 compatibility
#extension GL_NV_sample_mask_override_coverage: require
in noperspective vec2 vshapeCoords_Stage0;
in flat uint vcurrParamIdx_Stage0;
in flat vec4 vcolor_Stage0;
layout(override_coverage) out int gl_SampleMask[];
out vec4 fsColorOut;
layout(binding=0) buffer ParamsBuffer 
{
	vec4 data[];
}
paramsBuffer;
void main() 
{
	vec4 outputColor_Stage0;
	vec4 outputCoverage_Stage0;
	{
		// Stage 0, Instanced Shape Processor
		uint currParamIdx = vcurrParamIdx_Stage0;
		const int sampleCnt = 8;
		const int sampleMaskAll = 0xff;
		const vec2 samples[sampleCnt] = vec2[](vec2(0.062500, 0.187500), vec2(-0.062500, -0.187500), vec2(0.312500, -0.062500), vec2(-0.187500, 0.312500), vec2(-0.312500, -0.312500), vec2(-0.437500, 0.062500), vec2(0.187500, -0.437500), vec2(0.437500, 0.437500));
		if (gl_SampleMaskIn[0] == sampleMaskAll) 
		{
			gl_SampleMask[0] = sampleMaskAll;
		}
		else 
		{
			int rectMask = 0;
			for (int i = 0; i < sampleCnt; i++) 
			{
				vec2 pt = interpolateAtOffset(vshapeCoords_Stage0, samples[i]);
				if (all(lessThan(abs(pt), vec2(1)))) 
				{
					rectMask |= (1 << i);
				}
			}
			if ((rectMask & ~gl_SampleMaskIn[0]) > gl_SampleMaskIn[0]) 
			{
				discard;
			}
			gl_SampleMask[0] = rectMask;
		}
		outputColor_Stage0 = vcolor_Stage0;
		outputCoverage_Stage0 = vec4(1);
	}
	{
		// Xfer Processor: Porter Duff
		fsColorOut = outputColor_Stage0;
	}
}
#endif

void initShaderSet(size_t count, GLSLProgram set[], const char *source, const char *name)
{
  for (size_t i=0; i<count; i++) {
    char buffer[5000];
    int num_samples = 1<<(i+1);
    int all_samples_mask = (1<<num_samples)-1;
    sprintf(buffer, source, num_samples, all_samples_mask);
    printf("%s\n", buffer);
    GLuint id = set[i].compileVertexFragmentShaderPair(glslv_Tc, buffer);
    if (!id) {
      fatalError(name);
    }
  }
}

void initShaders()
{
  initShaderSet(countof(prog_RectangleShape), prog_RectangleShape, glsl_fms_rectangle, "prog_RectangleShape");
  initShaderSet(countof(prog_CircleShape), prog_CircleShape, glslf_fms_circle, "prog_SphereShape");
  initShaderSet(countof(prog_RoundedRectangleShape), prog_RoundedRectangleShape, glslf_fms_rrect, "prog_RoundedRectangleShape");

  printf("%s\n", glslf_color);
  rectPath(rect_path_obj, 150, 150, 80, 100);
  circlePath(circle_path_obj, 150, 150, 80, 100);
  drawPaths(MAKE_PATH, allRectsPathBase);
}

int main(int argc, char **argv)
{
  GLenum status;
  int i;

  glutInitWindowSize(canvas_width, canvas_height);
  glutInit(&argc, argv);
  for (i=1; i<argc; i++) {
    if (!strcmp(argv[i], "-vsync")) {
      frame_sync = true;
      continue;
    }
    if (!strcmp(argv[i], "-animate")) {
      animating = 1;
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
    fprintf(stderr, "usage: %s [-animate]\n",
      program_name);
    exit(1);
  }

  // No stencil, depth, or alpha needed since using GLFramebuffer for real framebuffer.
  glutInitDisplayString("rgb double");

  glutCreateWindow("High-quality mixed sample shapes");
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

  initModelAndViewMatrices(rect_bounds);

  createMenu(hasFramebufferMixedSamples);

  initFPScontext(&gl_fps_context, FPS_USAGE_TEXTURE);
  scaleFPS(3);

  initShaders();

  glutMainLoop();
  return 0;
}
