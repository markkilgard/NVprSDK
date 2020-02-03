
/* showfps.c - OpenGL code for rendering frames per second */

// Copyright (c) NVIDIA Corporation. All rights reserved.

#include <assert.h>

#include <GL/glew.h>

#ifdef _WIN32
#include <windows.h>  /* for QueryPerformanceCounter */
#endif

#if __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <stdio.h>
#include <string.h>
#include "showfps.h"

// Pre-defined constant image text_image containing characters "0123456789.fps-m";
// Enough characters for strings "fps" or "ms" and "--" and all decimal numbers
#include "fps_text_image.h"

static int reportFPS = 1;
static int displayMS = 0;
static GLfloat textColor[3] = { 1,1,0 };  // yellow
static int validFPS = 0;
static float scale = 1.0;
static FPSorigin origin = FPS_LOWER_RIGHT;

int highlight = 0;  // enable for better contrast with the background

void initFPScontext(FPScontext *ctx, FPSusage usage)
{
    ctx->usage = usage;
    ctx->width = 0;
    ctx->height = 0;
    ctx->count = 0;
    ctx->last_fpsRate = -666;  // bogus
    ctx->fps_text_texture = 0;
    if (usage == FPS_USAGE_TEXTURE) {
        glGenTextures(1, &ctx->fps_text_texture);
        assert(ctx->fps_text_texture);
        glBindTexture(GL_TEXTURE_2D, ctx->fps_text_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, text_image);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    glEnableClientState(GL_VERTEX_ARRAY);
}

void reshapeFPScontext(FPScontext *ctx, int w, int h)
{
    if (w != ctx->width || h != ctx->height) {
        validFPS = 0;
        ctx->width = w;
        ctx->height = h;
    }
}

void releaseFPScontext(FPScontext *ctx)
{
    if (ctx->fps_text_texture) {
        assert(ctx->usage == FPS_USAGE_TEXTURE);
        assert(glIsTexture(ctx->fps_text_texture));
        glDeleteTextures(1, &ctx->fps_text_texture);
    }
}

#define adv 9
static const float advance = adv/256.0f;

static void computeLocation(FPScontext *ctx, float *x, float *y)
{
    const int w = ctx->width, h = ctx->height;

    switch (origin) {
    case FPS_LOWER_RIGHT:
        *x = w-10*adv*scale;
        *y = 15;
        break;
    case FPS_LOWER_LEFT:
        *x = 15;
        *y = 15;
        break;
    case FPS_UPPER_RIGHT:
        *x = w-10*adv*scale;
        *y = h-15-16*scale;
        break;
    default:
        assert(!"bogus origin");
        // Fallthrough...
    case FPS_UPPER_LEFT:
        *x = 15;
        *y = h-15-16*scale;
        break;
    }
}

static void drawTexturedFPS(FPScontext *ctx, double fpsRate)
{
#if __APPLE__  // XXX to avoid assertion below
    glActiveTexture(GL_TEXTURE0);
#endif
#ifndef NDEBUG
    GLenum got_error;

    GLint active_texture = -666;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture);
    assert(active_texture == GL_TEXTURE0);

    got_error = glGetError();
    assert(got_error == GL_NO_ERROR);
#endif

    assert(!glIsEnabled(GL_DEPTH_TEST));
    //assert(!glIsEnabled(GL_STENCIL_TEST));
    //assert(!glIsEnabled(GL_BLEND));
    assert(!glIsEnabled(GL_SCISSOR_TEST));
    assert(!glIsEnabled(GL_ALPHA_TEST));
    assert(!glIsEnabled(GL_TEXTURE_1D));
    assert(!glIsEnabled(GL_TEXTURE_2D));
    assert(!glIsEnabled(GL_TEXTURE_3D));

    assert(!glIsEnabled(GL_TEXTURE_GEN_S));
    assert(!glIsEnabled(GL_TEXTURE_GEN_T));
    assert(!glIsEnabled(GL_TEXTURE_GEN_R));
    assert(!glIsEnabled(GL_TEXTURE_GEN_Q));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); {
        glLoadIdentity();
        glMatrixMode(GL_PROJECTION);
        glPushMatrix(); {
            const int w = ctx->width, h = ctx->height;

            assert(w > 0);
            assert(h > 0);
            glLoadIdentity();
            glOrtho(0, w, 0, h, -1, 1);

            glActiveTexture(GL_TEXTURE0);
            glEnable(GL_TEXTURE_2D);
            assert(glIsTexture(ctx->fps_text_texture));
            glBindTexture(GL_TEXTURE_2D, ctx->fps_text_texture);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glColor3fv(textColor);
            glDisable(GL_BLEND);
            glDisable(GL_STENCIL_TEST);
            glEnable(GL_ALPHA_TEST);
            glAlphaFunc(GL_GREATER, 0);

#define t2f(x,y) *v++ = x, *v++ = y
#define v2f(x,y) *v++ = x, *v++ = y, count++

            if (fpsRate != ctx->last_fpsRate || scale != ctx->last_scale) {
                char buffer[MAX_FPS_QUADS-1];
                GLfloat *v = ctx->varray;
                float x, y;
                int count = 0;
                int i;

                computeLocation(ctx, &x, &y);

                // Construct "ms" or "fps" quadrilaterals
                if (displayMS) {
                    // Milliseconds reports "ms"
                    // drawn as two textured rectangle
                    // one for "m", one for "s"
                    const int m_ndx = 15;
                    const int s_ndx = 13;

                    // "m"
                    t2f(m_ndx*advance, 0);
                    v2f(x,y);
                    t2f(m_ndx*advance+advance, 0);
                    v2f(x+adv*scale,y);
                    t2f(m_ndx*advance+advance, 1);
                    v2f(x+adv*scale,y+16*scale);
                    t2f(m_ndx*advance, 1);
                    v2f(x,y+16*scale);
                    x += adv*scale;
                    // "s"
                    t2f(s_ndx*advance, 0);
                    v2f(x,y);
                    t2f(s_ndx*advance+advance, 0);
                    v2f(x+adv*scale,y);
                    t2f(s_ndx*advance+advance, 1);
                    v2f(x+adv*scale,y+16*scale);
                    t2f(s_ndx*advance, 1);
                    v2f(x,y+16*scale);
                    x += adv*scale;
                    x += adv*scale;  // for space
                } else {
                    // Frames/second reports "fps"
                    // drawn as one textured rectangle
                    const int fps_start_ndx = 11;
                    const int fps_chars = sizeof("fps")-1;
                    t2f(fps_start_ndx*advance, 0);
                    v2f(x,y);
                    t2f(fps_start_ndx*advance+fps_chars*advance, 0);
                    v2f(x+fps_chars*adv*scale,y);
                    t2f(fps_start_ndx*advance+fps_chars*advance, 1);
                    v2f(x+fps_chars*adv*scale,y+16*scale);
                    t2f(fps_start_ndx*advance, 1);
                    v2f(x,y+16*scale);
                    x += (fps_chars+1/*for space*/)*adv*scale;
                }

                // Is the frames/second measurement valid?
                if (fpsRate > 0 || !validFPS) { 
                    // Yes, construct the string for it.
                    const char *c;

#ifdef _WIN32
#define snprintf _snprintf
#endif
                    double value = displayMS ? 1000.0/fpsRate : fpsRate;
                    // Is the frames/second fast, meaning more than 10 fps?
                    if (value > 10) {
                        // Yes, show less decimal precsion.
                        snprintf(buffer, sizeof(buffer), "%0.1f", value);
                    } else if (value > 1) {
                        // Show medium decimal precision.
                        snprintf(buffer, sizeof(buffer), "%0.2f", value);
                    } else {
                        // Show more decimal precision.
                        snprintf(buffer, sizeof(buffer), "%0.3f", value);
                    }
                    // For each character of the string...
                    for (c = buffer; *c; c++) {
                        const int is_digit = *c >= '0' && *c <= '9';
                        const int decimal_point_ndx = 10;  // 11th offset into texture.
                        const int ndx = is_digit ? *c-'0' : decimal_point_ndx;

                        // Generate quadrilateral for each digit or decimal point.
                        t2f(ndx*advance, 0);
                        v2f(x,y);
                        t2f(ndx*advance+advance, 0);
                        v2f(x+adv*scale,y);
                        t2f(ndx*advance+advance, 1);
                        v2f(x+adv*scale,y+16*scale);
                        t2f(ndx*advance, 1);
                        v2f(x,y+16*scale);

                        x += adv*scale;
                    }
                } else {
                    // No, setup "--" string to indicate indefinite fps for now.
                    for (i=0; i<2; i++) {
                        const int dash_ndx = 14;  // 15th offset into texture.
                        t2f(dash_ndx*advance, 0);
                        v2f(x,y);
                        t2f(dash_ndx*advance+advance, 0);
                        v2f(x+adv*scale,y);
                        t2f(dash_ndx*advance+advance, 1);
                        v2f(x+adv*scale,y+16*scale);
                        t2f(dash_ndx*advance, 1);
                        v2f(x,y+16*scale);
                        x += adv*scale;
                    }
                }
                ctx->last_fpsRate = fpsRate;
                ctx->last_scale = scale;
                ctx->count = count;
            }

            assert(ctx->count <= MAX_FPS_QUADS);
            assert((ctx->count % 4) == 0);
            assert(!glIsEnabled(GL_TEXTURE_COORD_ARRAY));
            assert(glIsEnabled(GL_VERTEX_ARRAY));
            assert(glGetError() == GL_NO_ERROR);
            //glEnableClientState(GL_VERTEX_ARRAY);
            glClientActiveTexture(GL_TEXTURE0);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glBindBuffer(GL_ARRAY_BUFFER, 0);  // source client-memory arrays
            glTexCoordPointer(2, GL_FLOAT, 4*sizeof(GLfloat), ctx->varray);
            glVertexPointer(2, GL_FLOAT, 4*sizeof(GLfloat), ctx->varray+2);
            glDrawArrays(GL_QUADS, 0, ctx->count);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            assert(glGetError() == GL_NO_ERROR);

            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_ALPHA_TEST);
        } glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    } glPopMatrix();
}

