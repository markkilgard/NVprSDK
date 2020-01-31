
/* nvpr_tiger_fms.c - NV_framebuffer_mixed_samples version of nvpr_tiger */

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
#include "tiger.h"
#include "countof.h"
#include "gl_debug_callback.h"
#include "showfps.h"
#include "request_vsync.h"
#include "sRGB_math.h"
#include "srgb_table.h"
#include "parse_fms_mode.hpp"
#include "gl_framebuffer.hpp"
#include "cg4cpp_xform.hpp"

const char *program_name = "nvpr_tiger_fms";
int stroking = 1,
    filling = 1;
bool animating = false;
bool sRGB_blending = true;
bool use_supersample = false;
bool use_dlist = true;
bool draw_bounds = false;
bool frame_sync = false;

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

int initial_stencil_samples = 8;
int initial_color_samples = 1;

float tiger_bounds[4] = { 0, 0, 0, 0 };

float3x3 model, view;

FPScontext gl_fps_context;

static void fatalError(const char *message)
{
  fprintf(stderr, "%s: %s\n", program_name, message);
  exit(1);
}

void initColorModel()
{
  static const GLfloat rgb[3] = { 0.1f, 0.3f, 0.6f };
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

void mySendColor(GLuint color)
{
  GLubyte red = (color>> 16)&0xFF,
          green = (color >> 8)&0xFF,
          blue  = (color >> 0)&0xFF;
  if (sRGB_blending) {
    glColor3f(SRGB_to_LinearRGB[red], SRGB_to_LinearRGB[green], SRGB_to_LinearRGB[blue]);
  } else {
    glColor3ub(red, green, blue);
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
      GL_SRGB8_ALPHA8, GL_STENCIL_INDEX8,
      w, h, 1,
      initial_stencil_samples, initial_color_samples,
      use_supersample ? GLFramebuffer::Supersample : GLFramebuffer::Normal);
  }

  configureProjection(tiger_bounds);
}

void fastDrawTiger()
{
  GLuint tiger_dlist = 1;
  static unsigned int last_path_count = -1;
  static int last_filling = -1;
  static int last_stroking = -1;
  static bool last_sRGB_blending = false;

  if (last_path_count != path_count ||
      last_filling != filling ||
      last_stroking != stroking ||
      last_sRGB_blending != sRGB_blending) {
    glNewList(tiger_dlist, GL_COMPILE); {
      drawTigerRange(filling, stroking, 0, path_count);
    } glEndList();
    last_filling = filling;
    last_stroking = stroking;
    last_path_count = path_count;
    last_sRGB_blending = sRGB_blending;
  }
  glCallList(tiger_dlist);
}

void drawBounds(float bounds[4])
{
  glDisable(GL_STENCIL_TEST);
  glEnable(GL_LINE_STIPPLE);
  glLineStipple(4, 0x5555);
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
  glMatrixPushEXT(GL_MODELVIEW); {
    float3x3 mat = mul(view, model);
    MatrixLoadToGL(mat);
    if (draw_bounds) {
      drawBounds(tiger_bounds);
    }
    if (use_dlist) {
      fastDrawTiger();
    } else {
      drawTigerRange(filling, stroking, 0, path_count);
    }
  } glMatrixPopEXT(GL_MODELVIEW);

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
    invalidateFPS();
    break;
  case 'B':
    draw_bounds = !draw_bounds;
    printf("draw_bounds = %d\n", draw_bounds);
    break;
  case 'S':
    use_supersample = !use_supersample;
    fb->setSupersampled(use_supersample);
    invalidateFPS();
    printf("supersampe = %d\n", use_supersample);
    break;
  case 's':
    stroking = !stroking;
    invalidateFPS();
    break;
  case 'f':
    filling = !filling;
    invalidateFPS();
    break;
  case 'F':
    frame_sync = !frame_sync;
    printf("frame_sync = %d\n", frame_sync);
    requestSynchornizedSwapBuffers(frame_sync);
    invalidateFPS();
    break;
  case 'r':
    initModelAndViewMatrices(tiger_bounds);
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
  glutAddMenuEntry("[b] Toggle sRGB-correct blending", 'b');
  glutAddMenuEntry("[S] Toggle 2x2 supersampling", 'S');
  glutAddMenuEntry("[B] Toggle showing bounds", 'B');
  glutAddMenuEntry("[f] Toggle filling", 'f');
  glutAddMenuEntry("[s] Toggle stroking", 's');
  glutAddMenuEntry("[d] Toggle display lists", 'd');
  glutAddMenuEntry("[F] Toggle vsync", 'F');
  glutAddMenuEntry("[r] Reset view", 'r');
  glutAddMenuEntry("[Esc] Quit", 27);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
}

int main(int argc, char **argv)
{
  GLenum status;
  int i;

  glutInitWindowSize(canvas_width, canvas_height);
  glutInit(&argc, argv);
  for (i=1; i<argc; i++) {
    if (!strcmp(argv[i], "-fillonly")) {
      stroking = 0;
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
    fprintf(stderr, "usage: %s [-fillonly]\n",
      program_name);
    exit(1);
  }

  // No stencil, depth, or alpha needed since using GLFramebuffer for real framebuffer.
  glutInitDisplayString("rgb double");

  glutCreateWindow("High-quality PostScript tiger rendering");
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

  initFPScontext(&gl_fps_context, FPS_USAGE_TEXTURE);
  scaleFPS(3);

  glutMainLoop();
  return 0;
}
