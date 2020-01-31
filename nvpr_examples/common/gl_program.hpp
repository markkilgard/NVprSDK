/*
 * Copyright 1993-2017 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */
 
 // Simple class to contain GLSL shaders/programs

#ifndef GLSL_PROGRAM_H
#define GLSL_PROGRAM_H

#include <stdio.h>
#include <assert.h>
#include <GL/glew.h>

#include <vector>
#include <string>

class GLSLProgram;
class GLSLIncludeProgram;

class GLSLMinimalProgram {
  GLuint program_obj;  // OpenGL driver-assigned program object name (initially zero).

  void addSource(GLuint shader, const char* common_defines, const char* source, const char* dump_filename);

protected:
  GLuint compileProgram(
    const char* vsource,  // Vertex shader source.
    const char* gsource,  // Optional geometry shader source (NULL indicates no geometry shader).
    const char* fsource,  // Fragment shader source.
    GLenum gsInput, GLenum gsOutput, int maxVerts,
    const char* common_defines,  // Optional, NULL means no common defines.
    void (*compileShader)(GLuint shader, const std::vector<std::string>*),
    const std::vector<std::string>* indirect);
  // As above, but with tessellation shader support too.
  GLuint compileTessellationProgram(
    const char* vsource,
    const char* tcsource, const char *tesource,
    const char* gsource,
    const char* fsource,
    GLenum gsInput, GLenum gsOutput, int maxVerts,
    const char* common_defines,
    void (*compileShader)(GLuint shader, const std::vector<std::string>*),
    const std::vector<std::string>* possible_include_paths);

public:
  // construct program from strings
  GLSLMinimalProgram();
  GLSLMinimalProgram(const char *vsource, const char *fsource);
  GLSLMinimalProgram(const char *vsource, const char *gsource, const char *fsource,
    GLenum gsInput = GL_POINTS, GLenum gsOutput = GL_TRIANGLE_STRIP, int maxVerts = 4);

  ~GLSLMinimalProgram();

  GLuint compileTessellationProgram(
    const char *vsource,
    const char *tcsource, const char *tesource,
    const char *gsource,
    const char *fsource,
    GLenum gsInput, GLenum gsOutput, int maxVerts);

  GLuint compileFromFiles(
    const char *vFilename,
    const char *gFilename,
    const char *fFilename,
    GLenum gsInput = GL_POINTS, GLenum gsOutput = GL_TRIANGLE_STRIP, int maxVerts = 4);
  GLuint compileTessellationFromFiles(
    const char *vFilename,
    const char *tcFilename, const char *teFilename,  // tessellation control & evaluation shaders
    const char *gFilename,
    const char *fFilename,
    GLenum gsInput = GL_POINTS, GLenum gsOutput = GL_TRIANGLE_STRIP, int maxVerts = 4);

  void aquireState(GLSLProgram& donor);
  void aquireState(GLSLIncludeProgram& donor);

  inline GLuint getProgId() { return program_obj; }

  void resetProgram();  // reset program, reuse will force lazy compile

  void use();

  GLint getUniformLocation(const char *name);  // Inefficient, does OpenGL query!

protected:
  inline bool programBoundNow() {
    assert(program_obj);
    GLint current_program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
    return GLuint(current_program) == program_obj;
  }

  char *readTextFile(const char *filename);
  void compileShader(GLuint shader);
  void deleteProgram();
  bool linkProgram();

  void adoptState(GLSLMinimalProgram& donor);
  void abandonState();
};

class GLSLProgram : public GLSLMinimalProgram {
public:
  // construct program from strings
  GLSLProgram();
  GLSLProgram(const char *vsource, const char *fsource);
  GLSLProgram(const char *vsource, const char *gsource, const char *fsource,
    GLenum gsInput = GL_POINTS, GLenum gsOutput = GL_TRIANGLE_STRIP, int maxVerts = 4);

  ~GLSLProgram();

