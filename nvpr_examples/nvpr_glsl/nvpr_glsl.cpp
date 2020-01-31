
/* nvpr_glsl.c - GLSL shaders with NV_path_rendering */

// The example draws the text "Noise!" with noise shading.

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

#include "nvpr_glew_init.h"
#include "sRGB_math.h"
#include "xform.h"
#include "read_file.hpp"
#include "nv_dds.h"

int stroking = 1;
int filling = 1;
int underline = 2;
int use_sRGB = 0;
int hasPathRendering = 0;
int hasFramebufferSRGB = 0;
unsigned int dualColorMode = 0;
bool verbose = false;
GLint sRGB_capable = 0;
const char *program_name = "nvpr_glsl";

static const char myProgramName[] = "nvpr_glsl";
static const char myFragmentProgramFileName[] = "mandelbrot.glsl";

/* Scaling and rotation state. */
float anchor_x = 0,
      anchor_y = 0;  /* Anchor for rotation and rotating. */
int scale_y = 0, 
    rotate_x = 0;  /* Prior (x,y) location for rotating (vertical) or rotation (horizontal)? */
int zooming = 0;  /* Are we zooming currently? */
int rotating = 0;  /* Are we rotating (zooming) currently? */

/* Sliding (translation) state. */
float slide_x = 0,
      slide_y = 0;  /* Prior (x,y) location for sliding. */
int sliding = 0;  /* Are we sliding currently? */

float time = 0.05;
float last_time;
bool show_text = true; // otherwise show rounded rectangle

GLfloat yMin, yMax, underline_position, underline_thickness;
GLfloat totalAdvance, xBorder;

int emScale = 2048;

Transform3x2 model,
             view;

static GLuint const_color_program = 0;
static GLuint varying_color_program = 0;
static GLuint fractal_program = 0;

GLint aspectRatio_location = 0;

double MYscale;
double offsetX;
double offsetY;
float max_iterations;
float juliaOffsetX;
float juliaOffsetY;
bool julia = false;

void initglext(void)
{
  hasPathRendering = glutExtensionSupported("GL_NV_path_rendering");
  hasFramebufferSRGB = glutExtensionSupported("GL_EXT_framebuffer_sRGB");
}

/* Global variables */
GLuint glyphBase;
GLuint rect_path;
GLuint pathTemplate;
const char *message = "Noise!"; /* the message to show */
size_t messageLen;
GLfloat *xtranslate;

int background = 2;

