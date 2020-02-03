
/* nvpr_gradient.c - render linear & radial gradients with NV_path_rendering */

// Copyright (c) NVIDIA Corporation. All rights reserved.

#include <stdio.h>
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

#include "mjkimage.h"

const char *program_name = "nvpr_gradient";
int stroking = 1,
    filling = 1,
    do_grid = 1;

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

Transform3x2 model,
             view;

enum {
  TEXTURE_GRADIENT_3X3 = 1,
  TEXTURE_FACE = 2
} texobj = TEXTURE_FACE;

static void fatalError(const char *message)
{
  fprintf(stderr, "%s: %s\n", program_name, message);
  exit(1);
}

static void reportPathObjectBounds(GLuint pathObj)
{
  GLfloat object_bbox[4],
          fill_bbox[4],
          stroke_bbox[4];

  glGetPathParameterfvNV(pathObj, GL_PATH_OBJECT_BOUNDING_BOX_NV, object_bbox);
  glGetPathParameterfvNV(pathObj, GL_PATH_FILL_BOUNDING_BOX_NV, fill_bbox);
  glGetPathParameterfvNV(pathObj, GL_PATH_STROKE_BOUNDING_BOX_NV, stroke_bbox);
  printf("object_bbox = %f,%f to %f,%f\n", object_bbox[0], object_bbox[1], object_bbox[2], object_bbox[3]);
  printf("fill_bbox   = %f,%f to %f,%f\n", fill_bbox[0], fill_bbox[1], fill_bbox[2], fill_bbox[3]);
  printf("stroke_bbox = %f,%f to %f,%f\n", stroke_bbox[0], stroke_bbox[1], stroke_bbox[2], stroke_bbox[3]);
}

void makeRectangle(GLuint pathObj, float x, float y, float w, float h, float r)
{
  GLubyte cmd[1000];  /* XXX array dimensioning sloppy */
  GLfloat coord[5000];
  int m = 0, n = 0;
  int i, j, k, num = 0;

  cmd[m++] = GL_MOVE_TO_NV;
  coord[n++] = x;
  coord[n++] = y;
  cmd[m++] = GL_CUBIC_CURVE_TO_NV;
  coord[n++] = x+w/3;
  coord[n++] = y+h*0.1;
  coord[n++] = x+2*w/3;
  coord[n++] = y-h*0.1;
  coord[n++] = x+w;
  coord[n++] = y;
  cmd[m++] = GL_CUBIC_CURVE_TO_NV;
  coord[n++] = x+w-w*0.1;
  coord[n++] = y+h/3;
  coord[n++] = x+w+w*0.1;
  coord[n++] = y+2*h/3;
  coord[n++] = x+w;
  coord[n++] = y+h;
  cmd[m++] = GL_RELATIVE_HORIZONTAL_LINE_TO_NV;
  coord[n++] = -w;
  cmd[m++] = GL_CLOSE_PATH_NV;
  /* Carve out holes of radius r in a 5x5 grid */
  for (i=0; i<5; i++) {
    for (j=0; j<5; j++) {
      int circle = num&1;
      if (circle) {
        cmd[m++] = GL_MOVE_TO_NV;
        coord[n++] = x + (i+0.5)/5.0*w + r;
        coord[n++] = y + (j+0.5)/5.0*h;
        cmd[m++] = GL_RELATIVE_ARC_TO_NV;
        coord[n++] = r;
        coord[n++] = r;
        coord[n++] = 0; /* x-axis-rotation */
        coord[n++] = 1; /* large-arc-flag */
        coord[n++] = 0; /* sweep-flag */
        coord[n++] = -2*r;
        coord[n++] = 0;
        cmd[m++] = GL_RELATIVE_ARC_TO_NV;
        coord[n++] = r;
        coord[n++] = r;
        coord[n++] = 0; /* x-axis-rotation */
        coord[n++] = 1; /* large-arc-flag */
        coord[n++] = 0; /* sweep-flag */
        coord[n++] = 2*r;
        coord[n++] = 0;
      } else {
        float cx = x + (i+0.5)/5.0*w,
              cy = y + (j+0.5)/5.0*h;
        int sides = 5;
        float delta = (2*3.14159)/sides;
        cmd[m++] = GL_MOVE_TO_NV;
        coord[n++] = x + (i+0.5)/5.0*w;
        coord[n++] = y + (j+0.5)/5.0*h + r;
        for (k=1; k<=sides; k++) {
          float angle = k*delta;
          float xx = cx + r*sin(angle),
                yy = cy + r*cos(angle);
          cmd[m++] = GL_LINE_TO_NV;
          coord[n++] = xx;
          coord[n++] = yy;
        }
        cmd[m++] = GL_CLOSE_PATH_NV;
      }
      num++;
    }
  }
  glPathCommandsNV(pathObj, m, cmd, n, GL_FLOAT, coord);
}

const GLuint rectangle_path = 42;

void initPaths()
{
  makeRectangle(rectangle_path, -100, -100, 200, 200, 10);
  reportPathObjectBounds(rectangle_path);
}

void drawPaths()
{
  GLfloat data[2][3] = { { 1,0,0 },    /* s = 1*x + 0*y + 0 */
                         { 0,1,0 } };  /* t = 0*x + 1*y + 0 */

  glEnable(GL_STENCIL_TEST);
  
  glColor3f(1,1,1);
  if (filling) {
    glEnable(GL_TEXTURE_2D);
    glPathTexGenNV(GL_TEXTURE0, GL_PATH_OBJECT_BOUNDING_BOX_NV, 2, &data[0][0]);
    glStencilFillPathNV(rectangle_path, GL_COUNT_UP_NV, 0x1F);
    glCoverFillPathNV(rectangle_path, GL_BOUNDING_BOX_NV);
  }

  if (stroking) {
    GLint reference = 0x1;
    glDisable(GL_TEXTURE_2D);
    glPathTexGenNV(GL_TEXTURE0, GL_NONE, 0, NULL);
    glPathColorGenNV(GL_PRIMARY_COLOR, GL_NONE, GL_RGBA, NULL);
    glColor3f(1,1,0);
    glStencilStrokePathNV(rectangle_path, reference, 0x1F);
    glCoverStrokePathNV(rectangle_path, GL_BOUNDING_BOX_NV);
  }
}

