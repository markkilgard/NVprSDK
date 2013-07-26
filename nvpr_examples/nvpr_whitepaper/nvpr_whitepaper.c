
/* nvpr_whitepaper.c - "Getting Started with NV_path_rendering" whitepaper example */

// Copyright (c) NVIDIA Corporation. All rights reserved.

/* This example requires the OpenGL Utility Toolkit (GLUT) and an OpenGL driver
   supporting the NV_path_rendering extension for GPU-accelerated path rendering.
   NVIDIA's latest Release 275 drivers support this extension on GeForce 8 and
   later GPU generations. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef __APPLE__
# include <GLUT/glut.h>  // Apple puts GLUT header in non-standard location
#else
# include <GL/glut.h>
# ifdef _WIN32
#  include <windows.h>  // for wglGetProcAddress
# else
#  include <GL/glx.h>  // for glXGetProcAddress
# endif
#endif

#ifndef GLAPIENTRY
# ifdef _WIN32
// Windows uses x86 Pascal (standard call) rather than C calling conventions for OpenGL entry points
#  define GLAPIENTRYP __stdcall *
# else
#  define GLAPIENTRYP *
# endif
#endif

// Only include the minimal NV_path_rendering API used by this example for brevity.
#ifndef GL_NV_path_rendering
// Tokens
#define GL_CLOSE_PATH_NV                                    0x00
#define GL_MOVE_TO_NV                                       0x02
#define GL_LINE_TO_NV                                       0x04
#define GL_PATH_STROKE_WIDTH_NV                             0x9075
#define GL_PATH_JOIN_STYLE_NV                               0x9079
#define GL_ROUND_NV                                         0x90A4
#define GL_PATH_FORMAT_SVG_NV                               0x9070
#define GL_PATH_FORMAT_PS_NV                                0x9071
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
#define GL_SKIP_MISSING_GLYPH_NV                            0x90A9
#define GL_USE_MISSING_GLYPH_NV                             0x90AA
#define GL_TRANSLATE_2D_NV                                  0x9090
#define GL_COUNT_UP_NV                                      0x9088
#define GL_CONVEX_HULL_NV                                   0x908B
#define GL_BOUNDING_BOX_NV                                  0x908D
// Command and query function types
typedef GLint (GLAPIENTRYP PFNGLGENPATHSNVPROC) (GLsizei range);
typedef void (GLAPIENTRYP PFNGLDELETEPATHSNVPROC) (GLuint path, GLsizei range);
typedef void (GLAPIENTRYP PFNGLPATHSTRINGNVPROC) (GLuint path, GLenum format, GLsizei length, const GLvoid *pathString);
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
typedef void (GLAPIENTRYP PFNGLSTENCILFILLPATHNVPROC) (GLuint path, GLenum fillMode, GLuint mask);
typedef void (GLAPIENTRYP PFNGLSTENCILSTROKEPATHNVPROC) (GLuint path, GLint reference, GLuint mask);
typedef void (GLAPIENTRYP PFNGLCOVERFILLPATHNVPROC) (GLuint path, GLenum coverMode);
typedef void (GLAPIENTRYP PFNGLCOVERSTROKEPATHNVPROC) (GLuint path, GLenum coverMode);
typedef void (GLAPIENTRYP PFNGLPATHGLYPHSNVPROC) (GLuint firstPathName, GLenum fontTarget, const GLvoid *fontName, GLbitfield fontStyle, GLsizei numGlyphs, GLenum type, const GLvoid *charcodes, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
#endif

#ifdef GL_FONT_Y_MIN_BOUNDS_NV
/* Due to an error in an early NV_path_rendering specification, the
   _BIT suffix was left out of the GL_FONT_* and GL_GLYPH_* token names.
   Some versions of glext.h in Mesa have this error.  Workaround... */
#define GL_FONT_Y_MIN_BOUNDS_BIT_NV                         0x00020000
#define GL_FONT_Y_MAX_BOUNDS_BIT_NV                         0x00080000
#endif