  GLuint compileFromFiles(
    const char *vFilename,
    const char *gFilename,
    const char *fFilename,
    GLenum gsInput = GL_POINTS, GLenum gsOutput = GL_TRIANGLE_STRIP, int maxVerts = 4);
  GLuint compileTessellationFromFiles(
    const char *vFilename,
    const char *tcFilename, const char *teFilename,  // tessellation control & evaluation shaders
    const char *gFilename,
    const char *fFilename, GLenum gsInput = GL_POINTS, GLenum gsOutput = GL_TRIANGLE_STRIP, int maxVerts = 4);

  inline GLuint compileFragmentShaderOnlyFromFiles(const char *fFilename) {
    return compileFromFiles(NULL, NULL, fFilename);
  }
  inline GLuint compileVertexFragmentShaderPairFromFiles(const char *vsource, const char *fsource) {
    return compileFromFiles(vsource, NULL, fsource);
  }

  GLuint compileProgram(const char *vsource, const char *gsource, const char *fsource,
    GLenum gsInput = GL_POINTS, GLenum gsOutput = GL_TRIANGLE_STRIP, int maxVerts = 4);
  GLuint compileTessellationProgram(const char *vsource, const char *tcsource, const char *tesource, const char *gsource, const char *fsource, GLenum gsInput = GL_POINTS, GLenum gsOutput = GL_TRIANGLE_STRIP, int maxVerts = 4);

  // Reduced argument helpers.
  inline GLuint compileTessellationProgram(const char *vsource, const char *tcsource, const char *tesource, const char *fsource) {
    return compileTessellationProgram(vsource, tcsource, tesource, NULL, fsource);
  }
  inline GLuint compileVertexFragmentShaderPair(const char *vsource, const char *fsource) {
    return compileProgram(vsource, NULL, fsource);
  }
  inline GLuint compileFragmentShaderOnly(const char *fsource) {
    return compileProgram(NULL, NULL, fsource);
  }
  inline GLuint compileGeometryShaderOnly(const char *gsource) {
    return compileProgram(NULL, gsource, NULL);
  }
  GLint getUniformLocation(const char *name);

  void resetProgram();  // reset program, reuse will force lazy compile

  // Semi-efficient uniform update routines using the uniform_locs to map strings to locations.
  // setUniform* routines assume program is already bound!!
  void setUniform1f(const GLchar *name, GLfloat x);
  void setUniform2f(const GLchar *name, GLfloat x, GLfloat y);
  void setUniform2fv(const char *name, const float xy[2]);
  void setUniform2fv(const char *name, int count, const float xy[]);
  void setUniform3f(const char *name, float x, float y, float z);
  void setUniform3fv(const char *name, const float xyz[3]);
  void setUniform3fv(const char *name, int count, const float xyz[]);
  void setUniform4f(const char *name, float x, float y=0.0f, float z=0.0f, float w=0.0f);
  void setUniform4fv(const char *name, const float xyzw[4]);
  void setUniform4fv(const char *name, int count, const float xyzw[]);
  void setUniformfv(const GLchar *name, GLfloat *v, int elementSize, int count=1);
  void setUniformMatrix4fv(const GLchar *name, GLfloat *m, bool transpose);
  void setUniform2i(const GLchar *name, GLint x, GLint y);
  void setUniform3i(const char *name, int x, int y, int z);

  // Semi-efficient uniform update routines using the uniform_locs to map strings to locations.
  void set1f(const GLchar *name, GLfloat x);
  void set2f(const GLchar *name, GLfloat x, GLfloat y);
  void set2fv(const char *name, const float xy[2]);
  void set2fv(const char *name, int count, const float xy[]);
  void set3f(const char *name, float x, float y, float z);
  void set3fv(const char *name, const float xyz[3]);
  void set3fv(const char *name, int count, const float xyz[]);
  void set4f(const char *name, float x, float y=0.0f, float z=0.0f, float w=0.0f);
  void set4fv(const char *name, const float xyzw[4]);
  void set4fv(const char *name, int count, const float xyzw[]);
  void setfv(const GLchar *name, GLfloat *v, int elementSize, int count=1);
  void setMatrix4fv(const GLchar *name, GLfloat *m, bool transpose);

  void set1i(const GLchar *name, GLint x);
  void set2i(const GLchar *name, GLint x, GLint y);
  void set3i(const GLchar *name, GLint x, GLint y, GLint z);

