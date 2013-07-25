
/* nvpr_text_wheel.cpp - spinning projected text on a wheel with NV_path_rendering */

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
#  include <windows.h>
# else
#  include <GL/glx.h>
# endif
#endif

#include <string>
#include <vector>

#include <Cg/double.hpp>
#include <Cg/vector/xyzw.hpp>
#include <Cg/matrix/1based.hpp>
#include <Cg/matrix/rows.hpp>
#include <Cg/vector.hpp>
#include <Cg/matrix.hpp>
#include <Cg/mul.hpp>
#include <Cg/radians.hpp>
#include <Cg/transpose.hpp>
#include <Cg/stdlib.hpp>
#include <Cg/iostream.hpp>

using namespace Cg;
using std::endl;
using std::cout;
using std::string;
using std::vector;

#include "nvpr_init.h"
#include "sRGB_math.h"
#include "countof.h"
#include "request_vsync.h"
#include "showfps.h"
#include "xform.hpp"
#include "render_font.hpp"

bool stroking = true;
bool filling = true;
int underline = 0;
int regular_aspect = 1;
int fill_gradient = 0;
int use_sRGB = 0;
int hasPathRendering = 0;
int hasFramebufferSRGB = 0;
GLint sRGB_capable = 0;
const char *programName = "nvpr_text_wheel";
float angle = 0;
bool animating = false;
bool enable_vsync = true;
int canvas_width = 640, canvas_height = 480;

/* Scaling and rotation state. */
float anchor_x = 0,
      anchor_y = 0;  /* Anchor for rotation and scaling. */
int scale_y = 0, 
    rotate_x = 0;  /* Prior (x,y) location for scaling (vertical) or rotation (horizontal)? */
int zooming = 0;  /* Are we zooming currently? */
int scaling = 0;  /* Are we scaling (zooming) currently? */

/* Sliding (translation) state. */
float slide_x = 0,
      slide_y = 0;  /* Prior (x,y) location for sliding. */
int sliding = 0;  /* Are we sliding currently? */

float3x3 model, view;

void initglext(void)
{
  hasPathRendering = glutExtensionSupported("GL_NV_path_rendering");
  hasFramebufferSRGB = glutExtensionSupported("GL_EXT_framebuffer_sRGB");
}

/* Global variables */
int background = 2;

void setBackground()
{
  float r, g, b, a;

  switch (background) {
  default:
  case 0:
    r = g = b = 0.0;
    break;
  case 1:
    r = g = b = 1.0;
    break;
  case 2:
    r = 0.1;
    g = 0.3;
    b = 0.6;
    break;
  case 3:
    r = g = b = 0.5;
    break;
  }
  if (sRGB_capable) {
    r = convertSRGBColorComponentToLinearf(r);
    g = convertSRGBColorComponentToLinearf(g);
    b = convertSRGBColorComponentToLinearf(b);
  }
  a = 1.0;
  glClearColor(r,g,b,a);
}

GLfloat yMin, yMax, underline_position, underline_thickness;
GLfloat totalAdvance;

int emScale = 2048;

static void fatalError(const char *message)
{
    fprintf(stderr, "%s: %s\n", programName, message);
    exit(1);
}

static const char *phrases[] = {
  "Free spin",
  "Collect $100",
  "Bankrupt",
  "New car",
  "Spin again",
  "Lose your turn",
  "Take a chance",
  "Three wishes",
  "Lose $25",
  "Your lucky day",
  "Blue screen of death",
  "One million dollars",
  "Go directly to jail"
};

vector<Message*> msg_list;

