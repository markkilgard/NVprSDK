
/* nvpr_gears.cpp - gears rendered with NV_path_rendering (or 3D) */

// Inspired by Brian Paul's glxgears.c demo...

// http://cgit.freedesktop.org/mesa/demos/tree/src/demos/gears.c
// http://www.opensource.apple.com/source/X11apps/X11apps-13/glxgears.c

/*
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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

#include <vector>

#include <Cg/double.hpp>
#include <Cg/vector/xyzw.hpp>
#include <Cg/vector.hpp>
#include <Cg/matrix.hpp>
#include <Cg/mul.hpp>
#include <Cg/degrees.hpp>
#include <Cg/iostream.hpp>

using namespace Cg;
using std::endl;
using std::cout;
using std::vector;

#ifndef M_PI  // grr, Win32!
#define M_PI 3.14159265
#endif

#include "countof.h"
#include "request_vsync.h"
#include "showfps.h"
#include "cg4cpp_xform.hpp"

const char *program_name = "nvpr_gears";
bool enable_vsync = true;
int canvas_width = 800, canvas_height = 800;
bool useRoundTeeth = true;

FPScontext gl_fps_context;

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

float3x3 view;

/* Global variables */
int background = 0;

void setBackground()
{
  float r, g, b, a;

  switch (background) {
  default:
  case 0:
    r = g = b = 0.0;  // black
    break;
  case 1:
    r = g = b = 1.0;  // white
    break;
  case 2:
    r = 0.1;
    g = 0.3;
    b = 0.6;  // nice blue background
    break;
  case 3:
    r = g = b = 0.5;  // gray
    break;
  }
  a = 1.0;
  glClearColor(r,g,b,a);
}

static void fatalError(const char *message)
{
  fprintf(stderr, "%s: %s\n", program_name, message);
  exit(1);
}

// Variables controlling the 3D view
static GLfloat viewDist = 40.0;
static GLfloat view_rotx, view_roty, view_rotz;

void initViewMatrix()
{
  view = float3x3(1,0,0,
                  0,1,0,
                  0,0,1);
  viewDist = 40.0;
  view_rotx = 20.0, view_roty = 30.0, view_rotz = 0.0;
}

float window_width, window_height;
float view_width, view_height;
float3x3 win2obj;

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
    float text_wheel_angle = 0;
    float zoom = 1;
    if (scaling) {
      text_wheel_angle = 0.3 * (mouse_space_x - rotate_x) * canvas_width/window_width;
    }
    if (zooming) {
      zoom = pow(1.003f, (mouse_space_y - scale_y) * canvas_height/window_height);
    }

    t = translate(anchor_x, anchor_y);
    r = rotate(text_wheel_angle);
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

static GLint win = 0;
static GLboolean Visible = GL_TRUE;
static GLboolean Animate = GL_TRUE;

static GLboolean view3d = GL_FALSE;  // falses means use 2D view
static GLboolean path_rendering = GL_TRUE;  // false means use 3D geometry

struct GearData {
  GLfloat inner_radius;
  GLfloat outer_radius;
  GLfloat width;
  GLint teeth;
  GLfloat tooth_depth;
};

/**

  Draw a gear wheel.  You'll probably want to call this function when
  building a display list since we do a lot of trig here.
 
  Input:  inner_radius - radius of hole at center
          outer_radius - radius at center of teeth
          width - width of gear
          teeth - number of teeth
          tooth_depth - depth of tooth

 **/

