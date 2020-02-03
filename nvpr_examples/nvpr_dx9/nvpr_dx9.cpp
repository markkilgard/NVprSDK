
/* nvpr_dx9.c - "Getting Started with NV_path_rendering" dx9 example */

// Copyright (c) NVIDIA Corporation. All rights reserved.

/* This example requires the OpenGL Utility Toolkit (GLUT) and an OpenGL driver
   supporting the NV_path_rendering extension for GPU-accelerated path rendering.
   NVIDIA's latest Release 275 drivers support this extension on GeForce 8 and
   later GPU generations. */

#include <assert.h>
#include <windows.h>
#include <stdio.h>
#include <d3d9.h>     /* Can't include this?  Is DirectX SDK installed? */
#include <D3dx9math.h>     /* Can't include this?  Is DirectX SDK installed? */
#include <GL/glew.h>
#include <GL/wglew.h>

#include "DXUT.h"  /* DirectX Utility Toolkit (part of the DirectX SDK) */

#include "tiger.h"

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "opengl32.lib")

// NV_path_rendering code lifted straight from nvpr_basic.c...

GLuint pathObj = 42;
int path_specification_mode = 0;
int filling = 1;
int stroking = 1;
int even_odd = 0;

void
initPathFromSVG()
{
    /* Here is an example of specifying and then rendering a five-point
    star and a heart as a path using Scalable Vector Graphics (SVG)
    path description syntax: */
    
    const char *svgPathString =
      // star
      "M100,180 L40,10 L190,120 L10,120 L160,10 z"
      // heart
      "M300 300 C 100 400,100 200,300 100,500 200,500 400,300 300Z";
    glPathStringNV(pathObj, GL_PATH_FORMAT_SVG_NV,
                   (GLsizei)strlen(svgPathString), svgPathString);
}

void
initPathFromPS()
{
    /* Alternatively applications oriented around the PostScript imaging
    model can use the PostScript user path syntax instead: */

    const char *psPathString =
      // star
      "100 180 moveto"
      " 40 10 lineto 190 120 lineto 10 120 lineto 160 10 lineto closepath"
      // heart
      " 300 300 moveto"
      " 100 400 100 200 300 100 curveto"
      " 500 200 500 400 300 300 curveto closepath";
    glPathStringNV(pathObj, GL_PATH_FORMAT_PS_NV,
                   (GLsizei)strlen(psPathString), psPathString);
}

void
initPathFromData()
{
    /* The PostScript path syntax also supports compact and precise binary
    encoding and includes PostScript-style circular arcs.

    Or the path's command and coordinates can be specified explicitly: */

    static const GLubyte pathCommands[10] =
      { GL_MOVE_TO_NV, GL_LINE_TO_NV, GL_LINE_TO_NV, GL_LINE_TO_NV,
        GL_LINE_TO_NV, GL_CLOSE_PATH_NV,
        'M', 'C', 'C', 'Z' };  // character aliases
    static const GLshort pathCoords[12][2] =
      { {100, 180}, {40, 10}, {190, 120}, {10, 120}, {160, 10},
        {300,300}, {100,400}, {100,200}, {300,100},
        {500,200}, {500,400}, {300,300} };
    glPathCommandsNV(pathObj, 10, pathCommands, 24, GL_SHORT, pathCoords);
}

void initGraphics()
{
    switch (path_specification_mode) {
    case 0:
        printf("specifying path via SVG string\n");
        initPathFromSVG();
        break;
    case 1:
        printf("specifying path via PS string\n");
        initPathFromPS();
        break;
    case 2:
        printf("specifying path via explicit data\n");
        initPathFromData();
        break;
    }

    /* Before rendering, configure the path object with desirable path
    parameters for stroking.  Specify a wider 6.5-unit stroke and
    the round join style: */

    glPathParameteriNV(pathObj, GL_PATH_JOIN_STYLE_NV, GL_ROUND_NV);
    glPathParameterfNV(pathObj, GL_PATH_STROKE_WIDTH_NV, 6.5);
}

