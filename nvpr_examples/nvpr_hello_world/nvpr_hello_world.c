
/* nvpr_hello_world.c - smooth filled text in OpenGL via NV_path_rendering */

// Copyright (c) NVIDIA Corporation. All rights reserved.

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
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

#include "sRGB_math.h"

#ifndef GLAPIENTRY
# ifdef _WIN32
#  define GLAPIENTRYP __stdcall *
# else
#  define GLAPIENTRYP *
# endif
#endif

#ifndef GL_NV_path_rendering
/* Tokens */
#define GL_STANDARD_FONT_NAME_NV                            0x9072
#define GL_SYSTEM_FONT_NAME_NV                              0x9073
#define GL_PATH_FILL_MODE_NV                                0x9080
#define GL_TRANSLATE_X_NV                                   0x908E
#define GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV                0x909C
#define GL_BOLD_BIT_NV                                      0x01
#define GL_ITALIC_BIT_NV                                    0x02
#define GL_GLYPH_HORIZONTAL_BEARING_ADVANCE_BIT_NV          0x10
#define GL_FONT_Y_MIN_BOUNDS_BIT_NV                         0x00020000
#define GL_FONT_Y_MAX_BOUNDS_BIT_NV                         0x00080000
#define GL_FONT_UNDERLINE_POSITION_BIT_NV                   0x04000000
#define GL_FONT_UNDERLINE_THICKNESS_BIT_NV                  0x08000000
#define GL_ACCUM_ADJACENT_PAIRS_NV                          0x90AD
#define GL_PATH_STROKE_WIDTH_NV                             0x9075
#define GL_PATH_JOIN_STYLE_NV                               0x9079
#define GL_MITER_TRUNCATE_NV                                0x90A8
#define GL_PATH_MITER_LIMIT_NV                              0x907A
#define GL_PRIMARY_COLOR                                    0x8577
#define GL_PATH_OBJECT_BOUNDING_BOX_NV                      0x908A
#define GL_USE_MISSING_GLYPH_NV                             0x90AA
/* Command and query function types */
typedef GLint (GLAPIENTRYP PFNGLGENPATHSNVPROC) (GLsizei range);
typedef void (GLAPIENTRYP PFNGLDELETEPATHSNVPROC) (GLuint path, GLsizei range);
typedef void (GLAPIENTRYP PFNGLPATHCOMMANDSNVPROC) (GLuint path, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const GLvoid *coords);
typedef void (GLAPIENTRYP PFNGLPATHGLYPHRANGENVPROC) (GLuint firstPathName, GLenum fontTarget, const GLvoid *fontName, GLbitfield fontStyle, GLuint firstGlyph, GLsizei numGlyphs, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
typedef void (GLAPIENTRYP PFNGLSTENCILFILLPATHINSTANCEDNVPROC) (GLsizei numPaths, GLenum pathNameType, const GLvoid *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum transformType, const GLfloat *transformValues);
typedef void (GLAPIENTRYP PFNGLSTENCILSTROKEPATHINSTANCEDNVPROC) (GLsizei numPaths, GLenum pathNameType, const GLvoid *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum transformType, const GLfloat *transformValues);
typedef void (GLAPIENTRYP PFNGLCOVERFILLPATHINSTANCEDNVPROC) (GLsizei numPaths, GLenum pathNameType, const GLvoid *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues);
typedef void (GLAPIENTRYP PFNGLCOVERSTROKEPATHINSTANCEDNVPROC) (GLsizei numPaths, GLenum pathNameType, const GLvoid *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues);
typedef void (GLAPIENTRYP PFNGLGETPATHMETRICRANGENVPROC) (GLbitfield metricQueryMask, GLuint firstPathName, GLsizei numPaths, GLsizei stride, GLfloat *metrics);
typedef void (GLAPIENTRYP PFNGLGETPATHSPACINGNVPROC) (GLenum pathListMode, GLsizei numPaths, GLenum pathNameType, const GLvoid *paths, GLuint pathBase, GLfloat advanceScale, GLfloat kerningScale, GLenum transformType, GLfloat *returnedSpacing);
typedef void (GLAPIENTRYP PFNGLPATHPARAMETERINVPROC) (GLuint path, GLenum pname, GLint value);
typedef void (GLAPIENTRYP PFNGLPATHPARAMETERFNVPROC) (GLuint path, GLenum pname, GLfloat value);
typedef void (GLAPIENTRYP PFNGLPATHCOLORGENNVPROC) (GLenum color, GLenum genMode, GLenum colorFormat, const GLfloat *coeffs);
#endif

#ifdef GL_FONT_Y_MIN_BOUNDS_NV
/* Due to an error in an early NV_path_rendering specification, the
   _BIT suffix was left out of the GL_FONT_* and GL_GLYPH_* token names.
   Some versions of glext.h in Mesa have this error.  Workaround... */
#define GL_FONT_Y_MIN_BOUNDS_BIT_NV                         0x00020000
#define GL_FONT_Y_MAX_BOUNDS_BIT_NV                         0x00080000
#define GL_FONT_UNDERLINE_POSITION_BIT_NV                   0x04000000
#define GL_FONT_UNDERLINE_THICKNESS_BIT_NV                  0x08000000
#endif

#define GL_TRANSLATE_2D_NV                                  0x9090

#ifndef __APPLE__
PFNGLGENPATHSNVPROC glGenPathsNV = NULL;
PFNGLDELETEPATHSNVPROC glDeletePathsNV = NULL;
PFNGLPATHCOMMANDSNVPROC glPathCommandsNV = NULL;
PFNGLPATHGLYPHRANGENVPROC glPathGlyphRangeNV = NULL;
PFNGLSTENCILFILLPATHINSTANCEDNVPROC glStencilFillPathInstancedNV = NULL;
PFNGLCOVERFILLPATHINSTANCEDNVPROC glCoverFillPathInstancedNV = NULL;

PFNGLSTENCILSTROKEPATHINSTANCEDNVPROC glStencilStrokePathInstancedNV = NULL;
PFNGLCOVERSTROKEPATHINSTANCEDNVPROC glCoverStrokePathInstancedNV = NULL;

PFNGLGETPATHMETRICRANGENVPROC glGetPathMetricRangeNV = NULL;
PFNGLGETPATHSPACINGNVPROC glGetPathSpacingNV = NULL;

PFNGLPATHPARAMETERINVPROC glPathParameteriNV = NULL;
PFNGLPATHPARAMETERFNVPROC glPathParameterfNV = NULL;
PFNGLPATHCOLORGENNVPROC glPathColorGenNV = NULL;
#endif

#ifndef GL_EXT_framebuffer_sRGB
/* Tokens */
#define GL_FRAMEBUFFER_SRGB_EXT                             0x8DB9
#define GL_FRAMEBUFFER_SRGB_CAPABLE_EXT                     0x8DBA
#endif

#if defined(linux) || defined(sun)
# define GET_PROC_ADDRESS(name)  glXGetProcAddressARB((const GLubyte *) #name)
#elif defined(vxworks)
# define GET_PROC_ADDRESS(name)  rglGetProcAddress(#name)
#elif defined(__APPLE__)
# define GET_PROC_ADDRESS(name)  /*nothing*/
#elif defined(_WIN32)
# define GET_PROC_ADDRESS(name)  wglGetProcAddress(#name)
#else
# error unimplemented code!
#endif

#ifdef __APPLE__
#define LOAD_PROC(type, name)  /*nothing*/
#else
#define LOAD_PROC(type, name) \
  name = (type) GET_PROC_ADDRESS(name); \
  if (!name) { \
    fprintf(stderr, "%s: failed to GetProcAddress for %s\n", programName, #name); \
    exit(1); \
  }
#endif

int stroking = 1;
int filling = 1;
int underline = 1;
int regular_aspect = 1;
int fill_gradient = 0;
int use_sRGB = 0;
int hasPathRendering = 0;
int hasFramebufferSRGB = 0;
GLint sRGB_capable = 0;
const char *programName = "nvpr_hello_world";

void initglext(void)
{
  hasPathRendering = glutExtensionSupported("GL_NV_path_rendering");
  hasFramebufferSRGB = glutExtensionSupported("GL_EXT_framebuffer_sRGB");

  if (hasPathRendering) {
    LOAD_PROC(PFNGLGENPATHSNVPROC, glGenPathsNV);
    LOAD_PROC(PFNGLDELETEPATHSNVPROC, glDeletePathsNV);
    LOAD_PROC(PFNGLPATHCOMMANDSNVPROC, glPathCommandsNV);
    LOAD_PROC(PFNGLPATHGLYPHRANGENVPROC, glPathGlyphRangeNV);
    LOAD_PROC(PFNGLSTENCILFILLPATHINSTANCEDNVPROC, glStencilFillPathInstancedNV);
    LOAD_PROC(PFNGLSTENCILSTROKEPATHINSTANCEDNVPROC, glStencilStrokePathInstancedNV);
    LOAD_PROC(PFNGLCOVERFILLPATHINSTANCEDNVPROC, glCoverFillPathInstancedNV);
    LOAD_PROC(PFNGLCOVERSTROKEPATHINSTANCEDNVPROC, glCoverStrokePathInstancedNV);
    LOAD_PROC(PFNGLGETPATHMETRICRANGENVPROC, glGetPathMetricRangeNV);
    LOAD_PROC(PFNGLGETPATHSPACINGNVPROC, glGetPathSpacingNV);
    LOAD_PROC(PFNGLPATHPARAMETERINVPROC, glPathParameteriNV);
    LOAD_PROC(PFNGLPATHPARAMETERFNVPROC, glPathParameterfNV);
    LOAD_PROC(PFNGLPATHCOLORGENNVPROC, glPathColorGenNV);
  }
}

/* Global variables */
GLuint glyphBase, pathTemplate;
const char *message = "Hello world!"; /* the message to show */
size_t messageLen;
GLfloat *xtranslate = NULL;

int background = 2;  // initial background is blue

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
GLfloat totalAdvance, initialShift;

int emScale = 2048;

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

  pathTemplate = ~0;
  glPathCommandsNV(pathTemplate, 0, NULL, 0, GL_FLOAT, NULL);
  glPathParameterfNV(pathTemplate, GL_PATH_STROKE_WIDTH_NV, emScale*0.1f);
  glPathParameteriNV(pathTemplate, GL_PATH_JOIN_STYLE_NV, GL_MITER_TRUNCATE_NV);
  glPathParameterfNV(pathTemplate, GL_PATH_MITER_LIMIT_NV, 1.0);

  /* Create a range of path objects corresponding to Latin-1 character codes. */
  glyphBase = glGenPathsNV(numChars);
  /* Choose a bold sans-serif font face, preferring Veranda over Arial; if
     neither font is available as a system font, settle for the "Sans" standard
     (built-in) font. */
#if 0
  glPathGlyphRangeNV(glyphBase, 
                     GL_SYSTEM_FONT_NAME_NV, "Liberation Sans", GL_BOLD_BIT_NV,
                     0, numChars,
                     GL_USE_MISSING_GLYPH_NV, pathTemplate,
                     emScale);
  glPathGlyphRangeNV(glyphBase, 
                     GL_SYSTEM_FONT_NAME_NV, "Verdana", GL_BOLD_BIT_NV,
                     0, numChars,
                     GL_USE_MISSING_GLYPH_NV, pathTemplate,
                     emScale);
#endif
  glPathGlyphRangeNV(glyphBase, 
                     GL_SYSTEM_FONT_NAME_NV, "Arial", GL_BOLD_BIT_NV,
                     0, numChars,
                     GL_USE_MISSING_GLYPH_NV, pathTemplate,
                     emScale);
  glPathGlyphRangeNV(glyphBase,
                     GL_STANDARD_FONT_NAME_NV, "Sans", GL_BOLD_BIT_NV,
                     0, numChars,
                     GL_USE_MISSING_GLYPH_NV, pathTemplate,
                     emScale);
  
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
  free(xtranslate);
  xtranslate = (GLfloat*) malloc(2*sizeof(GLfloat)*messageLen);
  if (!xtranslate) {
    fprintf(stderr, "%s: malloc of spacing failed\n", programName);
    exit(1);
  }
  xtranslate[0] = 0;
  xtranslate[1] = 0;
  glGetPathSpacingNV(GL_ACCUM_ADJACENT_PAIRS_NV,
                     (GLsizei)messageLen, GL_UNSIGNED_BYTE, message,
                     glyphBase,
                     1.1, 1.0, GL_TRANSLATE_2D_NV,
                     xtranslate+2);

  /* Total advance is accumulated spacing plus horizontal advance of
     the last glyph */
  totalAdvance = xtranslate[2*(messageLen-1)] +
                 horizontalAdvance[message_ub[messageLen-1]];
  initialShift = totalAdvance / messageLen;

  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_NOTEQUAL, 0, ~0);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
}