static void
gear(const GearData &gd)
{
  GLfloat angle;
  GLfloat u, v, len;

  const GLfloat r0 = gd.inner_radius;
  const GLfloat r1 = gd.outer_radius - gd.tooth_depth / 2.0;
  const GLfloat r2 = gd.outer_radius + gd.tooth_depth / 2.0;

  const int teeth = gd.teeth;
  const GLfloat width = gd.width;

  GLfloat da = 2.0 * M_PI / gd.teeth / 4.0;

  glShadeModel(GL_FLAT);

  glNormal3f(0.0, 0.0, 1.0);

  /* draw front face */
  glBegin(GL_QUAD_STRIP); {
    for (GLint i = 0; i <= gd.teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;
      glVertex3f(r0 * ::cos(angle), r0 * ::sin(angle), width * 0.5);
      glVertex3f(r1 * ::cos(angle), r1 * ::sin(angle), width * 0.5);
      if (i < teeth) {
        glVertex3f(r0 * ::cos(angle), r0 * ::sin(angle), width * 0.5);
        glVertex3f(r1 * ::cos(angle + 3 * da), r1 * ::sin(angle + 3 * da), width * 0.5);
      }
    }
  } glEnd();

  /* draw front sides of teeth */
  glBegin(GL_QUADS); {
    da = 2.0 * M_PI / gd.teeth / 4.0;
    for (GLint i = 0; i < teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;

      glVertex3f(r1 * ::cos(angle), r1 * ::sin(angle), width * 0.5);
      glVertex3f(r2 * ::cos(angle + da), r2 * ::sin(angle + da), width * 0.5);
      glVertex3f(r2 * ::cos(angle + 2 * da), r2 * ::sin(angle + 2 * da), width * 0.5);
      glVertex3f(r1 * ::cos(angle + 3 * da), r1 * ::sin(angle + 3 * da), width * 0.5);
    }
  } glEnd();

  glNormal3f(0.0, 0.0, -1.0);

  /* draw back face */
  glBegin(GL_QUAD_STRIP); {
    for (GLint i = 0; i <= teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;
      glVertex3f(r1 * ::cos(angle), r1 * ::sin(angle), -width * 0.5);
      glVertex3f(r0 * ::cos(angle), r0 * ::sin(angle), -width * 0.5);
      if (i < teeth) {
        glVertex3f(r1 * ::cos(angle + 3 * da), r1 * ::sin(angle + 3 * da), -width * 0.5);
        glVertex3f(r0 * ::cos(angle), r0 * ::sin(angle), -width * 0.5);
      }
    }
  } glEnd();

  /* draw back sides of teeth */
  glBegin(GL_QUADS); {
    da = 2.0 * M_PI / teeth / 4.0;
    for (GLint i = 0; i < teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;

      glVertex3f(r1 * ::cos(angle + 3 * da), r1 * ::sin(angle + 3 * da), -width * 0.5);
      glVertex3f(r2 * ::cos(angle + 2 * da), r2 * ::sin(angle + 2 * da), -width * 0.5);
      glVertex3f(r2 * ::cos(angle + da), r2 * ::sin(angle + da), -width * 0.5);
      glVertex3f(r1 * ::cos(angle), r1 * ::sin(angle), -width * 0.5);
    }
  } glEnd();

  /* draw outward faces of teeth */
  glBegin(GL_QUAD_STRIP); {
    for (GLint i = 0; i < teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;

      glVertex3f(r1 * ::cos(angle), r1 * ::sin(angle), width * 0.5);
      glVertex3f(r1 * ::cos(angle), r1 * ::sin(angle), -width * 0.5);
      u = r2 * ::cos(angle + da) - r1 * ::cos(angle);
      v = r2 * ::sin(angle + da) - r1 * ::sin(angle);
      len = ::sqrt(u * u + v * v);
      u /= len;
      v /= len;
      glNormal3f(v, -u, 0.0);
      glVertex3f(r2 * ::cos(angle + da), r2 * ::sin(angle + da), width * 0.5);
      glVertex3f(r2 * ::cos(angle + da), r2 * ::sin(angle + da), -width * 0.5);
      glNormal3f(::cos(angle), ::sin(angle), 0.0);
      glVertex3f(r2 * ::cos(angle + 2 * da), r2 * ::sin(angle + 2 * da), width * 0.5);
      glVertex3f(r2 * ::cos(angle + 2 * da), r2 * ::sin(angle + 2 * da), -width * 0.5);
      u = r1 * ::cos(angle + 3 * da) - r2 * ::cos(angle + 2 * da);
      v = r1 * ::sin(angle + 3 * da) - r2 * ::sin(angle + 2 * da);
      glNormal3f(v, -u, 0.0);
      glVertex3f(r1 * ::cos(angle + 3 * da), r1 * ::sin(angle + 3 * da), width * 0.5);
      glVertex3f(r1 * ::cos(angle + 3 * da), r1 * ::sin(angle + 3 * da), -width * 0.5);
      glNormal3f(::cos(angle), ::sin(angle), 0.0);
    }

    glVertex3f(r1 * ::cos(0.f), r1 * ::sin(0.f), width * 0.5);
    glVertex3f(r1 * ::cos(0.f), r1 * ::sin(0.f), -width * 0.5);

  } glEnd();

  glShadeModel(GL_SMOOTH);

  /* draw inside radius cylinder */
  glBegin(GL_QUAD_STRIP); {
    for (GLint i = 0; i <= teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;
      glNormal3f(-::cos(angle), -::sin(angle), 0.0);
      glVertex3f(r0 * ::cos(angle), r0 * ::sin(angle), -width * 0.5);
      glVertex3f(r0 * ::cos(angle), r0 * ::sin(angle), width * 0.5);
    }
  } glEnd();
}