#ifdef _WIN32
#define snprintf _snprintf
#endif

static void setBitmapOrigin(int offset)
{
    GLubyte dummy;
    GLfloat x, y;

    switch (origin) {
    case FPS_LOWER_RIGHT:
        glRasterPos2f(1,1);
        x = -10*9;
        y = 15;
        break;
    case FPS_LOWER_LEFT:
        glRasterPos2f(0,1);
        x = 9;
        y = 15;
        break;
    case FPS_UPPER_RIGHT:
        glRasterPos2f(1,0);
        x = -10*9;
        y = -20;
        break;
    default:
        assert(!"bogus origin");
    case FPS_UPPER_LEFT:
        glRasterPos2f(0,0);
        x = 9;
        y = -20;
        break;
    }
    glBitmap(0, 0, 0, 0, x+offset, y+offset, &dummy);
}

static void drawBitmapFPS(double fpsRate)
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); {
        glLoadIdentity();
        glMatrixMode(GL_PROJECTION);
        glPushMatrix(); {
            char buffer[200], *c;

            glLoadIdentity();
            glOrtho(0, 1, 1, 0, -1, 1);
            //glDisable(GL_DEPTH_TEST);
            if (displayMS) {
                if (fpsRate > 0 || !validFPS) {
                    double milliseconds = 1000.0/fpsRate;
                    // Is the frames/second slow, meaning less than 1 fps?
                    if (milliseconds < 1) {
                        // Yes, show more decimal precision?
                        snprintf(buffer, sizeof(buffer), "ms %0.3f", milliseconds);
                    } else if (milliseconds < 10) {
                        // Yes, show more decimal precision?
                        snprintf(buffer, sizeof(buffer), "ms %0.2f", milliseconds);
                    } else {
                        snprintf(buffer, sizeof(buffer), "ms %0.1f", milliseconds);
                    }
                } else {
                    strcpy(buffer, "ms --");
                }
            } else {
                if (fpsRate > 0 || !validFPS) {
                    // Is the frames/second slow, meaning less than 1 fps?
                    if (fpsRate < 1) {
                        // Yes, show more decimal precision?
                        snprintf(buffer, sizeof(buffer), "fps %0.3f", fpsRate);
                    } else if (fpsRate < 10) {
                        // Yes, show more decimal precision?
                        snprintf(buffer, sizeof(buffer), "fps %0.2f", fpsRate);
                    } else {
                        snprintf(buffer, sizeof(buffer), "fps %0.1f", fpsRate);
                    }
                } else {
                    strcpy(buffer, "fps --");
                }
            }
            if (highlight) {
                int i;
                GLfloat altColor[3] = { textColor[0]-0.66f, textColor[1]-0.66f, textColor[2]-0.66 };
                for (i = 0; i<3; i++) {
                    if (altColor[i] < 0) {
                        altColor[i] = 1;
                    }
                }
                glColor3fv(altColor);
                // Draw text shifted lower-left by a pixel and upper-right by a pixel.
                for (i=-1; i<=1; i+=2) {
                    setBitmapOrigin(i);
                    for (c = buffer; *c != '\0'; c++) {
                        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *c);
                    }
                }
            }
            glColor3fv(textColor);
            setBitmapOrigin(0);
            for (c = buffer; *c != '\0'; c++) {
                glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *c);
            }

