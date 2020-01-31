
/* tiger.c - draw classic PostScript tiger */

// Copyright (c) NVIDIA Corporation. All rights reserved.

#include <string.h>
#include <math.h>

#include <GL/glew.h>
//#include "nvpr_init.h"

static const unsigned int tiger_path_count = 240;
const char *tiger_path[240] = {
#include "tiger_paths.h"
};
static const struct TigerStyle {
  GLuint fill_color;
  GLuint stroke_color;
  GLfloat stroke_width;
} tiger_style[240] = {
#include "tiger_style.h"
};
static GLuint tiger_path_base;

static int use_dlist = 1;

void initTiger()
{
  unsigned int i;

  tiger_path_base = glGenPathsNV(tiger_path_count);

  for (i=0; i<tiger_path_count; i++) {
    const char *svg_str = tiger_path[i];
    size_t svg_len = strlen(tiger_path[i]);
    GLfloat stroke_width = (GLfloat) fabs(tiger_style[i].stroke_width);

    glPathStringNV(tiger_path_base+i, GL_PATH_FORMAT_SVG_NV,
      (GLsizei)svg_len, svg_str);
    glPathParameterfNV(tiger_path_base+i, GL_PATH_STROKE_WIDTH_NV, stroke_width);
  }
  use_dlist = 0;
}

static void sendColor(GLuint color)
{
  GLubyte red = (color>> 16)&0xFF,
          green = (color >> 8)&0xFF,
          blue  = (color >> 0)&0xFF;
  glColor3ub(red, green, blue);
}

typedef void (*SendColorFunc)(GLuint color);

static SendColorFunc sendColorFunc = sendColor;

void setTigerSendColorFunc(SendColorFunc func)
{
  sendColorFunc = func;
}

static void drawTigerLayer(int layer, int filling, int stroking)
{
  const struct TigerStyle *style = &tiger_style[layer];
  GLuint fill_color = style->fill_color;
  GLuint stroke_color = style->stroke_color;
  GLfloat stroke_width = style->stroke_width;

  // Should this path be filled?
  if (filling && stroke_width >= 0) {
    sendColorFunc(fill_color);
    glStencilThenCoverFillPathNV(tiger_path_base+layer, GL_COUNT_UP_NV, 0x1F, GL_BOUNDING_BOX_NV);
  } else {
    // Negative stroke_width means "stroke only" (no fill)
  }

  // Should this path be stroked?
  if (stroking && stroke_width != 0) {
    const GLint reference = 0x1;
    sendColorFunc(stroke_color);
    glStencilThenCoverStrokePathNV(tiger_path_base+layer, reference, 0x1F, GL_BOUNDING_BOX_NV);
  } else {
    // Zero stroke widths means "fill only" (no stroke)
  }
}

void drawTiger(int filling, int stroking)
{
  unsigned int i;

  for (i=0; i<tiger_path_count; i++) {
    drawTigerLayer(i, filling, stroking);
  }
}

void drawTigerRange(int filling, int stroking, unsigned int start, unsigned int end)
{
  unsigned int i;

  if (end > tiger_path_count) {
    end = tiger_path_count;
  }

  for (i=start; i<end; i++) {
    drawTigerLayer(i, filling, stroking);
  }
}

void tigerDlistUsage(int b)
{
  use_dlist = b;
}

void renderTiger(int filling, int stroking)
{
  if (use_dlist) {
    static int beenhere = 0;
    int dlist = !!filling*2 + !!stroking + 1;
    if (!beenhere) {
      int i;
      for (i=0; i<4; i++) {
        glNewList(i+1, GL_COMPILE); {
          int filling = !!(i&2);
          int stroking = !!(i&1);
          drawTiger(filling, stroking);
        } glEndList();
      }
      beenhere = 1;
    }
    glCallList(dlist);
  } else {
    drawTiger(filling, stroking);
  }
}

GLuint getTigerBasePath()
{
  return tiger_path_base;
}

unsigned int getTigerPathCount()
{
  return tiger_path_count;
}

void getTigerBounds(GLfloat bounds[4], int filling, int stroking)
{
  unsigned int i;

  for (i=0; i<tiger_path_count; i++) {
    const struct TigerStyle *style = &tiger_style[i];
    GLfloat stroke_width = style->stroke_width;

    GLfloat layer_bounds[4] = { 0, 0, 0, 0 };
    if (stroking && stroke_width != 0) {
      glGetPathParameterfvNV(tiger_path_base+i, GL_PATH_OBJECT_BOUNDING_BOX_NV, layer_bounds);
      layer_bounds[0] -= stroke_width;
      layer_bounds[1] -= stroke_width;
      layer_bounds[2] += stroke_width;
      layer_bounds[3] += stroke_width;
    } else if (filling && stroke_width >= 0) {
      glGetPathParameterfvNV(tiger_path_base+i, GL_PATH_OBJECT_BOUNDING_BOX_NV, layer_bounds);
    }
    if (i==0) {
      bounds[0] = layer_bounds[0];
      bounds[1] = layer_bounds[1];
      bounds[2] = layer_bounds[2];
      bounds[3] = layer_bounds[3];
    } else {
      if (layer_bounds[0] < bounds[0])
        bounds[0] = layer_bounds[0];
      if (layer_bounds[1] < bounds[1])
        bounds[1] = layer_bounds[1];
      if (layer_bounds[2] > bounds[2])
        bounds[2] = layer_bounds[2];
      if (layer_bounds[3] > bounds[3])
        bounds[3] = layer_bounds[3];
    }
  }
}