void
doGraphics(void)
{
    /* Before rendering to a window with a stencil buffer, clear the stencil
    buffer to zero and the color buffer to black: */

    glClearStencil(0);
    glClearColor(0,0,0,0);
    glStencilMask(~0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    /* Use an orthographic path-to-clip-space transform to map the
    [0..500]x[0..400] range of the star's path coordinates to the [-1..1]
    clip space cube: */

    glMatrixLoadIdentityEXT(GL_PROJECTION);
    glMatrixLoadIdentityEXT(GL_MODELVIEW);
    // Direct3D has an upper-left origin so 
    // Flip Y axis from nvpr_basic, hence "400, 0"
    glMatrixOrthoEXT(GL_MODELVIEW, 0, 500, 400, 0, -1, 1);  

    if (filling) {

        /* Stencil the path: */

        glStencilFillPathNV(pathObj, GL_COUNT_UP_NV, 0x1F);

        /* The 0x1F mask means the counting uses modulo-32 arithmetic. In
        principle the star's path is simple enough (having a maximum winding
        number of 2) that modulo-4 arithmetic would be sufficient so the mask
        could be 0x3.  Or a mask of all 1's (~0) could be used to count with
        all available stencil bits.

        Now that the coverage of the star and the heart have been rasterized
        into the stencil buffer, cover the path with a non-zero fill style
        (indicated by the GL_NOTEQUAL stencil function with a zero reference
        value): */

        glEnable(GL_STENCIL_TEST);
        if (even_odd) {
            glStencilFunc(GL_NOTEQUAL, 0, 0x1);
        } else {
            glStencilFunc(GL_NOTEQUAL, 0, 0x1F);
        }
        glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
        glColor3f(0,1,0); // green
        glCoverFillPathNV(pathObj, GL_BOUNDING_BOX_NV);

    }

    /* The result is a yellow star (with a filled center) to the left of
    a yellow heart.

    The GL_ZERO stencil operation ensures that any covered samples
    (meaning those with non-zero stencil values) are zero'ed when
    the path cover is rasterized. This allows subsequent paths to be
    rendered without clearing the stencil buffer again.

    A similar two-step rendering process can draw a white outline
    over the star and heart. */

     /* Now stencil the path's stroked coverage into the stencil buffer,
     setting the stencil to 0x1 for all stencil samples within the
     transformed path. */

    if (stroking) {

        glStencilStrokePathNV(pathObj, 0x1, ~0);

         /* Cover the path's stroked coverage (with a hull this time instead
         of a bounding box; the choice doesn't really matter here) while
         stencil testing that writes white to the color buffer and again
         zero the stencil buffer. */

        glColor3f(1,1,0); // yellow
        glCoverStrokePathNV(pathObj, GL_CONVEX_HULL_NV);

         /* In this example, constant color shading is used but the application
         can specify their own arbitrary shading and/or blending operations,
         whether with Cg compiled to fragment program assembly, GLSL, or
         fixed-function fragment processing.

         More complex path rendering is possible such as clipping one path to
         another arbitrary path.  This is because stencil testing (as well
         as depth testing, depth bound test, clip planes, and scissoring)
         can restrict path stenciling. */
    }
}

// DXUT code...

/* Forward declared DXUT callbacks registered by WinMain. */
static HRESULT CALLBACK OnResetDevice(IDirect3DDevice9*, const D3DSURFACE_DESC*, void*);
static void CALLBACK OnFrameRender(IDirect3DDevice9*, double, float, void*);
static void CALLBACK OnLostDevice(void*);

#define APP_NAME "nvpr_dx9"

INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
  DXUTSetCallbackDeviceReset(OnResetDevice);
  DXUTSetCallbackDeviceLost(OnLostDevice);
  DXUTSetCallbackFrameRender(OnFrameRender);

  /* Parse  command line, handle  default hotkeys, and show messages. */
  DXUTInit();

  DXUTCreateWindow(L"nvpr_dx9 - NV_path_rendering into DirectX 9 window");

  // Jump through hoops to make sure we have the right device settings
  // DXUTCreateDevice is insufficient since it won't provide stencil
  // So use DXUTFindValidDeviceSettings/DXUTCreateDeviceFromSettings instead

  DXUTDeviceSettings in_deviceSettings;
  DXUTDeviceSettings out_deviceSettings;
  DXUTMatchOptions matchOptions;
  ZeroMemory( &in_deviceSettings, sizeof(in_deviceSettings) ); 
  ZeroMemory( &out_deviceSettings, sizeof(out_deviceSettings) ); 
  ZeroMemory( &matchOptions, sizeof(matchOptions) ); 

  // Request depth-stencil explicitly
  in_deviceSettings.pp.AutoDepthStencilFormat = D3DFMT_D24S8;
  matchOptions.eStencilFormat = DXUTMT_PRESERVE_INPUT;

  // We always want an (NVIDIA) HAL device
  in_deviceSettings.DeviceType = D3DDEVTYPE_HAL;
  matchOptions.eDeviceType = DXUTMT_PRESERVE_INPUT;

#if 0
  in_deviceSettings.pp.SwapEffect =  D3DSWAPEFFECT_COPY;
  matchOptions.eSwapEffect = DXUTMT_PRESERVE_INPUT;
#endif

#if 0
  // XXX need to figure out antialiasing still
  matchOptions.eMultiSample = DXUTMT_PRESERVE_INPUT;
#endif

  in_deviceSettings.pp.BackBufferWidth = 640;
  in_deviceSettings.pp.BackBufferHeight = 480;
  matchOptions.eResolution = DXUTMT_PRESERVE_INPUT;

  // XXX Triple buffering doesn't work with interop.
  // So force a single back buffer (double buffering).
  in_deviceSettings.pp.BackBufferCount = 1;
  matchOptions.eBackBufferCount = DXUTMT_PRESERVE_INPUT;

  HRESULT result = DXUTFindValidDeviceSettings( &out_deviceSettings, &in_deviceSettings, &matchOptions );
  assert(result == S_OK);
  DXUTCreateDeviceFromSettings(&out_deviceSettings );

  DXUTMainLoop();

  return DXUTGetExitCode();
}

