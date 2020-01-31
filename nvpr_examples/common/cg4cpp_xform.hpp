
// cg4cpp_xform.hpp - C++ transform routines for path rendering

#ifndef __cg4cpp_xform_hpp__
#define __cg4cpp_xform_hpp__

#include <GL/glew.h>

#include <Cg/double.hpp>
#include <Cg/vector.hpp>
#include <Cg/matrix.hpp>

extern Cg::float3x3 ortho(float l, float r, float b, float t);
extern Cg::float3x3 inverse_ortho(float l, float r, float b, float t);
extern Cg::double4x4 glortho(double left, double right, double bottom, double top, double znear, double zfar);
extern Cg::double4x4 glfrustum(double left, double right, double bottom, double top, double znear, double zfar);
extern Cg::double4x4 glperspective(double fovy, double aspect, double znear, double zfar);
extern Cg::float3x3 translate(float x, float y);
extern Cg::float3x3 translate(float x, float y, float z);
extern Cg::float3x3 translate(Cg::float2 xy);
extern Cg::float3x3 scale(float x, float y);
extern Cg::float3x3 scale(float x, float y, float z);
extern Cg::float3x3 scale(Cg::float2 xy);
extern Cg::float3x3 scale(Cg::float3 xyz);
extern Cg::float3x3 scale(float s);
extern Cg::float3x3 rotate(float degrees);
extern Cg::float4x4 glrotate(float degrees, Cg::float3 xyz);

extern Cg::float4x4 lookat(const Cg::float3 &eye, const Cg::float3 &at, const Cg::float3 &up);
extern Cg::double4x4 lookat(const Cg::double3 &eye, const Cg::double3 &at, const Cg::double3 &up);

extern Cg::float1x1 identity1x1();
extern Cg::float2x2 identity2x2();
extern Cg::float3x3 identity3x3();
extern Cg::float4x4 identity4x4();

extern Cg::double3x3 square2quad(const Cg::float2 v[4]);
extern Cg::double3x3 quad2square(const Cg::float2 v[4]);
extern Cg::double3x3 quad2quad(const Cg::float2 from[4], const Cg::float2 to[4]);
extern Cg::double3x3 box2quad(const Cg::float4 &box, const Cg::float2 to[4]);

extern void MatrixMultToGL(Cg::float3x3 m);
extern void MatrixLoadToGL(Cg::float3x3 m);

extern void MatrixMultToGL(GLenum matrix_mode, Cg::float3x3 m);
extern void MatrixLoadToGL(GLenum matrix_mode, Cg::float3x3 m);

extern void MatrixMultToGL(Cg::double4x4 m);
extern void MatrixLoadToGL(Cg::double4x4 m);

extern void MatrixMultToGL(GLenum matrix_mode, Cg::double4x4 m);
extern void MatrixLoadToGL(GLenum matrix_mode, Cg::double4x4 m);

#endif // __cg4cpp_xform_hpp__
