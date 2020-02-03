
/* nvpr_init.c - initialize NV_path_rendering API */

/* Copyright NVIDIA Corporation, 2010. */

#include <stdlib.h>  // for exit
#include <stdio.h>  // for fprintf and NULL

#ifdef __APPLE__
# include <GLUT/glut.h>
# include <OpenGL/glext.h>
#else
# include <GL/glut.h>

/* <GL/glut.h> should pull in <GL/gl.h> which should pull in <GL/glext.h> */

# ifdef _WIN32
#  include <windows.h>
# else
#  include <GL/glx.h>
# endif

#endif

#ifdef sun
#define NO_NVPR_API_REMAP
#endif
#include "nvpr_init.h"

#ifndef __APPLE__

PFNGLGENPATHSNVPROC FUNC(glGenPathsNV) = NULL;
PFNGLDELETEPATHSNVPROC FUNC(glDeletePathsNV) = NULL;
PFNGLISPATHNVPROC FUNC(glIsPathNV) = NULL;
PFNGLPATHCOMMANDSNVPROC FUNC(glPathCommandsNV) = NULL;
PFNGLPATHCOORDSNVPROC FUNC(glPathCoordsNV) = NULL;
PFNGLPATHSUBCOMMANDSNVPROC FUNC(glPathSubCommandsNV) = NULL;
PFNGLPATHSUBCOORDSNVPROC FUNC(glPathSubCoordsNV) = NULL;
PFNGLPATHSTRINGNVPROC FUNC(glPathStringNV) = NULL;
PFNGLPATHGLYPHSNVPROC FUNC(glPathGlyphsNV) = NULL;
PFNGLPATHGLYPHRANGENVPROC FUNC(glPathGlyphRangeNV) = NULL;
PFNGLWEIGHTPATHSNVPROC FUNC(glWeightPathsNV) = NULL;
PFNGLCOPYPATHNVPROC FUNC(glCopyPathNV) = NULL;
PFNGLINTERPOLATEPATHSNVPROC FUNC(glInterpolatePathsNV) = NULL;
PFNGLTRANSFORMPATHNVPROC FUNC(glTransformPathNV) = NULL;
PFNGLPATHPARAMETERIVNVPROC FUNC(glPathParameterivNV) = NULL;
PFNGLPATHPARAMETERINVPROC FUNC(glPathParameteriNV) = NULL;
PFNGLPATHPARAMETERFVNVPROC FUNC(glPathParameterfvNV) = NULL;
PFNGLPATHPARAMETERFNVPROC FUNC(glPathParameterfNV) = NULL;
PFNGLPATHDASHARRAYNVPROC FUNC(glPathDashArrayNV) = NULL;
PFNGLSTENCILFILLPATHNVPROC FUNC(glStencilFillPathNV) = NULL;
PFNGLPATHSTENCILDEPTHOFFSETNVPROC FUNC(glPathStencilDepthOffsetNV) = NULL;
PFNGLSTENCILSTROKEPATHNVPROC FUNC(glStencilStrokePathNV) = NULL;
PFNGLSTENCILFILLPATHINSTANCEDNVPROC FUNC(glStencilFillPathInstancedNV) = NULL;
PFNGLSTENCILSTROKEPATHINSTANCEDNVPROC FUNC(glStencilStrokePathInstancedNV) = NULL;
PFNGLPATHCOVERDEPTHFUNCNVPROC FUNC(glPathCoverDepthFuncNV) = NULL;
PFNGLPATHCOLORGENNVPROC FUNC(glPathColorGenNV) = NULL;
PFNGLPATHTEXGENNVPROC FUNC(glPathTexGenNV) = NULL;
PFNGLPATHFOGGENNVPROC FUNC(glPathFogGenNV) = NULL;
PFNGLCOVERFILLPATHNVPROC FUNC(glCoverFillPathNV) = NULL;
PFNGLCOVERSTROKEPATHNVPROC FUNC(glCoverStrokePathNV) = NULL;
PFNGLCOVERFILLPATHINSTANCEDNVPROC FUNC(glCoverFillPathInstancedNV) = NULL;
PFNGLCOVERSTROKEPATHINSTANCEDNVPROC FUNC(glCoverStrokePathInstancedNV) = NULL;
PFNGLGETPATHPARAMETERIVNVPROC FUNC(glGetPathParameterivNV) = NULL;
PFNGLGETPATHPARAMETERFVNVPROC FUNC(glGetPathParameterfvNV) = NULL;
PFNGLGETPATHCOMMANDSNVPROC FUNC(glGetPathCommandsNV) = NULL;
PFNGLGETPATHCOORDSNVPROC FUNC(glGetPathCoordsNV) = NULL;
PFNGLGETPATHDASHARRAYNVPROC FUNC(glGetPathDashArrayNV) = NULL;
PFNGLGETPATHMETRICSNVPROC FUNC(glGetPathMetricsNV) = NULL;
PFNGLGETPATHMETRICRANGENVPROC FUNC(glGetPathMetricRangeNV) = NULL;
PFNGLGETPATHSPACINGNVPROC FUNC(glGetPathSpacingNV) = NULL;
PFNGLGETPATHCOLORGENIVNVPROC FUNC(glGetPathColorGenivNV) = NULL;
PFNGLGETPATHCOLORGENFVNVPROC FUNC(glGetPathColorGenfvNV) = NULL;
PFNGLGETPATHTEXGENIVNVPROC FUNC(glGetPathTexGenivNV) = NULL;
PFNGLGETPATHTEXGENFVNVPROC FUNC(glGetPathTexGenfvNV) = NULL;
PFNGLISPOINTINFILLPATHNVPROC FUNC(glIsPointInFillPathNV) = NULL;
PFNGLISPOINTINSTROKEPATHNVPROC FUNC(glIsPointInStrokePathNV) = NULL;
PFNGLGETPATHLENGTHNVPROC FUNC(glGetPathLengthNV) = NULL;
PFNGLPOINTALONGPATHNVPROC FUNC(glPointAlongPathNV) = NULL;
PFNGLPATHSTENCILFUNCNVPROC FUNC(glPathStencilFuncNV) = NULL;
// NVpr 1.2 additions
PFNGLSTENCILTHENCOVERFILLPATHNVPROC FUNC(glStencilThenCoverFillPathNV) = NULL;
PFNGLSTENCILTHENCOVERSTROKEPATHNVPROC FUNC(glStencilThenCoverStrokePathNV) = NULL;
PFNGLSTENCILTHENCOVERFILLPATHINSTANCEDNVPROC FUNC(glStencilThenCoverFillPathInstancedNV) = NULL;
PFNGLSTENCILTHENCOVERSTROKEPATHINSTANCEDNVPROC FUNC(glStencilThenCoverStrokePathInstancedNV) = NULL;
PFNGLPROGRAMPATHFRAGMENTINPUTGENNVPROC FUNC(glProgramPathFragmentInputGenNV) = NULL;
PFNGLMATRIXLOAD3X2FNVPROC FUNC(glMatrixLoad3x2fNV) = NULL;
PFNGLMATRIXLOAD3X3FNVPROC FUNC(glMatrixLoad3x3fNV) = NULL;
PFNGLMATRIXLOADTRANSPOSE3X3FNVPROC FUNC(glMatrixLoadTranspose3x3fNV) = NULL;
PFNGLMATRIXMULT3X2FNVPROC FUNC(glMatrixMult3x2fNV) = NULL;
PFNGLMATRIXMULT3X3FNVPROC FUNC(glMatrixMult3x3fNV) = NULL;
PFNGLMATRIXMULTTRANSPOSE3X3FNVPROC FUNC(glMatrixMultTranspose3x3fNV) = NULL;
PFNGLGETPROGRAMRESOURCEFVNVPROC FUNC(glGetProgramResourcefvNV) = NULL;

