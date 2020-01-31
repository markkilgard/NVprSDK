
// cg4cpp_xform.cpp - C++ transform routines for path rendering

// Copyright (c) NVIDIA Corporation. All rights reserved.

// Matrix convention: row major (C style arrays)

#define _USE_MATH_DEFINES
#include <math.h>

#include <GL/glew.h>

#include <Cg/double.hpp>
#include <Cg/vector/xyzw.hpp>
#include <Cg/matrix/1based.hpp>
#include <Cg/matrix/rows.hpp>
#include <Cg/vector.hpp>
#include <Cg/matrix.hpp>
#include <Cg/cross.hpp>
#include <Cg/dot.hpp>
#include <Cg/mul.hpp>
#include <Cg/normalize.hpp>
#include <Cg/radians.hpp>
#include <Cg/transpose.hpp>
#include <Cg/stdlib.hpp>
#include <Cg/iostream.hpp>

#include "cg4cpp_xform.hpp"

using namespace Cg;

float3x3 ortho(float l, float r, float b, float t)
{
  float3x3 rv = float3x3(2/(r-l),0,-(r+l)/(r-l),
                         0,2/(t-b),-(t+b)/(t-b),
                         0,0,1);
  return rv;
}

float3x3 inverse_ortho(float l, float r, float b, float t)
{
  float3x3 rv = float3x3((r-l)/2,0,(r+l)/2,
                         0,(t-b)/2,(t+b)/2,
                         0,0,1);
  return rv;
}

double4x4 glortho(double left, double right, double bottom, double top, double znear, double zfar)
{
  assert(right-left != 0);
  assert(top-bottom != 0);
  assert(zfar-znear != 0);
  double4x4 rv = double4x4(
      (right-left)/2, 0,              0,               -(right+left)/(right-left),
      0,              (top-bottom)/2, 0,               -(top+bottom)/(top-bottom),
      0,              0,              -(zfar-znear)/2, -(zfar+znear)/(zfar-znear),
      0,              0,              0,               1);
  return rv;
}

double4x4 glfrustum(double left, double right, double bottom, double top, double znear, double zfar)
{
  assert(right-left != 0);
  assert(top-bottom != 0);
  assert(zfar-znear != 0);
  double4x4 rv = double4x4(
      2*znear/(right-left), 0,                    0,                          (right+left)/(right-left),
      0,                    2*znear/(top-bottom), 0,                          (top+bottom)/(top-bottom),
      0,                    0,                    -1,                         -(zfar+znear)/(zfar-znear),
      0,                    0,                    -2*znear*zfar/(zfar-znear), 0);
  return rv;
}

double4x4 glperspective(double fovy, double aspect, double znear, double zfar)
{
  double deltaz = zfar - znear;

  float radians = float(fovy / 2 * M_PI / 180);
  float sine = sinf(radians);
  assert(deltaz != 0);
  assert(sine != 0);
  assert(aspect != 0);
  float cotangent = cosf(radians)/sine;

  double4x4 rv = double4x4(
    cotangent/aspect, 0, 0, 0,
    0, cotangent, 0, 0,
    0, 0, -(zfar+znear)/deltaz, -2*znear*zfar/deltaz, 
    0, 0, -1, 0);
  return rv;
}

float3x3 translate(float x, float y)
{
  float3x3 rv = float3x3(1,0,x,
                         0,1,y,
                         0,0,1);
  return rv;
}

float3x3 translate(float x, float y, float z)
{
  float3x3 rv = float3x3(1,0,x,
                         0,1,y,
                         0,0,z);
  return rv;
}

float3x3 translate(float2 xy)
{
  float3x3 rv = float3x3(1,0,xy.x,
                         0,1,xy.y,
                         0,0,1);
  return rv;
}


float3x3 scale(float x, float y)
{
  float3x3 rv = float3x3(x,0,0,
                         0,y,0,
                         0,0,1);
  return rv;
}

float3x3 scale(float x, float y, float z)
{
  float3x3 rv = float3x3(x,0,0,
                         0,y,0,
                         0,0,z);
  return rv;
}