static void addMove(vector<GLubyte> &cmds,
                    vector<GLfloat> &coords,
                    float x,
                    float y)
{
  cmds.push_back(GL_MOVE_TO_NV);
  coords.push_back(x);
  coords.push_back(y);
}


static void addLine(vector<GLubyte> &cmds,
                    vector<GLfloat> &coords,
                    float x,
                    float y)
{
  cmds.push_back(GL_LINE_TO_NV);
  coords.push_back(x);
  coords.push_back(y);
}

static void addClose(vector<GLubyte> &cmds,
                     vector<GLfloat> &coords)
{
  cmds.push_back(GL_CLOSE_PATH_NV);
}

static void addCircle(vector<GLubyte> &cmds,
                      vector<GLfloat> &coords,
                      float radius)
{
  cmds.push_back(GL_MOVE_TO_NV);
  coords.push_back(0);
  coords.push_back(radius);
  cmds.push_back(GL_ARC_TO_NV);
  coords.push_back(radius);
  coords.push_back(radius);
  coords.push_back(180.0);
  coords.push_back(true);  // large
  coords.push_back(false);  // false means clock-wise
  coords.push_back(0);
  coords.push_back(-radius);
  cmds.push_back(GL_ARC_TO_NV);
  coords.push_back(radius);
  coords.push_back(radius);
  coords.push_back(180.0);
  coords.push_back(true);  // large
  coords.push_back(false);  // false means clock-wise
  coords.push_back(0);
  coords.push_back(radius);
  addClose(cmds, coords);
}

static void
gear_path(GLuint path, const GearData &gd, bool round_gear_teeth)
{
  const GLfloat r0 = gd.inner_radius;
  const GLfloat r1 = gd.outer_radius - gd.tooth_depth / 2.0;
  const GLfloat r2 = gd.outer_radius + gd.tooth_depth / 2.0;

  const GLfloat da = 2.0 * M_PI / gd.teeth / 4.0;

  vector<GLubyte> cmds;
  vector<GLfloat> coords;

  /* draw outward faces of teeth */
  GLfloat last_angle = 0;
  for (GLint i = 0; i < gd.teeth; i++) {
    const GLfloat angle = i * 2.0 * M_PI / gd.teeth;

    if (i==0) {
      addMove(cmds, coords, r1 * ::cos(angle), r1 * ::sin(angle));
    } else {
      if (round_gear_teeth) {
        addLine(cmds, coords, r1 * ::cos(angle), r1 * ::sin(angle));
      } else {
        cmds.push_back(GL_CIRCULAR_CCW_ARC_TO_NV);
        coords.push_back(0);
        coords.push_back(0);
        coords.push_back(r1);
        coords.push_back(degrees(last_angle));
        coords.push_back(degrees(angle));
      }
    }
    addLine(cmds, coords, r2 * ::cos(angle + da), r2 * ::sin(angle + da));
    if (round_gear_teeth) {
      cmds.push_back(GL_CIRCULAR_CCW_ARC_TO_NV);
      coords.push_back(0);
      coords.push_back(0);
      coords.push_back(r2);
      coords.push_back(degrees(angle + da));
      coords.push_back(degrees(angle + 2*da));
    } else {
      addLine(cmds, coords, r2 * ::cos(angle + 2 * da), r2 * ::sin(angle + 2 * da));
    }
    addLine(cmds, coords, r1 * ::cos(angle + 3 * da), r1 * ::sin(angle + 3 * da));
    last_angle = angle;
  }
  addClose(cmds, coords);

  // Add an inner circle, winding the opposite direction
  addCircle(cmds, coords, r0);

  glPathCommandsNV(path,
    GLsizei(cmds.size()), &cmds[0],
    GLsizei(coords.size()), GL_FLOAT, &coords[0]);
  glPathParameterfNV(path, GL_PATH_STROKE_WIDTH_NV, 0.25f);
  glPathParameterfNV(path, GL_PATH_JOIN_STYLE_NV, GL_ROUND_NV);
}

