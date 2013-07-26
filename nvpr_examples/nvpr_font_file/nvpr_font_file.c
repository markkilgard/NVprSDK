
/* nvpr_font_file.c - fonts loaded from files rendered with NV_path_rendering */

/* Copyright (c) NVIDIA Corporation. All rights reserved. */

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

#include "nvpr_init.h"
#include "sRGB_math.h"
#include "xform.h"

int stroking = 1;
int filling = 1;
int underline = 2;
int regular_aspect = 1;
int fill_gradient = 2;
int use_sRGB = 0;
int hasFramebufferSRGB = 0;
GLint sRGB_capable = 0;
const char *programName = "nvpr_font_file";

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

Transform3x2 model,
             view;

void initglext(void)
{
  hasFramebufferSRGB = glutExtensionSupported("GL_EXT_framebuffer_sRGB");
}

/* Global variables */
GLuint glyphBase;
GLuint pathTemplate;
const char *message = "Hello world!"; /* the message to show */
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

GLfloat yMin, yMax, underline_position, underline_thickness;
GLfloat totalAdvance, xBorder;

int emScale = 2048;

static void fatalError(const char *message)
{
  fprintf(stderr, "%s: %s\n", programName, message);
  exit(1);
}

void
initGraphics(int emScale)
{
  const unsigned char *message_ub = (const unsigned char*)message;
  float font_data[4];
  const int numChars = 256;  /* ISO/IEC 8859-1 8-bit character range */
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
  /* Use the path object at the end of the range as a template. */
  pathTemplate = glyphBase+numChars;
  glPathCommandsNV(pathTemplate, 0, NULL, 0, GL_FLOAT, NULL);
  /* Stroke width is 5% of the em scale. */
  glPathParameteriNV(pathTemplate, GL_PATH_STROKE_WIDTH_NV, 0.05 * emScale);
  glPathParameteriNV(pathTemplate, GL_PATH_JOIN_STYLE_NV, GL_ROUND_NV);
  glyphBase++;
  /* Choose a bold sans-serif font face, preferring Veranda over Arial; if
     neither font is available as a system font, settle for the "Sans" standard
     (built-in) font. */
  glPathGlyphRangeNV(glyphBase, 
                     GL_FILE_NAME_NV, "Pacifico.ttf", 0,
                     0, numChars,
                     GL_SKIP_MISSING_GLYPH_NV, pathTemplate, emScale);
  glPathGlyphRangeNV(glyphBase,
                     GL_STANDARD_FONT_NAME_NV, "Sans", GL_BOLD_BIT_NV,
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
    fprintf(stderr, "%s: malloc of xtranslate failed\n", programName);
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

  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_NOTEQUAL, 0, ~0);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
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
    glStencilStrokePathInstancedNV((GLsizei)messageLen,
      GL_UNSIGNED_BYTE, message, glyphBase,
      1, ~0,  /* Use all stencil bits */
      GL_TRANSLATE_X_NV, xtranslate);
    glColor3f(0.5,0.5,0.5);  /* gray */
    glCoverStrokePathInstancedNV((GLsizei)messageLen,
      GL_UNSIGNED_BYTE, message, glyphBase,
      GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
      GL_TRANSLATE_X_NV, xtranslate);
  }

  if (filling) {
    /* STEP 1: stencil message into stencil buffer.  Results in samples
       within the message's glyphs to have a non-zero stencil value. */
    glStencilFillPathInstancedNV((GLsizei)messageLen,
                                 GL_UNSIGNED_BYTE, message, glyphBase,
                                 GL_PATH_FILL_MODE_NV, ~0,  /* Use all stencil bits */
                                 GL_TRANSLATE_X_NV, xtranslate);

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
      glColor3ub(192, 192, 192);  /* gray */
      break;
    case 2:
      glColor3ub(255, 255, 255);  /* white */
      break;
    case 3:
      glColor3ub(0, 0, 0);  /* black */
      break;
    }
    glCoverFillPathInstancedNV((GLsizei)messageLen,
                               GL_UNSIGNED_BYTE, message, glyphBase,
                               GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
                               GL_TRANSLATE_X_NV, xtranslate);
    if (fill_gradient == 0) {
      /* Disable gradient. */
      glPathColorGenNV(GL_PRIMARY_COLOR, GL_NONE, 0, NULL);
    }
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
  if (regular_aspect) {
    float w = totalAdvance,
          h = yMax-yMin;
    if (h < w) {
      left = -xBorder;
      right = totalAdvance+xBorder;
      top = -0.5*totalAdvance*aspect_ratio + (yMax+yMin)/2;
      bottom = 0.5*totalAdvance*aspect_ratio + (yMax+yMin)/2;
      /* Configure canvas so text is centered nicely with spacing on sides. */
    } else {
      left = -0.5*h*aspect_ratio + totalAdvance/2;
      right = 0.5*h*aspect_ratio + totalAdvance/2;
      top = yMin;
      bottom = yMax;
      /* Configure canvas so text is centered nicely with spacing on sides. */
    }
  } else {
    left = 0;
    right = totalAdvance;
    top = yMin;
    bottom = yMax;
    /* Configure canvas coordinate system from (0,yMin) to (totalAdvance,yMax). */
  }
  glOrtho(left, right, top, bottom,
    -1, 1);
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
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
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
  mouse_space_y = iheight - mouse_space_y;

  if (zooming || scaling) {
    Transform3x2 t, r, s, m;
    float angle = 0;
    float zoom = 1;
    if (scaling) {
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
    underline = (underline+1)%3;
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
  int i;

  glutInitWindowSize(640, 480);
  glutInit(&argc, argv);
  for (i=1; i<argc; i++) {
    if (!strcmp("-emscale", argv[i])) {
      i++;
      if (i < argc) {
        emScale = atoi(argv[i]);
        printf("emScale = %d\n", emScale);
        continue;
      } else {
        printf("%s: -emscale must be followed by value\n",
            programName);
        exit(1);
      }
    } else if (argv[i][0] == '-') {
      /* Set number of samples per pixel. */
      int value = atoi(argv[i]+1);
      if (value >= 1) {
        samples = value;
        continue;
      }
    }
    /* Unrecognized option so print usage message and exit. */
    fprintf(stderr, "usage: %s [-#] [-emscale n]\n"
                    "where # is the number of samples/pixel\n",
                    "where n is the em scale (default is 2048)\n",
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

  glutCreateWindow("Fonts loaded from files with NV_path_rendering");
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

  initializeNVPR(programName);
  if (!has_NV_path_rendering) {
    fatalError("required NV_path_rendering OpenGL extension is not present");
  }
  initGraphics(emScale);

  glutMainLoop();
  return 0;
}