float3x3 scale(float2 xy)
{
  float3x3 rv = float3x3(xy.x,0,0,
                         0,xy.y,0,
                         0,0,1);
  return rv;
}

float3x3 scale(float3 xyz)
{
  float3x3 rv = float3x3(xyz.x,0,0,
                         0,xyz.y,0,
                         0,0,xyz.z);
  return rv;
}

float3x3 scale(float s)
{
  float3x3 rv = float3x3(s,0,0,
                         0,s,0,
                         0,0,1);
  return rv;
}

float3x3 rotate(float angle)
{
  float radians = angle*3.14159f/180.0f,
        s = ::sin(radians),
        c = ::cos(radians);
  float3x3 rv = float3x3(c,-s,0,
                         s,c,0,
                         0,0,1);
  return rv;
}

float4x4 glrotate(float degrees, float3 xyz)
{
    float3 axis = normalize(xyz);
    float rads = radians(degrees);
    float sine   = ::sin(rads),
          cosine = ::cos(rads);
    float3 abc = axis * axis.yzx * (1-cosine);
    float3 t = axis*axis;
    float4x4 m = float4x4(t.x + cosine*(1-t.x), abc.x - axis.z*sine,  abc.z + axis.y*sine,  0,
                          abc.x + axis.z*sine,  t.y + cosine*(1-t.y), abc.y - axis.x*sine,  0,
                          abc.z + axis.y*sine,  abc.y + axis.x*sine,  t.z + cosine*(1-t.z), 0,
                          0,                    0,                    0,                    1);
    return m;
}

template <typename T>
__CGmatrix<T,4,4> lookat(const __CGvector<T,3> &eye,
                         const __CGvector<T,3> &at,
                         const __CGvector<T,3> &up)
{
    typedef __CGvector<T,4> vec4;
    typedef __CGvector<T,3> vec3;

    vec3 forward = normalize(at - eye),
         side    = normalize(cross(forward, up)),
         new_up  = cross(side, forward);

    __CGmatrix<T,4,4> rv;
    rv[0] = vec4( side,    -dot(eye,side));
    rv[1] = vec4( new_up,  -dot(eye,new_up));
    rv[2] = vec4(-forward,  dot(eye,forward));
    rv[3] = vec4(0,0,0,     1);
    return rv;
}

float4x4 lookat(const float3 &eye, const float3 &at, const float3 &up)
{
    return lookat<float>(eye, at, up);
}

double4x4 lookat(const double3 &eye, const double3 &at, const double3 &up)
{
    return lookat<double>(eye, at, up);
}

float1x1 identity1x1()
{
    return float1x1(1);
}

float2x2 identity2x2()
{
    return float2x2(1,0,
                    0,1);
}

float3x3 identity3x3()
{
    return float3x3(1,0,0,
                    0,1,0,
                    0,0,1);
}

float4x4 identity4x4()
{
    return float4x4(1,0,0,0,
                    0,1,0,0,
                    0,0,1,0,
                    0,0,0,1);
}

// Math from page 54-56 of "Digital Image Warping" by George Wolberg,
// though credited to Paul Heckert's "Fundamentals of Texture
// Mapping and Image Warping" 1989 Master's thesis.
//
// NOTE: This matrix assumes vectors are treated as rows so pre-multiplied
// by the matrix; use the transpose when transforming OpenGL-style column
// vectors.
double3x3 square2quad(const float2 v[4])
{
  double3x3 a;

  double2 d1 = double2(v[1]-v[2]),
          d2 = double2(v[3]-v[2]),
          d3 = double2(v[0]-v[1]+v[2]-v[3]);

  double denom = d1.x*d2.y - d2.x*d1.y;
  a._13 = (d3.x*d2.y - d2.x*d3.y) / denom;
  a._23 = (d1.x*d3.y - d3.x*d1.y) / denom;
  a._11_12 = v[1] - v[0] + a._13*v[1];
  a._21_22 = v[3] - v[0] + a._23*v[3];
  a._31_32 = v[0];
  a._33 = 1;

  return transpose(a);
}