#endif /* __APPLE__ */

#if defined(_WIN32)
# define GET_PROC_ADDRESS(name)  wglGetProcAddress(#name)
#elif defined(__APPLE__)
# define GET_PROC_ADDRESS(name)  /*nothing*/
#elif defined(vxworks)
# define GET_PROC_ADDRESS(name)  rglGetProcAddress(#name)
#else
  /* Assume using GLX */
# define GET_PROC_ADDRESS(name)  glXGetProcAddressARB((const GLubyte *) #name)
#endif

#ifdef __APPLE__
#define LOAD_PROC(type, name)  /*nothing*/
#define LOAD_PROC12(type, name)  /*nothing*/
#define LOAD_PROC12_ALT(type, name)  /*nothing*/
#define LOAD_PROC_DSA(type, name)  /*nothing*/
#else
#define LOAD_PROC(type, name) \
  FUNC(name) = (type) GET_PROC_ADDRESS(name); \
  if (!FUNC(name)) { \
    fprintf(stderr, "%s: failed to GetProcAddress for %s\n", program_name, #name); \
    exit(1); \
  }
#define LOAD_PROC12(type, name) \
  FUNC(name) = (type) GET_PROC_ADDRESS(name); \
  if (!FUNC(name)) { \
    has_NV_path_rendering_1_2 = 0; \
  }
#define LOAD_PROC12_ALT(type, name) \
  FUNC(name) = (type) GET_PROC_ADDRESS(name); \
  FUNC(name) = NULL; \
  if (!FUNC(name)) { \
    has_NV_path_rendering_1_2 = 0; \
    FUNC(name) = name ## _emulation; \
  }
#define LOAD_PROC_DSA(type, name) \
  name ## _dsa = (type) GET_PROC_ADDRESS(name); \
  if (!FUNC(name ## _dsa)) { \
    name ## _dsa = name ## _emulation; \
  }
#endif

int has_NV_path_rendering = 0;
int has_NV_path_rendering_1_2 = 0;
int has_EXT_direct_state_access = 0;

// Many of the NVpr 1.2 commands can be emulated when they do not exist...

static void GLAPIENTRY glStencilThenCoverFillPathNV_emulation(GLuint path, GLenum fillMode, GLuint mask, GLenum coverMode)
{
  glStencilFillPathNV(path, fillMode, mask);
  glCoverFillPathNV(path, coverMode);
}

static void GLAPIENTRY glStencilThenCoverStrokePathNV_emulation(GLuint path, GLint reference, GLuint mask, GLenum coverMode)
{
  glStencilStrokePathNV(path, reference, mask);
  glCoverStrokePathNV(path, coverMode);
}

static void GLAPIENTRY glStencilThenCoverFillPathInstancedNV_emulation(GLsizei numPaths, GLenum pathNameType, const GLvoid *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues)
{
  glStencilFillPathInstancedNV(numPaths, pathNameType, paths, pathBase, fillMode, mask, transformType, transformValues);
  glCoverFillPathInstancedNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues);
}

static void GLAPIENTRY glStencilThenCoverStrokePathInstancedNV_emulation(GLsizei numPaths, GLenum pathNameType, const GLvoid *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues)
{
  glStencilStrokePathInstancedNV(numPaths, pathNameType, paths, pathBase, reference, mask, transformType, transformValues);
  glCoverStrokePathInstancedNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues);
}

