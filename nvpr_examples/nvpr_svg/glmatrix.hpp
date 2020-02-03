
/* glmatrix.hpp - prototypes for OpenGL-style cg4cpp matrix utilities */

// Copyright (c) NVIDIA Corporation. All rights reserved.

#ifndef __glmatrix_hpp__
#define __glmatrix_hpp__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <Cg/double.hpp>
#include <Cg/vector.hpp>
#include <Cg/matrix.hpp>

namespace Cg {

template <typename T>
__CGmatrix<T,2,2> rotate2x2(const __CGvector<T,1> & angle);
float2x2 rotate2x2(float1 angle);
double2x2 rotate2x2(double1 angle);
template <typename T>
__CGmatrix<T,3,3> rotate3x3(const __CGvector<T,1> & angle, const __CGvector<T,3> & v);
float3x3 rotate3x3(float1 angle, float3 vector);
double3x3 rotate3x3(double1 angle, double3 vector);
float4x4 rotate4x4(float1 angle, float3 vector);
double4x4 rotate4x4(double1 angle, double3 vector);
template <typename T>
__CGmatrix<T,3,3> scale3x3(const __CGvector<T,3> & v);
float4x4 scale4x4(float2 vector);
float4x4 scale4x4(float3 vector);
double4x4 scale4x4(double3 vector);
template <typename T>
__CGmatrix<T,4,4> translate4x4(const __CGvector<T,3> & v);
float4x4 translate4x4(float2 vector);
float4x4 translate4x4(float3 vector);
inline float4x4 translate4x4(float x, float y) { return translate4x4(float2(x,y)); }
inline float4x4 translate4x4(float x, float y, float z) { return translate4x4(float3(x,y,z)); }

float4x4 ortho(float left, float right, float bottom, float top, float znear, float zfar);
double4x4 ortho(double left, double right, double bottom, double top, double znear, double zfar);

float4x4 ortho2D(float left, float right, float bottom, float top);
double4x4 ortho2D(double left, double right, double bottom, double top);

float4x4 frustum(float fov, float aspect_ratio, float znear, float zfar);
double4x4 frustum(double fov, double aspect_ratio, double znear, double zfar);

float4x4 perspective(float fov, float aspect_ratio, float znear, float zfar);
double4x4 perspective(double fov, double aspect_ratio, double znear, double zfar);

float4x4 lookat(const float3 &eye, const float3 &at, const float3 &up);
double4x4 lookat(const double3 &eye, const double3 &at, const double3 &up);

float1x1 identity1x1();
float2x2 identity2x2();
float3x3 identity3x3();
float4x4 identity4x4();

double3x3 square2quad(const float2 v[4]);
double3x3 quad2square(const float2 v[4]);
double3x3 quad2quad(const float2 from[4], const float2 to[4]);
double3x3 box2quad(const float4 &box, const float2 to[4]);

float4x4 make_float4x4(const double3x3 &m);
float4x4 make_float4x4(const double2x2 &m);

} // namespace Cg

#endif // __glmatrix_hpp__
