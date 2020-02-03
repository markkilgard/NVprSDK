
/* nv_tiger3d.c - render 3D scene of PostScript tigers & teapot with NV_path_rendering */

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

#include <Cg/double.hpp>
#include <Cg/vector/xyzw.hpp>
#include <Cg/vector.hpp>
#include <Cg/matrix.hpp>
#include <Cg/radians.hpp>

using namespace Cg;

#include "nvpr_glew_init.h"
#include "tiger.h"
#include "trackball.h"
#include "showfps.h"
#include "request_vsync.h"
#include "cg4cpp_xform.hpp"
#include "render_font.hpp"

const char *program_name = "nvpr_tiger3d";
int stroking = 1,
    filling = 1;

bool draw_overlaid_text = true;

int using_dlist = 1;

bool enable_vsync = true;
float teapotSize = 30;
bool path_depth_offset = true;

float curquat[4];
/* Initial slight rotation */
float lastquat[4] = { 1.78721e-006f, -0.00139029f, 3.47222e-005f, 0.999999f };
GLfloat m[4][4];
int spinning = 0, moving = 0;
int beginx, beginy;
int newModel = 1;
float ztrans = -150;
int numTigers = 4;
bool wireframe_teapot = true;
bool force_stencil_clear = true;
bool texture_tigers = false;
bool draw_center_teapot = true;

typedef enum _Mode {
    FBO_TEXTURE,
    FBO_RENDERBUFFER,
} Mode;

Mode fbo_mode = FBO_RENDERBUFFER;
int fbo_width = 512, fbo_height = 512;
int renderbuffer_samples = 8;
GLenum blit_filter = GL_NEAREST;
GLuint tex_color = 0;
GLfloat tiger_bounds[4];  // (x1,y1,x2,y2) for lower-left and upper-right of tiger scene

FPScontext gl_fps_context;

static void fatalError(const char *message)
{
  fprintf(stderr, "%s: %s\n", program_name, message);
  exit(1);
}

float window_width, window_height;

static void reshape(int w, int h)
{
  reshapeFPScontext(&gl_fps_context, w, h);
  glViewport(0,0, w,h);
  window_width = w;
  window_height = h;
  float aspect_ratio = window_width/window_height;
  glMatrixLoadIdentityEXT(GL_PROJECTION);
  float near = 1,
        far = 1200;
  glMatrixFrustumEXT(GL_PROJECTION, -aspect_ratio,aspect_ratio, 1,-1, near,far);
  force_stencil_clear = true;
}

int emScale = 2048;

FontFace *font;
Message *msg;

enum ClearColor {
  CC_BLUE,
  CC_BLACK
} clear_color = CC_BLUE;

void clearToBlue()
{
  glClearColor(0.1, 0.3, 0.6, 0.0);
  clear_color = CC_BLUE;
}

void clearToBlack()
{
  glClearColor(0, 0, 0, 0);
  clear_color = CC_BLACK;
}

void restoreClear()
{
  switch (clear_color) {
  case CC_BLACK:
    glClearColor(0,0,0,0);
    break;
  case CC_BLUE:
    glClearColor(0.1, 0.3, 0.6, 0.0);
    break;
  }
}

