
/* renderer.cpp - generic renderer back-end implementation. */

// Copyright (c) NVIDIA Corporation. All rights reserved.

#include "renderer.hpp"

#include "showfps.h"

void GLBlitRenderer::swapBuffers()
{
    glutSwapBuffers();
}

void GLBlitRenderer::reportFPS()
{
    extern FPScontext gl_fps_context;
    double thisFPS = handleFPS(&gl_fps_context);
    thisFPS = thisFPS; // force used
}