// Only include the minimal EXT_direct_state_access API used by this example for brevity.
#ifndef GL_EXT_direct_state_access
typedef void (GLAPIENTRYP PFNGLMATRIXLOADIDENTITYEXTPROC) (GLenum mode);
typedef void (GLAPIENTRYP PFNGLMATRIXORTHOEXTPROC) (GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
typedef void (GLAPIENTRYP PFNGLMATRIXTRANSLATEFEXTPROC) (GLenum matrixMode, GLfloat x, GLfloat y, GLfloat z);
typedef void (GLAPIENTRYP PFNGLMATRIXROTATEFEXTPROC) (GLenum matrixMode, GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
#endif

#ifndef __APPLE__
// Path object management
PFNGLGENPATHSNVPROC glGenPathsNV = NULL;
PFNGLDELETEPATHSNVPROC glDeletePathsNV = NULL;
// Path object specification
PFNGLPATHCOMMANDSNVPROC glPathCommandsNV = NULL;
PFNGLPATHSTRINGNVPROC glPathStringNV = NULL;
PFNGLPATHGLYPHRANGENVPROC glPathGlyphRangeNV = NULL;
PFNGLPATHGLYPHSNVPROC glPathGlyphsNV = NULL;

// "Stencil, then Cover" path rendering commands
PFNGLCOVERFILLPATHNVPROC glCoverFillPathNV = NULL;
PFNGLCOVERSTROKEPATHNVPROC glCoverStrokePathNV = NULL;
PFNGLSTENCILFILLPATHNVPROC glStencilFillPathNV = NULL;
PFNGLSTENCILSTROKEPATHNVPROC glStencilStrokePathNV = NULL;

// Instanced path rendering
PFNGLSTENCILFILLPATHINSTANCEDNVPROC glStencilFillPathInstancedNV = NULL;
PFNGLCOVERFILLPATHINSTANCEDNVPROC glCoverFillPathInstancedNV = NULL;
PFNGLSTENCILSTROKEPATHINSTANCEDNVPROC glStencilStrokePathInstancedNV = NULL;
PFNGLCOVERSTROKEPATHINSTANCEDNVPROC glCoverStrokePathInstancedNV = NULL;

// Glyph metric queries
PFNGLGETPATHMETRICRANGENVPROC glGetPathMetricRangeNV = NULL;
PFNGLGETPATHSPACINGNVPROC glGetPathSpacingNV = NULL;

// Path object parameter specification
PFNGLPATHPARAMETERINVPROC glPathParameteriNV = NULL;
PFNGLPATHPARAMETERFNVPROC glPathParameterfNV = NULL;

// Color generation
PFNGLPATHCOLORGENNVPROC glPathColorGenNV = NULL;

// Direct state acces (DSA) matrix manipulation
PFNGLMATRIXLOADIDENTITYEXTPROC glMatrixLoadIdentityEXT = NULL;
PFNGLMATRIXORTHOEXTPROC glMatrixOrthoEXT = NULL;
PFNGLMATRIXTRANSLATEFEXTPROC glMatrixTranslatefEXT = NULL;
PFNGLMATRIXROTATEFEXTPROC glMatrixRotatefEXT = NULL;
#endif

// Multi-platform macro to get OpenGL entrypoints
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

// Booleans for extension support
int hasPathRendering = 0;  // Is NV_path_rendering supported?
int hasDirectStateAccess = 0;  // Is EXT_direct_state_access supported?

// Modes to control rendering state
int drawMode = 3;       // 1=just star & heart, 2=just text, 3=both
int drawString = 1;     // draw string if true, other draw glyph instances
int useGradient = 1;    // use vertical color gradient on text
int background = 2;     // initial background is brown
int sceneInitMode = 0;  // how "star and heart" path object is initialized

int animating = 0;  // Should text spin?
float angle = 0;    // in degrees

const char *programName = "nvpr_whitepaper";

// Initialize OpenGL functions needed by this example
void initglext(void)
{
  hasPathRendering = glutExtensionSupported("GL_NV_path_rendering");
  hasDirectStateAccess = glutExtensionSupported("GL_EXT_direct_state_access");

  if (hasPathRendering) {
    // Initialize a subset of NV_path_rendering API
    LOAD_PROC(PFNGLGENPATHSNVPROC, glGenPathsNV);
    LOAD_PROC(PFNGLDELETEPATHSNVPROC, glDeletePathsNV);
    LOAD_PROC(PFNGLPATHCOMMANDSNVPROC, glPathCommandsNV);
    LOAD_PROC(PFNGLPATHSTRINGNVPROC, glPathStringNV);
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

    LOAD_PROC(PFNGLPATHGLYPHSNVPROC, glPathGlyphsNV);

    LOAD_PROC(PFNGLSTENCILFILLPATHNVPROC, glStencilFillPathNV);
    LOAD_PROC(PFNGLSTENCILSTROKEPATHNVPROC, glStencilStrokePathNV);
    LOAD_PROC(PFNGLCOVERFILLPATHNVPROC, glCoverFillPathNV);
    LOAD_PROC(PFNGLCOVERSTROKEPATHNVPROC, glCoverStrokePathNV);
  }
  if (hasDirectStateAccess) {
    // Initialize a subset of EXT_direct_state_access API
    LOAD_PROC(PFNGLMATRIXLOADIDENTITYEXTPROC, glMatrixLoadIdentityEXT);
    LOAD_PROC(PFNGLMATRIXORTHOEXTPROC, glMatrixOrthoEXT);

    LOAD_PROC(PFNGLMATRIXTRANSLATEFEXTPROC, glMatrixTranslatefEXT);
    LOAD_PROC(PFNGLMATRIXROTATEFEXTPROC, glMatrixRotatefEXT);
  }
}

const GLfloat emScale = 2048;  // match TrueType convention
const GLuint templatePathObject = ~0;  // Non-existant path object

GLuint pathObj = 42;

void initSVGpath()
{
  const char *svgPathString =
    // star
    "M100,180 L40,10 L190,120 L10,120 L160,10 z"
    // heart
    "M300 300 C 100 400,100 200,300 100,500 200,500 400,300 300Z";
  // Create "star and heart" scene from SVG string
  glPathStringNV(pathObj, GL_PATH_FORMAT_SVG_NV,
                 (GLsizei)strlen(svgPathString), svgPathString);
}

void initPSpath()
{
  const char *psPathString =
    // star	
    "100 180 moveto"
    " 40 10 lineto 190 120 lineto 10 120 lineto 160 10 lineto closepath"
    // heart	
    " 300 300 moveto"
    " 100 400 100 200 300 100 curveto"
    " 500 200 500 400 300 300 curveto closepath";
  // Create "star and heart" scene from PostScript user path string
  glPathStringNV(pathObj, GL_PATH_FORMAT_PS_NV,
                 (GLsizei)strlen(psPathString), psPathString);
}

void initDATApath()
{
    static const GLubyte pathCommands[10] =
      { GL_MOVE_TO_NV, GL_LINE_TO_NV, GL_LINE_TO_NV, GL_LINE_TO_NV,
        GL_LINE_TO_NV, GL_CLOSE_PATH_NV,	
        'M', 'C', 'C', 'Z' };  // character aliases
    static const GLshort pathCoords[12][2] =
      { {100, 180}, {40, 10}, {190, 120}, {10, 120}, {160, 10},
        {300,300}, {100,400}, {100,200}, {300,100},
        {500,200}, {500,400}, {300,300} };
    // Create "star and heart" scene from raw command and coordinate data arrays
    glPathCommandsNV(pathObj, 10, pathCommands, 24, GL_SHORT, pathCoords);
}

// Global variables for path objects for glyph instances
GLfloat xtranslate[6+1];  // wordLen+1
GLfloat yMinMax[2];
GLuint glyphBase;

void initOpenGLglyphs()
{
  const char *word = "OpenGL";
  const GLsizei wordLen = (const GLsizei)strlen(word);

  glyphBase = glGenPathsNV(6);
  glPathGlyphsNV(glyphBase,
                 GL_SYSTEM_FONT_NAME_NV, "Helvetica", GL_BOLD_BIT_NV,
                 wordLen, GL_UNSIGNED_BYTE, word,
                 GL_SKIP_MISSING_GLYPH_NV, templatePathObject, emScale);
  glPathGlyphsNV(glyphBase, 
                 GL_SYSTEM_FONT_NAME_NV, "Arial", GL_BOLD_BIT_NV,
                 wordLen, GL_UNSIGNED_BYTE, word,
                 GL_SKIP_MISSING_GLYPH_NV, templatePathObject, emScale);
  glPathGlyphsNV(glyphBase,
                 GL_STANDARD_FONT_NAME_NV, "Sans", GL_BOLD_BIT_NV,
                 wordLen, GL_UNSIGNED_BYTE, word,
                 GL_USE_MISSING_GLYPH_NV, templatePathObject, emScale);

  xtranslate[0] = 0;
  glGetPathSpacingNV(GL_ACCUM_ADJACENT_PAIRS_NV,
                     wordLen+1, GL_UNSIGNED_BYTE,
                     "\000\001\002\003\004\005\005", // repeat last
                                                     // letter twice
                     glyphBase,
                     1.0f, 1.0f,
                     GL_TRANSLATE_X_NV,
                     xtranslate+1);

glGetPathMetricRangeNV(GL_FONT_Y_MIN_BOUNDS_BIT_NV|GL_FONT_Y_MAX_BOUNDS_BIT_NV,
                       glyphBase, /*count*/1,
                       2*sizeof(GLfloat),
                       yMinMax);
}

// Global variables for path objects for a complete font
GLuint fontBase;
GLfloat kerning[6+1];  // wordLen+1
GLfloat yMinMaxFont[2];

void initFont()
{
  const int numChars = 256;  // ISO/IEC 8859-1 8-bit character range
  fontBase = glGenPathsNV(numChars);
  glPathGlyphRangeNV(fontBase,
                     GL_SYSTEM_FONT_NAME_NV, "Helvetica", GL_BOLD_BIT_NV,
                     0, numChars,
                     GL_SKIP_MISSING_GLYPH_NV, templatePathObject,
                     emScale);
  glPathGlyphRangeNV(fontBase, 
                     GL_SYSTEM_FONT_NAME_NV, "Arial", GL_BOLD_BIT_NV,
                     0, numChars,
                     GL_SKIP_MISSING_GLYPH_NV, templatePathObject,
                     emScale);
  glPathGlyphRangeNV(fontBase,
                     GL_STANDARD_FONT_NAME_NV, "Sans", GL_BOLD_BIT_NV,
                     0, numChars,
                     GL_USE_MISSING_GLYPH_NV, templatePathObject,
                     emScale);

  kerning[0] = 0;  // Initial glyph offset is zero
  glGetPathSpacingNV(GL_ACCUM_ADJACENT_PAIRS_NV,
                     7, GL_UNSIGNED_BYTE, "OpenGLL", // repeat L to get
                                                     // final spacing
                     fontBase,
                     1.0f, 1.0f,	
                     GL_TRANSLATE_X_NV,
                     kerning+1);

  glGetPathMetricRangeNV(GL_FONT_Y_MIN_BOUNDS_BIT_NV|GL_FONT_Y_MAX_BOUNDS_BIT_NV,
                         glyphBase, /*count*/1,
                         2*sizeof(GLfloat),
                         yMinMaxFont);
}


// Set a current background color
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
      r = 0.5;
      g = 0.3;
      b = 0.2;
      break;
  case 3:
      r = g = b = 0.75;
      break;
  }
  a = 1.0;
  glClearColor(r,g,b,a);
}

