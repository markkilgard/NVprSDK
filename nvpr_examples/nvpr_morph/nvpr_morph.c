
/* nvpr_morph.c - animated path morph of treble clef */

// Copyright (c) NVIDIA Corporation. All rights reserved.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES  // so <math.h> has M_PI
#include <math.h>
#include <GL/glew.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "nvpr_glew_init.h"
#include "xform.h"
#define countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))

const char *program_name = "nvpr_morph";
int animating = 0;
static float t = 0;
static GLuint weight_path = 666;
static GLboolean cubic = 1;

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

static const char *treble_pose[] = {
    // original pose
    "M 131.4,430.9 C 114,491.1 112,555.3 130.7,615.9 141.8,651.9 168.4,673.3 203,691.1 236.3,708.2 261.8,711.1 293.2,714.1 336.2,718.1 374.1,727.4 408.3,693.4 452.6,649.3 456.8,571.1 436.4,509.3 422.1,465.9 374.5,449.7 334.1,442.4 288.9,434.3 260.4,469.4 245.8,508.5 236.4,533.7 234.1,557.8 233,583.4 232.2,602.8 237.6,618.6 253.4,632 267.3,643.7 296.4,647.4 276.5,623.5 254.2,596.6 245,535.8 289.2,527.2 331.3,519.1 383.7,546.8 387.6,594.7 391.6,642.4 335.7,660 297.3,662.9 260.1,665.6 213.6,674.4 190.3,632.8 162.3,582.5 158.4,516.3 199.7,472.9 232.5,438.7 266.8,406.7 306.2,380.1 342.2,355.9 365.2,313.9 376.6,272.3 385.8,238.6 384.2,203 385.1,168.3 386.2,125.6 368.2,81.16 338.6,50.86 308.3,19.86 265.2,49.66 255.7,87.16 241.2,144.5 244.6,204 244.4,262.2 244.3,301.1 256.7,340 266.6,378.1 277.7,421.1 288.2,455.5 289.1,499.7 289.9,539.7 300,588.6 311.9,626.8 323.6,664.1 328.9,692.9 339.4,738.6 348.2,776.7 376.7,857.8 341.2,874.7 310.5,889.4 234.7,830.3 260.1,838.3 297.5,850 319.1,849.6 311.1,813.7 301,767.8 248.1,758.5 209.7,762.6 161.2,767.7 166,828.1 180.4,861 199,903.4 247.8,918.6 288.8,930.5 315.3,938.3 351.4,947.6 373.3,926.3 401.7,898.7 394.3,858.2 392.7,819.3 390.1,755.7 369.8,694.9 364.6,630.3 361.2,587.6 345.9,546.7 331,506.4 316.1,466.1 308.5,423.4 313.1,380.7 317.9,335.2 295.5,295 289.4,251.4 283.7,210.7 258.4,155.8 294.1,124.5 323.1,99.14 352.8,144.1 353.2,174.4 353.8,211.4 327.2,240.4 302.5,263.2 275.5,288.1 249.5,303.3 216.2,328.2 180.6,354.9 144.3,386.1 131.4,430.9 z",
    // second pose
    "M 131.3,430.7 C 113.9,490.9 111.9,555.1 130.6,615.7 141.7,651.7 168.5,673.1 203.1,690.9 236.3,708 261.8,710.9 293.2,713.9 336.2,717.9 360.2,696.2 408.3,693.2 469.8,689.5 478,680 541.5,652.5 583.4,634.4 636.8,592.9 661.4,565.4 692.9,530.3 710.1,497 671.1,482.1 623.3,463.8 609.2,459.2 583.5,460 547.3,461.1 538.1,464 522.1,477.2 506.7,490 486.3,508.8 547.2,518.2 581.7,523.5 581.3,540.6 566,560.4 539.6,594.2 504.8,599.8 454.3,618.7 401.4,638.5 335.7,659.8 297.3,662.7 260.1,665.4 213.6,674.2 190.4,632.6 162.4,582.3 158.5,516.1 199.8,472.7 232.5,438.5 266.8,406.5 306.2,379.9 342.2,355.7 380.6,323.3 398.8,284.2 414.1,251.6 428.5,194.2 434.6,160.1 447.8,86.1 393.8,53.81 348.7,40.54 290.5,23.42 251.9,33.59 225.4,61.69 183.6,105.9 219.3,202.7 244.4,262 259.6,297.8 256.7,339.8 266.6,377.9 277.7,420.9 288.2,455.3 289.1,499.5 289.9,539.5 300,588.4 311.9,626.6 323.6,663.9 328.9,692.7 339.4,738.4 348.2,776.5 376.7,857.6 341.2,874.5 310.5,889.2 287.2,875.6 260.1,838.1 237,806.3 192.7,783.1 156.6,776.1 86.85,762.6 77.25,771.8 61.35,806.8 48.45,835.4 127.6,865.2 169.4,885 211.3,904.9 247.8,918.4 288.8,930.3 315.3,938.1 351.4,947.4 373.3,926.1 401.7,898.5 394.3,858 392.7,819.1 390.1,755.5 369.8,694.7 364.6,630.1 361.2,587.4 345.9,546.5 331,506.2 316.1,465.9 308.5,423.2 313.1,380.5 317.9,335 295.5,294.8 289.4,251.2 283.7,210.5 258.4,155.6 294.1,124.3 323.1,98.92 352.8,143.9 353.2,174.2 353.8,211.2 327.2,240.2 302.5,263 275.5,287.9 249.5,303.1 216.2,328 180.7,354.7 144.2,385.9 131.3,430.7 z",
    // third pose
    "M 54.52,449.9 C 25.33,505.3 21.19,560.3 53.82,614.7 76.04,651.7 121,670.9 158.7,680.8 196.9,690.8 261.8,710.9 293.2,713.9 336.2,717.9 360.2,696.2 408.3,693.2 469.8,689.5 478,680 541.5,652.5 583.4,634.4 606.8,542.5 603.8,504.8 599.4,449 590.9,360.7 570.1,324.5 541.5,274.9 504.2,208.4 483.5,217.6 455.5,230 480.5,317.5 526.1,356 541.5,368.9 554,410.8 558.3,455.6 561.7,490.3 569.2,537.6 553.9,557.4 527.5,591.2 504.8,599.8 454.3,618.7 401.4,638.5 335.8,664.4 297.3,662.7 231.4,659.8 166.5,644 132.8,610.4 44.21,522 108.4,491 135.1,437.3 158.8,390 241.5,379.2 280.9,352.6 316.9,328.4 380.6,323.3 398.8,284.2 414.1,251.6 428.5,194.2 434.6,160.1 447.8,86.1 393.8,53.81 348.7,40.54 290.5,23.42 251.9,33.59 225.4,61.69 183.6,105.9 196.1,187.5 221.2,246.8 236.4,282.6 256.7,339.8 266.6,377.9 277.7,420.9 288.2,455.3 289.1,499.5 289.9,539.5 300,588.4 311.9,626.6 323.6,663.9 328.9,692.7 339.4,738.4 348.2,776.5 376.7,857.6 341.2,874.5 310.5,889.2 287.2,875.6 260.1,838.1 237,806.3 192.7,783.1 156.6,776.1 86.85,762.6 77.25,771.8 61.35,806.8 48.45,835.4 127.6,865.2 169.4,885 211.3,904.9 247.8,918.4 288.8,930.3 315.3,938.1 351.4,947.4 373.3,926.1 401.7,898.5 394.3,858 392.7,819.1 390.1,755.5 369.8,694.7 364.6,630.1 361.2,587.4 345.9,546.5 331,506.2 316.1,465.9 308.5,423.2 313.1,380.5 317.9,335 295.5,294.8 289.4,251.2 283.7,210.5 258.4,155.6 294.1,124.3 323.1,98.92 352.8,143.9 353.2,174.2 353.8,211.2 327.2,240.2 302.5,263 275.5,287.9 204,321.3 170.7,346.2 135.2,372.9 76.52,408.1 54.52,449.9 z",
    // fourth pose
    "M 54.52,449.9 C 25.33,505.3 21.19,560.3 53.82,614.7 76.04,651.7 121,670.9 158.7,680.8 196.9,690.8 261.8,710.9 293.2,713.9 336.2,717.9 360.2,696.2 408.3,693.2 469.8,689.5 478,680 541.5,652.5 583.4,634.4 606.8,542.5 603.8,504.8 599.4,449 586.3,266.2 584.2,224.5 580.9,156.7 566.8,76.06 546.1,85.26 518.1,97.66 529,188.2 547.3,262.1 552.2,281.5 554,410.8 558.3,455.6 561.7,490.3 569.2,537.6 553.9,557.4 527.5,591.2 504.8,599.8 454.3,618.7 401.4,638.5 335.8,664.4 297.3,662.7 231.4,659.8 166.5,644 132.8,610.4 44.21,522 108.4,491 135.1,437.3 158.8,390 231.4,370.1 270.8,343.5 306.8,319.3 376.6,306.1 394.8,267 410.1,234.4 428.5,194.2 434.6,160.1 447.8,86.1 393.8,53.81 348.7,40.54 290.5,23.42 127.6,56.82 95.08,96.04 56.22,142.8 72.86,172.3 104,230.6 122.4,264.9 235.5,358 245.4,396.1 256.5,439.1 270,451.3 278,497.5 284.8,536.9 300,588.4 311.9,626.6 323.6,663.9 328.9,692.7 339.4,738.4 348.2,776.5 376.7,857.6 341.2,874.5 310.5,889.2 276.1,862 235.9,839.1 190.5,813.4 171.3,801.8 134.4,792.3 89.06,780.6 77.25,771.8 61.35,806.8 48.45,835.4 131.6,855.1 173.4,874.9 215.3,894.8 247.8,918.4 288.8,930.3 315.3,938.1 351.4,947.4 373.3,926.1 401.7,898.5 394.3,858 392.7,819.1 390.1,755.5 369.8,694.7 364.6,630.1 361.2,587.4 345.9,546.5 331,506.2 316.1,465.9 301,426.7 288.9,385.6 272.4,329.9 189.5,244.3 168.2,205.7 147.3,168.1 124.2,130.2 166.8,109.1 205.9,89.83 346.7,77.23 371.4,111.6 393,141.6 372.7,210.9 352,253.9 336.1,287 204,321.3 170.7,346.2 135.2,372.9 76.52,408.1 54.52,449.9 z"
};
static const GLuint treble_path[] = { 1, 2, 3, 4 };

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
  glMatrixOrthoEXT(GL_PROJECTION, 0, 2*canvas_width, 2*canvas_height, 0, -1, 1);
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