void
initGraphics(int emScale)
{
  if (hasFramebufferSRGB) {
    glGetIntegerv(GL_FRAMEBUFFER_SRGB_CAPABLE_EXT, &sRGB_capable);
    if (sRGB_capable) {
      glEnable(GL_FRAMEBUFFER_SRGB_EXT);
    }
  }

  setBackground();

  // Create a null path object to use as a parameter template for creating fonts.
  GLuint path_template = glGenPathsNV(1);
  glPathCommandsNV(path_template, 0, NULL, 0, GL_FLOAT, NULL);
  glPathParameterfNV(path_template, GL_PATH_STROKE_WIDTH_NV, 0.1*emScale);  // 10% of emScale
  glPathParameteriNV(path_template, GL_PATH_JOIN_STYLE_NV, GL_ROUND_NV);

  FontFace *font = new FontFace(GL_STANDARD_FONT_NAME_NV, "Sans", 256, path_template);

  int count = countof(phrases);
  float2 to_quad[4];
  float rad = radians(0.5 * 360.0/count);
  float inner_radius = 4,
        outer_radius = 20;
  to_quad[0] = float2(::cos(-rad)*inner_radius, ::sin(-rad)*inner_radius);
  to_quad[1] = float2(::cos(-rad)*outer_radius, ::sin(-rad)*outer_radius);
  to_quad[2] = float2(::cos(rad)*outer_radius, ::sin(rad)*outer_radius);
  to_quad[3] = float2(::cos(rad)*inner_radius, ::sin(rad)*inner_radius);
  for (int i=0; i<count; i++) {
    msg_list.push_back(new Message(font, phrases[i], to_quad));
    msg_list.back()->setUnderline(underline);
  }

  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_NOTEQUAL, 0, ~0);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
}

void initViewMatrix()
{
  view = float3x3(1,0,0,
                  0,1,0,
                  0,0,1);
}

float window_width, window_height, aspect_ratio;
float view_width, view_height;
float3x3 win2obj;

void configureProjection()
{
  float3x3 iproj, viewport;

  viewport = ortho(0,window_width, 0,window_height);
  float left = -20, right = 20, top = 20, bottom = -20;
  if (aspect_ratio > 1) {
    top *= aspect_ratio;
    bottom *= aspect_ratio;
  } else {
    left /= aspect_ratio;
    right /= aspect_ratio;
  }
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(left, right, bottom, top,
    -1, 1);
  iproj = inverse_ortho(left, right, top, bottom);
  view_width = right - left;
  view_height = bottom - top;
  win2obj = mul(iproj, viewport);
}

int iheight;

void reshape(int w, int h)
{
  glViewport(0,0,w,h);
  iheight = h-1;
  window_width = w;
  window_height = h;
  aspect_ratio = window_height/window_width;

  configureProjection();
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  glEnable(GL_STENCIL_TEST);
  int count = countof(phrases);
  for (int i=0; i<count; i++) {
    float deg = i*360.0/count + angle;
    float3x3 rot = rotate(deg);
    float3x3 model = msg_list[i]->getMatrix();
    float3x3 mat = mul(view, mul(rot, model));
    MatrixLoadToGL(mat);
    msg_list[i]->render();
  }

  glDisable(GL_STENCIL_TEST);
  handleFPS();

  glutSwapBuffers();
}

void
mouse(int button, int state, int mouse_space_x, int mouse_space_y)
{
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {

      float2 win = float2(mouse_space_x, mouse_space_y);
      float3 tmp = mul(win2obj, float3(win,1));
      anchor_x = tmp.x/tmp.z;
      anchor_y = tmp.y/tmp.z;

      rotate_x = mouse_space_x;
      scale_y = mouse_space_y;
      if (!(glutGetModifiers() & GLUT_ACTIVE_CTRL)) {
        scaling = 1;
      } else {
        scaling = 0;
      }
      if (!(glutGetModifiers() & GLUT_ACTIVE_SHIFT)) {
        zooming = 1;
      } else {
        zooming = 0;
      }
    } else {
      zooming = 0;
      scaling = 0;
    }
  }
  if (button == GLUT_MIDDLE_BUTTON) {
    if (state == GLUT_DOWN) {
      slide_x = mouse_space_x;
      slide_y = mouse_space_y;
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
    float3x3 t, r, s, m;
    float angle = 0;
    float zoom = 1;
    if (scaling) {
      angle = 0.3 * (mouse_space_x - rotate_x) * canvas_width/window_width;
    }
    if (zooming) {
      zoom = pow(1.003f, (mouse_space_y - scale_y) * canvas_height/window_height);
    }

    t = translate(anchor_x, anchor_y);
    r = rotate(angle);
    s = scale(zoom, zoom);

    r = mul(r, s);
    m = mul(t, r);
    t = translate(-anchor_x, -anchor_y);
    m = mul(m, t);
    view = mul(m, view);
    rotate_x = mouse_space_x;
    scale_y = mouse_space_y;
    glutPostRedisplay();
  }
  if (sliding) {
    float3x3 m;

    float x_offset = (mouse_space_x - slide_x) * view_width/window_width;
    float y_offset = (mouse_space_y - slide_y) * view_height/window_height;
    m = translate(x_offset, y_offset);
    view = mul(m, view);
    slide_y = mouse_space_y;
    slide_x = mouse_space_x;
    glutPostRedisplay();
  }
}