void initGraphics()
{
  trackball(curquat, 0.0, 0.0, 0.0, 0.0);
  glMatrixLoadIdentityEXT(GL_MODELVIEW);
  glMatrixTranslatefEXT(GL_MODELVIEW, 0,0,ztrans);

  /* Before rendering to a window with a stencil buffer, clear the stencil
  buffer to zero and the color buffer to blue: */
  glClearStencil(0);
  glClearColor(0.1, 0.3, 0.6, 0.0);

  initTiger();
  getTigerBounds(tiger_bounds, 1, 1);

  glStencilFunc(GL_NOTEQUAL, 0, 0x1F);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
  glEnable(GL_DEPTH_TEST);
  GLfloat slope = -0.05;
  GLint bias = -1;
  glPathStencilDepthOffsetNV(slope, bias);
  glPathCoverDepthFuncNV(GL_ALWAYS);

  // Create a null path object to use as a parameter template for creating fonts.
  GLuint path_template = glGenPathsNV(1);
  glPathCommandsNV(path_template, 0, NULL, 0, GL_FLOAT, NULL);
  glPathParameterfNV(path_template, GL_PATH_STROKE_WIDTH_NV, 0.1*emScale);  // 10% of emScale
  glPathParameteriNV(path_template, GL_PATH_JOIN_STYLE_NV, GL_ROUND_NV);

  font = new FontFace(GL_SYSTEM_FONT_NAME_NV, "ParkAvenue BT", 256, path_template);
  float2 to_quad[4];
  to_quad[0] = float2(30,300);
  to_quad[1] = float2(610,230);
  to_quad[2] = float2(610,480);
  to_quad[3] = float2(30,480);
  msg = new Message(font, "Path rendering and 3D meet!", to_quad);
}

GLuint
initFBO(Mode fbo_mode, int width, int height, int num_samples)
{
  GLuint fbo = 0, tex_depthstencil = 0;
  GLuint rb_color = 0, rb_depth_stencil = 0;
  GLint got_samples = -666;

  int tex_width = width, tex_height = height;

  if (tex_color == 0) {
    glGenTextures(1, &tex_color);
  }
  glBindTexture(GL_TEXTURE_2D, tex_color);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
    tex_width, tex_height, 0, GL_RGBA, GL_INT, NULL);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  if (fbo_mode == FBO_RENDERBUFFER) {
    glGenRenderbuffers(1, &rb_color);
    glGenRenderbuffers(1, &rb_depth_stencil);

    glBindRenderbuffer(GL_RENDERBUFFER, rb_depth_stencil);
    if (num_samples > 1) {
      glRenderbufferStorageMultisample(GL_RENDERBUFFER,
        num_samples, GL_DEPTH24_STENCIL8, 
        width, height);
    } else {
      glRenderbufferStorage(GL_RENDERBUFFER,
        GL_DEPTH24_STENCIL8, 
        width, height);
    }
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER,
      GL_DEPTH_ATTACHMENT,
      GL_RENDERBUFFER, rb_depth_stencil);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER,
      GL_STENCIL_ATTACHMENT,
      GL_RENDERBUFFER, rb_depth_stencil);

    glBindRenderbuffer(GL_RENDERBUFFER, rb_color);
    if (num_samples > 1) {
      glRenderbufferStorageMultisample(GL_RENDERBUFFER,
        num_samples, GL_RGBA8,
        width, height);
    } else {
      glRenderbufferStorage(GL_RENDERBUFFER,
        GL_RGBA8,
        width, height);
    }
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER,
      GL_COLOR_ATTACHMENT0,
      GL_RENDERBUFFER, rb_color);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glGetIntegerv(GL_SAMPLES, &got_samples);
  } else {
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
      GL_COLOR_ATTACHMENT0,
      GL_TEXTURE_2D, tex_color, 0);

    // Setup depth_stencil texture (not mipmap)
    glGenTextures(1, &tex_depthstencil);
    glBindTexture(GL_TEXTURE_2D, tex_depthstencil);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8,
      width, height, 0, GL_DEPTH_STENCIL,
      GL_UNSIGNED_INT_24_8, NULL);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
      GL_DEPTH_ATTACHMENT,
      GL_TEXTURE_2D, tex_depthstencil, 0);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
      GL_STENCIL_ATTACHMENT,
      GL_TEXTURE_2D, tex_depthstencil, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  glViewport(0, 0, width, height);
  glClearColor(0,0,0,0);
  glClearStencil(0);
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glMatrixLoadIdentityEXT(GL_PROJECTION);
  glMatrixLoadIdentityEXT(GL_MODELVIEW);

  glStencilFunc(GL_NOTEQUAL, 0, 0x1F);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
  glDisable(GL_DEPTH_TEST);

  glEnable(GL_STENCIL_TEST);
  glDisable(GL_DEPTH_TEST);
  glMatrixOrthoEXT(GL_MODELVIEW,
    tiger_bounds[0], tiger_bounds[2],  // left,right
    tiger_bounds[1], tiger_bounds[3],  // bottom,top
    -1, 1);
  {
    GLint stencil = -666;
    glGetIntegerv(GL_STENCIL_BITS, &stencil);
    printf("stencil = %d\n", stencil);
  }
  drawTiger(filling, stroking);

  if (fbo_mode == FBO_RENDERBUFFER && num_samples > 1) {
    GLuint fbo_texture = 0;
    glGenFramebuffers(1, &fbo_texture);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_texture);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
      GL_COLOR_ATTACHMENT0,
      GL_TEXTURE_2D, tex_color, 0);

    glBlitFramebuffer(0, 0, width, height,  // source (x,y,w,h)
                      0, 0, tex_width, tex_height,  // destination (x,y,w,h)
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo_texture);
  } 
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, tex_color);
  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);

  if (tex_depthstencil) {
    glDeleteTextures(1, &tex_depthstencil);
  }
  if (rb_color) {
    glDeleteRenderbuffers(1, &rb_color);
  }
  if (rb_depth_stencil) {
    glDeleteRenderbuffers(1, &rb_depth_stencil);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDeleteFramebuffers(1, &fbo);
  return tex_color;
}

