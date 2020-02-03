
/* nvpr_warp_tiger.cpp - warp classic PostScript tiger with NV_path_rendering */

// Copyright (c) NVIDIA Corporation. All rights reserved.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <Cg/double.hpp>
#include <Cg/vector/xyzw.hpp>
#include <Cg/matrix/1based.hpp>
#include <Cg/matrix/rows.hpp>
#include <Cg/vector.hpp>
#include <Cg/matrix.hpp>
#include <Cg/distance.hpp>
#include <Cg/mul.hpp>
#include <Cg/radians.hpp>
#include <Cg/transpose.hpp>
#include <Cg/stdlib.hpp>
#include <Cg/iostream.hpp>

using namespace Cg;
using std::cout;
using std::endl;

#include "nvpr_glew_init.h"
#include "tiger.h"
#include "request_vsync.h"
#include "showfps.h"
#include "cg4cpp_xform.hpp"

const char *program_name = "nvpr_warp_tiger";
int stroking = 1,
    filling = 1;

/* Scaling and rotation state. */
int2 anchor;  /* Anchor for rotation and scaling. */
int scale_y = 0, 
    rotate_x = 0;  /* Prior (x,y) location for scaling (vertical) or rotation (horizontal)? */
int zooming = 0;  /* Are we zooming currently? */
int scaling = 0;  /* Are we scaling (zooming) currently? */

/* Sliding (translation) state. */
float slide_x = 0,
      slide_y = 0;  /* Prior (x,y) location for sliding. */
int sliding = 0;  /* Are we sliding currently? */
unsigned int path_count;
int canvas_width = 640, canvas_height = 480;
float4 box;
bool show_bounding_box = true;
bool verbose = false;
bool animating = false;
bool enable_vsync = true;
bool show_fps = false;

float2 corners[4];
float2 moved_corners[4];
float3x3 model, view, inverse_view;

FPScontext gl_fps_context;

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

  glPointSize(5.0);

  initTiger();

  int path_count = getTigerPathCount();
  assert(path_count >= 1);
  GLuint path_base = getTigerBasePath();
  GLfloat bounds[4];
  glGetPathParameterfvNV(path_base, GL_PATH_OBJECT_BOUNDING_BOX_NV, bounds);
  for (int i=1; i<path_count; i++) {
    GLfloat tmp[4];
    glGetPathParameterfvNV(path_base+i, GL_PATH_OBJECT_BOUNDING_BOX_NV, tmp);
    if (tmp[0] < bounds[0]) {
      bounds[0] = tmp[0];
    }
    if (tmp[1] < bounds[1]) {
      bounds[1] = tmp[1];
    }
    if (tmp[2] > bounds[2]) {
      bounds[2] = tmp[2];
    }
    if (tmp[3] > bounds[3]) {
      bounds[3] = tmp[3];
    }
  }
  box = float4(bounds[0], bounds[1], bounds[2], bounds[3]);
  if (verbose) {
    printf("tiger bounds = (%f,%f) to (%f,%f)\n", bounds[0], bounds[1], bounds[2], bounds[3]);
  }

  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_NOTEQUAL, 0, 0x1F);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

  glEnable(GL_LINE_STIPPLE);
  glLineStipple(3, 0x8888);
}

void setModelMatrix()
{
  model = quad2quad(corners, moved_corners);
}

void initCornersFromBox(const float4 box, float2 corners[4])
{
  corners[0] = box.xy;
  corners[1] = box.zy;
  corners[2] = box.zw;
  corners[3] = box.xw;
}

void initModelAndViewMatrices()
{
  initCornersFromBox(box, corners);
  float4 moved_box = float4(canvas_width/2-0.95*canvas_height/2, 0.05*canvas_height,
                            canvas_width/2+0.95*canvas_height/2, 0.95*canvas_height);
  initCornersFromBox(moved_box, moved_corners);
  model = quad2quad(corners, moved_corners);

  setModelMatrix();
  view = float3x3(1,0,0,
                  0,1,0,
                  0,0,1);
  inverse_view = inverse(view);
}