void setBackground()
{
    float r, g, b, a;

    switch (background) {
    case 0:
        r = g = b = 0.0;
        break;
    case 1:
        r = g = b = 1.0;
        break;
    case 2:
        r = 0;
        g = 0.666;
        b = 0.5;
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

static void fatalError(const char *message)
{
  fprintf(stderr, "%s: %s\n", program_name, message);
  exit(1);
}

void
initGraphics(int emScale)
{
  const unsigned char *message_ub = (const unsigned char*)message;
  float font_data[4];
  const int numChars = 256;  // ISO/IEC 8859-1 8-bit character range
  GLfloat horizontalAdvance[256];

  if (hasFramebufferSRGB) {
    glGetIntegerv(GL_FRAMEBUFFER_SRGB_CAPABLE_EXT, &sRGB_capable);
    if (sRGB_capable) {
      glEnable(GL_FRAMEBUFFER_SRGB_EXT);
    }
  }

  setBackground();

  glDeletePathsNV(glyphBase, numChars);

  /* Create a range of path objects corresponding to Latin-1 character codes. */
  glyphBase = glGenPathsNV(1+numChars);
  pathTemplate = glyphBase;
  glPathCommandsNV(pathTemplate, 0, NULL, 0, GL_FLOAT, NULL);
  glPathParameteriNV(pathTemplate, GL_PATH_STROKE_WIDTH_NV, 180);
  glPathParameteriNV(pathTemplate, GL_PATH_JOIN_STYLE_NV, GL_ROUND_NV);
  glyphBase++;
  /* Use the "Sans" standard (built-in) font. */
  glPathGlyphRangeNV(glyphBase,
                     GL_STANDARD_FONT_NAME_NV, "Serif", GL_BOLD_BIT_NV,
                     0, numChars,
                     GL_SKIP_MISSING_GLYPH_NV, pathTemplate, emScale);
  
  /* Query font and glyph metrics. */
  glGetPathMetricRangeNV(GL_FONT_Y_MIN_BOUNDS_BIT_NV|GL_FONT_Y_MAX_BOUNDS_BIT_NV|
                         GL_FONT_UNDERLINE_POSITION_BIT_NV|GL_FONT_UNDERLINE_THICKNESS_BIT_NV,
                         glyphBase+' ', /*count*/1,
                         4*sizeof(GLfloat),
                         font_data);
  yMin = font_data[0];
  yMax = font_data[1];
  underline_position = font_data[2];
  underline_thickness = font_data[3];
  printf("Y min,max = %f,%f\n", yMin,yMax);
  printf("underline: pos=%f, thickness=%f\n", underline_position, underline_thickness);
  glGetPathMetricRangeNV(GL_GLYPH_HORIZONTAL_BEARING_ADVANCE_BIT_NV,
                         glyphBase, numChars,
                         0, /* stride of zero means sizeof(GLfloat) since 1 bit in mask */
                         horizontalAdvance);

  /* Query spacing information for example's message. */
  messageLen = strlen(message);
  xtranslate = (GLfloat*) malloc(sizeof(GLfloat)*messageLen);
  if (!xtranslate) {
    fprintf(stderr, "%s: malloc of xtranslate failed\n", program_name);
    exit(1);
  }
  xtranslate[0] = 0.0;  /* Initial xtranslate is zero. */
  {
    /* Use 100% spacing; use 0.9 for both for 90% spacing. */
    GLfloat advanceScale = 1.0,
            kerningScale = 1.0; /* Set this to zero to ignore kerning. */
    glGetPathSpacingNV(GL_ACCUM_ADJACENT_PAIRS_NV,
                       (GLsizei)messageLen, GL_UNSIGNED_BYTE, message,
                       glyphBase,
                       advanceScale,kerningScale,
                       GL_TRANSLATE_X_NV,
                       &xtranslate[1]);  /* messageLen-1 accumulated translates are written here. */
  }

  /* Total advance is accumulated spacing plus horizontal advance of
     the last glyph */
  totalAdvance = xtranslate[messageLen-1] +
                 horizontalAdvance[message_ub[messageLen-1]];
  xBorder = totalAdvance / messageLen;

  const GLfloat coords[] = { 0, yMin, totalAdvance, yMax-yMin, 0.2f*(yMax-yMin) };
  const GLubyte cmds[] = { GL_ROUNDED_RECT_NV };

  rect_path = glGenPathsNV(1);
  glPathCommandsNV(rect_path, 1, cmds, 5, GL_FLOAT, coords);
  printf("%g x %g\n", totalAdvance, yMax-yMin);

  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_NOTEQUAL, 0, ~0);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
  glEnable(GL_DEPTH_TEST);
}

void
doGraphics(void)
{
  if (!show_text) {
    glStencilThenCoverFillPathNV(rect_path, GL_COUNT_UP_NV, ~0, GL_CONVEX_HULL_NV);
    return;
  }
  if (underline) {
    /* Draw an underline with conventional OpenGL rendering. */
    float position = underline_position,
          half_thickness = underline_thickness/2;
    glDisable(GL_STENCIL_TEST);
    glUseProgram(0);
    if (underline == 2) {
        glColor3f(0,0,0);
    } else {
        glColor3f(0.5, 0.5, 0.5);
    }
    glBegin(GL_QUAD_STRIP); {
      glVertex2f(0, position+half_thickness);
      glVertex2f(0, position-half_thickness);
      glVertex2f(totalAdvance, position+half_thickness);
      glVertex2f(totalAdvance, position-half_thickness);
    } glEnd();
    glEnable(GL_STENCIL_TEST);
  }

  if (stroking) {
    /* Add stroking "behind" the filled characters. */
    glColor3f(1,1,0.5);  // gray
    glUseProgram(const_color_program);
    const GLuint mask = ~0;
    const GLint ref = 1;
    glStencilThenCoverStrokePathInstancedNV((GLsizei)messageLen,
      GL_UNSIGNED_BYTE, message, glyphBase,
      ref, mask,  /* Use all stencil bits */
      GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
      GL_TRANSLATE_X_NV, xtranslate);
  }

  if (filling) {
    glUseProgram(fractal_program);

    const GLuint mask = ~0;
    glStencilThenCoverFillPathInstancedNV((GLsizei)messageLen,
      GL_UNSIGNED_BYTE, message, glyphBase,
      GL_PATH_FILL_MODE_NV, mask,  /* Use all stencil bits */
      GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
      GL_TRANSLATE_X_NV, xtranslate);
  }
}

void initModelAndViewMatrices()
{
  translate(model, 0, 0);
  translate(view, 0, 0);
}

float window_width, window_height, aspect_ratio;
float view_width, view_height;
Transform3x2 win2obj;

void configureProjection()
{
  Transform3x2 iproj, viewport;
  float left, right, top, bottom;

  ortho(viewport, 0,window_width, 0,window_height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  float w = totalAdvance,
        h = yMax-yMin;
  if (h < w) {
    left = -xBorder;
    right = totalAdvance+xBorder;
    top = (yMax+yMin)/2 - (right-left)/2;
    bottom = (yMax+yMin)/2 + (right-left)/2;
  } else {
    float yBorder = 0;  // Zero border since yMin and yMax have sufficient space.
    top = yMin - yBorder;
    bottom = yMax + yBorder;
    left = totalAdvance/2 - (bottom-top)/2;
    right = totalAdvance/2 + (bottom-top)/2;
  }
  if (aspect_ratio < 1) {
    glScalef(aspect_ratio,1,1);
  } else {
    glScalef(1,1/aspect_ratio,1);
  }
  const float nere = -5000;  // Avoid Windows "near" keyword.
  const float fer = 5000;    // Avoid Windows "far" keyword.
  glOrtho(left, right, top, bottom, nere, fer);
  inverse_ortho(iproj, left, right, top, bottom);
  view_width = right - left;
  view_height = bottom - top;
  mul(win2obj, iproj, viewport);
}

int iheight;

void updateFractalVaryingGen(GLuint program);

void reshape(int w, int h)
{
  glViewport(0,0,w,h);
  iheight = h;
  window_width = w;
  window_height = h;
  aspect_ratio = window_height/window_width;

  assert(fractal_program);
  glProgramUniform2f(fractal_program, aspectRatio_location, aspect_ratio, 1.0);

  updateFractalVaryingGen(fractal_program);

  configureProjection();
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixPushEXT(GL_MODELVIEW); {
    Transform3x2 mat;

    mul(mat, view, model);
    MatrixLoadToGL(mat);
    doGraphics();
  } glMatrixPopEXT(GL_MODELVIEW);

  glutSwapBuffers();
}

void
mouse(int button, int state, int mouse_space_x, int mouse_space_y)
{
  mouse_space_y = iheight - mouse_space_y;

  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {

      float win[2], tmp[2];
      win[0] = mouse_space_x;
      win[1] = mouse_space_y;
      xform(tmp, win2obj, win);
      anchor_x = tmp[0];
      anchor_y = tmp[1];

      rotate_x = mouse_space_x;
      scale_y = mouse_space_y;
      if (!(glutGetModifiers() & GLUT_ACTIVE_CTRL)) {
        rotating = 1;
      } else {
        rotating = 0;
      }
      if (!(glutGetModifiers() & GLUT_ACTIVE_SHIFT)) {
        zooming = 1;
      } else {
        zooming = 0;
      }
    } else {
      zooming = 0;
      rotating = 0;
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
  mouse_space_y = iheight - mouse_space_y;

  if (zooming || rotating) {
    Transform3x2 t, r, s, m;
    float angle = 0;
    float zoom = 1;
    if (rotating) {
      angle = 0.3 * (mouse_space_x - rotate_x) * 640.0/window_width;
    }
    if (zooming) {
      zoom = pow(1.003, (scale_y - mouse_space_y) * 480.0/window_height);
    }

    translate(t, anchor_x, anchor_y);
    rotate(r, angle);
    scale(s, zoom, zoom);
    mul(r, r, s);
    mul(m, t, r);
    translate(t, -anchor_x, -anchor_y);
    mul(m, m, t);
    mul(view, m, view);
    rotate_x = mouse_space_x;
    scale_y = mouse_space_y;
    glutPostRedisplay();
  }
  if (sliding) {
    float m[2][3];

    float x_offset = (mouse_space_x - slide_x) * view_width/window_width;
    float y_offset = (mouse_space_y - slide_y) * view_height/window_height;
    translate(m, x_offset, y_offset);
    mul(view, m, view);
    slide_y = mouse_space_y;
    slide_x = mouse_space_x;
    glutPostRedisplay();
  }
}

void updateFractalProgramDualColors(GLuint program, unsigned int mode)
{
  static const GLfloat red[4] = { 0.8f, 0.2f, 0.1f, 1.f };
  static const GLfloat blue[4] = { 0.1f, 0.5f, 0.9f, 1.f };
  static const GLfloat green[4] = { 0.1f, 0.9f, 0.3f, 1.f };

  static const GLfloat hexBAFFEC[4] = { 0xBA / 255.0f, 0xFF / 255.0f, 0xEC / 255.0f, 1.f };
  static const GLfloat hex8CE2E8[4] = { 0x8C / 255.0f, 0xE2 / 255.0f, 0xE8 / 255.0f, 1.f };

  static const GLfloat black[4] = { 1, 1, 1, 1 };
  static const GLfloat white[4] = { 0, 0, 0, 0 };

  switch (mode) {
  case 0:
    glProgramUniform4fv(program, glGetUniformLocation(program, "loColor"), 1, red);
    glProgramUniform4fv(program, glGetUniformLocation(program, "hiColor"), 1, blue);
    break;
  case 1:
    glProgramUniform4fv(program, glGetUniformLocation(program, "loColor"), 1, blue);
    glProgramUniform4fv(program, glGetUniformLocation(program, "hiColor"), 1, red);
    break;
  case 2:
    glProgramUniform4fv(program, glGetUniformLocation(program, "loColor"), 1, green);
    glProgramUniform4fv(program, glGetUniformLocation(program, "hiColor"), 1, blue);
    break;
  case 3:
    glProgramUniform4fv(program, glGetUniformLocation(program, "loColor"), 1, hexBAFFEC);
    glProgramUniform4fv(program, glGetUniformLocation(program, "hiColor"), 1, hex8CE2E8);
    break;
  case 4:
    glProgramUniform4fv(program, glGetUniformLocation(program, "loColor"), 1, black);
    glProgramUniform4fv(program, glGetUniformLocation(program, "hiColor"), 1, white);
    break;
  default:
    assert(!"bogus dualColorMode");
  }
}

void updateFractalVaryingGen(GLuint program)
{
  Transform3x2 xform, s, t;
  identity(xform);
  scale(s, aspect_ratio*MYscale/(yMax-yMin), MYscale/(yMax-yMin));
  translate(t, offsetX, offsetY);
  mul(xform, s, t);

  GLint position_loc = glGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "position");
  if (position_loc >= 0) {
    glProgramPathFragmentInputGenNV(program, position_loc, GL_OBJECT_LINEAR, 2, &xform[0][0]);
  } else {
    printf("no position!\n");
  }
}

void updateFractalProgram(GLuint program)
{
  glProgramUniform2f(program, glGetUniformLocation(program, "aspectRatio"), aspect_ratio, 1.0);
  glProgramUniform1f(program, glGetUniformLocation(program, "max_iterations"), max_iterations);
  glProgramUniform1i(program, glGetUniformLocation(program, "julia"), (int)julia);
  glProgramUniform2f(program, glGetUniformLocation(program, "juliaOffsets"), juliaOffsetX, juliaOffsetY);

  updateFractalProgramDualColors(program, dualColorMode);

  if (0/*program == dpProgram*/) {
#if 0
    GLdouble mat2x3[6];
    glProgramUniform1d(program, glGetUniformLocation(program, "scale"), MYscale);
    glProgramUniform2d(program, glGetUniformLocation(program, "offset"), offsetX, offsetY);
    glProgramUniformMatrix2x3dv(program, glGetUniformLocation(program, "matrix"), 1, GL_FALSE, mat2x3);
#endif
  } else {
    GLfloat mat2x3[6] = {
      1,0,0,
      0,1,0
    };
    glProgramUniform1f(program, glGetUniformLocation(program, "scale"), (float)MYscale);
    glProgramUniform2f(program, glGetUniformLocation(program, "offset"), (float)offsetX, (float)offsetY);
    //glProgramUniform2f(program, glGetUniformLocation(program, "offset"), 0, 0);
    glProgramUniformMatrix2x3fv(program, glGetUniformLocation(program, "matrix"), 1, GL_FALSE, mat2x3);
  }

  updateFractalVaryingGen(program);
}

static void updateFractal()
{
    offsetY += 0.4 * MYscale;
    offsetX += 0.2 * MYscale;
    updateFractalProgram(fractal_program);
}

static void idle()
{
  float now = glutGet(GLUT_ELAPSED_TIME);
  time += (now - last_time) / 1000.0 * (3.1419/2);
  last_time = now;

  updateFractal();
  glutPostRedisplay();
}

bool animation = false;

static void doAnimation()
{
  if (animation) {
    glutIdleFunc(idle);
  } else {
    glutIdleFunc(NULL);
  }
}

void resetSettings()
{
    initModelAndViewMatrices();
    juliaOffsetX = 0.0;
    juliaOffsetY = 0.0;

    MYscale = 1.0;
    offsetX = 0.0;
    offsetY = 0.0;

    if (julia) {
        MYscale = 2.0;
        offsetX = 0.0;
    } else {
        MYscale = 19;
        offsetX = -0.7;
    }


    max_iterations = 100.0;
}

void loadFractalTextureLUT(GLint program)
{
  nv_dds::CDDSImage image;
  image.load("gradient4.dds");
  if (!image.is_valid()) {
    printf("could not load gradient file gradient4.dds\n");
    exit(1);
  }

  GLuint tex;
  glGenTextures(1, &tex);
  const int texUnit = 0;
  glActiveTexture(GL_TEXTURE0 + texUnit);
  glBindTextureEXT(GL_TEXTURE_1D, tex);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

  image.upload_texture1D();

  glProgramUniform1i(program, glGetUniformLocation(program, "lut"), (int)texUnit);
}

void printInfoLog(GLuint program, bool shader)
{
  GLint logLength = 0;
  char *infoLog;

  if (shader) {
    glGetShaderiv(program, GL_INFO_LOG_LENGTH, &logLength);
    assert(logLength != 0);

    infoLog = new char[logLength];
    glGetShaderInfoLog(program, logLength, &logLength, infoLog);
  } else {
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    assert(logLength != 0);

    infoLog = new char[logLength];
    glGetProgramInfoLog(program, logLength, &logLength, infoLog);
  }

  if (strlen(infoLog) > 0) {
    printf("%s\n", infoLog);
  }

  delete [] infoLog;
}

static void glslInit()
{
  GLint linked;

  const GLchar *const_color_source =
#if 1 
    // OpenGL 4.3 usage
    "#version 430\n"
#else
    // Alternative for OpenGL 4.0
    "#version 400\n"
    "#extension GL_ARB_explicit_uniform_location : enable\n"
#endif
    "precision highp float;\n"
    "layout(location = 0) uniform vec3 color;\n"
    "layout(location = 0) out vec4 outColor;\n"
    "void main() {\n"
    "    outColor = vec4(color, 1.0);\n"
    "}\n";
  const_color_program = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &const_color_source);
  glGetProgramiv(const_color_program, GL_LINK_STATUS, &linked);
  if (!linked) {
    printInfoLog(const_color_program, false);
  }
  // Color uniform really should be at location zero
  assert(0 == glGetUniformLocation(const_color_program, "color"));
  GLfloat black[3] = {0,0,0};
  glProgramUniform3fv(const_color_program, 0, 1, black);

  const GLchar *varying_color_source =
#if 1 
    // OpenGL 4.3 usage
    "#version 430\n"
#else
    // Alternative for OpenGL 4.0
    "#version 400\n"
    "#extension GL_ARB_explicit_uniform_location : enable\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
#endif
    "precision highp float;\n"
    "layout(location = 0) in vec3 color;\n"
    "layout(location = 0) out vec4 outColor;\n"
    "void main() {\n"
    "    outColor = vec4(color, 1.0);\n"
    "}\n";
  varying_color_program = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &varying_color_source);
  glGetProgramiv(varying_color_program, GL_LINK_STATUS, &linked);
  if (!linked) {
    printInfoLog(varying_color_program, false);
  }
  const GLint color_input_location = 0;
  // Color uniform really should be at location zero
  assert(color_input_location == glGetProgramResourceLocation(varying_color_program, GL_PROGRAM_INPUT, "color"));
  assert(color_input_location == glGetProgramResourceLocation(varying_color_program, GL_FRAGMENT_INPUT_NV, "color"));

  GLfloat blue[3] = {0,0,1};
  glProgramPathFragmentInputGenNV(varying_color_program, color_input_location, GL_CONSTANT, 3, blue);
  glUseProgram(varying_color_program);

  const char *fractal_source = read_text_file("fractal.glsl");
  fractal_program = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &fractal_source);
  glGetProgramiv(fractal_program, GL_LINK_STATUS, &linked);
  if (!linked) {
    printInfoLog(fractal_program, false);
  }

  const GLfloat coeffs[6] = {
    1, 0, 0,
    0, 1, 0
  };
  GLint position_loc = glGetProgramResourceLocation(fractal_program, GL_PROGRAM_INPUT, "position");
  if (position_loc >= 0) {
    glProgramPathFragmentInputGenNV(fractal_program, position_loc, GL_OBJECT_LINEAR, 2, coeffs);
  } else {
    printf("no position!\n");
  }
  delete fractal_source;
  updateFractalProgram(fractal_program);
  loadFractalTextureLUT(fractal_program);

  glUseProgram(fractal_program);

  glutReportErrors();
}