void
recalcModelView()
{
  build_rotmatrix(m, curquat);
  glMatrixLoadIdentityEXT(GL_MODELVIEW);
  glMatrixTranslatefEXT(GL_MODELVIEW, 0,0,ztrans);
  newModel = 0;
}

void scene()
{
  float separation = 60;
  if (texture_tigers) {
    glBindTexture(GL_TEXTURE_2D, tex_color);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.25);
    for (int i=0; i<numTigers; i++) {
      float angle = i*360/numTigers;
      glMatrixPushEXT(GL_MODELVIEW); {
        glRotatef(angle, 0,1,0);
        glTranslatef(0, 0, -separation);
        glScalef(0.3, 0.3, 1);
        // Draw a textured rectangle.
        glBegin(GL_TRIANGLE_FAN); {
          glTexCoord2f(0,0);
          glVertex2f(tiger_bounds[0],tiger_bounds[1]);
          glTexCoord2f(1,0);
          glVertex2f(tiger_bounds[2],tiger_bounds[1]);
          glTexCoord2f(1,1);
          glVertex2f(tiger_bounds[2],tiger_bounds[3]);
          glTexCoord2f(0,1);
          glVertex2f(tiger_bounds[0],tiger_bounds[3]);
        } glEnd();
      } glMatrixPopEXT(GL_MODELVIEW);
    }
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_TEXTURE_2D);
  } else {
    glEnable(GL_STENCIL_TEST);
    for (int i=0; i<numTigers; i++) {
      float angle = i*360/numTigers;
      glMatrixPushEXT(GL_MODELVIEW); {
        glRotatef(angle, 0,1,0);
        glTranslatef(0, 0, -separation);
        glScalef(0.3, 0.3, 1);
        renderTiger(filling, stroking);
      } glMatrixPopEXT(GL_MODELVIEW);
    }
  }

  if (draw_center_teapot) {
    glDisable(GL_STENCIL_TEST);
    glColor3f(0,1,0);
    glMatrixPushEXT(GL_MODELVIEW); {
      glScalef(1,-1,1);
      if (wireframe_teapot) {
        glutWireTeapot(teapotSize);
      } else {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glutSolidTeapot(teapotSize);
        glDisable(GL_LIGHTING);
      }
    } glMatrixPopEXT(GL_MODELVIEW);
  }
}



