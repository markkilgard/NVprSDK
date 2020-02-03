
/* nvpr_cursive.c - animated cursive tracing of text with dashing */

// Copyright (c) NVIDIA Corporation. All rights reserved.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "nvpr_glew_init.h"
#include "xform.h"

const char *program_name = "nvpr_cursive";
int animating = 0;

/* Scaling and rotation state. */
int anchor_x = 0,
    anchor_y = 0;  /* Anchor for rotation and scaling. */
int scale_y = 0, 
    rotate_x = 0;  /* Prior (x,y) location for scaling (vertical) or rotation (horizontal)? */
int zooming = 0;  /* Are we zooming currently? */
int scaling = 0;  /* Are we scaling (zooming) currently? */

/* Sliding (translation) state. */
float slide_x = 0,
      slide_y = 0;  /* Prior (x,y) location for sliding. */
int sliding = 0;  /* Are we sliding currently? */
int canvas_width = 640, canvas_height = 480;

Transform3x2 model,
             view;

GLuint crazy_path = 1;  // path object name
GLfloat percent = 1.0;  // initially 100%

extern const char crazy_svg[];  // SVG string in this_is_crazy.c

static void fatalError(const char *message)
{
  fprintf(stderr, "%s: %s\n", program_name, message);
  exit(1);
}

void initGraphics()
{
  /* Use an orthographic path-to-clip-space transform to map the
  [0..640]x[0..480] range of the star's path coordinates to the [-1..1]
  clip space cube: */
  glMatrixLoadIdentityEXT(GL_PROJECTION);
  glMatrixOrthoEXT(GL_PROJECTION, 0, canvas_width, canvas_height, 0, -1, 1);
  glMatrixLoadIdentityEXT(GL_MODELVIEW);

  /* Before rendering to a window with a stencil buffer, clear the stencil
  buffer to zero and the color buffer to blue: */
  glClearStencil(0);
  glClearColor(0.1, 0.3, 0.6, 0.0);

  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_NOTEQUAL, 0, 0x1F);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
}

void initModelAndViewMatrices()
{
  float tmp[2][3];

  scale(model, 0.75, 0.75);
  translate(tmp, -10, 120);  /* magic values to get text centered */
  mul(model, model, tmp);
  translate(view, 0, 0);
}

GLfloat totalLength;

static void setLength(float percent)
{
  GLfloat pattern[2];

  pattern[0] = percent*totalLength;
  pattern[1] = (1.01-percent)*totalLength;
  glPathDashArrayNV(crazy_path, 2, pattern);
}

static void initPath()
{
  GLint segments = 0;
  GLfloat stroke_width = 3.1f;

  glPathStringNV(crazy_path, GL_PATH_FORMAT_SVG_NV,
    (GLsizei)strlen(crazy_svg), crazy_svg);

  glGetPathParameterivNV(crazy_path, GL_PATH_COMMAND_COUNT_NV, &segments);
  printf("segments = %d\n", segments);
  totalLength = glGetPathLengthNV(crazy_path, 0, segments);
  printf("totalLength = %f\n", totalLength);
  glPathParameterfNV(crazy_path, GL_PATH_STROKE_WIDTH_NV, stroke_width);
  glPathParameterfNV(crazy_path, GL_PATH_END_CAPS_NV, GL_ROUND_NV);
  glPathParameterfNV(crazy_path, GL_PATH_DASH_CAPS_NV, GL_ROUND_NV);
  glPathParameterfNV(crazy_path, GL_PATH_DASH_CAPS_NV, GL_ROUND_NV);
}

static void drawPath()
{
  GLint reference = 0x1;
  GLuint mask = 0xFF;
  glStencilStrokePathNV(crazy_path, reference, mask);
  glCoverStrokePathNV(crazy_path, GL_BOUNDING_BOX_NV);
}

void display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glMatrixPushEXT(GL_MODELVIEW); {
    Transform3x2 mat;

    mul(mat, view, model);
    MatrixLoadToGL(mat);
    drawPath();
  } glMatrixPopEXT(GL_MODELVIEW);
  glutSwapBuffers();
}

float window_width, window_height;

static void reshape(int w, int h)
{
  glViewport(0,0, w,h);
  window_width = w;
  window_height = h;
}

static void idle(void)
{
  percent += 0.0025;
  if (percent > 1.0) {
    percent = 0.0;
  }
  setLength(percent);
  glutPostRedisplay();
}