void
keyboard(unsigned char c, int x, int y)
{
  bool dirty = false;

  switch (c) {
  case 27:  /* Esc quits */
    exit(0);
    return;
  case 'R':
    resetSettings();
    dirty = true;
    break;
  case 'j':
  case 'J':
    julia = !julia;
    resetSettings();
    dirty = true;
    break;
  case 13:  /* Enter redisplays */
    break;
  case 's':
    stroking = !stroking;
    break;
  case 'r':
    initModelAndViewMatrices();
    configureProjection();
    break;
  case 't':
    show_text = !show_text;
    break;
  case 'f':
    filling = !filling;
    break;
  case 'i':
    initGraphics(emScale);
    break;
  case 'u':
    underline = (underline+1)%3;
    break;
  case 'b':
    background = (background+1)%4;
    setBackground();
    break;
  case ' ':
    animation = !animation;
    last_time = glutGet(GLUT_ELAPSED_TIME);
    doAnimation();
    return;
  case '+':
  case '=':
    MYscale -= 0.1 * MYscale;
    dirty = true;
    break;
  case '-':
  case '_':
    MYscale += 0.1 * MYscale;
    dirty = true;
    break;
  case 'a':
  case 'A':
    max_iterations += 10.0;
    dirty = true;
    break;
  case 'z':
  case 'Z':
    max_iterations -= 10.0;
    dirty = true;
    break;
  case 'c':
    dualColorMode = (dualColorMode + 1) % 5;
    dirty = true;
    break;
  default:
    return;
  }
  if (dirty) {
    updateFractalProgram(fractal_program);
  }
  glutPostRedisplay();
}