void idle()
{
  angle += 0.5;
  glutPostRedisplay();
}

void
keyboard(unsigned char c, int x, int y)
{
  switch (c) {
  case 27:  /* Esc quits */
    exit(0);
    return;
  case 'F':
    colorFPS(0,1,0);
    toggleFPS();
    break;
  case 'v':
    enable_vsync = !enable_vsync;
    requestSynchornizedSwapBuffers(enable_vsync);
    break;
  case ' ':
    animating = !animating;
    if (animating) {
        glutIdleFunc(idle);
    } else {
        glutIdleFunc(NULL);
    }
    return;
  case 'r':
    initViewMatrix();
    break;
  case '+':
    angle += 0.25;
    break;
  case '-':
    angle -= 0.25;
    break;
  case 13:  /* Enter redisplays */
    break;
  case 'a':
    regular_aspect = !regular_aspect;
    configureProjection();
    break;
  case 's':
    stroking = !stroking;
    for (size_t i=0; i<msg_list.size(); i++) {
      msg_list[i]->setStroking(stroking);
    }
    break;
  case 'f':
    filling = !filling;
    for (size_t i=0; i<msg_list.size(); i++) {
      msg_list[i]->setFilling(filling);
    }
    break;
  case 'i':
    initGraphics(emScale);
    break;
  case 'u':
    underline = (underline+1)%3;
    for (size_t i=0; i<msg_list.size(); i++) {
      msg_list[i]->setUnderline(underline);
    }
    break;
  case 'b':
    background = (background+1)%4;
    setBackground();
    break;
  case 'g':
    fill_gradient = (fill_gradient + 1) % 4;
    break;
  default:
    return;
  }
  glutPostRedisplay();
}

int
main(int argc, char **argv)
{
  GLenum status;
  GLboolean hasDSA;
  int samples = 0;

  glutInitWindowSize(canvas_width, canvas_height);
  glutInit(&argc, argv);
  for (int i=1; i<argc; i++) {
    if (argv[i][0] == '-') {
      int value = atoi(argv[i]+1);
      if (value >= 1) {
        samples = value;
        continue;
      }
    }
    fprintf(stderr, "usage: %s [-#]\n       where # is the number of samples/pixel\n",
      programName);
    exit(1);
  }

  if (samples > 0) {
    if (samples == 1) 
      samples = 0;
    printf("requesting %d samples\n", samples);
    char buffer[200];
    sprintf(buffer, "rgb stencil~4 double samples~%d", samples);
    glutInitDisplayString(buffer);
  } else {
    /* Request a double-buffered window with at least 4 stencil bits and
       8 samples per pixel. */
    glutInitDisplayString("rgb stencil~4 double samples~8");
  }
  if (!glutGet(GLUT_DISPLAY_MODE_POSSIBLE)) {
    glutInitDisplayString(NULL);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_STENCIL);
  }

  glutCreateWindow("Text wheel rendered with NV_path_rendering");
  printf("vendor: %s\n", glGetString(GL_VENDOR));
  printf("version: %s\n", glGetString(GL_VERSION));
  printf("renderer: %s\n", glGetString(GL_RENDERER));
  printf("samples = %d\n", glutGet(GLUT_WINDOW_NUM_SAMPLES));
  printf("Executable: %d bit\n", (int)8*sizeof(int*));

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  initViewMatrix();

  status = glewInit();
  if (status != GLEW_OK) {
    fatalError("OpenGL Extension Wrangler (GLEW) failed to initialize");
  }
  hasDSA = glewIsSupported("GL_EXT_direct_state_access");
  if (!hasDSA) {
    fatalError("OpenGL implementation doesn't support GL_EXT_direct_state_access (you should be using NVIDIA GPUs...)");
  }

  initializeNVPR(programName);
  if (!has_NV_path_rendering) {
    fatalError("required NV_path_rendering OpenGL extension is not present");
  }
  initGraphics(emScale);
  disableFPS();

  glutMainLoop();
  return 0;
}