HGLRC hGLRC;

void DbgPrintf(const char * fmt,...    )
{
    va_list marker;
    char szBuf[256];
 
    va_start(marker, fmt);
    wvsprintfA(szBuf, fmt, marker);
    va_end(marker);
 
    OutputDebugStringA(szBuf);
}

HANDLE handleD3D;
HANDLE handleD3DColorBuffer;
HANDLE handleD3DDepthBuffer;

GLuint nameColorBuffer = 0;
GLuint nameDepthBuffer = 0;
GLuint fbo = 0;

static HRESULT CALLBACK OnResetDevice(IDirect3DDevice9* pDev, 
                                      const D3DSURFACE_DESC* backBuf,
                                      void* userContext)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    static bool classRegistered = false;

    if (!classRegistered) {

        /* Clear and then fill in the window class structure. */
        WNDCLASSA  wc;
        memset(&wc, 0, sizeof(WNDCLASSA));
        wc.style         = CS_OWNDC;
        wc.lpfnWndProc   = DefWindowProc;
        wc.hInstance     = hInstance;
        wc.hIcon         = LoadIconA(hInstance, "GLUT_ICON");
        wc.hCursor       = LoadCursor(hInstance, IDC_ARROW);
        wc.hbrBackground = NULL;
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = "FAKE_GLUT";  /* Poor name choice. */

        /* Fill in a default icon if one isn't specified as a resource. */
        if(!wc.hIcon) {
            wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
        }

        /* Register fake window class. */
        if(RegisterClassA(&wc) == 0) {
            DWORD err = GetLastError();
            DbgPrintf("RegisterClassA failed: "
                "Cannot register fake GLUT window class.");
            MessageBoxA(NULL,
                "RegisterClassA failed\n"
                "Cannot register fake GLUT window class\n"
                "Exiting...", APP_NAME, MB_OK);
            exit(1);
            return false;
        }
        classRegistered = true;
    }

    // create the window
    HWND hWnd = CreateWindowA("FAKE_GLUT",                  // lpClassName
                        "GLUT",   // lpWindowName
                        WS_OVERLAPPEDWINDOW,    // dwStyle
                        10,                      // x
                        10,                      // y
                        10,                     // nWidth
                        10,                     // nHeight
                        NULL,     // hWndParent
                        NULL,                   // hMenu
                        hInstance,              // hInstance
                        NULL);                  // lpParam

    if (hWnd == NULL) {
        DWORD err = GetLastError();
        printf("CreateWindow failed\n");
        MessageBoxA(NULL,
            "CreateWindow failed\n"
            "Exiting...", APP_NAME, MB_OK);
        exit(1);
        return false;
    }

    // grab the device context for the window
    HDC hDC = GetDC(hWnd);

    // set an appropriate pixel format

    int pxFormat;
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof (PIXELFORMATDESCRIPTOR),                    // Size Of This Pixel Format Descriptor
        1,                                                // Version Number
        PFD_DRAW_TO_WINDOW |                            // Format Must Support Window
        PFD_SUPPORT_OPENGL |                            // Format Must Support OpenGL
        PFD_DOUBLEBUFFER,                                // Must Support Double Buffering
        PFD_TYPE_RGBA,                                    // Request An RGBA Format
        24,                                                // Select Our Color Depth
        0, 0, 0, 0, 0, 0,                                // Color Bits Ignored
        1,                                                // Alpha Buffer
        0,                                                // Shift Bit Ignored
        0,                                                // No Accumulation Buffer
        0, 0, 0, 0,                                        // Accumulation Bits Ignored
        24,                                                // 24 Bit Z-Buffer (Depth Buffer)  
        8,                                                // 8 Bit Stencil Buffer
        0,                                                // No Auxiliary Buffer
        PFD_MAIN_PLANE,                                    // Main Drawing Layer
        0,                                                // Reserved
        0, 0, 0                                            // Layer Masks Ignored
    };

    // choose a pixel format that matches pfd
    pxFormat = ChoosePixelFormat(hDC, &pfd);
    if (pxFormat == 0) {
        DbgPrintf("ChoosePixelFormat failed\n");
        MessageBoxA(NULL,
            "ChoosePixelFormat failed\n"
            "Exiting...", APP_NAME, MB_OK);
        exit(1);
        return false;
    }

    // set the pixel format
    if (SetPixelFormat(hDC, pxFormat, &pfd) == FALSE) {
        DbgPrintf("SetPixelFormat failed\n");
        MessageBoxA(NULL,
            "SetPixelFormat failed\n"
            "Exiting...", APP_NAME, MB_OK);
        exit(1);
        return false;
    }

    // create a temporary rendering context
    HGLRC tmp_hGLRC = wglCreateContext(hDC);
    if(tmp_hGLRC == NULL) {
        printf("wglCreateContext for dummy context failed\n");
        MessageBoxA(NULL,
            "wglCreateContext for dummy context failed\n"
            "Exiting...", APP_NAME, MB_OK);
        exit(1);
        return false;
    }
    wglMakeCurrent(hDC, tmp_hGLRC);

    static bool announced_gl = false;
    if (!announced_gl) {
        DbgPrintf("Version: %s\n", glGetString(GL_VERSION));
        DbgPrintf("Renderer: %s\n", glGetString(GL_RENDERER));
        DbgPrintf("Vendor: %s\n", glGetString(GL_VENDOR));
        announced_gl = true;
    }

    // get the function pointer for wglChoosePixelFormat
    static PFNWGLCHOOSEPIXELFORMATARBPROC   wglChoosePixelFormatARB;
    wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
    if (!wglChoosePixelFormatARB) {
        DbgPrintf("wglGetProcAddress failed\n");
        MessageBoxA(NULL,
            "wglGetProcAddress failed\n"
            "for wglChoosePixelFormatARB\n"
            "Exiting...", APP_NAME, MB_OK);
        exit(1);
        return false;
    }

    // delete temporary rendering context
    wglDeleteContext(tmp_hGLRC);
    tmp_hGLRC = NULL;

    // Setup WGL pixel format attributes.
    int iAttribs[] = { WGL_DRAW_TO_WINDOW_ARB,  GL_TRUE,
                        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
                        WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
                        WGL_COLOR_BITS_ARB,     24,
                        WGL_ALPHA_BITS_ARB,     8,
                        WGL_DEPTH_BITS_ARB,     24,
                        WGL_STENCIL_BITS_ARB,   8,
                        WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
    //                    WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
    //                    WGL_SAMPLES_ARB,        4,
                        0, 0};
    float fAttribs[] = {0, 0};
    unsigned int numFormats;
    BOOL ret;

    // find compatible WGL pixel formats
    ret = wglChoosePixelFormatARB(hDC, iAttribs, fAttribs, 1, &pxFormat, &numFormats);
    if (ret == FALSE) {
        DbgPrintf("wglChoosePixelFormatARB failed\n");
        MessageBoxA(NULL,
            "wglChoosePixelFormatARB failed\n"
            "Exiting...", APP_NAME, MB_OK);
        exit(1);
        return false;
    }

    if (numFormats == 0) {
        DbgPrintf("wglChoosePixelFormatARB: no compatible pixel format found\n");
        MessageBoxA(NULL,
            "wglChoosePixelFormatARB: no compatible pixel format found\n"
            "Exiting...", APP_NAME, MB_OK);
        exit(1);
        return false;
    }

    ReleaseDC(hWnd, hDC);
    DestroyWindow(hWnd);
    hWnd = NULL;
    hDC = NULL;

    hWnd = DXUTGetHWND();
    hDC = GetDC(hWnd);

    // set the pixel format we chose
    if (SetPixelFormat(hDC, pxFormat, &pfd) == FALSE) {
        DbgPrintf("SetPixelFormat for extended PF failed\n");
        MessageBoxA(NULL,
            "SetPixelFormat for extended PF failed\n"
            "Exiting...", APP_NAME, MB_OK);
        exit(1);
        return false;
    }

    // create the OpenGL context
    hGLRC = wglCreateContext(hDC);
    if(hGLRC == NULL) {
        printf("wglCreateContext failed\n");
        MessageBoxA(NULL,
            "wglCreateContext failed\n"
            "Exiting...", APP_NAME, MB_OK);
        exit(1);
        return false;
    }

    wglMakeCurrent(hDC, hGLRC);

    GLenum result = glewInit();
    if (result != GLEW_OK) {
        printf("glewInit failed\n");
        MessageBoxA(NULL,
            "glewInit failed\n"
            "Exiting...", APP_NAME, MB_OK);
        exit(1);
        return false;
    }

    if (!glewIsSupported("GL_VERSION_3_0")) {
        MessageBoxA(NULL,
            "OpenGL 3.0 or better not required\n"
            "Exiting...", APP_NAME, MB_OK);
        exit(1);
        return false;
    }

    if (!wglewIsSupported("WGL_NV_DX_interop")) {
        MessageBoxA(NULL,
            "WGL_NV_DX_interop OpenGL extension required\n"
            "Exiting...", APP_NAME, MB_OK);
        exit(1);
        return false;
    }

