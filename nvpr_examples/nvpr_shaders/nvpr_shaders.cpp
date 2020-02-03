
/* nvpr_shaders.c - Cg bump mapping of text with NV_path_rendering */

// The example draws the text "Brick wall!" with diffuse bump mapping.

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

#include <Cg/cg.h>    /* Can't include this?  Is Cg Toolkit installed! */
#include <Cg/cgGL.h>

#include "nvpr_glew_init.h"
#include "sRGB_math.h"
#include "xform.h"

int stroking = 1;
int filling = 1;
int underline = 2;
int use_sRGB = 0;
int hasPathRendering = 0;
int hasFramebufferSRGB = 0;
GLint sRGB_capable = 0;
const char *program_name = "nvpr_shaders";

static const char myProgramName[] = "nvpr_shaders";
static const char myFragmentProgramFileName[] = "bumpmap.cg";
static const char myFragmentProgramName[] = "bumpmap";

static CGcontext   myCgContext;
static CGprofile   myCgFragmentProfile;
static CGprogram   myCgFragmentProgram;
static CGparameter myCgFragmentParam_normalMap;
static CGparameter myCgFragmentParam_lightPos;

/* Scaling and rotation state. */
float anchor_x = 0,
      anchor_y = 0;  /* Anchor for rotation and scaling. */
int scale_y = 0, 
    rotate_x = 0;  /* Prior (x,y) location for scaling (vertical) or rotation (horizontal)? */
int rotating = 0;  /* Are we rotating currently? */
int scaling = 0;  /* Are we scaling (rotating) currently? */

/* Sliding (translation) state. */
float slide_x = 0,
      slide_y = 0;  /* Prior (x,y) location for sliding. */
int sliding = 0;  /* Are we sliding currently? */

float time = 0.05;
float last_time;
float dolly = 5;
float lx, ly, lz;
int draw_light = 1;
int light_view = 1;

GLfloat yMin, yMax, underline_position, underline_thickness;
GLfloat totalAdvance, xBorder;

int emScale = 2048;

Transform3x2 model,
             view;

static const GLubyte
myBrickNormalMapImage[3*(128*128+64*64+32*32+16*16+8*8+4*4+2*2+1*1)] = {
/* RGB8 image data for a mipmapped 128x128 normal map for a brick pattern */
#include "brick_image.h"
};

enum {
  TO_NORMAL_MAP = 1
};

static void initBrickNormalMap()
{
  const GLubyte *image;
  unsigned int size, level;

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1); /* Tightly packed texture data. */

  glBindTexture(GL_TEXTURE_2D, TO_NORMAL_MAP);
  /* Load each mipmap level of range-compressed 128x128 brick normal
     map texture. */
  for (size = 128, level = 0, image = myBrickNormalMapImage;
       size > 0;
       image += 3*size*size, size /= 2, level++) {
    glTexImage2D(GL_TEXTURE_2D, level,
      GL_RGB8, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
  }
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
    GL_LINEAR_MIPMAP_LINEAR);
}

void initglext(void)
{
  hasPathRendering = glutExtensionSupported("GL_NV_path_rendering");
  hasFramebufferSRGB = glutExtensionSupported("GL_EXT_framebuffer_sRGB");
}

/* Global variables */
GLuint glyphBase;
GLuint pathTemplate;
const char *message = "Brick wall!"; /* the message to show */
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
  glPathParameteriNV(pathTemplate, GL_PATH_STROKE_WIDTH_NV, 50);
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
    const GLfloat advanceScale = 1.0,
                  kerningScale = 1.0; /* Set this to zero to ignore kerning. */
    glGetPathSpacingNV(GL_ACCUM_ADJACENT_PAIRS_NV,
                       (GLsizei)messageLen, GL_UNSIGNED_BYTE, message,
                       glyphBase,
                       advanceScale, kerningScale,
                       GL_TRANSLATE_X_NV,
                       &xtranslate[1]);  /* messageLen-1 accumulated translates are written here. */
  }

  /* Total advance is accumulated spacing plus horizontal advance of
     the last glyph */
  totalAdvance = xtranslate[messageLen-1] +
                 horizontalAdvance[message_ub[messageLen-1]];
  xBorder = totalAdvance / messageLen;

  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_NOTEQUAL, 0, ~0);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
  glEnable(GL_DEPTH_TEST);
}