static GLuint gear1, gear2, gear3;
static GLfloat angle = 0.0f;

static void
cleanup()
{
   glDeleteLists(gear1, 1);
   glDeleteLists(gear2, 1);
   glDeleteLists(gear3, 1);
   glDeletePathsNV(gear1, 1);
   glDeletePathsNV(gear2, 1);
   glDeletePathsNV(gear3, 1);
   glutDestroyWindow(win);
}

static void configView3D()
{
  assert(view3d);

  glMatrixTranslatefEXT(GL_MODELVIEW, 0.0, 0.0, -viewDist);

  glMatrixRotatefEXT(GL_MODELVIEW, view_rotx, 1.0, 0.0, 0.0);
  glMatrixRotatefEXT(GL_MODELVIEW, view_roty, 0.0, 1.0, 0.0);
  glMatrixRotatefEXT(GL_MODELVIEW, view_rotz, 0.0, 0.0, 1.0);
}

static void
draw3D()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  MatrixLoadToGL(view);

  glMatrixPushEXT(GL_MODELVIEW); {

    if (view3d) {
      configView3D();
    }

    glMatrixPushEXT(GL_MODELVIEW); {
      glMatrixTranslatefEXT(GL_MODELVIEW, -3.0, -2.0, 0.0);
      glMatrixRotatefEXT(GL_MODELVIEW, angle, 0.0, 0.0, 1.0);
      glCallList(gear1);
    } glMatrixPopEXT(GL_MODELVIEW);

    glMatrixPushEXT(GL_MODELVIEW); {
      glMatrixTranslatefEXT(GL_MODELVIEW, 3.1, -2.0, 0.0);
      glMatrixRotatefEXT(GL_MODELVIEW, -2.0 * angle - 9.0, 0.0, 0.0, 1.0);
      glCallList(gear2);
    } glMatrixPopEXT(GL_MODELVIEW);

    glMatrixPushEXT(GL_MODELVIEW); {
      glMatrixTranslatefEXT(GL_MODELVIEW, -3.1, 4.2, 0.0);
      glMatrixRotatefEXT(GL_MODELVIEW, -2.0 * angle - 25.0, 0.0, 0.0, 1.0);
      glCallList(gear3);
    } glMatrixPopEXT(GL_MODELVIEW);

  } glMatrixPopEXT(GL_MODELVIEW);
}

static GLfloat red[4] = {0.8f, 0.1f, 0.0, 1.0f};
static GLfloat green[4] = {0.0f, 0.8f, 0.2f, 1.0f};
static GLfloat blue[4] = {0.2f, 0.2f, 1.0f, 1.0f};

bool stroking = true;

