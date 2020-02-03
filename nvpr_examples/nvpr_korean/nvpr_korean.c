
/* nvpr_korean.c - Render Korean UTF-8 name via NV_path_rendering */

// Copyright (c) NVIDIA Corporation. All rights reserved.

#include <assert.h>
#include <stdio.h>
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
#define GL_UTF8_NV                                          0x909A
#define GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV                0x909C
#define GL_GLYPH_HORIZONTAL_BEARING_ADVANCE_BIT_NV          0x10
#define GL_FONT_Y_MIN_BOUNDS_BIT_NV                         0x00020000
#define GL_FONT_Y_MAX_BOUNDS_BIT_NV                         0x00080000
#define GL_ADJACENT_PAIRS_NV                                0x90AE
#define GL_SKIP_MISSING_GLYPH_NV                            0x90A9
#define GL_USE_MISSING_GLYPH_NV                             0x90AA
/* Command and query function types */
typedef GLint (GLAPIENTRYP PFNGLGENPATHSNVPROC) (GLsizei range);
typedef void (GLAPIENTRYP PFNGLPATHGLYPHRANGENVPROC) (GLuint firstPathName, GLenum fontTarget, const GLvoid *fontName, GLbitfield fontStyle, GLuint firstGlyph, GLsizei numGlyphs, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
typedef void (GLAPIENTRYP PFNGLSTENCILFILLPATHINSTANCEDNVPROC) (GLsizei numPaths, GLenum pathNameType, const GLvoid *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum transformType, const GLfloat *transformValues);
typedef void (GLAPIENTRYP PFNGLCOVERFILLPATHINSTANCEDNVPROC) (GLsizei numPaths, GLenum pathNameType, const GLvoid *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues);
typedef void (GLAPIENTRYP PFNGLGETPATHMETRICRANGENVPROC) (GLbitfield metricQueryMask, GLuint firstPathName, GLsizei numPaths, GLsizei stride, GLfloat *metrics);
typedef void (GLAPIENTRYP PFNGLGETPATHMETRICSNVPROC) (GLbitfield metricQueryMask, GLsizei numPaths, GLenum pathNameType, const GLvoid *paths, GLuint pathBase, GLsizei stride, GLfloat *metrics);
#endif

#ifdef GL_FONT_Y_MIN_BOUNDS_NV
/* Due to an error in an early NV_path_rendering specification, the
   _BIT suffix was left out of the GL_FONT_* and GL_GLYPH_* token names.
   Some versions of glext.h in Mesa have this error.  Workaround... */
#define GL_FONT_Y_MIN_BOUNDS_BIT_NV                         0x00020000
#define GL_FONT_Y_MAX_BOUNDS_BIT_NV                         0x00080000
#endif

#ifndef __APPLE__
PFNGLGENPATHSNVPROC glGenPathsNV = NULL;
PFNGLPATHGLYPHRANGENVPROC glPathGlyphRangeNV = NULL;
PFNGLSTENCILFILLPATHINSTANCEDNVPROC glStencilFillPathInstancedNV = NULL;
PFNGLCOVERFILLPATHINSTANCEDNVPROC glCoverFillPathInstancedNV = NULL;
PFNGLGETPATHMETRICRANGENVPROC glGetPathMetricRangeNV = NULL;
PFNGLGETPATHMETRICSNVPROC glGetPathMetricsNV = NULL;
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
    fprintf(stderr, "%s: failed to GetProcAddress for %s\n", program_name, #name); \
    exit(1); \
  }
#endif

int hasPathRendering = 0;
const char *program_name = "nvpr_korean";

void initglext(void)
{
  hasPathRendering = glutExtensionSupported("GL_NV_path_rendering");

  if (hasPathRendering) {
    LOAD_PROC(PFNGLGENPATHSNVPROC, glGenPathsNV);
    LOAD_PROC(PFNGLPATHGLYPHRANGENVPROC, glPathGlyphRangeNV);
    LOAD_PROC(PFNGLSTENCILFILLPATHINSTANCEDNVPROC, glStencilFillPathInstancedNV);
    LOAD_PROC(PFNGLCOVERFILLPATHINSTANCEDNVPROC, glCoverFillPathInstancedNV);
    LOAD_PROC(PFNGLGETPATHMETRICSNVPROC, glGetPathMetricsNV);
    LOAD_PROC(PFNGLGETPATHMETRICRANGENVPROC, glGetPathMetricRangeNV);
  }
}

/* Global variables */
GLuint glyphBase;
const char *koreanName = 
  "\xec\x8b\xac\xec\x9d\x80\xed\x95\x98"; /* the Korean name in UTF-8 of SHIM Eun-Ha (a Korean actress) */
int numUTF8chars;
GLfloat *xoffset;

static const char byteLengthOfUTF8[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 0, 0
};

int utf8_char_length_in_bytes(const unsigned char *utf8char)
{
  if (*utf8char & 0x80) {
    return byteLengthOfUTF8[*utf8char ^ 0x80];
  } else {
    return 1;
  }
}

int utf8_string_length(const char *s)
{
  const unsigned char * utf8str = (const unsigned char*)s;
  int chars = 0;
  // Assume nul character terminates string.
  while (*utf8str) {
    int bytes = utf8_char_length_in_bytes(utf8str);
    assert(bytes != 0);  // zero indicates invalid UTF-8 encoding!
    utf8str += bytes;
    chars++;
  }
  return chars;
}

void
initGraphics(void)
{
  // Map entire million+ character point Unicode range!  That's up to 0x10FFFF in hex.
  const int allOfUnicode = 1114112;
  const int emScale = 2048;
  GLfloat *horizontalAdvance;
  GLfloat yMinMax[2];
  GLfloat totalAdvance;
  int i;

  numUTF8chars = utf8_string_length(koreanName);

  /* Create a range of path objects corresponding to Latin-1 character
     codes. */
  glyphBase = glGenPathsNV(allOfUnicode);
  /* Try a Korean font. */
  glPathGlyphRangeNV(glyphBase, 
                     GL_SYSTEM_FONT_NAME_NV, "Malgun Gothic", GL_NONE,
                     0, allOfUnicode, GL_SKIP_MISSING_GLYPH_NV, ~0, emScale);
  /* While Arial is a well-populated Unicode font, it isn't well populated
     with Korean characters so expect missing glyphs! */
  glPathGlyphRangeNV(glyphBase, 
                     GL_SYSTEM_FONT_NAME_NV, "Arial", GL_NONE,
                     0, allOfUnicode, GL_SKIP_MISSING_GLYPH_NV, ~0, emScale);
  /* The standard font name "Missing" provides the ultimate backstop.
     It might not be a Korean character but it will be some distinctive
     glyph.  Probably a box. */
  glPathGlyphRangeNV(glyphBase,
                     GL_STANDARD_FONT_NAME_NV, "Missing", GL_NONE,
                     0, allOfUnicode, GL_USE_MISSING_GLYPH_NV, ~0, emScale);
  
  /* Query font and glyph metrics. */
  glGetPathMetricRangeNV(GL_FONT_Y_MIN_BOUNDS_BIT_NV|GL_FONT_Y_MAX_BOUNDS_BIT_NV,
                         glyphBase, /*count*/1,
                         2*sizeof(GLfloat),
                         yMinMax);
  horizontalAdvance = (GLfloat*) malloc(sizeof(GLfloat)*numUTF8chars);
  glGetPathMetricsNV(GL_GLYPH_HORIZONTAL_BEARING_ADVANCE_BIT_NV,
                     numUTF8chars,
                     GL_UTF8_NV, koreanName,
                     glyphBase,
                     1*sizeof(GLfloat),
                     horizontalAdvance);

  /* Perform simple horizontal non-kerned layout suitable for the
     GL_TRANSLATE_X_NV xoffset needed by glStencilFillPathInstancedNV
     and glCoverFillPathInstancedNV. */
  xoffset = (GLfloat*) malloc(sizeof(GLfloat)*numUTF8chars);
  if (!xoffset) {
    fprintf(stderr, "%s: malloc of transformValues failed\n", program_name);
    exit(1);
  }
  xoffset[0] = 0;
  for (i=1; i<numUTF8chars; i++) {
    xoffset[i] = xoffset[i-1] + horizontalAdvance[i-1];
  }
  /* Total advance is accumulated horizontal advances plus horizontal
     advance of the last glyph */
  totalAdvance = xoffset[numUTF8chars-1] +
                 horizontalAdvance[numUTF8chars-1];
  free(horizontalAdvance);

  /* Configure canvas coordinate system from (0,yMin) to (totalAdvance,yMax). */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, totalAdvance,
          yMinMax[0], yMinMax[1],
          -1, 1);
}