// Set stroking parameters of the "star and heart" scene path object
void setPathParameters()
{
  glPathParameteriNV(pathObj, GL_PATH_JOIN_STYLE_NV, GL_ROUND_NV);
  glPathParameterfNV(pathObj, GL_PATH_STROKE_WIDTH_NV, 6.5);
}

void initStarAndHearScene()
{
  switch (sceneInitMode) {
  case 0:
    printf("Initializing star & heart scene from SVG string\n");
    initSVGpath();
    break;
  case 1:
    printf("Initializing star & heart scene from PostScript user path string\n");
    initPSpath();
    break;
  case 2:
  default:
    printf("Initializing star & heart scene from command and coordinate data arrays\n");
    initDATApath();
    break;
  }
  setPathParameters();
}

void initGraphics(int emScale)
{
  setBackground();

  initStarAndHearScene();
  initOpenGLglyphs();
  initFont();
}

void drawStarAndHeart()
{
  glMatrixLoadIdentityEXT(GL_PROJECTION);
  glMatrixOrthoEXT(GL_PROJECTION, 0, 500, 0, 400, -1, 1);
  glMatrixLoadIdentityEXT(GL_MODELVIEW);

  glStencilFillPathNV(pathObj, GL_COUNT_UP_NV, 0x1F);

  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_NOTEQUAL, 0, 0x1F);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
  glColor3f(1,1,0); // yellow
  glCoverFillPathNV(pathObj, GL_BOUNDING_BOX_NV);

  glStencilStrokePathNV(pathObj, 0x1, ~0);
  glColor3f(1,1,1); // white
  glCoverStrokePathNV(pathObj, GL_CONVEX_HULL_NV);
}

