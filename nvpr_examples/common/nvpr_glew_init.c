
/* nvpr_glew_init.c - initialize NV_path_rendering API */

/* Copyright NVIDIA Corporation, 2010-2015. */

#include <stdlib.h>  // for exit
#include <stdio.h>  // for fprintf and NULL

// Avoid actually including <GL/glew.h>!

#include "nvpr_glew_init.h"
#include <GL/glut.h>

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

#if 0
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
#endif

#define GLEW_NVPR_EMULATION(pfn,name) { \
  extern pfn __glew ## name; \
  if (!__glew ## name) { \
    __glew ## name = gl ## name ## _emulation; \
    nvpr_emulation = 1; \
  } \
} // end of macro

int has_NV_path_rendering = 0;

int initialize_NVPR_GLEW_emulation(FILE *output, const char *program_name, int quiet)
{
  int nvpr_emulation = 0;
  has_NV_path_rendering = glutExtensionSupported("GL_NV_path_rendering");
  GLEW_NVPR_EMULATION(PFNGLSTENCILTHENCOVERFILLPATHNVPROC, StencilThenCoverFillPathNV);
  GLEW_NVPR_EMULATION(PFNGLSTENCILTHENCOVERSTROKEPATHNVPROC, StencilThenCoverStrokePathNV);
  GLEW_NVPR_EMULATION(PFNGLSTENCILTHENCOVERFILLPATHINSTANCEDNVPROC, StencilThenCoverFillPathInstancedNV);
  GLEW_NVPR_EMULATION(PFNGLSTENCILTHENCOVERSTROKEPATHINSTANCEDNVPROC, StencilThenCoverStrokePathInstancedNV);
  if (output) {
    if (nvpr_emulation) {
      fprintf(output, "%s: installed NV_path_rendering 1.3 functions for GLEW\n", program_name);
    } else {
      if (!quiet) {
        fprintf(output, "%s: no GLEW emulation needed for NV_path_rendering 1.3\n", program_name);
      }
    }
  }
  return nvpr_emulation;
}