#if 0
    DbgPrintf("Version: %s\n", glGetString(GL_VERSION));
    DbgPrintf("Renderer: %s\n", glGetString(GL_RENDERER));
    DbgPrintf("Vendor: %s\n", glGetString(GL_VENDOR));
#endif

    IDirect3D9* d3d = DXUTGetD3DObject();
    IDirect3DDevice9* d3ddevice = DXUTGetD3DDevice();

    handleD3D = wglDXOpenDeviceNV(d3ddevice);
    if (handleD3D == NULL) {
        DbgPrintf("wglDXOpenDeviceNV failed\n");
        return false;
    }

    glGenTextures(1, &nameColorBuffer);
    glGenTextures(1, &nameDepthBuffer);
    // create a framebuffer to attach the DX color/depth buffers
    glGenFramebuffers(1, &fbo);

    HRESULT hr;
    IDirect3DSurface9* pBackBuffer = NULL;
    hr = d3ddevice->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer );
    assert(hr == S_OK);
    pBackBuffer->Release();

    const GLenum targetColorBuffer = GL_TEXTURE_RECTANGLE_NV;
    handleD3DColorBuffer = wglDXRegisterObjectNV(handleD3D, pBackBuffer,
                                                 nameColorBuffer, targetColorBuffer, WGL_ACCESS_READ_WRITE_NV);

    if (handleD3DColorBuffer == NULL) {
        printf("wglDXRegisterObjectNV failed for back buffer\n");
        MessageBoxA(NULL,
            "wglDXRegisterObjectNV failed for back buffer\n"
            "Exiting...", APP_NAME, MB_OK);
        exit(1);
        return false;
    }

    const GLenum targetDepthBuffer = GL_TEXTURE_RECTANGLE_NV;
    IDirect3DSurface9* pDepthStencilBuffer = NULL;
    hr = d3ddevice->GetDepthStencilSurface(&pDepthStencilBuffer);
    assert(hr == S_OK);
    pDepthStencilBuffer->Release();

    handleD3DDepthBuffer = wglDXRegisterObjectNV(handleD3D, pDepthStencilBuffer,
                                                  nameDepthBuffer, targetDepthBuffer, WGL_ACCESS_READ_WRITE_NV);

    if (handleD3DDepthBuffer == NULL) {
        printf("wglDXRegisterObjectNV failed for depth-stencil buffer\n");
        MessageBoxA(NULL,
            "wglDXRegisterObjectNV failed for depth-stencil buffer\n"
            "Exiting...", APP_NAME, MB_OK);
        exit(1);
        return false;
    }

    // bind the D3D color buffer to the fbo
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, targetColorBuffer, nameColorBuffer, 0);
    // bind the D3D depth/stencil buffer to the fbo
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, targetDepthBuffer, nameDepthBuffer, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, targetDepthBuffer, nameDepthBuffer, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        DbgPrintf("err = 0x%x\n", err);
    }

