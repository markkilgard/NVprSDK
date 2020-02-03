
/* Copyright (c) Mark J. Kilgard, 1994, 2001. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "glutint.h"

/* strdup is actually not a standard ANSI C or POSIX routine
   so implement a private one for GLUT.  OpenVMS does not have a
   strdup; Linux's standard libc doesn't declare strdup by default
   (unless BSD or SVID interfaces are requested). */
char *
__glutStrdup(const char *string)
{
  char *copy;

  copy = (char*) malloc(strlen(string) + 1);
  if (copy == NULL) {
    return NULL;
  }
  strcpy(copy, string);
  return copy;
}

void
__glutWarning(char *format,...)
{
  va_list args;

  va_start(args, format);
  fprintf(stderr, "GLUT: Warning in %s: ",
    __glutProgramName ? __glutProgramName : "(unamed)");
  vfprintf(stderr, format, args);
  va_end(args);
  putc('\n', stderr);
}

/* CENTRY */
void GLUTAPIENTRY 
glutReportErrors(void)
{
  GLenum error;

  while ((error = glGetError()) != GL_NO_ERROR)
    __glutWarning("GL error: %s", gluErrorString(error));
}
/* ENDCENTRY */

#ifndef	NORETURN
# if defined(__clang__) || defined(__GNUC__)
   /* The `volatile' keyword tells GCC that a function never returns.  */
#  define NORETURN void __attribute__((noreturn))
# else /* Not GCC.  */
#  if defined(_MSC_VER) && _MSC_VER >= 1310
#   define NORETURN __declspec(noreturn) void
#  else
#   define NORETURN void
#  endif
# endif /* GCC.  */
#endif /* __NORETURN not defined.  */

NORETURN
__glutFatalError(char *format,...)
{
  va_list args;

  va_start(args, format);
  fprintf(stderr, "GLUT: Fatal Error in %s: ",
    __glutProgramName ? __glutProgramName : "(unamed)");
  vfprintf(stderr, format, args);
  va_end(args);
  putc('\n', stderr);
#ifdef _WIN32
  if (__glutExitFunc) {
    __glutExitFunc(1);
  }
#endif
  exit(1);
}

NORETURN
__glutFatalUsage(char *format,...)
{
  va_list args;

  va_start(args, format);
  fprintf(stderr, "GLUT: Fatal API Usage in %s: ",
    __glutProgramName ? __glutProgramName : "(unamed)");
  vfprintf(stderr, format, args);
  va_end(args);
  putc('\n', stderr);
  abort();
}
