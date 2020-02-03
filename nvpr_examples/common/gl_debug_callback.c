
// gl_debug_callback.c - register OpenGL debug callback

#include <GL/glew.h>

#include <stdio.h>
#include <string.h>

#include "gl_debug_callback.h"

// Put breakpoint here!
static void GLOnError(void)
{}

#ifdef _WIN32
#define MY_STDCALL __stdcall
#else
#define MY_STDCALL
#endif

static const char* debugSource(GLenum source)
{
  switch (source) {
  case GL_DEBUG_SOURCE_API:
    return "API";
  case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
    return "WindowSystem";
  case GL_DEBUG_SOURCE_SHADER_COMPILER:
    return "ShaderCompiler";
  case GL_DEBUG_SOURCE_THIRD_PARTY:
    return "ThirdParty";
  case GL_DEBUG_SOURCE_APPLICATION:
    return "Application";
  case GL_DEBUG_SOURCE_OTHER:
    return "Other";
  }
  return "Unknown";
}

static const char* debugType(GLenum type)
{
  switch (type) {
  case GL_DEBUG_TYPE_ERROR_ARB:
    return "Error";
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
    return "DeprecatedBehavior";
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
    return "UndefinedBehavior";
  case GL_DEBUG_TYPE_PORTABILITY_ARB:
    return "Portability";
  case GL_DEBUG_TYPE_PERFORMANCE_ARB:
    return "Performance";
  case GL_DEBUG_TYPE_OTHER_ARB:
    return "Other";
  }
  return "Unknown";
}

static const char* debugSeverity(GLenum severity)
{
  switch (severity) {
  case GL_DEBUG_SEVERITY_HIGH_ARB:
    return "High";
  case GL_DEBUG_SEVERITY_MEDIUM_ARB:
    return "Medium";
  case GL_DEBUG_SEVERITY_LOW_ARB:
    return "Low";
  }
  return "Unknown";
}

static int ignore_debug_callbacks = 0;
static int severitySkipMask = 0x3;
static int sourceSkipMask = 0x1;
static int typeSkipMask = 0x1;

static void MY_STDCALL GLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
  if (ignore_debug_callbacks) {
    return;
  }
  if (severitySkipMask & 1) {
    if (severity == GL_DEBUG_SEVERITY_LOW) {
      return;
    }
  }
  if (severitySkipMask & 2) {
    if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
      return;
    }
  }
  if (sourceSkipMask & 1) {
    if (source == GL_DEBUG_SOURCE_OTHER) {
      return;
    }
  }
  if (typeSkipMask & 1) {
    if (type == GL_DEBUG_TYPE_OTHER) {
      return;
    }
  }
  printf("Debug callback:\n  source=%s (0x%x)\n  type=%s (0x%x)\n  id=%u\n  severity=%s (0x%x)\n",
    debugSource(source), source,
    debugType(type), type,
    id,
    debugSeverity(severity), severity);
  printf("  message: %s\n", message);
  GLOnError();
}

static int supportsMajorDotMinor(int needed_major, int needed_minor)
{
  const char *version;
  int major, minor;

  version = (char *) glGetString(GL_VERSION);
  if (sscanf(version, "%d.%d", &major, &minor) == 2) {
    return major > needed_major || (major == needed_major && minor >= needed_minor);
  }
  return 0;            /* OpenGL version string malformed! */
}

// A private implementation of glutExtensionSupported or glewIsSupported
static int isExtensionSupported(const char *extension)
{
    const GLubyte *extensions;
    const GLubyte *start;
    const GLubyte *where, *terminator;

    /* Extension names should not have spaces. */
    where = (GLubyte *) strchr(extension, ' ');
    if (where || *extension == '\0') {
        return 0;
    }

    extensions = glGetString(GL_EXTENSIONS);

    /* It takes a bit of care to be fool-proof about parsing the
       OpenGL extensions string.  Don't be fooled by sub-strings,
       etc. */
    start = extensions;
    for (;;) {
        /* If your application crashes in the strstr routine below,
           you are probably calling glutExtensionSupported without
           having a current window.  Calling glGetString without
           a current OpenGL context has unpredictable results.
           Please fix your program. */
        where = (const GLubyte *) strstr((const char *) start, extension);
        if (!where) {
            break;
        }
        terminator = where + strlen(extension);
        if (where == start || *(where - 1) == ' ') {
            if (*terminator == ' ' || *terminator == '\0') {
                return 1;
            }
        }
        start = terminator;
    }
    return 0;
}

void GLRegisterDebugCallback(void)
{
    // Check for KHR_debug functionality; either in OpenGL 4.3 or the extension.
    // https://www.opengl.org/registry/specs/KHR/debug.txt
    if (supportsMajorDotMinor(4,3) || isExtensionSupported("GL_KHR_debug")) {
        // Android warns without this cast.
        GLDEBUGPROC callback = (void*)GLDebugCallback;
        printf("Registering OpenGL error callback\n");
        glDebugMessageCallback(callback, NULL);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
        glEnable(GL_DEBUG_OUTPUT);
    } else {
        printf("Lacks core debug support; NOT registering debug callback\n");
    }
}

int GLIgnoreDebugCallback(int ignore)
{
    int prior_value = ignore_debug_callbacks;
    ignore_debug_callbacks = ignore;
    return prior_value;
}