#if 0
    initGraphics();
#else
    // Tiger rendering
    glMatrixLoadIdentityEXT(GL_PROJECTION);
    glMatrixOrthoEXT(GL_PROJECTION, 0, 640, 0, 480, -1, 1);
    glMatrixLoadIdentityEXT(GL_MODELVIEW);
    glMatrixScalefEXT(GL_MODELVIEW, 0.9f, 0.9f, 1.0f);
    glMatrixTranslatefEXT(GL_MODELVIEW, 270, 160, 0);

    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_NOTEQUAL, 0, ~0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

    initTiger();
#endif

    return S_OK;
}

static void lockDXObjects()
{
    HANDLE objects[4];
    int count = 0;

    if (handleD3DColorBuffer) {
        objects[count] = handleD3DColorBuffer;
        count++;
    }

    if (handleD3DDepthBuffer) {
        objects[count] = handleD3DDepthBuffer;
        count++;
    }

    BOOL worked = wglDXLockObjectsNV(handleD3D, count, objects);
    assert(worked);
}

static void unlockDXObjects()
{
    HANDLE objects[4];
    int count = 0;

    if (handleD3DColorBuffer) {
        objects[count] = handleD3DColorBuffer;
        count++;
    }

    if (handleD3DDepthBuffer) {
        objects[count] = handleD3DDepthBuffer;
        count++;
    }

    BOOL worked = wglDXUnlockObjectsNV(handleD3D, count, objects);
    assert(worked);
}