void
doGraphics(void)
{
  if (underline) {
    /* Draw an underline with conventional OpenGL rendering. */
    float position = underline_position,
          half_thickness = underline_thickness/2;
    glDisable(GL_STENCIL_TEST);
    if (underline == 2) {
        glColor3f(1,1,1);
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
    const GLint stencil_value = 1;
    const GLuint write_mask = ~0;  // Use all stencil bits
    glStencilThenCoverStrokePathInstancedNV((GLsizei)messageLen,
      GL_UNSIGNED_BYTE, message, glyphBase,
      stencil_value, write_mask,
      GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
      GL_TRANSLATE_X_NV, xtranslate);
  }

  if (filling) {
    const GLfloat coeffs[2*3] = { 10,0,0, 0,1,0 };
    glPathTexGenNV(GL_TEXTURE0, GL_PATH_OBJECT_BOUNDING_BOX_NV, 2, coeffs);

    cgGLBindProgram(myCgFragmentProgram);
    cgGLEnableTextureParameter(myCgFragmentParam_normalMap);
    cgGLEnableProfile(myCgFragmentProfile); {
      const GLuint write_mask = ~0;  // Use all stencil bits
      glStencilThenCoverFillPathInstancedNV((GLsizei)messageLen,
        GL_UNSIGNED_BYTE, message, glyphBase,
        GL_PATH_FILL_MODE_NV, write_mask,
        GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
        GL_TRANSLATE_X_NV, xtranslate);
    } cgGLDisableProfile(myCgFragmentProfile);
  }

  if (draw_light) {
    glDisable(GL_STENCIL_TEST);
    glMatrixPushEXT(GL_MODELVIEW); {
      glMatrixTranslatefEXT(GL_MODELVIEW, lx*1000, ly*1000, lz*100);  // XXX 1000s here are hacks
      glColor3f(1,1,0);
      glutSolidSphere(100, 12, 12);
    } glMatrixPopEXT(GL_MODELVIEW);
    glEnable(GL_STENCIL_TEST);
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

void reshape(int w, int h)
{
  glViewport(0,0,w,h);
  iheight = h;
  window_width = w;
  window_height = h;
  aspect_ratio = window_height/window_width;

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
        scaling = 1;
      } else {
        scaling = 0;
      }
      if (!(glutGetModifiers() & GLUT_ACTIVE_SHIFT)) {
        rotating = 1;
      } else {
        rotating = 0;
      }
    } else {
      rotating = 0;
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
  mouse_space_y = iheight - mouse_space_y;

  if (rotating || scaling) {
    Transform3x2 t, r, s, m;
    float angle = 0;
    float zoom = 1;
    if (scaling) {
      angle = 0.3 * (mouse_space_x - rotate_x) * 640.0/window_width;
    }
    if (rotating) {
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
#if 0
    MatrixPrint("s", s);
    MatrixPrint("r", r);
    MatrixPrint("t", t);
    MatrixPrint("view", view);
#endif
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

static void updateLightPost()
{
  float center = 6.25;
  float r = 5.5;

  if (light_view) {
    lx = r*cos(time)+center;
    ly = r*sin(time);
    lz = dolly;
  } else {
    lx = r*cos(time)+center;
    ly = dolly;
    lz = r*sin(time);
  }
  cgSetParameter3f(myCgFragmentParam_lightPos, lx, ly, lz);
}

static void idle()
{
  float now = glutGet(GLUT_ELAPSED_TIME);
  time += (now - last_time) / 1000.0 * (3.1419/2);
  last_time = now;

  updateLightPost();
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

void
keyboard(unsigned char c, int x, int y)
{
  switch (c) {
  case 27:  // Esc quits
    exit(0);
    return;
  case 13:  // Enter redisplays
    break;
  case 's':
    stroking = !stroking;
    break;
  case 'r':  // Reset to original view
    initModelAndViewMatrices();
    configureProjection();
    break;
  case 'f':  // Toggle filling
    filling = !filling;
    break;
  case 'i':
    initGraphics(emScale);
    break;
  case 'u':  // Cycle underline modes
    underline = (underline+1)%3;
    break;
  case 'b':  // Toggle clear color backgrounds
    background = (background+1)%4;
    setBackground();
    break;
  case ' ':  // Toggle animation
    animation = !animation;
    last_time = glutGet(GLUT_ELAPSED_TIME);
    doAnimation();
    return;
  case 'l':  // Toggle rendering light source position
    draw_light = !draw_light;
    break;
  case 'a':
    dolly += 0.25;
    updateLightPost();
    break;
  case 'z':
    dolly -= 0.25;
    updateLightPost();
    break;
  case 'v':
    light_view = 1-light_view;
    updateLightPost();
    break;
  default:
    return;
  }
  glutPostRedisplay();
}

static void checkForCgError(const char *situation)
{
  CGerror error;
  const char *string = cgGetLastErrorString(&error);

  if (error != CG_NO_ERROR) {
    printf("%s: %s: %s\n",
      myProgramName, situation, string);
    if (error == CG_COMPILER_ERROR) {
      printf("%s\n", cgGetLastListing(myCgContext));
    }
    exit(1);
  }
}

static void cgInit()
{
  myCgContext = cgCreateContext();
  checkForCgError("creating context");
  cgGLSetDebugMode(CG_FALSE);
  cgGLSetManageTextureParameters(myCgContext, CG_TRUE);
  cgSetParameterSettingMode(myCgContext, CG_DEFERRED_PARAMETER_SETTING);

  myCgFragmentProfile = cgGLGetLatestProfile(CG_GL_FRAGMENT);
  cgGLSetOptimalOptions(myCgFragmentProfile);
  checkForCgError("selecting fragment profile");

  myCgFragmentProgram =
    cgCreateProgramFromFile(
      myCgContext,                /* Cg runtime context */
      CG_SOURCE,                  /* Program in human-readable form */
      myFragmentProgramFileName,  /* Name of file containing program */
      myCgFragmentProfile,        /* Profile: OpenGL ARB vertex program */
      myFragmentProgramName,      /* Entry function name */
      NULL);                      /* No extra compiler options */
  checkForCgError("creating fragment program from file");
  cgGLLoadProgram(myCgFragmentProgram);
  checkForCgError("loading fragment program");

  myCgFragmentParam_normalMap =
    cgGetNamedParameter(myCgFragmentProgram, "normalMap");
  checkForCgError("getting normalMap parameter");

  cgGLSetTextureParameter(myCgFragmentParam_normalMap,
    TO_NORMAL_MAP);
  checkForCgError("setting normal map 2D texture");

  myCgFragmentParam_lightPos =
    cgGetNamedParameter(myCgFragmentProgram, "lightPos");
  checkForCgError("getting lightPos parameter");
  updateLightPost();
}

int
main(int argc, char **argv)
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

  glutCreateWindow("Cg programmable shading with NV_path_rendering");
  printf("vendor: %s\n", glGetString(GL_VENDOR));
  printf("version: %s\n", glGetString(GL_VERSION));
  printf("renderer: %s\n", glGetString(GL_RENDERER));
  printf("samples = %d\n", glutGet(GLUT_WINDOW_NUM_SAMPLES));
  printf("Executable: %d bit\n", (int)(8*sizeof(int*)));

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  initModelAndViewMatrices();

  status = glewInit();
  if (status != GLEW_OK) {
    fatalError("OpenGL Extension Wrangler (GLEW) failed to initialize");
  }
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
  cgInit();
  initBrickNormalMap();

  glutMainLoop();
  return 0;
}