void keyboard(unsigned char c, int x, int y)
{
  switch (c) {
  case 27:  /* Esc quits */
    exit(0);
    return;
  case 13:  /* Enter redisplays */
    break;
  case 'r':
    initModelAndViewMatrices();
    break;
  case '+':
    percent += 0.0025f;
    if (percent > 1) {
      percent = 0.0f;
    }
    setLength(percent);
    break;
  case '-':
    percent -= 0.0025f;
    if (percent < 0) {
      percent = 1.0f;
    }
    setLength(percent);
    break;
  case ' ':
    animating = !animating;
    if (animating) {
      glutIdleFunc(idle);
    } else {
      glutIdleFunc(NULL);
    }
    break;
  default:
    return;
  }
  glutPostRedisplay();
}

void mouse(int button, int state, int mouse_space_x, int mouse_space_y)
{
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {
      anchor_x = mouse_space_x;
      anchor_y = mouse_space_y;
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
      slide_y = mouse_space_y;
      slide_x = mouse_space_x;
      sliding = 1;
    } else {
      sliding = 0;
    }
  }
}

void motion(int mouse_space_x, int mouse_space_y)
{
  if (zooming || scaling) {
    Transform3x2 t, r, s, m;
    float angle = 0;
    float zoom = 1;
    if (scaling) {
      angle = 0.3 * (rotate_x - mouse_space_x) * canvas_width/window_width;
    }
    if (zooming) {
      zoom = pow(1.003, (mouse_space_y - scale_y) * canvas_height/window_height);
    }

    translate(t, anchor_x* canvas_width/window_width, anchor_y* canvas_height/window_height);
    rotate(r, angle);
    scale(s, zoom, zoom);
    mul(r, r, s);
    mul(m, t, r);
    translate(t, -anchor_x* canvas_width/window_width, -anchor_y* canvas_height/window_height);
    mul(m, m, t);
    mul(view, m, view);
    rotate_x = mouse_space_x;
    scale_y = mouse_space_y;
    glutPostRedisplay();
  }
  if (sliding) {
    float m[2][3];

    float x_offset = (mouse_space_x - slide_x) * canvas_width/window_width;
    float y_offset = (mouse_space_y - slide_y) * canvas_height/window_height;
    translate(m, x_offset, y_offset);
    mul(view, m, view);
    slide_y = mouse_space_y;
    slide_x = mouse_space_x;
    glutPostRedisplay();
  }
}

static void menu(int choice)
{
  keyboard(choice, 0, 0);
}

int main(int argc, char **argv)
{
  GLenum status;
  GLboolean hasDSA;
  int samples = 0;
  int i;

  glutInitWindowSize(canvas_width, canvas_height);
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

  glutCreateWindow("Animated cursive with NV_path_rendering dashing");
  printf("vendor: %s\n", glGetString(GL_VENDOR));
  printf("version: %s\n", glGetString(GL_VERSION));
  printf("renderer: %s\n", glGetString(GL_RENDERER));
  printf("samples per pixel = %d\n", glutGet(GLUT_WINDOW_NUM_SAMPLES));
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

  initModelAndViewMatrices();

  glutCreateMenu(menu);
  glutAddMenuEntry("[ ] Toggle animation", ' ');
  glutAddMenuEntry("[r] Reset view", 'r');
  glutAddMenuEntry("[+] Increase stroke", '+');
  glutAddMenuEntry("[-] Decrease stroke", 'r');
  glutAddMenuEntry("[Esc] Quit", 27);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  status = glewInit();
  if (status != GLEW_OK) {
    fatalError("OpenGL Extension Wrangler (GLEW) failed to initialize");
  }
  // Use glutExtensionSupported because glewIsSupported is unreliable for DSA.
  hasDSA = glutExtensionSupported("GL_EXT_direct_state_access");
  if (!hasDSA) {
    fatalError("OpenGL implementation doesn't support GL_EXT_direct_state_access (you should be using NVIDIA GPUs...)");
  }

  initialize_NVPR_GLEW_emulation(stderr, program_name, 0);
  if (!has_NV_path_rendering) {
    fatalError("required NV_path_rendering OpenGL extension is not present");
  }
  initGraphics();
  initPath();
  setLength(percent);

  glutMainLoop();
  return 0;
}