static void CALLBACK OnFrameRender(IDirect3DDevice9* pDev,
                                   double time,
                                   float elapsedTime,
                                   void* userContext)
{
    if (SUCCEEDED(pDev->BeginScene())) {

        lockDXObjects();

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            DbgPrintf("err = 0x%x\n", err);
        }

        glClearColor(0.1f, 0.3f, 0.6f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
#if 0
        doGraphics();
#else
        // Tiger rendering
        renderTiger(filling, stroking);
#endif

        while ((err = glGetError()) != GL_NO_ERROR) {
            DbgPrintf("err = 0x%x\n", err);
        }

        unlockDXObjects();

        pDev->EndScene();
    }
}

static void CALLBACK OnLostDevice(void* userContext)
{
    if (handleD3DColorBuffer) {
        wglDXUnregisterObjectNV(handleD3D, handleD3DColorBuffer);
    }

    if (handleD3DDepthBuffer) {
        wglDXUnregisterObjectNV(handleD3D, handleD3DDepthBuffer);
    }

    if (nameColorBuffer) {
        glDeleteTextures(1, &nameColorBuffer);
    }

    if (nameDepthBuffer) {
        glDeleteTextures(1, &nameDepthBuffer);
    }

    if (fbo) {
        glDeleteFramebuffers(1, &fbo);
    }

    wglDXCloseDeviceNV(handleD3D);

    wglDeleteContext(hGLRC);
}