void
display(void)
{
  if (force_stencil_clear) {
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    force_stencil_clear = false;
  } else {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }
  if (newModel) {
    recalcModelView();
  }
  glEnable(GL_DEPTH_TEST);
  glMatrixPushEXT(GL_MODELVIEW); {
    glMatrixMultfEXT(GL_MODELVIEW, &m[0][0]);
    scene();
  } glMatrixPopEXT(GL_MODELVIEW);

  if (draw_overlaid_text) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glMatrixPushEXT(GL_PROJECTION); {
      glMatrixLoadIdentityEXT(GL_PROJECTION);
      glMatrixOrthoEXT(GL_PROJECTION, 0, 640, 0, 480, -1, 1);

      glMatrixPushEXT(GL_MODELVIEW); {
        glMatrixLoadIdentityEXT(GL_MODELVIEW);
        glColor3f(1,0,0);
        msg->multMatrix();
        msg->render();
      } glMatrixPopEXT(GL_MODELVIEW);
    } glMatrixPopEXT(GL_PROJECTION);
  } else {
    glDisable(GL_DEPTH_TEST);  // handleFPS expects this disabled.
  }

  glDisable(GL_STENCIL_TEST);
  handleFPS(&gl_fps_context);
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
  case '+':
    teapotSize += 1.5;
    break;
  case '-':
    teapotSize -= 1.5;
    break;
  case 's':
    stroking = !stroking;
    break;
  case 't':
    wireframe_teapot = !wireframe_teapot;
    break;
  case 'T':
    draw_overlaid_text = !draw_overlaid_text;
    break;
  case 'd':
    using_dlist = !using_dlist;
    printf("using_dlist = %d\n", using_dlist);
    tigerDlistUsage(using_dlist);
    break;
  case 'Z':
    path_depth_offset = !path_depth_offset;
    if (path_depth_offset) {
      GLfloat slope = -0.05;
      GLint bias = -1;
      glPathStencilDepthOffsetNV(slope, bias);
    } else {
      glPathStencilDepthOffsetNV(0, 0);
    }
    break;

  case 'f':
    filling = !filling;
    break;
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    numTigers = c - '1' + 1;
    break;
  case '!':
    clearToBlack();
    break;
  case '@':
    clearToBlue();
    break;
  case 'F':
    colorFPS(0,1,0);
    toggleFPS();
    break;
  case 'v':
    enable_vsync = !enable_vsync;
    printf("enable_vsync = %d\n", enable_vsync);
    requestSynchornizedSwapBuffers(enable_vsync);
    break;
  case 'p':
    texture_tigers = !texture_tigers;
    break;
  case 'c':
    draw_center_teapot = !draw_center_teapot;
    break;
  default:
    return;
  }
  glutPostRedisplay();
}

void
animate()
{
  add_quats(lastquat, curquat, curquat);
  newModel = 1;
  glutPostRedisplay();
}

void
visibility(int state)
{
  if (state == GLUT_VISIBLE) {
    if (spinning) {
      glutIdleFunc(animate);
    }
  } else {
    glutIdleFunc(NULL);
  }
}

void
stopSpinning()
{
  spinning = 0;
  glutIdleFunc(NULL);
}

bool zooming = false;
int zoom_trans = 0;

void
mouse(int button, int state, int mouse_space_x, int mouse_space_y)
{
    mouse_space_y = window_height - mouse_space_y;
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            if (glutGetModifiers() & GLUT_ACTIVE_CTRL) {
                zooming = true;
                zoom_trans = mouse_space_y;
            } else {
                stopSpinning();
                moving = 1;
                beginx = mouse_space_x;
                beginy = mouse_space_y;
            }
        } else {
            zooming = false;
            moving = 0;
        }
    }
}

