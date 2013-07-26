
#include "gl_headers.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define MY_STDCALL __stdcall
#else
#define MY_STDCALL
#endif

static void MY_STDCALL GLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, void *userParam)
{
    if (severity == GL_DEBUG_SEVERITY_LOW) {
        return;
    }
    printf("Debug callback: source=0x%x, type=0x%x, id=%u, severity=0x%x\n", source, type, id, severity);
    printf("message: %s\n", message);
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

void GLRegisterDebugCallback()
{
    if (supportsMajorDotMinor(4,3)) {
        printf("Registering OpenGL error callback\n");
        glDebugMessageCallback(GLDebugCallback, NULL);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
        glEnable(GL_DEBUG_OUTPUT);
    } else {
        printf("Lacks core debug support; NOT registering debug callback\n");
    }
}
