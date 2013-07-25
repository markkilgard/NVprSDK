
/* nvpr_basic.c - minimal example for NV_path_rendering */

// Copyright (c) NVIDIA Corporation. All rights reserved.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

/* Only include the minimal API used by this example for brevity. */
#ifndef GL_NV_path_rendering
/* Tokens */
#define GL_PATH_STROKE_WIDTH_NV                             0x9075
#define GL_PATH_JOIN_STYLE_NV                               0x9079
#define GL_ROUND_NV                                         0x90A4
#define GL_PATH_FORMAT_SVG_NV                               0x9070
#define GL_PATH_FORMAT_PS_NV                                0x9071
#define GL_CLOSE_PATH_NV                                    0x00
#define GL_MOVE_TO_NV                                       0x02
#define GL_LINE_TO_NV                                       0x04
#define GL_COUNT_UP_NV                                      0x9088
#define GL_CONVEX_HULL_NV                                   0x908B
#define GL_BOUNDING_BOX_NV                                  0x908D
/* Command and query function types */
typedef void (GLAPIENTRYP PFNGLPATHCOMMANDSNVPROC) (GLuint path, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const GLvoid *coords);
typedef void (GLAPIENTRYP PFNGLPATHSTRINGNVPROC) (GLuint path, GLenum format, GLsizei length, const GLvoid *pathString);
typedef void (GLAPIENTRYP PFNGLPATHPARAMETERINVPROC) (GLuint path, GLenum pname, GLint value);
typedef void (GLAPIENTRYP PFNGLPATHPARAMETERFNVPROC) (GLuint path, GLenum pname, GLfloat value);
typedef void (GLAPIENTRYP PFNGLSTENCILFILLPATHNVPROC) (GLuint path, GLenum fillMode, GLuint mask);
typedef void (GLAPIENTRYP PFNGLSTENCILSTROKEPATHNVPROC) (GLuint path, GLint reference, GLuint mask);
typedef void (GLAPIENTRYP PFNGLCOVERFILLPATHNVPROC) (GLuint path, GLenum coverMode);
typedef void (GLAPIENTRYP PFNGLCOVERSTROKEPATHNVPROC) (GLuint path, GLenum coverMode);
#endif

