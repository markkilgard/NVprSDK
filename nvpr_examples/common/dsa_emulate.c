
/* dsa_emulate.c - support EXT_direct_state_access for GLEW if unsupported */

// Copyright (c) NVIDIA Corporation. All rights reserved.

#include <assert.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/glew.h>
#if __APPLE__
# include <GLUT/glut.h>
#else
# include <GL/glut.h>
#endif

#if !defined(GLAPIENTRY)
# if defined(APIENTRY)
#  define GLAPIENTRY APIENTRY
# else
#  define GLAPIENTRY
# endif
#endif

#include "dsa_emulate.h"

#ifdef OPENGL_REGAL
void emulate_dsa_if_needed(int forceDSAemulation) { }
#else

void GLAPIENTRY dsa_glDisableClientStateIndexedEXT(GLenum array, GLuint index)
{
    assert(array == GL_TEXTURE_COORD_ARRAY);
    glClientActiveTexture(GL_TEXTURE0+index);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void GLAPIENTRY dsa_glEnableClientStateIndexedEXT(GLenum array, GLuint index)
{
    assert(array == GL_TEXTURE_COORD_ARRAY);
    glClientActiveTexture(GL_TEXTURE0+index);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}

void GLAPIENTRY dsa_glMultiTexCoordPointerEXT(GLenum texunit, GLint size, GLenum type, GLsizei stride, const void* pointer)
{
    glClientActiveTexture(texunit);
    glTexCoordPointer(size, type, stride, pointer);
}

void GLAPIENTRY dsa_glMatrixLoadIdentityEXT(GLenum matrixMode)
{
    glMatrixMode(matrixMode);
    glLoadIdentity();
}

void GLAPIENTRY dsa_glMatrixScalefEXT(GLenum matrixMode, GLfloat x, GLfloat y, GLfloat z)
{
    glMatrixMode(matrixMode);
    glScalef(x,y,z);
}

void GLAPIENTRY dsa_glMatrixTranslatefEXT(GLenum matrixMode, GLfloat x, GLfloat y, GLfloat z)
{
    glMatrixMode(matrixMode);
    glTranslatef(x,y,z);
}

void GLAPIENTRY dsa_glMatrixMultTransposefEXT(GLenum matrixMode, const GLfloat* m)
{
    glMatrixMode(matrixMode);
    glMultTransposeMatrixf(m);
}

void GLAPIENTRY dsa_glMatrixLoadTransposefEXT(GLenum matrixMode, const GLfloat* m)
{
    glMatrixMode(matrixMode);
    glLoadTransposeMatrixf(m);
}

void GLAPIENTRY dsa_glMatrixPopEXT(GLenum matrixMode)
{
    glMatrixMode(matrixMode);
    glPopMatrix();
}

void GLAPIENTRY dsa_glMatrixPushEXT(GLenum matrixMode)
{
    glMatrixMode(matrixMode);
    glPushMatrix();
}

void GLAPIENTRY dsa_glTextureBufferEXT(GLuint texture, GLenum target, GLenum internalformat, GLuint buffer)
{
    glBindTexture(target, texture);
    glTexBufferEXT(target, internalformat, buffer);
}

void GLAPIENTRY dsa_glTextureImage1DEXT(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void* pixels)
{
    glBindTexture(target, texture);
    glTexImage1D(target, level, internalformat, width, border, format, type, pixels);
}

void GLAPIENTRY dsa_glTextureImage2DEXT(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels)
{
    glBindTexture(target, texture);
    glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

void GLAPIENTRY dsa_glTextureImage3DEXT(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels)
{
    glBindTexture(target, texture);
    glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
}

void GLAPIENTRY dsa_glTextureParameteriEXT(GLuint texture, GLenum target, GLenum pname, GLint param)
{
    glBindTexture(target, texture);
    glTexParameteri(target, pname, param);
}

void GLAPIENTRY dsa_glTextureParameterivEXT(GLuint texture, GLenum target, GLenum pname, const GLint* param)
{
    glBindTexture(target, texture);
    glTexParameteriv(target, pname, param);
}

void GLAPIENTRY dsa_glTextureParameterfEXT(GLuint texture, GLenum target, GLenum pname, GLfloat param)
{
    glBindTexture(target, texture);
    glTexParameterf(target, pname, param);
}

void GLAPIENTRY dsa_glTextureParameterfvEXT(GLuint texture, GLenum target, GLenum pname, const GLfloat* param)
{
    glBindTexture(target, texture);
    glTexParameterfv(target, pname, param);
}

void GLAPIENTRY dsa_glMultiTexParameteriEXT(GLenum texunit, GLenum target, GLenum pname, GLint param)
{
    glActiveTexture(texunit);
    glTexParameteri(target, pname, param);
}

void GLAPIENTRY dsa_glMultiTexParameterivEXT(GLenum texunit, GLenum target, GLenum pname, const GLint* param)
{
    glActiveTexture(texunit);
    glTexParameteriv(target, pname, param);
}

void GLAPIENTRY dsa_glMultiTexParameterfEXT(GLenum texunit, GLenum target, GLenum pname, GLfloat param)
{
    glActiveTexture(texunit);
    glTexParameterf(target, pname, param);
}

void GLAPIENTRY dsa_glMultiTexParameterfvEXT(GLenum texunit, GLenum target, GLenum pname, const GLfloat* param)
{
    glActiveTexture(texunit);
    glTexParameterfv(target, pname, param);
}

void GLAPIENTRY dsa_glNamedBufferDataEXT(GLuint buffer, GLsizeiptr size, const GLvoid *data, GLenum usage)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, size, data, usage);
}