static void initPath()
{
  int i;

  for (i=0; i<countof(treble_pose); i++) {
    printf("create %d\n", i);
    glPathStringNV(treble_path[i], GL_PATH_FORMAT_SVG_NV,
      (GLsizei)strlen(treble_pose[i]), treble_pose[i]);
    glPathParameterfNV(treble_path[i], GL_PATH_STROKE_WIDTH_NV, 5);
  }
}

static void drawPath()
{
  const GLint reference = 0x1;
  const GLuint mask = 0xFF;
  glMatrixPushEXT(GL_MODELVIEW); {
    // Fill
    glMatrixTranslatefEXT(GL_MODELVIEW, 240,0,0);
#if 0
    glStencilFillPathNV(weight_path, GL_COUNT_UP_NV, mask);
    glCoverFillPathNV(weight_path, GL_CONVEX_HULL_NV);
#else
    glStencilThenCoverFillPathNV(weight_path, GL_COUNT_UP_NV, mask, GL_CONVEX_HULL_NV);
#endif

    // Stroke
    glMatrixTranslatefEXT(GL_MODELVIEW, 640,0,0);
#if 0
    glStencilStrokePathNV(weight_path, reference, mask);
    glCoverStrokePathNV(weight_path, GL_CONVEX_HULL_NV);
#else
    glStencilThenCoverStrokePathNV(weight_path, reference, mask, GL_CONVEX_HULL_NV);
#endif
  } glMatrixPopEXT(GL_MODELVIEW);
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

static void doWeight(void)
{
  float u = 0.5*sin(t)+0.5;
  if (cubic) {
    const float cubic_us[4] = { (1-u)*(1-u)*(1-u), 3*(1-u)*(1-u)*u, 3*(1-u)*u*u, u*u*u };

    glWeightPathsNV(weight_path, countof(treble_path), treble_path, cubic_us);
  } else {
    glInterpolatePathsNV(weight_path, treble_path[0], treble_path[3], u);
  }
}

static void animate(void)
{
  t += 0.03f;
  if (t > M_PI) {
    t -= M_PI;
  }
  doWeight();
}

static void idle(void)
{
  animate();
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
  case 'c':
    cubic = !cubic;
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

  glutCreateWindow("Animated path morph with NV_path_rendering");
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
  initPath();
  doWeight();

  glutMainLoop();
  return 0;
}