void
doGraphics(void)
{
  /* STEP 1: stencil message into stencil buffer.  Results in samples
     within the message's glyphs to have a non-zero stencil value. */
  glDisable(GL_STENCIL_TEST);
  glStencilFillPathInstancedNV(numUTF8chars,
                               GL_UTF8_NV, koreanName, glyphBase,
                               GL_PATH_FILL_MODE_NV, ~0,  /* Use all stencil bits */
                               GL_TRANSLATE_X_NV, xoffset);

  /* STEP 2: cover region of the message; color covered samples (those
     with a non-zero stencil value) and set their stencil back to zero. */
  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_NOTEQUAL, 0, ~0);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

  glColor3ub(192, 192, 192);  // gray
  glCoverFillPathInstancedNV(numUTF8chars,
                             GL_UTF8_NV, koreanName, glyphBase,
                             GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
                             GL_TRANSLATE_X_NV, xoffset);
}

void
display(void)
{
  glClearColor(0,1,0,0);
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
      program_name);
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

  glutCreateWindow("Korean text rendered via NV_path_rendering");

  printf("vendor: %s\n", glGetString(GL_VENDOR));
  printf("version: %s\n", glGetString(GL_VERSION));
  printf("renderer: %s\n", glGetString(GL_RENDERER));
  printf("samples per pixel = %d\n", glutGet(GLUT_WINDOW_NUM_SAMPLES));
  printf("Executable: %d bit\n", (int)(8*sizeof(int*)));

  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);

  initglext();
  if (!hasPathRendering) {
    fprintf(stderr, "%s: required NV_path_rendering OpenGL extension is not present\n", program_name);
    exit(1);
  }
  initGraphics();

  glutMainLoop();
  return 0;
}

