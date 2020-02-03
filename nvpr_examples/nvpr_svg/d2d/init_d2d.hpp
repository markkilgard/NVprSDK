
/* init_d2d.hpp - Direct2D initialization. */

// The idea is to support Direct2D without explicitly linking with D2D1.DLL and D3D10_1.DLL
// This is so a single binary can both run on Windows XP and also support Direct2D

#ifndef __init_d2d_hpp__
#define __init_d2d_hpp__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "nvpr_svg_config.h"  // configure path renderers to use

#if USE_D2D

// We don't want to depend on any actual Direct2D or Direct3D headers in this file

extern bool initDirect2D();

#endif // USE_D2D

#endif // __init_d2d_hpp__
