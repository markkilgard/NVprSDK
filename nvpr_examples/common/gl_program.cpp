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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>  // for OutputDebugStringA
#endif

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include "gl_program.hpp"

static void LOGE(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

bool gl_program_verbose = false;

static void compileShaderMinimal(GLuint shader, const std::vector<std::string>* ignored)
{
  assert(ignored == NULL);
  glCompileShader(shader);
}

static void compileShaderInclude(GLuint shader, const std::vector<std::string>* indirect)
{
  const std::vector<std::string>& include_paths = *indirect;
  if (include_paths.size() > 0) {
    std::vector<const char*> paths;
    for (size_t i=0; i<include_paths.size(); i++) {
      paths.push_back(include_paths[i].c_str());
    }
    GLint count = GLint(include_paths.size());
    glCompileShaderIncludeARB(shader, count, &paths[0], NULL);
  } else {
    glCompileShader(shader);
  }
}

GLSLMinimalProgram::GLSLMinimalProgram()
  : program_obj(0)
{
}

GLSLProgram::GLSLProgram()
{}

GLSLProgram::GLSLProgram(const char *vsource, const char *fsource)
{
  compileProgram(vsource, NULL, fsource);
}

GLSLProgram::GLSLProgram(const char *vsource, const char *gsource, const char *fsource,
                         GLenum gsInput, GLenum gsOutput, int maxVerts)
{
  compileProgram(vsource, gsource, fsource, gsInput, gsOutput, maxVerts);
}

GLSLProgram::~GLSLProgram()
{}

void GLSLIncludeProgram::resetCommonDefines()
{
  common_defines.clear();
}

void GLSLIncludeProgram::setCommonDefines(const char *s)
{
  common_defines = s;
}

void GLSLIncludeProgram::setCommonDefines(const std::string &s)
{
  common_defines = s;
}

void GLSLIncludeProgram::setCommonDefinesFromFile(char *filename)
{
  char *contents = readTextFile(filename);
  if (contents) {
    common_defines = contents;
    delete [] contents;
  }
}

void GLSLIncludeProgram::resetIncludeDirectories()
{
  include_paths.clear();
}

void GLSLIncludeProgram::setIncludeSingleDirectory(const char* path)
{
  resetIncludeDirectories();
  addIncludeSingleDirectory(path);
}

void GLSLIncludeProgram::setIncludeDirectories(int count, const char* path[])
{
  resetIncludeDirectories();
  addIncludeDirectories(count, path);
}

void GLSLIncludeProgram::setIncludeDirectories(const std::vector<std::string>& path)
{
  resetIncludeDirectories();
  addIncludeDirectories(path);
}

void GLSLIncludeProgram::addIncludeSingleDirectory(const char* path)
{
  include_paths.push_back(path);
}

void GLSLIncludeProgram::addIncludeDirectories(int count, const char* path[])
{
  for (int i=0; i<count; i++) {
    include_paths.push_back(path[i]);
  }
}

void GLSLIncludeProgram::addIncludeDirectories(const std::vector<std::string>& path)
{
  for (size_t i=0; i<path.size(); i++) {
    include_paths.push_back(path[i]);
  }
}

GLuint
GLSLProgram::compileFromFiles(
  const char *vFilename,
  const char *gFilename,
  const char *fFilename,
  GLenum gsInput, GLenum gsOutput, int maxVerts)  // Subject to default initializers.
{
  char *vsource = readTextFile(vFilename);
  char *gsource = 0;
  if (gFilename) {
    gsource = readTextFile(gFilename);
  }
  char *fsource = readTextFile(fFilename);

  GLuint program_obj = compileProgram(vsource, gsource, fsource, gsInput, gsOutput, maxVerts);
  if (program_obj) {
    collectUniforms();
  }

  delete [] vsource;
  delete [] gsource;
  delete [] fsource;
  return program_obj;
}

GLuint
GLSLProgram::compileTessellationFromFiles(
  const char *vFilename,
  const char *tcFilename, const char *teFilename,
  const char *gFilename,  // Optional geometry shader filename, NULL means no geometry shader.
  const char *fFilename,
  GLenum gsInput, GLenum gsOutput, int maxVerts)  // Subject to default initializers.
{
  char *vsource = readTextFile(vFilename);
  char *tcsource = readTextFile(tcFilename);
  char *tesource = readTextFile(teFilename);
  char *gsource = 0;
  if (gFilename) {
    gsource = readTextFile(gFilename);
  }
  char *fsource = readTextFile(fFilename);

  GLuint program_obj = compileTessellationProgram(vsource, tcsource, tesource, gsource, fsource, gsInput, gsOutput, maxVerts);
  if (program_obj) {
    collectUniforms();
  }

  delete [] vsource;
  delete [] tcsource;
  delete [] tesource;
  delete [] gsource;
  delete [] fsource;
  return program_obj;
}

GLuint
GLSLIncludeProgram::compileFromFiles(
  const char *vFilename,
  const char *gFilename,
  const char *fFilename,
  GLenum gsInput, GLenum gsOutput, int maxVerts)  // Subject to default initializers.
{
  char *vsource = readTextFile(vFilename);
  char *gsource = 0;
  if (gFilename) {
    gsource = readTextFile(gFilename);
  }
  char *fsource = readTextFile(fFilename);

  GLuint program_obj = GLSLMinimalProgram::compileProgram(vsource, gsource, fsource,
    gsInput, gsOutput, maxVerts,
    common_defines.c_str(), compileShaderInclude, &include_paths);
  if (program_obj) {
    collectUniforms();
  }

  delete [] vsource;
  delete [] gsource;
  delete [] fsource;
  return program_obj;
}

GLuint
GLSLIncludeProgram::compileTessellationFromFiles(
  const char *vFilename,
  const char *tcFilename, const char *teFilename,
  const char *gFilename,  // Optional geometry shader filename, NULL means no geometry shader.
  const char *fFilename,
  GLenum gsInput, GLenum gsOutput, int maxVerts)  // Subject to default initializers.
{
  char *vsource = readTextFile(vFilename);
  char *tcsource = readTextFile(tcFilename);
  char *tesource = readTextFile(teFilename);
  char *gsource = 0;
  if (gFilename) {
    gsource = readTextFile(gFilename);
  }
  char *fsource = readTextFile(fFilename);

  GLuint program_obj = GLSLMinimalProgram::compileTessellationProgram(vsource, tcsource, tesource, gsource, fsource,
    gsInput, gsOutput, maxVerts,
    common_defines.c_str(), compileShaderInclude, &include_paths);
  if (program_obj) {
    collectUniforms();
  }

  delete [] vsource;
  delete [] tcsource;
  delete [] tesource;
  delete [] gsource;
  delete [] fsource;
  return program_obj;
}

void GLSLMinimalProgram::deleteProgram()
{
  if (program_obj) {
    glDeleteProgram(program_obj);
    program_obj = 0;
  }
}

GLSLMinimalProgram::~GLSLMinimalProgram()
{
  deleteProgram();
}

void GLSLMinimalProgram::resetProgram()
{
  deleteProgram();
}

void GLSLProgram::resetProgram()
{
  GLSLMinimalProgram::deleteProgram();
  uniform_locs.clear();
}

void
GLSLMinimalProgram::use()
{
  assert(program_obj);  // Expect a non-zero compiled & linked program object.
  glUseProgram(program_obj);
}

GLint GLSLMinimalProgram::getUniformLocation(const char *name)  // Inefficient, does OpenGL query!
{
  return glGetUniformLocation(program_obj, name);
}

void
GLSLMinimalProgram::abandonState()
{
  program_obj = 0;  // Zero without actually deleting the GL program object!
}

void
GLSLProgram::abandonState()
{
  GLSLMinimalProgram::abandonState();
  uniform_locs.clear();
}

void
GLSLIncludeProgram::abandonState()
{
  GLSLProgram::abandonState();
  resetCommonDefines();
  resetIncludeDirectories();
}

void
GLSLMinimalProgram::aquireState(GLSLProgram& donor)
{
  // Take the program object from the donor.
  program_obj = donor.getProgId();

  // While abondoning the donor's state.
  donor.abandonState();
}

void
GLSLMinimalProgram::adoptState(GLSLMinimalProgram& donor)
{
  // Take the program object from the donor.
  program_obj = donor.getProgId();
}

void
GLSLMinimalProgram::aquireState(GLSLIncludeProgram& donor)
{
  adoptState(donor);

  // While abondoning the donor's state.
  donor.abandonState();
}

void
GLSLProgram::adoptState(GLSLProgram& donor)
{
  GLSLMinimalProgram::adoptState(donor);
  uniform_locs.clear();
  uniform_locs.swap(donor.uniform_locs);
}

void
GLSLProgram::aquireState(GLSLIncludeProgram& donor)
{
  // Take the program object & uniform_locs from the donor.
  adoptState(donor);

  // While abondoning the donor's state.
  donor.abandonState();
}

void
GLSLProgram::setUniform1f(const char *name, float value)
{
  assert(programBoundNow());
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glUniform1f(loc, value);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::setUniform2f(const char *name, float x, float y)
{
  assert(programBoundNow());
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glUniform2f(loc, x, y);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::setUniform2fv(const char *name, const float xy[2])
{
  assert(programBoundNow());
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    const GLsizei count = 1;
    glUniform2fv(loc, count, xy);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::setUniform2fv(const char *name, int count, const float xy[])
{
  assert(programBoundNow());
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glUniform2fv(loc, count, xy);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::setUniform2i(const char *name, int x, int y)
{
  assert(programBoundNow());
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glUniform2i(loc, x, y);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::setUniform3f(const char *name, float x, float y, float z)
{
  assert(programBoundNow());
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glUniform3f(loc, x, y, z);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::setUniform3fv(const char *name, const float xyz[3])
{
  assert(programBoundNow());
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    const GLsizei count = 1;
    glUniform3fv(loc, count, xyz);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::setUniform3fv(const char *name, int count, const float xyz[3])
{
  assert(programBoundNow());
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glUniform3fv(loc, count, xyz);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::setUniform4f(const char *name, float x, float y, float z, float w)
{
  assert(programBoundNow());
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glUniform4f(loc, x, y, z, w);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::setUniform4fv(const char *name, const float xyzw[4])
{
  assert(programBoundNow());
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    const GLsizei count = 1;
    glUniform4fv(loc, count, xyzw);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::setUniform4fv(const char *name, int count, const float xyzw[4])
{
  assert(programBoundNow());
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glUniform4fv(loc, count, xyzw);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::setUniformMatrix4fv(const GLchar *name, GLfloat *m, bool transpose)
{
  assert(programBoundNow());
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glUniformMatrix4fv(loc, 1, transpose, m);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::setUniformfv(const GLchar *name, GLfloat *v, int elementSize, int count)
{
  assert(programBoundNow());
  GLint loc = getUniformLocation(name);
  if (loc == -1) {
#ifdef _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
    return;
  }

  switch (elementSize) {
  case 1:
    glUniform1fv(loc, count, v);
    break;
  case 2:
    glUniform2fv(loc, count, v);
    break;
  case 3:
    glUniform3fv(loc, count, v);
    break;
  case 4:
    glUniform4fv(loc, count, v);
    break;
  }
}

void
GLSLProgram::setUniform3i(const char *name, int x, int y, int z)
{
  assert(programBoundNow());
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glUniform3i(loc, x, y, z);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::set1f(const char *name, float value)
{
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glProgramUniform1f(getProgId(), loc, value);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::set2f(const char *name, float x, float y)
{
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glProgramUniform2f(getProgId(), loc, x, y);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::set2fv(const char *name, const float xy[2])
{
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    const GLsizei count = 1;
    glProgramUniform2fv(getProgId(), loc, count, xy);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::set2fv(const char *name, int count, const float xy[])
{
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glProgramUniform2fv(getProgId(), loc, count, xy);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::set1i(const char *name, int x)
{
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glProgramUniform1i(getProgId(), loc, x);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::set2i(const char *name, int x, int y)
{
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glProgramUniform2i(getProgId(), loc, x, y);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::set1ui(const char *name, GLuint x)
{
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glProgramUniform1ui(getProgId(), loc, x);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::set2ui(const char *name, GLuint x, GLuint y)
{
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glProgramUniform2ui(getProgId(), loc, x, y);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}


void
GLSLProgram::set3f(const char *name, float x, float y, float z)
{
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glProgramUniform3f(getProgId(), loc, x, y, z);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::set3fv(const char *name, const float xyz[3])
{
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    const GLsizei count = 1;
    glProgramUniform3fv(getProgId(), loc, count, xyz);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::set3fv(const char *name, int count, const float xyz[3])
{
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glProgramUniform3fv(getProgId(), loc, count, xyz);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::set4f(const char *name, float x, float y, float z, float w)
{
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glProgramUniform4f(getProgId(), loc, x, y, z, w);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::set4fv(const char *name, const float xyzw[4])
{
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    const GLsizei count = 1;
    glProgramUniform4fv(getProgId(), loc, count, xyzw);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::set4fv(const char *name, int count, const float xyzw[4])
{
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glProgramUniform4fv(getProgId(), loc, count, xyzw);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::setMatrix4fv(const GLchar *name, GLfloat *m, bool transpose)
{
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glProgramUniformMatrix4fv(getProgId(), loc, 1, transpose, m);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::setfv(const GLchar *name, GLfloat *v, int elementSize, int count)
{
  GLint loc = getUniformLocation(name);
  if (loc == -1) {
#ifdef _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
    return;
  }

  switch (elementSize) {
  case 1:
    glProgramUniform1fv(getProgId(), loc, count, v);
    break;
  case 2:
    glProgramUniform2fv(getProgId(), loc, count, v);
    break;
  case 3:
    glProgramUniform3fv(getProgId(), loc, count, v);
    break;
  case 4:
    glProgramUniform4fv(getProgId(), loc, count, v);
    break;
  }
}

void
GLSLProgram::set3i(const GLchar *name, GLint x, GLint y, GLint z)
{
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glProgramUniform3i(getProgId(), loc, x, y, z);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

void
GLSLProgram::set3ui(const GLchar *name, GLuint x, GLuint y, GLuint z)
{
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glProgramUniform3ui(getProgId(), loc, x, y, z);
  } else {
#if _DEBUG
    LOGE("Warning: parameter '%s' unavailable\n", name);
#endif
  }
}

GLint GLSLProgram::getUniformLocation(const char *name)
{
  for (size_t i=0; i<uniform_locs.size(); i++) {
    UniformLoc &u = uniform_locs[i];
    if (!strcmp(name, u.name.c_str())) {
      return u.value;
    }
  }
  return -1;
}

void
GLSLProgram::setTexture(const char *name, GLint unit)
{
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glProgramUniform1i(getProgId(), loc, unit);
  } else {
#if _DEBUG
    LOGE("Error binding texture '%s'\n", name);
#endif
  }
}

void
GLSLProgram::bindTexture(const char *name, GLuint tex, GLenum target, GLint unit)
{
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(target, tex);
    use();
    glUniform1i(loc, unit);
    glActiveTexture(GL_TEXTURE0);
  } else {
#if _DEBUG
    LOGE("Error binding texture '%s'\n", name);
#endif
  }
}

void
GLSLProgram::bindImage(const char *name, GLint unit, GLuint tex, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
{
  GLint loc = getUniformLocation(name);
  if (loc >= 0) {
    glBindImageTexture(unit, tex, level, layered, layer, access, format);
    use();
    glUniform1i(loc, unit);
  } else {
#if _DEBUG
    LOGE("Error binding texture '%s'\n", name);
#endif
  }
}

bool GLSLMinimalProgram::linkProgram()
{
  glLinkProgram(program_obj);

  // check if program linked
  GLint success = 0;
  glGetProgramiv(program_obj, GL_LINK_STATUS, &success);

  if (!success) {
    char temp[1024];
    glGetProgramInfoLog(program_obj, 1024, 0, temp);
    LOGE("Failed to link program:\n%s\n", temp);
#ifdef _WIN32
    OutputDebugStringA(temp);
#endif
    glDeleteProgram(program_obj);
    program_obj = 0;
    return false;
  }
  return true;
}

void GLSLProgram::collectUniforms()
{
  uniform_locs.clear();
  GLint active_uniforms = 0;
  glGetProgramiv(getProgId(), GL_ACTIVE_UNIFORMS, &active_uniforms);
  if (gl_program_verbose) {
    printf("activeUniforms = %d\n", active_uniforms);
  }
  uniform_locs.reserve(active_uniforms);
  GLint maximum_uniform_name_length = 0;
  glGetProgramiv(getProgId(), GL_ACTIVE_UNIFORM_MAX_LENGTH, &maximum_uniform_name_length);
  if (gl_program_verbose) {
    printf("maximum_uniform_name_length = %d\n", maximum_uniform_name_length);
  }
  GLchar *name = new GLchar[maximum_uniform_name_length];
  for (int i = 0; i <active_uniforms; i++) {
    GLuint uniform_index = GLuint(i);
#ifndef NDEBUG
    GLint name_length = 0;
    glGetActiveUniformsiv(getProgId(), 1, &uniform_index, GL_UNIFORM_NAME_LENGTH, &name_length);
    assert(name_length <= maximum_uniform_name_length);
#endif
    GLsizei actual_name_length = 0;
    glGetActiveUniformName(getProgId(), uniform_index, maximum_uniform_name_length, &actual_name_length, name);
    assert(actual_name_length+1 <= maximum_uniform_name_length);
#ifndef NDEBUG
    assert(actual_name_length+1 == name_length);
#endif
    // Is the uniform is a GLSL built-in?
    if (!strncmp(name, "gl_", sizeof("gl_")-1)) {
      // Yes, skip it.
      assert(glGetUniformLocation(getProgId(), name) == -1);
      if (gl_program_verbose) {
        printf("  %d: %s BUILTIN\n", i, name);
      }
    } else {
      GLint location = glGetUniformLocation(getProgId(), name);
      if (gl_program_verbose) {
        printf("  %d: %s = %u\n", i, name, location);
      }
      assert(location >= 0);
      uniform_locs.push_back(UniformLoc(name, location));
    }
  }
  delete [] name;
}

// XXX improve this
void GLSLProgram::dumpUniforms()
{
  GLint active_uniforms = 0;
  glGetProgramiv(getProgId(), GL_ACTIVE_UNIFORMS, &active_uniforms);
  if (gl_program_verbose) {
    printf("activeUniforms = %d\n", active_uniforms);
  }
  GLint maximum_uniform_name_length = 0;
  glGetProgramiv(getProgId(), GL_ACTIVE_UNIFORM_MAX_LENGTH, &maximum_uniform_name_length);
  if (gl_program_verbose) {
    printf("maximum_uniform_name_length = %d\n", maximum_uniform_name_length);
  }
  GLchar *name = new GLchar[maximum_uniform_name_length];
  for (int i = 0; i <active_uniforms; i++) {
    GLuint uniform_index = GLuint(i);
#ifndef NDEBUG
    GLint name_length = 0;
    glGetActiveUniformsiv(getProgId(), 1, &uniform_index, GL_UNIFORM_NAME_LENGTH, &name_length);
    assert(name_length <= maximum_uniform_name_length);
#endif
    GLsizei actual_name_length = 0;
    glGetActiveUniformName(getProgId(), uniform_index, maximum_uniform_name_length, &actual_name_length, name);
    assert(actual_name_length+1 <= maximum_uniform_name_length);
#ifndef NDEBUG
    assert(actual_name_length+1 == name_length);
#endif
    GLint size = 0;
    GLenum type = GL_NONE;
    glGetActiveUniform(getProgId(), i, 0, NULL, &size, &type, NULL);
    // Is the uniform is a GLSL built-in?
    if (!strncmp(name, "gl_", sizeof("gl_")-1)) {
      // Yes, skip it.
      assert(glGetUniformLocation(getProgId(), name) == -1);
      if (gl_program_verbose) {
        printf("  %d: %s BUILTIN\n", i, name);
      }
      printf("  %d: %s BUILTIN\n", i, name);
    } else {
      GLint location = glGetUniformLocation(getProgId(), name);
      if (gl_program_verbose) {
        printf("  %d: %s = %u\n", i, name, location);
      }
      assert(location >= 0);
      switch (type) {
      case GL_FLOAT:
          {
              GLfloat data[16];
              printf("  %d: %s (location = %u) size=%d :: FLOAT :: ", i, name, location, size);
              glGetUniformfv(getProgId(), location, data);
              for (GLint j=0; j<size; j++) {
                  printf(" %g", data[j]);
              }
          }
          break;
      case GL_INT:
          {
              GLint data[16];
              printf("  %d: %s (location = %u) size=%d :: INT :: ", i, name, location, size);
              glGetUniformiv(getProgId(), location, data);
              for (GLint j=0; j<size; j++) {
                  printf(" %d", data[j]);
              }
          }
          break;
      case GL_UNSIGNED_INT:
          {
              GLint data[16];
              printf("  %d: %s (location = %u) size=%d :: INT :: ", i, name, location, size);
              glGetUniformiv(getProgId(), location, data);
              for (GLint j=0; j<size; j++) {
                  printf(" %d (0x%x)", data[j], data[j]);
              }
          }
          break;
      case GL_FLOAT_VEC2:
          {
              GLfloat data[16];
              printf("  %d: %s (location = %u) size=%d :: FLOAT4 :: ", i, name, location, size);
              glGetUniformfv(getProgId(), location, data);
              for (GLint j=0; j<size; j+=2) {
                  printf(" <%g,%g>", data[j], data[j+1]);
              }
          }
          break;
      case GL_FLOAT_VEC3:
          {
              GLfloat data[16];
              printf("  %d: %s (location = %u) size=%d :: FLOAT4 :: ", i, name, location, size);
              glGetUniformfv(getProgId(), location, data);
              for (GLint j=0; j<size; j+=3) {
                  printf(" <%g,%g,%g>", data[j], data[j+1], data[j+2]);
              }
          }
          break;
      case GL_FLOAT_VEC4:
          {
              GLfloat data[16];
              printf("  %d: %s (location = %u) size=%d :: FLOAT4 :: ", i, name, location, size);
              glGetUniformfv(getProgId(), location, data);
              for (GLint j=0; j<size; j+=4) {
                  printf(" <%g,%g,%g,%g>", data[j], data[j+1], data[j+2], data[j+3]);
              }
          }
          break;
      default:
          printf("  %d: %s (location = %u) :: unknown type (0x%x) and size (%d)", i, name, location, type, size);
          break;
      }
      printf("\n");
    }
  }
  delete [] name;
}

GLuint
GLSLMinimalProgram::compileProgram(
  const char* vsource,  // Vertex shader source.
  const char* gsource,  // Optional geometry shader source (NULL indicates no geometry shader).
  const char* fsource,  // Fragment shader source.
  GLenum gsInput, GLenum gsOutput, int maxVerts,
  const char* common_defines,  // Optional, NULL means no common defines.
  void (*compileShader)(GLuint shader, const std::vector<std::string>*),
  const std::vector<std::string>* possible_include_paths)
{
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  GLuint geometryShader = 0;

  if (program_obj) {
    glDeleteProgram(program_obj);
  }
  program_obj = glCreateProgram();
  if (program_obj) {
    if(vsource) {
      addSource(vertexShader, common_defines, vsource, "current.vert");
      compileShader(vertexShader, possible_include_paths);
      glAttachShader(program_obj, vertexShader);
    }
    if (fsource) {
      addSource(fragmentShader, common_defines, fsource, "current.frag");
      compileShader(fragmentShader, possible_include_paths);
      glAttachShader(program_obj, fragmentShader);
    }

    if (gsource) {
      geometryShader = glCreateShader(GL_GEOMETRY_SHADER_EXT);
      addSource(geometryShader, common_defines, gsource, "current.geom");
      compileShader(geometryShader, possible_include_paths);
      glAttachShader(program_obj, geometryShader);

      glProgramParameteriEXT(program_obj, GL_GEOMETRY_INPUT_TYPE_EXT, gsInput);
      glProgramParameteriEXT(program_obj, GL_GEOMETRY_OUTPUT_TYPE_EXT, gsOutput);
      glProgramParameteriEXT(program_obj, GL_GEOMETRY_VERTICES_OUT_EXT, maxVerts);
    }

    bool success = linkProgram();
    if (!success) {
      glDeleteProgram(program_obj);
      program_obj = 0;
    }
  }

  if (vertexShader) {
    glDeleteShader(vertexShader);
  }
  if (geometryShader) {
    glDeleteShader(geometryShader);
  }
  if (fragmentShader) {
    glDeleteShader(fragmentShader);
  }

  return program_obj;
}

GLuint
GLSLIncludeProgram::compileProgram(
  const char* vsource,  // Vertex shader source.
  const char* gsource,  // Optional geometry shader source (NULL indicates no geometry shader).
  const char* fsource,  // Fragment shader source.
  GLenum gsInput, GLenum gsOutput, int maxVerts)  // Subject to default initializers.
{
  return GLSLMinimalProgram::compileProgram(vsource, gsource, fsource, 
    gsInput, gsOutput, maxVerts,
    common_defines.c_str(),
    compileShaderInclude, &include_paths);
}

void writeFile(const char *filename, int count, const char *chucks[])
{
    FILE *file = fopen(filename, "wt");
    if (file) {
        for (int i=0; i<count; i++) {
            fwrite(chucks[i], sizeof(char), strlen(chucks[i]), file);
        }
        fclose(file);
    }
}

GLuint GLSLProgram::compileProgram(
  const char *vsource,
  const char *gsource,
  const char *fsource,
  GLenum gsInput, GLenum gsOutput, int maxVerts)
{
  GLuint program_obj = GLSLMinimalProgram::compileProgram(vsource, gsource, fsource, 
    gsInput, gsOutput, maxVerts,
    NULL, compileShaderMinimal, NULL);
  if (program_obj) {
    collectUniforms();
  }
  return program_obj;
}

static bool dump_source = false;

void GLSLDumpSource(bool v)
{
  dump_source = v;
}

void GLSLMinimalProgram::addSource(GLuint shader, const char* common_defines, const char* source, const char* dump_filename)
{
  if (common_defines) {
    const char* chunks[2] = { common_defines, source };
    glShaderSource(shader, 2, chunks, 0);
    if (dump_source) {
      writeFile(dump_filename, 2, chunks);
    }
  } else {
    glShaderSource(shader, 1, &source, 0);
    if (dump_source) {
      writeFile(dump_filename, 1, &source);
    }
  }
}

GLuint
GLSLMinimalProgram::compileTessellationProgram(
  const char* vsource,
  const char* tcsource, const char *tesource,
  const char* gsource,
  const char* fsource,
  GLenum gsInput, GLenum gsOutput, int maxVerts,
  const char* common_defines,
  void (*compileShader)(GLuint shader, const std::vector<std::string>*),
  const std::vector<std::string>* possible_include_paths)
{
  const GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  const GLuint controlShader = glCreateShader(GL_TESS_CONTROL_SHADER);
  const GLuint evaluationShader = glCreateShader(GL_TESS_EVALUATION_SHADER);
  const GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  GLuint geometryShader = 0;  // Optional, used if gsource is non-NULL.

  program_obj = glCreateProgram();

  if (program_obj) {
    if(vsource) {
      addSource(vertexShader, common_defines, vsource, "current.vert");
      compileShader(vertexShader, possible_include_paths);
      glAttachShader(program_obj, vertexShader);
    }
    if(tcsource) {
      addSource(controlShader, common_defines, tcsource, "current.tesc");
      compileShader(controlShader, possible_include_paths);
      glAttachShader(program_obj, controlShader);
    }
    if(tesource) {
      addSource(evaluationShader, common_defines, tesource, "current.tese");
      compileShader(evaluationShader, possible_include_paths);
      glAttachShader(program_obj, evaluationShader);
    }
    if (gsource) {
      geometryShader = glCreateShader(GL_GEOMETRY_SHADER_EXT);
      addSource(geometryShader, common_defines, gsource, "current.geom");
      compileShader(geometryShader, possible_include_paths);
      glAttachShader(program_obj, geometryShader);

      glProgramParameteriEXT(program_obj, GL_GEOMETRY_INPUT_TYPE_EXT, gsInput);
      glProgramParameteriEXT(program_obj, GL_GEOMETRY_OUTPUT_TYPE_EXT, gsOutput);
      glProgramParameteriEXT(program_obj, GL_GEOMETRY_VERTICES_OUT_EXT, maxVerts);
    }
    if (fsource) {
      addSource(fragmentShader, common_defines, fsource, "current.frag");
      compileShader(fragmentShader, possible_include_paths);
      glAttachShader(program_obj, fragmentShader);
    }

    bool success = linkProgram();
    if (!success) {
      glDeleteProgram(program_obj);
      program_obj = 0;
    }
  }

  if (vertexShader) {
    glDeleteShader(vertexShader);
  }
  if (controlShader) {
    glDeleteShader(controlShader);
  }
  if (evaluationShader) {
    glDeleteShader(evaluationShader);
  }
  if (geometryShader) {
    glDeleteShader(geometryShader);
  }
  if (fragmentShader) {
    glDeleteShader(fragmentShader);
  }

  return program_obj;
}

GLuint
GLSLMinimalProgram::compileTessellationProgram(
  const char *vsource,
  const char *tcsource, const char *tesource,
  const char *gsource,
  const char *fsource,
  GLenum gsInput, GLenum gsOutput, int maxVerts)  // Subject to default initializers.
{
  return compileTessellationProgram(vsource, tcsource, tesource, gsource, fsource,
    gsInput, gsOutput, maxVerts,
    NULL, compileShaderMinimal, NULL);
}

GLuint
GLSLProgram::compileTessellationProgram(
  const char *vsource,
  const char *tcsource, const char *tesource,
  const char *gsource,
  const char *fsource,
  GLenum gsInput, GLenum gsOutput, int maxVerts)  // Subject to default initializers.
{
  GLuint program_obj = GLSLMinimalProgram::compileTessellationProgram(
    vsource,
    tcsource, tesource,
    gsource,
    fsource,
    gsInput, gsOutput, maxVerts);
  if (program_obj) {
    collectUniforms();
  }
  return program_obj;
}

GLuint
GLSLIncludeProgram::compileTessellationProgram(
  const char *vsource,
  const char *tcsource, const char *tesource,
  const char *gsource,
  const char *fsource,
  GLenum gsInput, GLenum gsOutput, int maxVerts)  // Subject to default initializers.
{
  GLuint program_obj = GLSLMinimalProgram::compileTessellationProgram(
    vsource,
    tcsource, tesource,
    gsource,
    fsource,
    gsInput, gsOutput, maxVerts,
    common_defines.c_str(), compileShaderInclude, &include_paths);
  if (program_obj) {
    collectUniforms();
  }
  return program_obj;
}

char *
GLSLMinimalProgram::readTextFile(const char *filename)
{
  if (!filename) return 0;
  FILE *fp = 0;
  if (!(fp = fopen(filename, "r"))) {
    LOGE("Cannot open \"%s\" for read!\n", filename);
    return 0;
  }

  fseek(fp, 0L, SEEK_END);     // seek to end of file
  long size = ftell(fp);       // get file length
  rewind(fp);                  // rewind to start of file

  char * buf = new char[size+1];

  size_t bytes;
  bytes = fread(buf, 1, size, fp);

  buf[bytes] = 0;

  fclose(fp);
  return buf;
}