void display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glMatrixPushEXT(GL_MODELVIEW); {
    float3x3 mat;

    mat = mul(view, model);
    MatrixLoadToGL(mat);
    glEnable(GL_STENCIL_TEST);
    drawTigerRange(filling, stroking, 0, path_count);
  } glMatrixPopEXT(GL_MODELVIEW);
  glDisable(GL_STENCIL_TEST);
  if (show_bounding_box) {
    glMatrixPushEXT(GL_MODELVIEW); {
      MatrixLoadToGL(view);
      glDisable(GL_STENCIL_TEST);
      glColor3f(1,1,0);
      glBegin(GL_LINE_LOOP); {
        for (int i=0; i<4; i++) {
          glVertex2f(moved_corners[i].x, moved_corners[i].y);
        }
      } glEnd();
      glColor3f(1,0,1);
      glBegin(GL_POINTS); {
        for (int i=0; i<4; i++) {
          glVertex2f(moved_corners[i].x, moved_corners[i].y);
        }
      } glEnd();
    } glMatrixPopEXT(GL_MODELVIEW);
  }
  handleFPS(&gl_fps_context);

  glutSwapBuffers();
}

float window_width, window_height;
float view_width, view_height;
float3x3 win2view, view2win;

void configureProjection()
{
  float3x3 iproj, viewport;

  viewport = ortho(0,window_width, 0,window_height);
  float left = 0, right = canvas_width, top = 0, bottom = canvas_height;
  glMatrixLoadIdentityEXT(GL_PROJECTION);
  glMatrixOrthoEXT(GL_PROJECTION, 0, canvas_width, canvas_height, 0, -1, 1);
  iproj = inverse_ortho(left, right, top, bottom);
  view_width = right - left;
  view_height = bottom - top;
  win2view = mul(iproj, viewport);
  view2win = inverse(win2view);
}

int iheight;

void reshape(int w, int h)
{
  reshapeFPScontext(&gl_fps_context, w, h);
  glViewport(0,0,w,h);
  iheight = h-1;
  window_width = w;
  window_height = h;

  configureProjection();
}

int animated_point = 0;
int last_time = 0;
float2 initial_point;
int flip = 1;

void idle()
{
  int now = glutGet(GLUT_ELAPSED_TIME);
  int elapsed = now-last_time;
  float t = 0.0007*(elapsed);

  // (x,y) of Figure-eight knot
  // http://en.wikipedia.org/wiki/Figure-eight_knot_%28mathematics%29
  float x = (2+::cos(2*t))*::cos(3*t) - 3;  // bias by -3 so t=0 is @ (0,0)
  float y = (2+::cos(2*t))*::sin(3*t);
  // Flip the sense of x & y to avoid a bias in a particular direction over time.
  if (flip & 2) {
    x = -x;
    y = -y;
  }
  float radius = 20;
  float2 xy = radius*float2(x,y);
  moved_corners[animated_point] = initial_point + xy;
  setModelMatrix();

  if (elapsed > 3500) {
    if (animated_point == 3) {
      flip++;
    }
    animated_point = (animated_point+1)%4;
    initial_point = moved_corners[animated_point];
    last_time = now;
  }

  glutPostRedisplay();
}

void
keyboard(unsigned char c, int x, int y)
{
  switch (c) {
  case 27:  /* Esc quits */
    exit(0);
    return;
  case 'b':
    show_bounding_box = !show_bounding_box;
    invalidateFPS();
    break;
  case 13:  /* Enter redisplays */
    break;
  case 's':
    stroking = !stroking;
    invalidateFPS();
    break;
  case 'f':
    filling = !filling;
    invalidateFPS();
    break;
  case 'r':
    initModelAndViewMatrices();
    break;
  case '+':
    path_count++;
    break;
  case '-':
    path_count--;
    break;
  case ' ':
    animating = !animating;
    if (animating) {
      last_time = glutGet(GLUT_ELAPSED_TIME);
      initial_point = moved_corners[animated_point];
      glutIdleFunc(idle);
      if (show_fps) {
        enableFPS();
      }
    } else {
      disableFPS();
      glutIdleFunc(NULL);
    }
    break;
  case 'v':
    enable_vsync = !enable_vsync;
    requestSynchornizedSwapBuffers(enable_vsync);
    break;
  case 'F':
    show_fps = !show_fps;
    if (show_fps) {
      enableFPS();
    } else {
      disableFPS();
    }
    break;
  default:
    return;
  }
  glutPostRedisplay();
}

float2 project(float3 v)
{
  return v.xy/v.z;
}

int close_corner = -1;