static void
drawPaths()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  assert(glIsPathNV(gear1));
  assert(glIsPathNV(gear2));
  assert(glIsPathNV(gear3));

  MatrixLoadToGL(view);

  glMatrixPushEXT(GL_MODELVIEW); {

    if (view3d) {
      configView3D();
    }

    if (stroking) {
      glColor3f(0.4, 0.4, 0.4);
      float3x3 t[3], r[3];
      t[0] = translate(-3, -2);
      r[0] = rotate(angle);
      t[1] = translate(3.1, -2.0);
      r[1] = rotate(-2.0 * angle - 9.0);
      t[2] = translate(-3.1, 4.2);
      r[2] = rotate(-2.0 * angle - 25.0);
      GLfloat xform[3][2][3];
      for (int i=0; i<3; i++) {
        float3x3 combo = mul(t[i], r[i]);
        for (int j=0;j<2; j++) {
          for (int k=0; k<3; k++) {
            xform[i][j][k] = combo[j][k];
          }
        }
      }
      const GLuint gears[3] = { gear1, gear2, gear3 };
      glStencilStrokePathInstancedNV(countof(gears), GL_UNSIGNED_INT, gears, 0,
        1, ~0,
        GL_TRANSPOSE_AFFINE_2D_NV, &xform[0][0][0]);
      glCoverStrokePathInstancedNV(countof(gears), GL_UNSIGNED_INT, gears, 0,
        GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
        GL_TRANSPOSE_AFFINE_2D_NV, &xform[0][0][0]);
    }

    glMatrixPushEXT(GL_MODELVIEW); {
      glMatrixTranslatefEXT(GL_MODELVIEW, -3.0, -2.0, 0.0);
      glMatrixRotatefEXT(GL_MODELVIEW, angle, 0.0, 0.0, 1.0);
      glColor4fv(red);
      glStencilFillPathNV(gear1, GL_COUNT_UP_NV, ~0);
      glCoverFillPathNV(gear1, GL_CONVEX_HULL_NV);
    } glMatrixPopEXT(GL_MODELVIEW);

    glMatrixPushEXT(GL_MODELVIEW); {
      glMatrixTranslatefEXT(GL_MODELVIEW, 3.1, -2.0, 0.0);
      glMatrixRotatefEXT(GL_MODELVIEW, -2.0 * angle - 9.0, 0.0, 0.0, 1.0);
      glColor4fv(green);
      glStencilFillPathNV(gear2, GL_COUNT_UP_NV, ~0);
      glCoverFillPathNV(gear2, GL_CONVEX_HULL_NV);
    } glMatrixPopEXT(GL_MODELVIEW);

    glMatrixPushEXT(GL_MODELVIEW); {
      glMatrixTranslatefEXT(GL_MODELVIEW, -3.1, 4.2, 0.0);
      glMatrixRotatefEXT(GL_MODELVIEW, -2.0 * angle - 25.0, 0.0, 0.0, 1.0);
      glColor4fv(blue);
      glStencilFillPathNV(gear3, GL_COUNT_UP_NV, ~0);
      glCoverFillPathNV(gear3, GL_CONVEX_HULL_NV);
    } glMatrixPopEXT(GL_MODELVIEW);

  } glMatrixPopEXT(GL_MODELVIEW);

  glutReportErrors();
}

static void
draw()
{
  if (path_rendering) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    drawPaths();
    glDisable(GL_STENCIL_TEST);
    handleFPS(&gl_fps_context);
  } else {
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    draw3D();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING); {
      handleFPS(&gl_fps_context);
    } glEnable(GL_LIGHTING);
  }
  glutSwapBuffers();
}

static void
idle()
{
  static double t0 = -1.;
  double dt, t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
  if (t0 < 0.0) {
    t0 = t;
  }
  dt = t - t0;
  t0 = t;

  angle += 70.0 * dt;  /* 70 degrees per second */
  angle = ::fmod(angle, 360.0f); /* prevents eventual overflow */

  glutPostRedisplay();
}

static void
update_idle_func()
{
  invalidateFPS();
  if (Visible && Animate) {
    glutIdleFunc(idle);
  } else {
    glutIdleFunc(NULL);
  }
}