#ifndef GL_EXT_direct_state_access
typedef void (GLAPIENTRYP PFNGLMATRIXLOADTRANSPOSEFEXTPROC) (GLenum mode, const GLfloat *m);
typedef void (GLAPIENTRYP PFNGLMATRIXMULTTRANSPOSEFEXTPROC) (GLenum mode, const GLfloat *m);
#endif

typedef void (GLAPIENTRY * PFNGLLOADTRANSPOSEMATRIXFPROC) (const GLfloat m[16]);
typedef void (GLAPIENTRY * PFNGLMULTTRANSPOSEMATRIXFPROC) (const GLfloat m[16]);

static PFNGLLOADTRANSPOSEMATRIXFPROC glLoadTransposeMatrixf_dsa = NULL;
static PFNGLMULTTRANSPOSEMATRIXFPROC glMultTransposeMatrixf_dsa = NULL;

static PFNGLMATRIXLOADTRANSPOSEFEXTPROC glMatrixLoadTransposefEXT_dsa = NULL;
static PFNGLMATRIXMULTTRANSPOSEFEXTPROC glMatrixMultTransposefEXT_dsa = NULL;

static void GLAPIENTRY glLoadTransposeMatrixf_emulation(const GLfloat* m)
{
  const GLfloat mt[16] = {
      m[0], m[4], m[8], m[12],
      m[1], m[5], m[9], m[13],
      m[2], m[6], m[10], m[14],
      m[3], m[7], m[11], m[15]
  };
  glLoadMatrixf(mt);
}