void specialWithModifier(int key, int x, int y, int modifiers)
{
  bool dirty = false;

  bool shiftPressed = (modifiers & GLUT_ACTIVE_SHIFT) == GLUT_ACTIVE_SHIFT;
  switch (key) {
  case GLUT_KEY_LEFT:
    if (shiftPressed) {
      juliaOffsetX += 0.05;
    } else {
      offsetX += 1 * MYscale;
      if (verbose) {
        printf("offset = %g %g\n", offsetX, offsetY);
      }
    }
    dirty = true;
    break;
  case GLUT_KEY_RIGHT:
    if (shiftPressed) {
      juliaOffsetX -= 0.05;
    } else {
      offsetX -= 1 * MYscale;
      if (verbose) {
        printf("offset = %g %g\n", offsetX, offsetY);
      }
    }
    dirty = true;
    break;
  case GLUT_KEY_DOWN:
    if (shiftPressed) {
      juliaOffsetY += 0.05;
    } else {
      offsetY += 1 * MYscale;
      if (verbose) {
        printf("offset = %g %g\n", offsetX, offsetY);
      }
    }
    dirty = true;
    break;
  case GLUT_KEY_UP:
    if (shiftPressed) {
      juliaOffsetY -= 0.05;
    } else {
      offsetY -= 1 * MYscale;
      if (verbose) {
        printf("offset = %g %g\n", offsetX, offsetY);
      }
    }
    dirty = true;
    break;
  }

  if (dirty) {
    updateFractalProgram(fractal_program);
  }

  glutPostRedisplay();
}