void
doGraphics(void)
{
  if (underline) {
    float position = underline_position,
          half_thickness = underline_thickness/2;
    glDisable(GL_STENCIL_TEST);
    glColor3f(0.5, 0.5, 0.5);
    glBegin(GL_QUAD_STRIP); {
      glVertex2f(0, position+half_thickness);
      glVertex2f(0, position-half_thickness);
      glVertex2f(totalAdvance, position+half_thickness);
      glVertex2f(totalAdvance, position-half_thickness);
    } glEnd();
    glEnable(GL_STENCIL_TEST);
  }

  if (stroking) {
    glStencilStrokePathInstancedNV((GLsizei)messageLen,
      GL_UNSIGNED_BYTE, message, glyphBase,
      1, ~0,  /* Use all stencil bits */
      GL_TRANSLATE_2D_NV, xtranslate);
    glColor3ub(255, 255, 192);  // light yellow
    glCoverStrokePathInstancedNV((GLsizei)messageLen,
      GL_UNSIGNED_BYTE, message, glyphBase,
      GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
      GL_TRANSLATE_2D_NV, xtranslate);
  }

  if (filling) {
    /* STEP 1: stencil message into stencil buffer.  Results in samples
       within the message's glyphs to have a non-zero stencil value. */
    glStencilFillPathInstancedNV((GLsizei)messageLen,
                                 GL_UNSIGNED_BYTE, message, glyphBase,
                                 GL_PATH_FILL_MODE_NV, ~0,  /* Use all stencil bits */
                                 GL_TRANSLATE_2D_NV, xtranslate);

    /* STEP 2: cover region of the message; color covered samples (those
       with a non-zero stencil value) and set their stencil back to zero. */

    switch (fill_gradient) {
    case 0:
      {
        GLfloat rgbGen[3][3] = { {0,  0, 0},
                                 {0,  1, 0},
                                 {0, -1, 1} };
        glPathColorGenNV(GL_PRIMARY_COLOR, GL_PATH_OBJECT_BOUNDING_BOX_NV, GL_RGB, &rgbGen[0][0]);
      }
      break;
    case 1:
      glColor3ub(192, 192, 192);  // gray
      break;
    case 2:
      glColor3ub(255, 255, 255);  // white
      break;
    case 3:
      glColor3ub(0, 0, 0);  // black
      break;
    }

    glCoverFillPathInstancedNV((GLsizei)messageLen,
                               GL_UNSIGNED_BYTE, message, glyphBase,
                               GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
                               GL_TRANSLATE_2D_NV, xtranslate);
    if (fill_gradient == 0) {
      /* Disable gradient. */
      glPathColorGenNV(GL_PRIMARY_COLOR, GL_NONE, 0, NULL);
    }
  }
}