void GLAPIENTRY dsa_glBindMultiTextureEXT(GLenum texunit, GLenum target, GLuint texture)
{
    glActiveTexture(texunit);
    glBindTexture(target, texture);
}

void GLAPIENTRY dsa_glMultiTexGenfvEXT(GLenum texunit, GLenum coord, GLenum pname, const GLfloat* params)
{
    glActiveTexture(texunit);
    glTexGenfv(coord, pname, params);
}

void GLAPIENTRY dsa_glMultiTexGendvEXT(GLenum texunit, GLenum coord, GLenum pname, const GLdouble* params)
{
    glActiveTexture(texunit);
    glTexGendv(coord, pname, params);
}

void GLAPIENTRY dsa_glMultiTexGenivEXT(GLenum texunit, GLenum coord, GLenum pname, const GLint* params)
{
    glActiveTexture(texunit);
    glTexGeniv(coord, pname, params);
}

void GLAPIENTRY dsa_glMultiTexGenfEXT(GLenum texunit, GLenum coord, GLenum pname, GLfloat param)
{
    glActiveTexture(texunit);
    glTexGenf(coord, pname, param);
}

void GLAPIENTRY dsa_glMultiTexGendEXT(GLenum texunit, GLenum coord, GLenum pname, GLdouble param)
{
    glActiveTexture(texunit);
    glTexGend(coord, pname, param);
}

void GLAPIENTRY dsa_glMultiTexGeniEXT(GLenum texunit, GLenum coord, GLenum pname, GLint param)
{
    glActiveTexture(texunit);
    glTexGeni(coord, pname, param);
}

void GLAPIENTRY dsa_glMultiTexEnvfvEXT(GLenum texunit, GLenum coord, GLenum pname, const GLfloat* params)
{
    glActiveTexture(texunit);
    glTexEnvfv(coord, pname, params);
}

void GLAPIENTRY dsa_glMultiTexEnvivEXT(GLenum texunit, GLenum coord, GLenum pname, const GLint* params)
{
    glActiveTexture(texunit);
    glTexEnviv(coord, pname, params);
}

void GLAPIENTRY dsa_glMultiTexEnvfEXT(GLenum texunit, GLenum coord, GLenum pname, GLfloat param)
{
    glActiveTexture(texunit);
    glTexEnvf(coord, pname, param);
}

void GLAPIENTRY dsa_glMultiTexEnviEXT(GLenum texunit, GLenum coord, GLenum pname, GLint param)
{
    glActiveTexture(texunit);
    glTexEnvi(coord, pname, param);
}

static PFNGLDISABLEINDEXEDEXTPROC __glewDisableIndexedEXT_stash;
static PFNGLENABLEINDEXEDEXTPROC __glewEnableIndexedEXT_stash;

void GLAPIENTRY dsa_glEnableIndexedEXT(GLenum cap, GLuint index)
{
    switch (cap) {
    case GL_TEXTURE_GEN_S:
    case GL_TEXTURE_GEN_T:
    case GL_TEXTURE_GEN_R:
    case GL_TEXTURE_GEN_Q:
    case GL_TEXTURE_1D:
    case GL_TEXTURE_2D:
    case GL_TEXTURE_3D:
    case GL_TEXTURE_CUBE_MAP:
    case GL_TEXTURE_RECTANGLE:
        glActiveTexture(GL_TEXTURE0+index);
        glEnable(cap);
        break;
    default:
        __glewEnableIndexedEXT(cap, index);
        break;
    }
}