// Color gradient plane equations
const GLfloat rgbGen[3][3] = {
  { 0,  0, 0 },  // red   = constant zero
  { 0,  1, 0 },  // green = varies with y from bottom (0) to top (1)
  { 0, -1, 1 }   // blue  = varies with y from bottom (1) to top (0)
};

void drawOpenGLglyphs()
{
  glMatrixLoadIdentityEXT(GL_PROJECTION);
  glMatrixOrthoEXT(GL_PROJECTION, 
                   0, xtranslate[6], yMinMax[0], yMinMax[1],
                   -1, 1);
  glMatrixLoadIdentityEXT(GL_MODELVIEW);

  { // Spin the word "OpenGL" around it's center based on angle
    float center_x = (0 + xtranslate[6])/2;
    float center_y = (yMinMax[0] + yMinMax[1])/2;
    glMatrixTranslatefEXT(GL_MODELVIEW, center_x, center_y, 0);
    glMatrixRotatefEXT(GL_MODELVIEW, angle, 0, 0, 1);
    glMatrixTranslatefEXT(GL_MODELVIEW, -center_x, -center_y, 0);
  }

  glStencilFillPathInstancedNV(6, GL_UNSIGNED_BYTE,
                               "\000\001\002\003\004\005",
                               glyphBase,
                               GL_PATH_FILL_MODE_NV, 0xFF,
                               GL_TRANSLATE_X_NV, xtranslate);
  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

  if (useGradient) {
    glPathColorGenNV(GL_PRIMARY_COLOR, GL_PATH_OBJECT_BOUNDING_BOX_NV,
                     GL_RGB, &rgbGen[0][0]);
  } else {
    glColor3f(0.5,0.5,0.5); // 50% gray
  }
  glCoverFillPathInstancedNV(6, GL_UNSIGNED_BYTE,
                             "\000\001\002\003\004\005",
                             glyphBase,
                             GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
                             GL_TRANSLATE_X_NV, xtranslate);

  if (useGradient) {
    // Disable color gradient
    glPathColorGenNV(GL_PRIMARY_COLOR, GL_NONE, 0, NULL);
  }
}