float window_width, window_height, aspect_ratio;

void configureProjection()
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if (regular_aspect) {
    float w = totalAdvance,
          h = yMax-yMin;
    if (h < w) {
      /* Configure canvas so text is centered nicely with spacing on sides. */
      glOrtho(-initialShift, totalAdvance+initialShift,
        -0.5*totalAdvance*aspect_ratio + (yMax+yMin)/2,
        0.5*totalAdvance*aspect_ratio + (yMax+yMin)/2,
        -1, 1);
    } else {
      /* Configure canvas so text is centered nicely with spacing on sides. */
      glOrtho(
        -0.5*h*aspect_ratio + totalAdvance/2,
        0.5*h*aspect_ratio + totalAdvance/2,
        yMin, yMax,
        -1, 1);
    }
  } else {
    /* Configure canvas coordinate system from (0,yMin) to (totalAdvance,yMax). */
    glOrtho(0, totalAdvance,
      yMin, yMax,
      -1, 1);
  }
}

void reshape(int w, int h)
{
  glViewport(0,0,w,h);
  window_width = w;
  window_height = h;
  aspect_ratio = window_height/window_width;

  configureProjection();
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  doGraphics();
  glutSwapBuffers();
}

void
keyboard(unsigned char c, int x, int y)
{
  switch (c) {
  case 27:  /* Esc quits */
    exit(0);
    return;
  case 13:  /* Enter redisplays */
    break;
  case 's':
    stroking = !stroking;
    break;
  case 'a':
    regular_aspect = !regular_aspect;
    configureProjection();
    break;
  case 'f':
    filling = !filling;
    break;
  case 'i':
    initGraphics(emScale);
    break;
  case 'u':
    underline = !underline;
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
  int samples = 0;
  int i;

  glutInitWindowSize(640, 480);
  glutInit(&argc, argv);
  for (i=1; i<argc; i++) {
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
    char buffer[200];
    if (samples == 1) 
      samples = 0;
    printf("requesting %d samples\n", samples);
    sprintf(buffer, "rgb stencil~4 double samples~%d", samples);
    glutInitDisplayString(buffer);
  } else {
    /* Request a double-buffered window with at least 4 stencil bits and
    8 samples per pixel. */
    glutInitDisplayString("rgb stencil~4 double samples~8");
  }

  glutCreateWindow("OpenGL \"Hello World\" via NV_path_rendering");
  printf("vendor: %s\n", glGetString(GL_VENDOR));
  printf("version: %s\n", glGetString(GL_VERSION));
  printf("renderer: %s\n", glGetString(GL_RENDERER));
  printf("samples = %d\n", glutGet(GLUT_WINDOW_NUM_SAMPLES));
  printf("Executable: %d bit\n", (int)(8*sizeof(int*)));

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);

  initglext();
  if (!hasPathRendering) {
    fprintf(stderr, "%s: required NV_path_rendering OpenGL extension is not present\n", programName);
    exit(1);
  }
  initGraphics(emScale);

  glutMainLoop();
  return 0;
}