double3x3 quad2square(const float2 v[4])
{
  return inverse(square2quad(v));
}

double3x3 quad2quad(const float2 from[4], const float2 to[4])
{
  return mul(square2quad(to), quad2square(from));
}

double3x3 box2quad(const float4 &box, const float2 to[4])
{
  float2 from[4] = { box.xy, box.zy, box.zw, box.xw };

  return quad2quad(from, to);
}

static void float3x3_to_GLMatrix(GLfloat mm[16], const float3x3 &m)
{
  /* First column. */
  mm[0] = m[0][0];
  mm[1] = m[1][0];
  mm[2] = 0;
  mm[3] = m[2][0];

  /* Second column. */
  mm[4] = m[0][1];
  mm[5] = m[1][1];
  mm[6] = 0;
  mm[7] = m[2][1];;

  /* Third column. */
  mm[8] = 0;
  mm[9] = 0;
  mm[10] = 1;
  mm[11] = 0;

  /* Fourth column. */
  mm[12] = m[0][2];
  mm[13] = m[1][2];
  mm[14] = 0;
  mm[15] = m[2][2];;
}

void MatrixMultToGL(float3x3 m)
{
  GLfloat mm[16];  /* Column-major OpenGL-style 4x4 matrix. */

  float3x3_to_GLMatrix(mm, m);
  glMatrixMultfEXT(GL_MODELVIEW, &mm[0]);
}

void MatrixLoadToGL(float3x3 m)
{
  GLfloat mm[16];  /* Column-major OpenGL-style 4x4 matrix. */

  float3x3_to_GLMatrix(mm, m);
  glMatrixLoadfEXT(GL_MODELVIEW, &mm[0]);
}

void MatrixMultToGL(GLenum matrix_mode, float3x3 m)
{
  GLfloat mm[16];  /* Column-major OpenGL-style 4x4 matrix. */

  float3x3_to_GLMatrix(mm, m);
  glMatrixMultfEXT(matrix_mode, &mm[0]);
}

void MatrixLoadToGL(GLenum matrix_mode, float3x3 m)
{
  GLfloat mm[16];  /* Column-major OpenGL-style 4x4 matrix. */

  float3x3_to_GLMatrix(mm, m);
  glMatrixLoadfEXT(matrix_mode, &mm[0]);
}

static void double4x4_to_GLMatrix(GLdouble mm[16], const double4x4 &m)
{
  /* First column. */
  mm[0] = m[0][0];
  mm[1] = m[1][0];
  mm[2] = m[2][0];
  mm[3] = m[3][0];

  /* Second column. */
  mm[4] = m[0][1];
  mm[5] = m[1][1];
  mm[6] = m[2][1];
  mm[7] = m[3][1];

  /* Third column. */
  mm[8] = m[0][2];
  mm[9] = m[1][2];
  mm[10] = m[2][2];
  mm[11] = m[3][2];

  /* Fourth column. */
  mm[12] = m[0][3];
  mm[13] = m[1][3];
  mm[14] = m[2][3];
  mm[15] = m[3][3];
}

void MatrixMultToGL(double4x4 m)
{
  GLdouble mm[16];

  double4x4_to_GLMatrix(mm, m);
  glMatrixMultdEXT(GL_MODELVIEW, &mm[0]);
}

void MatrixLoadToGL(double4x4 m)
{
  GLdouble mm[16];  /* Column-major OpenGL-style 4x4 matrix. */

  double4x4_to_GLMatrix(mm, m);
  glMatrixLoaddEXT(GL_MODELVIEW, &mm[0]);
}

void MatrixMultToGL(GLenum matrix_mode, double4x4 m)
{
  GLdouble mm[16];

  double4x4_to_GLMatrix(mm, m);
  glMatrixMultdEXT(matrix_mode, &mm[0]);
}

void MatrixLoadToGL(GLenum matrix_mode, double4x4 m)
{
  GLdouble mm[16];  /* Column-major OpenGL-style 4x4 matrix. */

  double4x4_to_GLMatrix(mm, m);
  glMatrixLoaddEXT(matrix_mode, &mm[0]);
}