void
mouse(int button, int state, int mouse_space_x, int mouse_space_y)
{
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {
      float2 win = float2(mouse_space_x, mouse_space_y);

      float closest = 10;
      close_corner = -1;
      float2 close_corner_location;
      for (int i=0; i<4; i++) {
        float3 p = float3(moved_corners[i], 1);
        p = mul(view, p);
        p = mul(view2win, p);
        float2 window_p = project(p);
        float distance_to_corner = distance(window_p, win);
        if (distance_to_corner < closest) {
          close_corner = i;
          closest = distance_to_corner;
          close_corner_location = window_p;
        }
      }
      if (close_corner < 0) {
        if (verbose) {
          cout << "no corner near " << win << endl;
        }
      } else {
        if (verbose) {
          cout << "corner " << close_corner << " @ " << close_corner_location << " near " << win << endl;
        }
        zooming = 0;
        scaling = 0;
        return;
      }

      anchor = project(mul(win2view, float3(win,1)));
      rotate_x = mouse_space_x;
      scale_y = mouse_space_y;
      if (glutGetModifiers() & GLUT_ACTIVE_CTRL) {
        scaling = 0;
      } else {
        scaling = 1;
      }
      if (glutGetModifiers() & GLUT_ACTIVE_SHIFT) {
        zooming = 0;
      } else {
        zooming = 1;
      }
    } else {
      zooming = 0;
      scaling = 0;
      close_corner = -1;
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
  if (close_corner >= 0) {
    assert(!zooming);
    assert(!scaling);
    float3 win = float3(mouse_space_x, mouse_space_y, 1);
    float3 p = mul(win2view, win);
    p = mul(inverse_view, p);
    float2 new_corner = project(p);
    if (verbose) {
      cout << "old corner = " << moved_corners[close_corner] << endl;
      cout << "new corner = " << new_corner << endl;
    }
    // Are we animating and is the selected corner the animated corner?
    if (animating && (close_corner == animated_point)) {
      // Yes, so just adjust the initial point.
      initial_point += (new_corner - moved_corners[close_corner]);
    } else {
      // No, so reposition the actual moved_corner.
      moved_corners[close_corner] = new_corner;
    }
    setModelMatrix();
    glutPostRedisplay();
    return;
  }

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

    t = translate(anchor.x, anchor.y);
    r = rotate(angle);
    s = scale(zoom, zoom);

    r = mul(r, s);
    m = mul(t, r);
    t = translate(-anchor.x, -anchor.y);
    m = mul(m, t);
    view = mul(m, view);
    inverse_view = inverse(view);
    rotate_x = mouse_space_x;
    scale_y = mouse_space_y;
    glutPostRedisplay();
  }
  if (sliding) {
    float3x3 m;

    float x_offset = (mouse_space_x - slide_x) * canvas_width/window_width;
    float y_offset = (mouse_space_y - slide_y) * canvas_height/window_height;
    m = translate(x_offset, y_offset);
    view = mul(m, view);
    inverse_view = inverse(view);
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
  if (!glutGet(GLUT_DISPLAY_MODE_POSSIBLE)) {
    glutInitDisplayString(NULL);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_STENCIL);
  }

  glutCreateWindow("Warped Classic PostScript tiger NV_path_rendering example");
  printf("vendor: %s\n", glGetString(GL_VENDOR));
  printf("version: %s\n", glGetString(GL_VERSION));
  printf("renderer: %s\n", glGetString(GL_RENDERER));
  printf("samples per pixel = %d\n", glutGet(GLUT_WINDOW_NUM_SAMPLES));
  printf("Executable: %d bit\n", (int)(8*sizeof(int*)));
  printf("\n");
  printf("Use left mouse button to scale/zoom (vertical, up/down)\n");
  printf("                      and rotate (right=clockwise, left=ccw)\n");
  printf("Click with left mouse and drag magenta points to warp scene\n");
  printf("\n");
  printf("Rotate and zooming is centered where you first left mouse click\n");
  printf("Hold down Ctrl at left mouse click to JUST SCALE\n");
  printf("Hold down Shift at left mouse click to JUST ROTATE\n");
  printf("\n");
  printf("Use middle mouse button to slide (translate)\n");
  printf("\n");
  printf("Space bar toggles warping animation\n");
  printf("\n");

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);

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
  initModelAndViewMatrices();
  requestSynchornizedSwapBuffers(enable_vsync);
  colorFPS(0,1,0);
  disableFPS();

  path_count = getTigerPathCount();

  initFPScontext(&gl_fps_context, FPS_USAGE_TEXTURE);

  glutMainLoop();
  return 0;
}