/* Only include the minimal API used by this example for brevity. */
#ifndef GL_EXT_direct_state_access
typedef void (GLAPIENTRYP PFNGLMATRIXLOADIDENTITYEXTPROC) (GLenum mode);
typedef void (GLAPIENTRYP PFNGLMATRIXORTHOEXTPROC) (GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
#endif

#ifndef __APPLE__
PFNGLPATHCOMMANDSNVPROC glPathCommandsNV = NULL;
PFNGLPATHSTRINGNVPROC glPathStringNV = NULL;
PFNGLPATHPARAMETERINVPROC glPathParameteriNV = NULL;
PFNGLPATHPARAMETERFNVPROC glPathParameterfNV = NULL;
PFNGLCOVERFILLPATHNVPROC glCoverFillPathNV = NULL;
PFNGLCOVERSTROKEPATHNVPROC glCoverStrokePathNV = NULL;
PFNGLSTENCILFILLPATHNVPROC glStencilFillPathNV = NULL;
PFNGLSTENCILSTROKEPATHNVPROC glStencilStrokePathNV = NULL;

PFNGLMATRIXLOADIDENTITYEXTPROC glMatrixLoadIdentityEXT = NULL;
PFNGLMATRIXORTHOEXTPROC glMatrixOrthoEXT = NULL;
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
    fprintf(stderr, "%s: failed to GetProcAddress for %s\n", programName, #name); \
    exit(1); \
  }
#endif

int hasPathRendering = 0;
int hasDirectStateAccess = 0;
const char *programName = "nvpr_basic";
GLuint pathObj = 42;
int path_specification_mode = 0;
int filling = 1;
int stroking = 1;
int even_odd = 0;

void initglext(void)
{
    hasPathRendering = glutExtensionSupported("GL_NV_path_rendering");
    hasDirectStateAccess = glutExtensionSupported("GL_EXT_direct_state_access");

    if (hasPathRendering) {
        LOAD_PROC(PFNGLPATHCOMMANDSNVPROC, glPathCommandsNV);
        LOAD_PROC(PFNGLPATHSTRINGNVPROC, glPathStringNV);
        LOAD_PROC(PFNGLPATHPARAMETERINVPROC, glPathParameteriNV);
        LOAD_PROC(PFNGLPATHPARAMETERFNVPROC, glPathParameterfNV);
        LOAD_PROC(PFNGLSTENCILFILLPATHNVPROC, glStencilFillPathNV);
        LOAD_PROC(PFNGLSTENCILSTROKEPATHNVPROC, glStencilStrokePathNV);
        LOAD_PROC(PFNGLCOVERFILLPATHNVPROC, glCoverFillPathNV);
        LOAD_PROC(PFNGLCOVERSTROKEPATHNVPROC, glCoverStrokePathNV);
    }
    if (hasDirectStateAccess) {
        LOAD_PROC(PFNGLMATRIXLOADIDENTITYEXTPROC, glMatrixLoadIdentityEXT);
        LOAD_PROC(PFNGLMATRIXORTHOEXTPROC, glMatrixOrthoEXT);
    }
}

void
initPathFromSVG()
{
    /* Here is an example of specifying and then rendering a five-point
    star and a heart as a path using Scalable Vector Graphics (SVG)
    path description syntax: */
    
    const char *svgPathString =
      // star
      "M100,180 L40,10 L190,120 L10,120 L160,10 z"
      // heart
      "M300 300 C 100 400,100 200,300 100,500 200,500 400,300 300Z";
    glPathStringNV(pathObj, GL_PATH_FORMAT_SVG_NV,
                   (GLsizei)strlen(svgPathString), svgPathString);
}

void
initPathFromPS()
{
    /* Alternatively applications oriented around the PostScript imaging
    model can use the PostScript user path syntax instead: */

    const char *psPathString =
      // star
      "100 180 moveto"
      " 40 10 lineto 190 120 lineto 10 120 lineto 160 10 lineto closepath"
      // heart
      " 300 300 moveto"
      " 100 400 100 200 300 100 curveto"
      " 500 200 500 400 300 300 curveto closepath";
    glPathStringNV(pathObj, GL_PATH_FORMAT_PS_NV,
                   (GLsizei)strlen(psPathString), psPathString);
}

void
initPathFromData()
{
    /* The PostScript path syntax also supports compact and precise binary
    encoding and includes PostScript-style circular arcs.

    Or the path's command and coordinates can be specified explicitly: */

    static const GLubyte pathCommands[10] =
      { GL_MOVE_TO_NV, GL_LINE_TO_NV, GL_LINE_TO_NV, GL_LINE_TO_NV,
        GL_LINE_TO_NV, GL_CLOSE_PATH_NV,
        'M', 'C', 'C', 'Z' };  // character aliases
    static const GLshort pathCoords[12][2] =
      { {100, 180}, {40, 10}, {190, 120}, {10, 120}, {160, 10},
        {300,300}, {100,400}, {100,200}, {300,100},
        {500,200}, {500,400}, {300,300} };
    glPathCommandsNV(pathObj, 10, pathCommands, 24, GL_SHORT, pathCoords);
}

void initGraphics()
{
    switch (path_specification_mode) {
    case 0:
        printf("specifying path via SVG string\n");
        initPathFromSVG();
        break;
    case 1:
        printf("specifying path via PS string\n");
        initPathFromPS();
        break;
    case 2:
        printf("specifying path via explicit data\n");
        initPathFromData();
        break;
    }

    /* Before rendering, configure the path object with desirable path
    parameters for stroking.  Specify a wider 6.5-unit stroke and
    the round join style: */

    glPathParameteriNV(pathObj, GL_PATH_JOIN_STYLE_NV, GL_ROUND_NV);
    glPathParameterfNV(pathObj, GL_PATH_STROKE_WIDTH_NV, 6.5);
}

void
doGraphics(void)
{
    /* Before rendering to a window with a stencil buffer, clear the stencil
    buffer to zero and the color buffer to black: */

    glClearStencil(0);
    glClearColor(0,0,0,0);
    glStencilMask(~0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    /* Use an orthographic path-to-clip-space transform to map the
    [0..500]x[0..400] range of the star's path coordinates to the [-1..1]
    clip space cube: */

    glMatrixLoadIdentityEXT(GL_PROJECTION);
    glMatrixLoadIdentityEXT(GL_MODELVIEW);
    glMatrixOrthoEXT(GL_MODELVIEW, 0, 500, 0, 400, -1, 1);

    if (filling) {

        /* Stencil the path: */

        glStencilFillPathNV(pathObj, GL_COUNT_UP_NV, 0x1F);

        /* The 0x1F mask means the counting uses modulo-32 arithmetic. In
        principle the star's path is simple enough (having a maximum winding
        number of 2) that modulo-4 arithmetic would be sufficient so the mask
        could be 0x3.  Or a mask of all 1's (~0) could be used to count with
        all available stencil bits.

        Now that the coverage of the star and the heart have been rasterized
        into the stencil buffer, cover the path with a non-zero fill style
        (indicated by the GL_NOTEQUAL stencil function with a zero reference
        value): */

        glEnable(GL_STENCIL_TEST);
        if (even_odd) {
            glStencilFunc(GL_NOTEQUAL, 0, 0x1);
        } else {
            glStencilFunc(GL_NOTEQUAL, 0, 0x1F);
        }
        glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
        glColor3f(0,1,0); // green
        glCoverFillPathNV(pathObj, GL_BOUNDING_BOX_NV);

    }

    /* The result is a yellow star (with a filled center) to the left of
    a yellow heart.

    The GL_ZERO stencil operation ensures that any covered samples
    (meaning those with non-zero stencil values) are zero'ed when
    the path cover is rasterized. This allows subsequent paths to be
    rendered without clearing the stencil buffer again.

    A similar two-step rendering process can draw a white outline
    over the star and heart. */

     /* Now stencil the path's stroked coverage into the stencil buffer,
     setting the stencil to 0x1 for all stencil samples within the
     transformed path. */

    if (stroking) {

        glStencilStrokePathNV(pathObj, 0x1, ~0);

         /* Cover the path's stroked coverage (with a hull this time instead
         of a bounding box; the choice doesn't really matter here) while
         stencil testing that writes white to the color buffer and again
         zero the stencil buffer. */

        glColor3f(1,1,0); // yellow
        glCoverStrokePathNV(pathObj, GL_CONVEX_HULL_NV);

         /* In this example, constant color shading is used but the application
         can specify their own arbitrary shading and/or blending operations,
         whether with Cg compiled to fragment program assembly, GLSL, or
         fixed-function fragment processing.

         More complex path rendering is possible such as clipping one path to
         another arbitrary path.  This is because stencil testing (as well
         as depth testing, depth bound test, clip planes, and scissoring)
         can restrict path stenciling. */
    }
}

void
display(void)
{
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
    case 'p':
        path_specification_mode = (path_specification_mode+1)%3;
        initGraphics();
        break;
    case 'f':
        filling = !filling;
        break;
    case 'e':
        even_odd = !even_odd;
        break;
    case 's':
        stroking = !stroking;
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

    glutCreateWindow("Basic NV_path_rendering example");
    printf("vendor: %s\n", glGetString(GL_VENDOR));
    printf("version: %s\n", glGetString(GL_VERSION));
    printf("renderer: %s\n", glGetString(GL_RENDERER));
    printf("samples = %d\n", glutGet(GLUT_WINDOW_NUM_SAMPLES));
    printf("Executable: %d bit\n", (int)8*sizeof(int*));

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);

    initglext();
    if (!hasPathRendering) {
        fprintf(stderr, "%s: required NV_path_rendering OpenGL extension is not present\n", programName);
        exit(1);
    }
    if (!hasDirectStateAccess) {
        fprintf(stderr, "%s: required EXT_direct_state_access OpenGL extension is not present\n", programName);
        exit(1);
    }
    initGraphics();

    glutMainLoop();
    return 0;
}