void drawOpenGLstring()
{
  glMatrixLoadIdentityEXT(GL_PROJECTION);
  glMatrixOrthoEXT(GL_PROJECTION, 
                   0, kerning[6], yMinMaxFont[0], yMinMaxFont[1],
                   -1, 1);
  glMatrixLoadIdentityEXT(GL_MODELVIEW);

  { // Spin the word "OpenGL" around it's center based on angle
    float center_x = (0 + kerning[6])/2;
    float center_y = (yMinMaxFont[0] + yMinMaxFont[1])/2;
    glMatrixTranslatefEXT(GL_MODELVIEW, center_x, center_y, 0);
    glMatrixRotatefEXT(GL_MODELVIEW, angle, 0, 0, 1);
    glMatrixTranslatefEXT(GL_MODELVIEW, -center_x, -center_y, 0);
  }

  if (useGradient) {
    glPathColorGenNV(GL_PRIMARY_COLOR, GL_PATH_OBJECT_BOUNDING_BOX_NV,
                     GL_RGB, &rgbGen[0][0]);
  } else {
    glColor3f(0.5,0.5,0.5); // 50% gray
  }
  glStencilFillPathInstancedNV((GLsizei)strlen("OpenGL"), GL_UNSIGNED_BYTE, "OpenGL",
                               fontBase,
                               GL_PATH_FILL_MODE_NV, 0xFF,
                               GL_TRANSLATE_X_NV, kerning);

  glCoverFillPathInstancedNV((GLsizei)strlen("OpenGL"), GL_UNSIGNED_BYTE, "OpenGL",
                             fontBase,
                             GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
                             GL_TRANSLATE_X_NV, kerning);
  if (useGradient) {
    // Disable color gradient
    glPathColorGenNV(GL_PRIMARY_COLOR, GL_NONE, 0, NULL);
  }
}