#if 0  // code used to generate fps_text_image.h
            {
                char *str = "0123456789.fps-m";
                const str_len = strlen(str);
                GLubyte pixels[16*256*4+1];
                GLubyte *p = pixels;
                int i, j;
                assert(str_len <= 16);
                glColor3f(0,0,0.1);
                for (i=-1; i<=1; i++) {
                    for (j=-1; j<=1; j++) {
                        glWindowPos2i(0,4);
                        glBitmap(0, 0, 0, 0, i, j, &dummy);
                        for (c = str; *c != '\0'; c++) {
                            glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *c);
                        }
                    }
                }
                glColor3f(1,1,0);
                glWindowPos2i(0,4);
                for (c = str; *c != '\0'; c++) {
                    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *c);
                }
                pixels[16*256*4] = 42;
                glReadPixels(0,0, 256, 16, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                assert(pixels[16*256*4] == 42);
                printf("GLubyte text_image[16*256*4] = {\n");
                for (i=0; i<16; i++) {
                    for (j=0; j<256; j++) {
                        printf(" 0x%x,0x%x,0x%x,0x%x,", p[0], p[1], p[2], p[3]);
                        p += 4;
                    }
                    printf("\n");
                }
                printf("};\n");
                assert(p == &pixels[16*256*4]);
            }