static void GLAPIENTRY glMultTransposeMatrixf_emulation(const GLfloat* m)
{
  const GLfloat mt[16] = {
      m[0], m[4], m[8], m[12],
      m[1], m[5], m[9], m[13],
      m[2], m[6], m[10], m[14],
      m[3], m[7], m[11], m[15]
  };
  glMultMatrixf(mt);
}


static void GLAPIENTRY glMatrixLoadTransposefEXT_emulation(GLenum matrixMode, const GLfloat* m)
{
  GLint oldMatrixMode = GL_NONE;
  glGetIntegerv(GL_MATRIX_MODE, &oldMatrixMode);
  if (oldMatrixMode != matrixMode) {
    glMatrixMode(matrixMode);
    glLoadTransposeMatrixf_dsa(m);
    glMatrixMode(oldMatrixMode);
  } else {
    glLoadTransposeMatrixf_dsa(m);
  }
}

static void GLAPIENTRY glMatrixMultTransposefEXT_emulation(GLenum matrixMode, const GLfloat* m)
{
  GLint oldMatrixMode = GL_NONE;
  glGetIntegerv(GL_MATRIX_MODE, &oldMatrixMode);
  if (oldMatrixMode != matrixMode) {
    glMatrixMode(matrixMode);
    glMultTransposeMatrixf_dsa(m);
    glMatrixMode(oldMatrixMode);
  } else {
    glMultTransposeMatrixf_dsa(m);
  }
}

static void GLAPIENTRY glMatrixLoad3x2fNV_emulation(GLenum matrixMode, const GLfloat *m)
{
  const GLfloat equiv_3x2matrix[16] = {
    m[0], m[2], 0, m[4],
    m[1], m[3], 0, m[5],
    0,    0,    1, 0,
    0,    0,    0, 1
  };
  glMatrixLoadTransposefEXT_dsa(matrixMode, equiv_3x2matrix);
}

static void GLAPIENTRY glMatrixLoad3x3fNV_emulation(GLenum matrixMode, const GLfloat *m)
{
  GLfloat equiv_3x3matrix[16] = {
    m[0], m[3], 0, m[6],
    m[1], m[4], 0, m[7],
    0,    0,    1, 0,
    m[2], m[5], 0, m[8],
  };
  glMatrixLoadTransposefEXT_dsa(matrixMode, equiv_3x3matrix);
}

static void GLAPIENTRY glMatrixLoadTranspose3x3fNV_emulation(GLenum matrixMode, const GLfloat *m)
{
  GLfloat equiv_3x3matrix[16] = {
    m[0], m[1], 0, m[2],
    m[3], m[4], 0, m[5],
    0,    0,    1, 0,
    m[6], m[7], 0, m[8],
  };
  glMatrixLoadTransposefEXT_dsa(matrixMode, equiv_3x3matrix);
}

static void GLAPIENTRY glMatrixMult3x2fNV_emulation(GLenum matrixMode, const GLfloat *m)
{
  GLfloat equiv_3x2matrix[16] = {
    m[0], m[2], 0, m[4],
    m[1], m[3], 0, m[5],
    0,    0,    1, 0,
    0,    0,    0, 1
  };
  glMatrixMultTransposefEXT_dsa(matrixMode, equiv_3x2matrix);
}

static void GLAPIENTRY glMatrixMult3x3fNV_emulation(GLenum matrixMode, const GLfloat *m)
{
  GLfloat equiv_3x3matrix[16] = {
    m[0], m[3], 0, m[6],
    m[1], m[4], 0, m[7],
    0,    0,    1, 0,
    m[2], m[5], 0, m[8],
  };
  glMatrixMultTransposefEXT_dsa(matrixMode, equiv_3x3matrix);
}

static void GLAPIENTRY glMatrixMultTranspose3x3fNV_emulation(GLenum matrixMode, const GLfloat *m)
{
  GLfloat equiv_3x3matrix[16] = {
    m[0], m[1], 0, m[2],
    m[3], m[4], 0, m[5],
    0,    0,    1, 0,
    m[6], m[7], 0, m[8],
  };
  glMatrixMultTransposefEXT_dsa(matrixMode, equiv_3x3matrix);
}