void display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  if (drawMode & 1) {
    drawStarAndHeart();
  }
  if (drawMode & 2) {
    if (drawString) {
      drawOpenGLstring();
    } else {
      drawOpenGLglyphs();
    }
  }
  glutSwapBuffers();
}

int start;

void animate(void)
{
  int now = glutGet(GLUT_ELAPSED_TIME);
  angle += 0.1*(now-start);
  if (angle > 360) {
    angle -= 360;
  }
  glutPostRedisplay();
  start = now;
}

void keyboard(unsigned char c, int x, int y)
{
  switch (c) {
  case 27:  // Esc quits
    exit(0);
    return;
  case 13:  // Enter redisplays
    break;
  case ' ':
    animating = !animating;
    if (animating) {
      start = glutGet(GLUT_ELAPSED_TIME);
      glutIdleFunc(animate);
    } else {
      glutIdleFunc(NULL);
    }
    break;
  case 'm':
    drawMode = drawMode >= 3 ? 1 : drawMode+1;  // Cycle among 1, 2, and 3
    break;
  case 'b':
    background = (background+1)%4;
    setBackground();
    break;
  case 't':
    drawString = !drawString;
    if (drawString) {
      printf("Draw OpenGL string from individual glyph paths\n");
    } else {
      printf("Draw OpenGL string from paths in font\n");
    }
    break;
  case 'g':
    useGradient = !useGradient;
    if (useGradient) {
      printf("Render text with vertical color gradient\n");
    } else {
      printf("Render text with constant gray color\n");
    }
  case 's':
    sceneInitMode = (sceneInitMode+1)%3;
    initStarAndHearScene();
    break;
  default:
    return;
  }
  glutPostRedisplay();
}

static void menu(int choice)
{
  keyboard(choice, 0, 0);
}

int main(int argc, char **argv)
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

  glutCreateWindow("Getting Started with NV_path_rendering");
  printf("vendor: %s\n", glGetString(GL_VENDOR));
  printf("version: %s\n", glGetString(GL_VERSION));
  printf("renderer: %s\n", glGetString(GL_RENDERER));
  printf("samples = %d\n", glutGet(GLUT_WINDOW_NUM_SAMPLES));
  printf("Executable: %d bit\n", (int)(8*sizeof(int*)));

  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);

  // Initialize OpenGL extensions and check for required functionality
  initglext();
  if (!hasPathRendering) {
    fprintf(stderr, "%s: required NV_path_rendering OpenGL extension is not present\n", programName);
    exit(1);
  }
  if (!hasDirectStateAccess) {
    fprintf(stderr, "%s: required EXT_direct_state_access OpenGL extension is not present\n", programName);
    exit(1);
  }
  initGraphics(emScale);

  glutCreateMenu(menu);
  glutAddMenuEntry("[ ] Toggle spinning the word", ' ');
  glutAddMenuEntry("[s] Cycle how star & heart path is specified", 's');
  glutAddMenuEntry("[g] Toggle color gradient on text", 'g');
  glutAddMenuEntry("[m] Cycle drawing mode", 's');
  glutAddMenuEntry("[t] Toggle text as glyphs vs. string", 't');
  glutAddMenuEntry("[b] Cycle background color", 'b');
  glutAddMenuEntry("[Esc] Quit", 27);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  glutMainLoop();
  return 0;
}