#endif
            //glEnable(GL_DEPTH_TEST);
        } glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    } glPopMatrix();
}

static void drawFPS(FPScontext *ctx, double fpsRate)
{
    if (ctx->usage == FPS_USAGE_TEXTURE) {
        drawTexturedFPS(ctx, fpsRate);
    } else {
        drawBitmapFPS(fpsRate);
    }
}

#ifdef _WIN32
static __int64 freq = 0;
#else
#include <sys/time.h> /* for gettimeofday and struct timeval */
#endif

double getElapsedTime()
{
  static int firstTime = 1;
  double secs;
#ifdef _WIN32
  /* Use Win32 performance counter for high-accuracy timing. */
  static __int64 startTime = 0;  /* Timer count for last fps update */
  __int64 newCount;

  if (!freq) {
    QueryPerformanceFrequency((LARGE_INTEGER*) &freq);
  }

  /* Update the frames per second count if we have gone past at least
     a second since the last update. */

  QueryPerformanceCounter((LARGE_INTEGER*) &newCount);
  if (firstTime) {
    startTime = newCount;
    firstTime = 0;
  }
  secs = (double) (newCount - startTime) / (double)freq;
#else
  /* Use BSD 4.2 gettimeofday system call for high-accuracy timing. */
  static struct timeval start_tp;
  struct timeval new_tp;
  
  gettimeofday(&new_tp, NULL);
  if (firstTime) {
    start_tp.tv_sec = new_tp.tv_sec;
    start_tp.tv_usec = new_tp.tv_usec;
    firstTime = 0;
  }
  secs = (new_tp.tv_sec - start_tp.tv_sec) + (new_tp.tv_usec - start_tp.tv_usec)/1000000.0;
#endif
  return secs;
}