void
motion(int mouse_space_x, int mouse_space_y)
{
    mouse_space_y = window_height - mouse_space_y;
    if (moving) {
        float x = mouse_space_x,
              y = mouse_space_y;
        trackball(lastquat,
            (2.0 * beginx - window_width) / window_width,
            (window_height - 2.0 * beginy) / window_height,
            (2.0 * x - window_width) / window_width,
            (window_height - 2.0 * y) / window_height
            );
        beginx = x;
        beginy = y;
        spinning = 1;
        newModel = 1;
        glutIdleFunc(animate);
    }
    if (zooming) {
        ztrans += 60.0 * (mouse_space_y - zoom_trans) / window_height;
        zoom_trans = mouse_space_y;
        newModel = 1;
        glutPostRedisplay();
    }
}

void menu(int item)
{
  keyboard(item, 0, 0);
}

void fbo_menu(int item)
{
  switch (item) {
  case 1:
    initFBO(FBO_RENDERBUFFER, 512, 512, 8);
    break;
  case 2:
    initFBO(FBO_RENDERBUFFER, 1024, 1024, 2);
    break;
  case 3:
    initFBO(FBO_TEXTURE, 64, 64, 1);
    break;
  case 4:
    initFBO(FBO_RENDERBUFFER, 64, 64, 16);
    break;
  case 5:
    initFBO(FBO_RENDERBUFFER, 256, 256, 16);
    break;
  }
  reshape(window_width, window_height);
  restoreClear();
  recalcModelView();
  glutPostRedisplay();
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
    sprintf(buffer, "rgb depth stencil~4 double samples~%d", samples);
    glutInitDisplayString(buffer);
  } else {
    /* Request a double-buffered window with at least 4 stencil bits and
       8 samples per pixel. */
    glutInitDisplayString("rgb depth stencil~4 double samples~8");
  }
  if (!glutGet(GLUT_DISPLAY_MODE_POSSIBLE)) {
    glutInitDisplayString(NULL);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_STENCIL);
  }

  glutCreateWindow("3D spinning scene PostScript tigers + teapot");
  printf("vendor: %s\n", glGetString(GL_VENDOR));
  printf("version: %s\n", glGetString(GL_VERSION));
  printf("renderer: %s\n", glGetString(GL_RENDERER));
  printf("samples per pixel = %d\n", glutGet(GLUT_WINDOW_NUM_SAMPLES));
  printf("Executable: %d bit\n", (int)(8*sizeof(int*)));
  printf("\n");
  printf("Spin the scene by clicking and dragging with the left mouse button\n");
  printf("Hold down Shift at left mouse click to zoom in/out\n");

  glutDisplayFunc(display);
  glutVisibilityFunc(visibility);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);

  int fbo_menu_id = glutCreateMenu(fbo_menu);
  glutAddMenuEntry("512x512x8", 1);
  glutAddMenuEntry("1024x1024x2", 2);
  glutAddMenuEntry("64x64x1", 3);
  glutAddMenuEntry("64x64x16", 4);
  glutAddMenuEntry("256x256x16", 5);

  glutCreateMenu(menu);
  glutAddMenuEntry("[f] Toggle filling", 'f');
  glutAddMenuEntry("[s] Toggle stroking", 's');
  glutAddMenuEntry("[v] Toggle vsync", 'v');
  glutAddMenuEntry("[d] Toggle display lists", 'd');
  glutAddMenuEntry("[F] Toggle frame counter", 'F');
  glutAddMenuEntry("[+] Increase teapot size", '+');
  glutAddMenuEntry("[-] Decrease teapot size", '+');
  glutAddMenuEntry("[T] Toggle overlaid text", 'T');
  glutAddMenuEntry("[p] Toggle paths versus textures", 'p');
  glutAddMenuEntry("[3] 3 Tigers", '3');
  glutAddMenuEntry("[4] 4 Tigers", '4');
  glutAddMenuEntry("[5] 5 Tigers", '5');
  glutAddSubMenu("FBO texture...", fbo_menu_id);
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
  initFBO(fbo_mode, fbo_width, fbo_height, renderbuffer_samples);
  restoreClear();
  initFPScontext(&gl_fps_context, FPS_USAGE_TEXTURE);
  colorFPS(0,1,0);

  glutMainLoop();
  return 0;
}