  void set1ui(const GLchar *name, GLuint x);
  void set2ui(const GLchar *name, GLuint x, GLuint y);
  void set3ui(const GLchar *name, GLuint x, GLuint y, GLuint z);

  void dumpUniforms();

  void setTexture(const char *name, GLint unit);

  void bindTexture(const char *name, GLuint tex, GLenum target, GLint unit);
  void bindImage(const char *name, GLint unit, GLuint tex, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);

  void aquireState(GLSLIncludeProgram& donor);

private:
  // Helper type for app-cached mapping of names to locations
  struct UniformLoc {
    std::string name;
    GLint value;

    UniformLoc(const char *s, GLint v)
      : name(s)
      , value(v)
    {
      assert(value >= 0);
    }
  };

  std::vector<UniformLoc> uniform_locs;

protected:
  void collectUniforms();
  friend class GLSLMinimalProgram;
  void adoptState(GLSLProgram& donor);
  void abandonState();
};

class GLSLIncludeProgram : public GLSLProgram {
  std::string common_defines;
  std::vector<std::string> include_paths;

protected:
  friend class GLSLMinimalProgram;
  friend class GLSLProgram;
  void abandonState();

public:
  void resetCommonDefines();
  void setCommonDefines(const char *s);
  void setCommonDefines(const std::string &s);
  void setCommonDefinesFromFile(char *filename);

  // Assumes GL_ARB_shading_language_include
  void resetIncludeDirectories();
  void setIncludeSingleDirectory(const char* path);
  void setIncludeDirectories(int count, const char* path[]);
  void setIncludeDirectories(const std::vector<std::string>& path);
  void addIncludeSingleDirectory(const char* path);
  void addIncludeDirectories(int count, const char* path[]);
  void addIncludeDirectories(const std::vector<std::string>& path);

  GLuint compileFromFiles(
    const char *vFilename,
    const char *gFilename,
    const char *fFilename,
    GLenum gsInput = GL_POINTS, GLenum gsOutput = GL_TRIANGLE_STRIP, int maxVerts = 4);
  GLuint compileTessellationFromFiles(
    const char *vFilename,
    const char *tcFilename, const char *teFilename,  // tessellation control & evaluation shaders
    const char *gFilename,
    const char *fFilename, GLenum gsInput = GL_POINTS, GLenum gsOutput = GL_TRIANGLE_STRIP, int maxVerts = 4);

  inline GLuint compileFragmentShaderOnlyFromFiles(const char *fFilename) {
    return compileFromFiles(NULL, NULL, fFilename);
  }
  inline GLuint compileVertexFragmentShaderPairFromFiles(const char *vsource, const char *fsource) {
    return compileFromFiles(vsource, NULL, fsource);
  }

  GLuint compileProgram(
    const char* vsource,  // Vertex shader source.
    const char* gsource,  // Optional geometry shader source (NULL indicates no geometry shader).
    const char* fsource,  // Fragment shader source.
    GLenum gsInput = GL_POINTS, GLenum gsOutput = GL_TRIANGLE_STRIP, int maxVerts = 4);  // Subject to default initializers.
  GLuint compileTessellationProgram(
    const char *vsource,
    const char *tcsource, const char *tesource,
    const char *gsource,
    const char *fsource,
    GLenum gsInput = GL_POINTS, GLenum gsOutput = GL_TRIANGLE_STRIP, int maxVerts = 4);

  // Reduced argument helpers.
  inline GLuint compileTessellationProgram(const char *vsource, const char *tcsource, const char *tesource, const char *fsource) {
    return compileTessellationProgram(vsource, tcsource, tesource, NULL, fsource);
  }
  inline GLuint compileVertexFragmentShaderPair(const char *vsource, const char *fsource) {
    return compileProgram(vsource, NULL, fsource);
  }
  inline GLuint compileFragmentShaderOnly(const char *fsource) {
    return compileProgram(NULL, NULL, fsource);
  }
  inline GLuint compileGeometryShaderOnly(const char *gsource) {
    return compileProgram(NULL, gsource, NULL);
  }
};

extern void GLSLDumpSource(bool v);  // Setting true dumps combined GLSL source; initially false.

#endif