void
initializeNVPR(const char *program_name)
{
  has_NV_path_rendering = glutExtensionSupported("GL_NV_path_rendering");
  has_EXT_direct_state_access = glutExtensionSupported("GL_EXT_direct_state_access");
  // Optimistically assume NVpr 1.2 support until proven otherwise.
  has_NV_path_rendering_1_2 = has_NV_path_rendering;

  if (has_NV_path_rendering) {
    LOAD_PROC(PFNGLGENPATHSNVPROC, glGenPathsNV);
    LOAD_PROC(PFNGLDELETEPATHSNVPROC, glDeletePathsNV);
    LOAD_PROC(PFNGLISPATHNVPROC, glIsPathNV);
    LOAD_PROC(PFNGLPATHCOMMANDSNVPROC, glPathCommandsNV);
    LOAD_PROC(PFNGLPATHCOORDSNVPROC, glPathCoordsNV);
    LOAD_PROC(PFNGLPATHSUBCOMMANDSNVPROC, glPathSubCommandsNV);
    LOAD_PROC(PFNGLPATHSUBCOORDSNVPROC, glPathSubCoordsNV);
    LOAD_PROC(PFNGLPATHSTRINGNVPROC, glPathStringNV);
    LOAD_PROC(PFNGLPATHGLYPHSNVPROC, glPathGlyphsNV);
    LOAD_PROC(PFNGLPATHGLYPHRANGENVPROC, glPathGlyphRangeNV);
    LOAD_PROC(PFNGLWEIGHTPATHSNVPROC, glWeightPathsNV);
    LOAD_PROC(PFNGLCOPYPATHNVPROC, glCopyPathNV);
    LOAD_PROC(PFNGLINTERPOLATEPATHSNVPROC, glInterpolatePathsNV);
    LOAD_PROC(PFNGLTRANSFORMPATHNVPROC, glTransformPathNV);
    LOAD_PROC(PFNGLPATHPARAMETERIVNVPROC, glPathParameterivNV);
    LOAD_PROC(PFNGLPATHPARAMETERINVPROC, glPathParameteriNV);
    LOAD_PROC(PFNGLPATHPARAMETERFVNVPROC, glPathParameterfvNV);
    LOAD_PROC(PFNGLPATHPARAMETERFNVPROC, glPathParameterfNV);
    LOAD_PROC(PFNGLPATHDASHARRAYNVPROC, glPathDashArrayNV);
    LOAD_PROC(PFNGLSTENCILFILLPATHNVPROC, glStencilFillPathNV);
    LOAD_PROC(PFNGLSTENCILSTROKEPATHNVPROC, glStencilStrokePathNV);
    LOAD_PROC(PFNGLSTENCILFILLPATHINSTANCEDNVPROC, glStencilFillPathInstancedNV);
    LOAD_PROC(PFNGLSTENCILSTROKEPATHINSTANCEDNVPROC, glStencilStrokePathInstancedNV);
    LOAD_PROC(PFNGLPATHCOLORGENNVPROC, glPathColorGenNV);
    LOAD_PROC(PFNGLPATHTEXGENNVPROC, glPathTexGenNV);
    LOAD_PROC(PFNGLPATHFOGGENNVPROC, glPathFogGenNV);
    LOAD_PROC(PFNGLCOVERFILLPATHNVPROC, glCoverFillPathNV);
    LOAD_PROC(PFNGLCOVERSTROKEPATHNVPROC, glCoverStrokePathNV);
    LOAD_PROC(PFNGLCOVERFILLPATHINSTANCEDNVPROC, glCoverFillPathInstancedNV);
    LOAD_PROC(PFNGLCOVERSTROKEPATHINSTANCEDNVPROC, glCoverStrokePathInstancedNV);
    LOAD_PROC(PFNGLGETPATHPARAMETERIVNVPROC, glGetPathParameterivNV);
    LOAD_PROC(PFNGLGETPATHPARAMETERFVNVPROC, glGetPathParameterfvNV);
    LOAD_PROC(PFNGLGETPATHCOMMANDSNVPROC, glGetPathCommandsNV);
    LOAD_PROC(PFNGLGETPATHCOORDSNVPROC, glGetPathCoordsNV);
    LOAD_PROC(PFNGLGETPATHDASHARRAYNVPROC, glGetPathDashArrayNV);
    LOAD_PROC(PFNGLGETPATHMETRICSNVPROC, glGetPathMetricsNV);
    LOAD_PROC(PFNGLGETPATHMETRICRANGENVPROC, glGetPathMetricRangeNV);
    LOAD_PROC(PFNGLGETPATHSPACINGNVPROC, glGetPathSpacingNV);
    LOAD_PROC(PFNGLGETPATHCOLORGENIVNVPROC, glGetPathColorGenivNV);
    LOAD_PROC(PFNGLGETPATHCOLORGENFVNVPROC, glGetPathColorGenfvNV);
    LOAD_PROC(PFNGLGETPATHTEXGENIVNVPROC, glGetPathTexGenivNV);
    LOAD_PROC(PFNGLGETPATHTEXGENFVNVPROC, glGetPathTexGenfvNV);
    LOAD_PROC(PFNGLISPOINTINFILLPATHNVPROC, glIsPointInFillPathNV);
    LOAD_PROC(PFNGLISPOINTINSTROKEPATHNVPROC, glIsPointInStrokePathNV);
    LOAD_PROC(PFNGLGETPATHLENGTHNVPROC, glGetPathLengthNV);
    LOAD_PROC(PFNGLPOINTALONGPATHNVPROC, glPointAlongPathNV);
    LOAD_PROC(PFNGLPATHSTENCILFUNCNVPROC, glPathStencilFuncNV);
    LOAD_PROC(PFNGLPATHSTENCILDEPTHOFFSETNVPROC, glPathStencilDepthOffsetNV);
    LOAD_PROC(PFNGLPATHCOVERDEPTHFUNCNVPROC,  glPathCoverDepthFuncNV);
    // NVpr 1.2
    {
      // Commands with alternative emulation
      LOAD_PROC12_ALT(PFNGLSTENCILTHENCOVERFILLPATHNVPROC, glStencilThenCoverFillPathNV);
      LOAD_PROC12_ALT(PFNGLSTENCILTHENCOVERSTROKEPATHNVPROC, glStencilThenCoverStrokePathNV);
      LOAD_PROC12_ALT(PFNGLSTENCILTHENCOVERFILLPATHINSTANCEDNVPROC, glStencilThenCoverFillPathInstancedNV);
      LOAD_PROC12_ALT(PFNGLSTENCILTHENCOVERSTROKEPATHINSTANCEDNVPROC, glStencilThenCoverStrokePathInstancedNV);
      LOAD_PROC12_ALT(PFNGLMATRIXLOAD3X2FNVPROC, glMatrixLoad3x2fNV);
      LOAD_PROC12_ALT(PFNGLMATRIXLOAD3X3FNVPROC, glMatrixLoad3x3fNV);
      LOAD_PROC12_ALT(PFNGLMATRIXLOADTRANSPOSE3X3FNVPROC, glMatrixLoadTranspose3x3fNV);
      LOAD_PROC12_ALT(PFNGLMATRIXMULT3X2FNVPROC, glMatrixMult3x2fNV);
      LOAD_PROC12_ALT(PFNGLMATRIXMULT3X3FNVPROC, glMatrixMult3x3fNV);
      LOAD_PROC12_ALT(PFNGLMATRIXMULTTRANSPOSE3X3FNVPROC, glMatrixMultTranspose3x3fNV);
      // Commands without emulation, test
      LOAD_PROC12(PFNGLGETPROGRAMRESOURCEFVNVPROC, glGetProgramResourcefvNV);
      LOAD_PROC12(PFNGLPROGRAMPATHFRAGMENTINPUTGENNVPROC, glProgramPathFragmentInputGenNV);
      if (!has_NV_path_rendering_1_2 && !has_EXT_direct_state_access) {
        LOAD_PROC_DSA(PFNGLLOADTRANSPOSEMATRIXFPROC, glLoadTransposeMatrixf);
        LOAD_PROC_DSA(PFNGLMULTTRANSPOSEMATRIXFPROC, glMultTransposeMatrixf);
        LOAD_PROC_DSA(PFNGLMATRIXLOADTRANSPOSEFEXTPROC, glMatrixLoadTransposefEXT);
        LOAD_PROC_DSA(PFNGLMATRIXMULTTRANSPOSEFEXTPROC, glMatrixMultTransposefEXT);
      }
    }
  }
}