/* new window size or exposure */
static void
reshape(int width, int height)
{
  reshapeFPScontext(&gl_fps_context, width, height);
  glViewport(0, 0, width, height);

  window_width = width;
  window_height = height;

  float3x3 iproj, viewport;

  viewport = ortho(0,window_width, 0,window_height);
  float left = -8, right = 8, top = 8, bottom = -8;
  float aspect_ratio = window_height/window_width;
  if (aspect_ratio > 1) {
    top *= aspect_ratio;
    bottom *= aspect_ratio;
  } else {
    left /= aspect_ratio;
    right /= aspect_ratio;
  }

  glMatrixLoadIdentityEXT(GL_PROJECTION);
  if (view3d) {
    // 3D perspective view
    glMatrixFrustumEXT(GL_PROJECTION, -1.0, 1.0, -aspect_ratio, aspect_ratio, 5.0, 200.0);
  } else {
    // 2D orthographic view
    glMatrixOrthoEXT(GL_PROJECTION, left, right, bottom, top, -1, 1);
  }

  iproj = inverse_ortho(left, right, top, bottom);
  view_width = right - left;
  view_height = bottom - top;
  win2obj = mul(iproj, viewport);
}

static void updateRenderMode()
{
  if (path_rendering) {
    // Unwanted
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    // Wanted
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_NOTEQUAL, 0, ~0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

    glutSetWindowTitle("nvpr_gears via NV_path_rendering");
  } else {
    // Unwanted
    glDisable(GL_STENCIL_TEST);

    // Wanted
    glEnable(GL_DEPTH_TEST);
    if (view3d) {
      glEnable(GL_LIGHTING);
    } else {
      glDisable(GL_LIGHTING);
    }

    glutSetWindowTitle("nvpr_gears via 3D rendering");
  }
}

const GearData gear_data[3] = {
  { 1.0f, 4.0f, 1.0f, 20, 0.7f },
  { 0.5f, 2.0f, 2.0f, 10, 0.7f },
  { 1.3f, 2.0f, 0.5f, 10, 0.7f },
};

static void init_gear_paths()
{
  gear_path(gear1, gear_data[0], useRoundTeeth);
  gear_path(gear2, gear_data[1], useRoundTeeth);
  gear_path(gear3, gear_data[2], useRoundTeeth);
}

static void
init()
{
  static GLfloat pos[4] = {5.0, 5.0, 10.0, 0.0};

  glLightfv(GL_LIGHT0, GL_POSITION, pos);
  glEnable(GL_CULL_FACE);
  glEnable(GL_LIGHT0);
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

  updateRenderMode();
  setBackground();

  /* make the gears */
  gear1 = glGenLists(1);
  glNewList(gear1, GL_COMPILE); {
    glColor4fv(red);
    gear(gear_data[0]);
  } glEndList();

  gear2 = glGenLists(1);
  glNewList(gear2, GL_COMPILE); {
    glColor4fv(green);
    gear(gear_data[1]);
  } glEndList();
  
  gear3 = glGenLists(1);
  glNewList(gear3, GL_COMPILE); {
    glColor4fv(blue);
    gear(gear_data[2]);
  } glEndList();

  glEnable(GL_NORMALIZE);

  init_gear_paths();
}

static void 
visible(int vis)
{
   Visible = vis;
   update_idle_func();
}

/* change view angle, exit upon ESC */
/* ARGSUSED1 */
static void
keyboard(unsigned char k, int x, int y)
{
  switch (k) {
  case 'z':
    view_rotz += 5.0;
    break;
  case 'Z':
    view_rotz -= 5.0;
    break;
  case 'd':
     viewDist += 1.0;
     break;
  case 'D':
     viewDist -= 1.0;
     break;
  case 'a':
  case ' ':
     Animate = !Animate;
     update_idle_func();
     toggleFPS();
     break;
  case 27:  /* Escape */
    cleanup();
    exit(0);
    break;
  case '3':
    view3d = !view3d;
    reshape(window_width, window_height);
    break;
  case 'p':
    path_rendering = !path_rendering;
    updateRenderMode();
    reshape(window_width, window_height);
    break;
  case 's':
    stroking = !stroking;
    break;
  case 'b':
    background = (background+1)%4;
    setBackground();
    break;
  case 'v':
    enable_vsync = !enable_vsync;
    requestSynchornizedSwapBuffers(enable_vsync);
    break;
  case 't':
    useRoundTeeth = !useRoundTeeth;
    init_gear_paths();
    break;
  case 'r':
    initViewMatrix();
    break;
  case 'F':
    toggleFPS();
    break;
  default:
    return;
  }
  glutPostRedisplay();
}

