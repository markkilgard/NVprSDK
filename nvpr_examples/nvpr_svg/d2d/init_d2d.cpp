
/* init_d2d.cpp - Direct2D initialization */

// Copyright (c) NVIDIA Corporation. All rights reserved.

// The idea is to support Direct2D without explicitly linking with D2D1.DLL and D3D10_1.DLL
// This is so a single binary can both run on Windows XP and also support Direct2D

#include "nvpr_svg_config.h"  // configure path renderers to use

#if USE_D2D

#include "init_d2d.hpp"
#include "init_d2d_private.hpp"

#include <stdio.h>
#include <windows.h>

static void *d2d_handle = NULL;
static HINSTANCE d2d_dll = NULL;
static HINSTANCE d3d_dll = NULL;

// Function variables for GetProcAddress to load
D2D1CreateFactoryFunc LazyD2D1CreateFactory = NULL;
PFN_D3D10_CREATE_DEVICE1 LazyD3D10CreateDevice1 = NULL;

static bool initialized = false;

bool initDirect2D()
{
    if (initialized) {
        return true;
    }
    if (!d3d_dll) {
        d3d_dll = LoadLibrary(TEXT("D3D10_1.DLL"));
    }
    if (!d2d_dll) {
        d2d_dll = LoadLibrary(TEXT("D2D1.DLL"));
    }
    if (!d3d_dll || !d3d_dll) {
        if (!d3d_dll) {
            printf("LoadLibrary of D3D10_1.DLL failed\n");
        }
        if (!d2d_dll) {
            printf("LoadLibrary of D2D1.DLL failed\n");
        }
        return false;
    }
    LazyD2D1CreateFactory = reinterpret_cast<D2D1CreateFactoryFunc>(GetProcAddress(d2d_dll,TEXT("D2D1CreateFactory")));;
    if (!LazyD2D1CreateFactory) {
        printf("GetProcAddress of D2D1CreateFactory from D2D1.DLL failed\n");
    }
    LazyD3D10CreateDevice1 = reinterpret_cast<PFN_D3D10_CREATE_DEVICE1>(GetProcAddress(d3d_dll,TEXT("D3D10CreateDevice1")));;
    if (!LazyD3D10CreateDevice1) {
        printf("GetProcAddress of D3D10CreateDevice1 from D3D10_1.DLL failed\n");
    }
    if (LazyD2D1CreateFactory && LazyD3D10CreateDevice1) {
        initialized = true;
    }
    return initialized;
}

#endif // USE_D2D