void special(int key, int x, int y)
{
  specialWithModifier(key, x, y, glutGetModifiers());
}

int main(int argc, char **argv)
{
  GLenum status;
  GLboolean hasDSA;
  int samples = 0;

  glutInitWindowSize(640, 480);
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
      program_name);
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

  glutCreateWindow("GLSL shading with NV_path_rendering");
  printf("vendor: %s\n", glGetString(GL_VENDOR));
  printf("version: %s\n", glGetString(GL_VERSION));
  printf("renderer: %s\n", glGetString(GL_RENDERER));
  printf("samples = %d\n", glutGet(GLUT_WINDOW_NUM_SAMPLES));
  printf("Executable: %d bit\n", (int)(8*sizeof(int*)));

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(special);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  initModelAndViewMatrices();

  status = glewInit();
  if (status != GLEW_OK) {
    fatalError("OpenGL Extension Wrangler (GLEW) failed to initialize");
  }
  resetSettings();
  // Use glutExtensionSupported because glewIsSupported is unreliable for DSA.
  hasDSA = glutExtensionSupported("GL_EXT_direct_state_access");
  if (!hasDSA) {
    fatalError("OpenGL implementation doesn't support GL_EXT_direct_state_access (you should be using NVIDIA GPUs...)");
  }

  initialize_NVPR_GLEW_emulation(stdout, program_name, 0);
  if (!has_NV_path_rendering) {
    fatalError("required NV_path_rendering OpenGL extension is not present");
  }
  initGraphics(emScale);
  glslInit();

  glutMainLoop();
  return 0;
}