/* change view angle */
/* ARGSUSED1 */
static void
special(int k, int x, int y)
{
  switch (k) {
  case GLUT_KEY_UP:
    view_rotx += 5.0;
    break;
  case GLUT_KEY_DOWN:
    view_rotx -= 5.0;
    break;
  case GLUT_KEY_LEFT:
    view_roty += 5.0;
    break;
  case GLUT_KEY_RIGHT:
    view_roty -= 5.0;
    break;
  default:
    return;
  }
  glutPostRedisplay();
}

static void menu(int item)
{
  keyboard(char(item), 0,0);  // bogus (x,y) location
}

static void createMenu()
{
  glutCreateMenu(menu);
  glutAddMenuEntry("[ ] Toggle animation", ' ');
  glutAddMenuEntry("[p] Toggle 3D vs path rendering", 'p');
  glutAddMenuEntry("[3] Toggle 3D vs 2D view", '3');
  glutAddMenuEntry("[s] Toggle stroking (path rendering only)", 's');
  glutAddMenuEntry("[t] Toggle round vs straight teeth (path rendering only)", 't');
  glutAddMenuEntry("[b] Cycle background color", 'b');
  glutAddMenuEntry("[v] Toggle vsync", 'v');
  glutAddMenuEntry("[F] Toggle showing frame rate", 'F');
  glutAddMenuEntry("[Esc] Quit", '\027');
  glutAttachMenu(GLUT_RIGHT_BUTTON);
}

int main(int argc, char *argv[])
{
  glutInitWindowSize(canvas_width, canvas_height);
  glutInit(&argc, argv);

  int samples = 0;
  for (int i=1; i<argc; i++) {
    if (argv[i][0] == '-') {
      int value = atoi(argv[i]+1);
      if (value >= 1) {
        samples = value;
        continue;
      }
    } else
    if (strcmp(argv[i], "-noanim") == 0) {
      Animate = GL_FALSE;
      continue;
    }
    fprintf(stderr, "usage: %s [-#] [-noanim]\n"
                    "       where # is the number of samples/pixel\n"
                    "       and -noanim disables initial animation\n",
      program_name);
    exit(1);
  }

  if (samples > 0) {
    if (samples == 1) 
      samples = 0;
    printf("requesting %d samples\n", samples);
    char buffer[200];
    sprintf(buffer, "rgb stencil~4 depth double samples~%d", samples);
    glutInitDisplayString(buffer);
  } else {
    /* Request a double-buffered window with at least 4 stencil bits and
       8 samples per pixel. */
    glutInitDisplayString("rgb stencil~4 depth double samples~8");
  }
  if (!glutGet(GLUT_DISPLAY_MODE_POSSIBLE)) {
    glutInitDisplayString(NULL);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_STENCIL);
  }

  win = glutCreateWindow("nvpr_gears");

  printf("vendor: %s\n", glGetString(GL_VENDOR));
  printf("version: %s\n", glGetString(GL_VERSION));
  printf("renderer: %s\n", glGetString(GL_RENDERER));
  printf("samples = %d\n", glutGet(GLUT_WINDOW_NUM_SAMPLES));
  printf("Executable: %d bit\n", (int)(8*sizeof(int*)));

  GLenum status = glewInit();
  if (status != GLEW_OK) {
    fatalError("OpenGL Extension Wrangler (GLEW) failed to initialize");
  }
  // Use glutExtensionSupported because glewIsSupported is unreliable for DSA.
  GLboolean hasDSA = glutExtensionSupported("GL_EXT_direct_state_access");
  if (!hasDSA) {
    fatalError("OpenGL implementation doesn't support GL_EXT_direct_state_access (you should be using NVIDIA GPUs...)");
  }

  init();

  glutDisplayFunc(draw);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(special);
  glutVisibilityFunc(visible);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  createMenu();
  initViewMatrix();
  update_idle_func();

  initFPScontext(&gl_fps_context, FPS_USAGE_TEXTURE);
  colorFPS(0.7,0.7,0);  // dark yellow
  scaleFPS(2);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