void GLAPIENTRY dsa_glDisableIndexedEXT(GLenum cap, GLuint index)
{
    switch (cap) {
    case GL_TEXTURE_GEN_S:
    case GL_TEXTURE_GEN_T:
    case GL_TEXTURE_GEN_R:
    case GL_TEXTURE_GEN_Q:
    case GL_TEXTURE_1D:
    case GL_TEXTURE_2D:
    case GL_TEXTURE_3D:
    case GL_TEXTURE_CUBE_MAP:
    case GL_TEXTURE_RECTANGLE:
        glActiveTexture(GL_TEXTURE0+index);
        glDisable(cap);
        break;
    default:
        __glewDisableIndexedEXT_stash(cap, index);
        break;
    }
}

void emulate_dsa_if_needed(int forceDSAemulation)
{
    if (!forceDSAemulation && glutExtensionSupported("GL_EXT_direct_state_access")) {
        printf("supports EXT_direct_state_access\n");
    } else {
        printf("emulating lack of EXT_direct_state_access support...\n");
        __glewDisableClientStateIndexedEXT = dsa_glDisableClientStateIndexedEXT;
        __glewEnableClientStateIndexedEXT = dsa_glEnableClientStateIndexedEXT;
        __glewMultiTexCoordPointerEXT = dsa_glMultiTexCoordPointerEXT;
        __glewMatrixLoadIdentityEXT = dsa_glMatrixLoadIdentityEXT;
        __glewMatrixScalefEXT = dsa_glMatrixScalefEXT;
        __glewMatrixTranslatefEXT = dsa_glMatrixTranslatefEXT;
        __glewMatrixMultTransposefEXT = dsa_glMatrixMultTransposefEXT;
        __glewMatrixLoadTransposefEXT = dsa_glMatrixLoadTransposefEXT;
        __glewMatrixPopEXT = dsa_glMatrixPopEXT;
        __glewMatrixPushEXT = dsa_glMatrixPushEXT;
        __glewTextureBufferEXT = dsa_glTextureBufferEXT;
        __glewTextureImage1DEXT = dsa_glTextureImage1DEXT;
        __glewTextureImage2DEXT = dsa_glTextureImage2DEXT;
        __glewTextureImage3DEXT = dsa_glTextureImage3DEXT;

        __glewTextureParameterfEXT = dsa_glTextureParameterfEXT;
        __glewTextureParameterfvEXT = dsa_glTextureParameterfvEXT;
        __glewTextureParameteriEXT = dsa_glTextureParameteriEXT;
        __glewTextureParameterivEXT = dsa_glTextureParameterivEXT;

        __glewMultiTexParameterfEXT = dsa_glMultiTexParameterfEXT;
        __glewMultiTexParameterfvEXT = dsa_glMultiTexParameterfvEXT;
        __glewMultiTexParameteriEXT = dsa_glMultiTexParameteriEXT;
        __glewMultiTexParameterivEXT = dsa_glMultiTexParameterivEXT;

        __glewBindMultiTextureEXT = dsa_glBindMultiTextureEXT;
        __glewNamedBufferDataEXT = dsa_glNamedBufferDataEXT;

        __glewMultiTexGenfvEXT = dsa_glMultiTexGenfvEXT;
        __glewMultiTexGendvEXT = dsa_glMultiTexGendvEXT;
        __glewMultiTexGenivEXT = dsa_glMultiTexGenivEXT;

        __glewMultiTexGenfEXT = dsa_glMultiTexGenfEXT;
        __glewMultiTexGendEXT = dsa_glMultiTexGendEXT;
        __glewMultiTexGeniEXT = dsa_glMultiTexGeniEXT;

        __glewMultiTexEnvfvEXT = dsa_glMultiTexEnvfvEXT;
        __glewMultiTexEnvivEXT = dsa_glMultiTexEnvivEXT;

        __glewMultiTexEnvfEXT = dsa_glMultiTexEnvfEXT;
        __glewMultiTexEnviEXT = dsa_glMultiTexEnviEXT;

        __glewEnableIndexedEXT_stash = __glewEnableIndexedEXT;
        __glewEnableIndexedEXT = dsa_glEnableIndexedEXT;

        __glewDisableIndexedEXT_stash = __glewDisableIndexedEXT;
        __glewDisableIndexedEXT = dsa_glDisableIndexedEXT;
    }
}

#endif