void makeGradient3x3Texture(GLuint texobj)
{
  GLfloat pixels[3][3][3];
  int i, j;

  for (i=0; i<3; i++) {
    for (j=0; j<3; j++) {
      pixels[i][j][0] = i/3.0;
      pixels[i][j][1] = 1-j/3.0;
      pixels[i][j][2] = 1;
    }
  }

  pixels[1][1][2] = 0;  /* Force blue to zero for the middle texel. */

  glBindTexture(GL_TEXTURE_2D, texobj);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 3,3,0, GL_RGB, GL_FLOAT, pixels);
}

void makeFaceTexture(GLuint texobj)
{
  int width, height;
  unsigned char *bits;
  width = mjk_width;
  height = mjk_height;
  bits = mjk_image;

  glBindTexture(GL_TEXTURE_2D, texobj);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0,
    GL_RGB, GL_UNSIGNED_BYTE, bits);
}

void initGraphics()
{
  /* Use an orthographic path-to-clip-space transform to map the
     [0..640]x[0..480] range of the star's path coordinates to the [-1..1]
     clip space cube: */
  glMatrixLoadIdentityEXT(GL_PROJECTION);
  glMatrixOrthoEXT(GL_PROJECTION, 0, 640, 480, 0, -1, 1);
  glMatrixLoadIdentityEXT(GL_MODELVIEW);

  /* Before rendering to a window with a stencil buffer, clear the stencil
  buffer to zero and the color buffer to blue: */
  glClearStencil(0);
  glClearColor(0.1, 0.3, 0.6, 0.0);

  initPaths();

  glStencilFunc(GL_NOTEQUAL, 0, 0x1F);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

  makeGradient3x3Texture(TEXTURE_GRADIENT_3X3);
  makeFaceTexture(TEXTURE_FACE);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

void initModelAndViewMatrices()
{
  float tmp[2][3];

  identity(model);
  rotate(tmp, 10);
  mul(model, tmp, model);
  scale(tmp, 2.0, -2.0);
  mul(model, tmp, model);
  translate(tmp, 640/2,480/2);
  mul(model, tmp, model);
  identity(view);
}

void
drawGrid(float w, float h, int xspacing, int yspacing)
{
    float dx = w/xspacing,
          dy = h/yspacing;
    int i;

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_TEXTURE_2D);
    glColor3f(1,1,1);
    glBegin(GL_LINES); {
        for (i=1; i<xspacing; i++) {
            glVertex2f(i*dx, 0);
            glVertex2f(i*dx, h);
        }
        for (i=1; i<yspacing; i++) {
            glVertex2f(0, i*dy);
            glVertex2f(w, i*dy);
        }
    } glEnd();
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  if (do_grid) {
      drawGrid(640, 480, 30, 20);
  }
  glMatrixPushEXT(GL_MODELVIEW); {
    Transform3x2 mat;

    mul(mat, view, model);

    MatrixLoadToGL(mat);
    drawPaths();
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
  case 'f':
    filling = !filling;
    break;
  case 'r':
    initModelAndViewMatrices();
    break;
  case 'g':
    do_grid = !do_grid;
    break;
  case 't':
    texobj = 3-texobj;  /* Toggle between 1 and 2. */
    printf("texobj = %d\n", texobj);
    glBindTexture(GL_TEXTURE_2D, texobj);
    break;
  default:
    return;
  }
  glutPostRedisplay();
}

void
mouse(int button, int state, int mouse_space_x, int mouse_space_y)
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

void
motion(int mouse_space_x, int mouse_space_y)
{
  if (zooming || scaling) {
    Transform3x2 t, r, s, m;
    float angle = 0;
    float zoom = 1;
    if (scaling) {
      angle = 0.3 * (mouse_space_x - rotate_x) * 640.0/window_width;
    }
    if (zooming) {
      zoom = pow(1.003, (mouse_space_y - scale_y) * 480.0/window_height);
    }

    translate(t, anchor_x* 640.0/window_width, anchor_y* 480.0/window_height);
    rotate(r, angle);
    scale(s, zoom, zoom);
    mul(r, r, s);
    mul(m, t, r);
    translate(t, -anchor_x* 640.0/window_width, -anchor_y* 480.0/window_height);
    mul(m, m, t);
    mul(view, m, view);
    rotate_x = mouse_space_x;
    scale_y = mouse_space_y;
    glutPostRedisplay();
  }
  if (sliding) {
    float m[2][3];

    float x_offset = (mouse_space_x - slide_x) * 640.0/window_width;
    float y_offset = (mouse_space_y - slide_y) * 480.0/window_height;
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

  glutCreateWindow("NV_path_rendering gradient example");
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
  glutAddMenuEntry("[f] Toggle filling", 'f');
  glutAddMenuEntry("[s] Toggle stroking", 's');
  glutAddMenuEntry("[r] Reset view", 'r');
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

  initialize_NVPR_GLEW_emulation(stdout, program_name, 0);
  if (!has_NV_path_rendering) {
    fatalError("required NV_path_rendering OpenGL extension is not present");
  }
  initGraphics();

  glutMainLoop();
  return 0;
}