static double lastFpsRate = 0;
static int frameCount = 0;     /* Number of frames for timing */
#ifdef _WIN32
/* Use Win32 performance counter for high-accuracy timing. */
static __int64 lastCount = 0;  /* Timer count for last fps update */
#else
static struct timeval last_tp = { 0, 0 };
#endif

void invalidateFPS()
{
    validFPS = 0;
}

void restartFPS()
{
    frameCount = 0;
    validFPS = 1;
    lastFpsRate = -1;
#ifdef _WIN32
    QueryPerformanceCounter((LARGE_INTEGER*) &lastCount);
#else
    gettimeofday(&last_tp, NULL);
#endif
}

double just_handleFPS(void)
{
#ifdef _WIN32
  /* Use Win32 performance counter for high-accuracy timing. */
  __int64 newCount;

  if (!freq) {
    QueryPerformanceFrequency((LARGE_INTEGER*) &freq);
  }

  /* Update the frames per second count if we have gone past at least
     a second since the last update. */

  QueryPerformanceCounter((LARGE_INTEGER*) &newCount);
  frameCount++;
  if ((newCount - lastCount) > freq) {
    double fpsRate;

    fpsRate = (double) (freq * (__int64) frameCount)  / (double) (newCount - lastCount);
    lastCount = newCount;
    frameCount = 0;
    lastFpsRate = fpsRate;
  }
#else
  /* Use BSD 4.2 gettimeofday system call for high-accuracy timing. */
  struct timeval new_tp;
  double secs;
  
  gettimeofday(&new_tp, NULL);
  secs = (new_tp.tv_sec - last_tp.tv_sec) + (new_tp.tv_usec - last_tp.tv_usec)/1000000.0;
  if (secs >= 1.0) {
    lastFpsRate = frameCount / secs;
    last_tp = new_tp;
    frameCount = 0;
  }
  frameCount++;
#endif
  if (!validFPS) {
    restartFPS();
  }
  return lastFpsRate;
}

double handleFPS(FPScontext *ctx)
{
  double lastFpsRate = just_handleFPS();
  if (reportFPS) {
    drawFPS(ctx, lastFpsRate);
  }
  return lastFpsRate;
}

void colorFPS(float r, float g, float b)
{
  textColor[0] = r;
  textColor[1] = g;
  textColor[2] = b;
}

void scaleFPS(float new_scale)
{
  scale = new_scale;
}

void toggleFPS(void)
{
  reportFPS = !reportFPS;
}

void toggleFPSunits(void)
{
  displayMS = !displayMS;
}

void reportFPSinMS(void)
{
  displayMS = 1;
}

int showingFPSinMS(void)
{
    return displayMS;
}

void reportFPSinFPS(void)
{
    displayMS = 0;
}

void enableFPS(void)
{
  reportFPS = 1;
}

void disableFPS(void)
{
  reportFPS = 0;
}

void setFPSorigin(FPSorigin new_origin)
{
    origin = new_origin;
}
