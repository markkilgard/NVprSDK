
/* init_d2d_private.hpp - private functions resolved during Direct2D initialization */

#ifndef __init_d2d_private_hpp__
#define __init_d2d_private_hpp__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "nvpr_svg_config.h"  // configure path renderers to use

#if USE_D2D

// Direct2D graphics library
#include <d2d1.h>     // Download the DirectX SDK from http://msdn.microsoft.com/en-us/directx/
#include <d3d10_1.h>  // Direct3D 10.1

typedef HRESULT (WINAPI *D2D1CreateFactoryFunc)(D2D1_FACTORY_TYPE, REFIID,
                                                const D2D1_FACTORY_OPTIONS *,
                                                void **);

typedef void (WINAPI *D2D1MakeRotateMatrixFunc)(__in FLOAT angle,
                                                __in D2D1_POINT_2F center,
                                                __out D2D1_MATRIX_3X2_F *matrix);

// Success of initDirect2D initializes these function pointers
extern D2D1CreateFactoryFunc LazyD2D1CreateFactory;
extern D2D1MakeRotateMatrixFunc LazyD2D1MakeRotateMatrix;
extern PFN_D3D10_CREATE_DEVICE1 LazyD3D10CreateDevice1;

#endif // USE_D2D

#endif // __init_d2d_private_hpp__
