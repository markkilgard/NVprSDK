
/*
 * Copyright 2010 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */



#ifndef GrTypes_DEFINED
#define GrTypes_DEFINED

#include "SkTypes.h"
#include "GrConfig.h"

////////////////////////////////////////////////////////////////////////////////

/**
 * Defines overloaded bitwise operators to make it easier to use an enum as a
 * bitfield.
 */
#define GR_MAKE_BITFIELD_OPS(X) \
    inline X operator | (X a, X b) { \
        return (X) (+a | +b); \
    } \
    \
    inline X operator & (X a, X b) { \
        return (X) (+a & +b); \
    } \
    template <typename T> \
    inline X operator & (T a, X b) { \
        return (X) (+a & +b); \
    } \
    template <typename T> \
    inline X operator & (X a, T b) { \
        return (X) (+a & +b); \
    } \

#define GR_DECL_BITFIELD_OPS_FRIENDS(X) \
    friend X operator | (X a, X b); \
    \
    friend X operator & (X a, X b); \
    \
    template <typename T> \
    friend X operator & (T a, X b); \
    \
    template <typename T> \
    friend X operator & (X a, T b); \
////////////////////////////////////////////////////////////////////////////////


/**
 *  Macro to round n up to the next multiple of 4, or return it unchanged if
 *  n is already a multiple of 4
 */
#define GrALIGN4(n)     SkAlign4(n)
#define GrIsALIGN4(n)   SkIsAlign4(n)

template <typename T> const T& GrMin(const T& a, const T& b) {
	return (a < b) ? a : b;
}

template <typename T> const T& GrMax(const T& a, const T& b) {
	return (b < a) ? a : b;
}

// compile time versions of min/max
#define GR_CT_MAX(a, b) (((b) < (a)) ? (a) : (b))
#define GR_CT_MIN(a, b) (((b) < (a)) ? (b) : (a))

/**
 *  divide, rounding up
 */
static inline int32_t GrIDivRoundUp(int x, int y) {
    GrAssert(y > 0);
    return (x + (y-1)) / y;
}
static inline uint32_t GrUIDivRoundUp(uint32_t x, uint32_t y) {
    return (x + (y-1)) / y;
}
static inline size_t GrSizeDivRoundUp(size_t x, uint32_t y) {
    return (x + (y-1)) / y;
}

/**
 *  align up
 */
static inline uint32_t GrUIAlignUp(uint32_t x, uint32_t alignment) {
    return GrUIDivRoundUp(x, alignment) * alignment;
}
static inline uint32_t GrSizeAlignUp(size_t x, uint32_t alignment) {
    return GrSizeDivRoundUp(x, alignment) * alignment;
}

/**
 * amount of pad needed to align up
 */
static inline uint32_t GrUIAlignUpPad(uint32_t x, uint32_t alignment) {
    return (alignment - x % alignment) % alignment;
}
static inline size_t GrSizeAlignUpPad(size_t x, uint32_t alignment) {
    return (alignment - x % alignment) % alignment;
}

/**
 *  align down
 */
static inline uint32_t GrUIAlignDown(uint32_t x, uint32_t alignment) {
    return (x / alignment) * alignment;
}
static inline uint32_t GrSizeAlignDown(size_t x, uint32_t alignment) {
    return (x / alignment) * alignment;
}

/**
 *  Count elements in an array
 */
#define GR_ARRAY_COUNT(array)  SK_ARRAY_COUNT(array)

//!< allocate a block of memory, will never return NULL
extern void* GrMalloc(size_t bytes);

//!< free block allocated by GrMalloc. ptr may be NULL
extern void GrFree(void* ptr);

static inline void Gr_bzero(void* dst, size_t size) {
    memset(dst, 0, size);
}

///////////////////////////////////////////////////////////////////////////////

/**
 *  Return the number of leading zeros in n
 */
extern int Gr_clz(uint32_t n);

/**
 *  Return true if n is a power of 2
 */
static inline bool GrIsPow2(unsigned n) {
    return n && 0 == (n & (n - 1));
}

/**
 *  Return the next power of 2 >= n.
 */
static inline uint32_t GrNextPow2(uint32_t n) {
    return n ? (1 << (32 - Gr_clz(n - 1))) : 1;
}

static inline int GrNextPow2(int n) {
    GrAssert(n >= 0); // this impl only works for non-neg.
    return n ? (1 << (32 - Gr_clz(n - 1))) : 1;
}

///////////////////////////////////////////////////////////////////////////////

/**
 *  16.16 fixed point type
 */
typedef int32_t GrFixed;

#if GR_DEBUG

static inline int16_t GrToS16(intptr_t x) {
    GrAssert((int16_t)x == x);
    return (int16_t)x;
}

#else

#define GrToS16(x)  x

#endif


///////////////////////////////////////////////////////////////////////////////

/**
 * Possible 3D APIs that may be used by Ganesh.
 */
enum GrEngine {
    kOpenGL_Shaders_GrEngine,
    kOpenGL_Fixed_GrEngine,
};

/**
 * Engine-specific 3D context handle
 *      GrGLInterface* for OpenGL. If NULL will use the default GL interface.
 */
typedef intptr_t GrPlatform3DContext;

///////////////////////////////////////////////////////////////////////////////

/**
 * Type used to describe format of vertices in arrays
 * Values are defined in GrDrawTarget
 */
typedef int GrVertexLayout;

/**
* Geometric primitives used for drawing.
*/
enum GrPrimitiveType {
    kTriangles_PrimitiveType,
    kTriangleStrip_PrimitiveType,
    kTriangleFan_PrimitiveType,
    kPoints_PrimitiveType,
    kLines_PrimitiveType,     // 1 pix wide only
    kLineStrip_PrimitiveType  // 1 pix wide only
};

static inline bool GrIsPrimTypeLines(GrPrimitiveType type) {
    return kLines_PrimitiveType == type || kLineStrip_PrimitiveType == type;
}

static inline bool GrIsPrimTypeTris(GrPrimitiveType type) {
    return kTriangles_PrimitiveType == type     ||
           kTriangleStrip_PrimitiveType == type ||
           kTriangleFan_PrimitiveType == type;
}

/**
 * Coeffecients for alpha-blending.
 */
enum GrBlendCoeff {
    kZero_BlendCoeff,    //<! 0
    kOne_BlendCoeff,     //<! 1
    kSC_BlendCoeff,      //<! src color
    kISC_BlendCoeff,     //<! one minus src color
    kDC_BlendCoeff,      //<! dst color
    kIDC_BlendCoeff,     //<! one minus dst color
    kSA_BlendCoeff,      //<! src alpha
    kISA_BlendCoeff,     //<! one minus src alpha
    kDA_BlendCoeff,      //<! dst alpha
    kIDA_BlendCoeff,     //<! one minus dst alpha
    kConstC_BlendCoeff,  //<! constant color
    kIConstC_BlendCoeff, //<! one minus constant color
    kConstA_BlendCoeff,  //<! constant color alpha
    kIConstA_BlendCoeff, //<! one minus constant color alpha

    kPublicBlendCoeffCount
};

/**
 *  Formats for masks, used by the font cache.
 *  Important that these are 0-based.
 */
enum GrMaskFormat {
    kA8_GrMaskFormat,    //!< 1-byte per pixel
    kA565_GrMaskFormat,  //!< 2-bytes per pixel
    kA888_GrMaskFormat,  //!< 4-bytes per pixel

    kCount_GrMaskFormats //!< used to allocate arrays sized for mask formats
};

/**
 *  Return the number of bytes-per-pixel for the specified mask format.
 */
static inline int GrMaskFormatBytesPerPixel(GrMaskFormat format) {
    GrAssert((unsigned)format <= 2);
    // kA8   (0) -> 1
    // kA565 (1) -> 2
    // kA888 (2) -> 4
    return 1 << (int)format;
}

/**
 * Pixel configurations.
 *
 * Unpremultiplied configs are intended for converting pixel data in and out
 * from skia. Surfaces with these configs have limited support. As an input
 * (GrPaint texture) the corresponding GrSamplerState must have its filter set
 * to kNearest_Filter. Otherwise, the draw will fail. When the render target
 * has an unpremultiplied config draws must use blend coeffs 1,0 (AKA src-mode).
 * Other coeffs will cause the draw to fail.
 */
enum GrPixelConfig {
    kUnknown_GrPixelConfig,
    kAlpha_8_GrPixelConfig,
    kIndex_8_GrPixelConfig,
    kRGB_565_GrPixelConfig,
    /**
     * Premultiplied
     */
    kRGBA_4444_GrPixelConfig,
    /**
     * Premultiplied. Byte order is r,g,b,a
     */
    kRGBA_8888_PM_GrPixelConfig,
    /**
     * Unpremultiplied. Byte order is r,g,b,a
     */
    kRGBA_8888_UPM_GrPixelConfig,
    /**
     * Premultiplied. Byte order is b,g,r,a
     */
    kBGRA_8888_PM_GrPixelConfig,
    /**
     * Unpremultiplied. Byte order is b,g,r,a
     */
    kBGRA_8888_UPM_GrPixelConfig,

    kGrPixelConfigCount
};

// Aliases for pixel configs that match skia's byte order
#ifndef SK_CPU_LENDIAN
    #error "Skia gpu currently assumes little endian"
#endif
#if 24 == SK_A32_SHIFT && 16 == SK_R32_SHIFT && \
     8 == SK_G32_SHIFT &&  0 == SK_B32_SHIFT
    static const GrPixelConfig kSkia8888_PM_GrPixelConfig = kBGRA_8888_PM_GrPixelConfig;
    static const GrPixelConfig kSkia8888_UPM_GrPixelConfig = kBGRA_8888_UPM_GrPixelConfig;
#elif 24 == SK_A32_SHIFT && 16 == SK_B32_SHIFT && \
       8 == SK_G32_SHIFT &&  0 == SK_R32_SHIFT
    static const GrPixelConfig kSkia8888_PM_GrPixelConfig = kRGBA_8888_PM_GrPixelConfig;
    static const GrPixelConfig kSkia8888_UPM_GrPixelConfig = kRGBA_8888_UPM_GrPixelConfig;
#else
    #error "SK_*32_SHIFT values must correspond to GL_BGRA or GL_RGBA format."
#endif

// Returns true if the pixel config has 8bit r,g,b,a components in that byte
// order
static inline bool GrPixelConfigIsRGBA8888(GrPixelConfig config) {
    switch (config) {
        case kRGBA_8888_PM_GrPixelConfig:
        case kRGBA_8888_UPM_GrPixelConfig:
            return true;
        default:
            return false;
    }
}

// Returns true if the pixel config has 8bit b,g,r,a components in that byte
// order
static inline bool GrPixelConfigIsBGRA8888(GrPixelConfig config) {
    switch (config) {
        case kBGRA_8888_PM_GrPixelConfig:
        case kBGRA_8888_UPM_GrPixelConfig:
            return true;
        default:
            return false;
    }
}

// Returns true if the pixel config is 32 bits per pixel
static inline bool GrPixelConfigIs32Bit(GrPixelConfig config) {
    switch (config) {
        case kRGBA_8888_PM_GrPixelConfig:
        case kRGBA_8888_UPM_GrPixelConfig:
        case kBGRA_8888_PM_GrPixelConfig:
        case kBGRA_8888_UPM_GrPixelConfig:
            return true;
        default:
            return false;
    }
}

// Takes a config and returns the equivalent config with the R and B order
// swapped if such a config exists. Otherwise, kUnknown_GrPixelConfig
static inline GrPixelConfig GrPixelConfigSwapRAndB(GrPixelConfig config) {
    switch (config) {
        case kBGRA_8888_PM_GrPixelConfig:
            return kRGBA_8888_PM_GrPixelConfig;
        case kBGRA_8888_UPM_GrPixelConfig:
            return kRGBA_8888_UPM_GrPixelConfig;
        case kRGBA_8888_PM_GrPixelConfig:
            return kBGRA_8888_PM_GrPixelConfig;
        case kRGBA_8888_UPM_GrPixelConfig:
            return kBGRA_8888_UPM_GrPixelConfig;
        default:
            return kUnknown_GrPixelConfig;
    }
}

static inline size_t GrBytesPerPixel(GrPixelConfig config) {
    switch (config) {
        case kAlpha_8_GrPixelConfig:
        case kIndex_8_GrPixelConfig:
            return 1;
        case kRGB_565_GrPixelConfig:
        case kRGBA_4444_GrPixelConfig:
            return 2;
        case kRGBA_8888_PM_GrPixelConfig:
        case kRGBA_8888_UPM_GrPixelConfig:
        case kBGRA_8888_PM_GrPixelConfig:
        case kBGRA_8888_UPM_GrPixelConfig:
            return 4;
        default:
            return 0;
    }
}

static inline bool GrPixelConfigIsOpaque(GrPixelConfig config) {
    switch (config) {
        case kRGB_565_GrPixelConfig:
            return true;
        default:
            return false;
    }
}

/**
 * Premultiplied alpha is the usual for skia. Therefore, configs that are
 * ambiguous (alpha-only or color-only) are considered premultiplied.
 */
static inline bool GrPixelConfigIsUnpremultiplied(GrPixelConfig config) {
    switch (config) {
        case kRGBA_8888_UPM_GrPixelConfig:
        case kBGRA_8888_UPM_GrPixelConfig:
            return true;
        default:
            return false;
    }
}

static inline bool GrPixelConfigIsAlphaOnly(GrPixelConfig config) {
    switch (config) {
        case kAlpha_8_GrPixelConfig:
            return true;
        default:
            return false;
    }
}

/**
 * Optional bitfield flags that can be passed to createTexture.
 */
enum GrTextureFlags {
    kNone_GrTextureFlags            = 0x0,
    /**
     * Creates a texture that can be rendered to as a GrRenderTarget. Use
     * GrTexture::asRenderTarget() to access.
     */
    kRenderTarget_GrTextureFlagBit  = 0x1,  
    /**
     * By default all render targets have an associated stencil buffer that
     * may be required for path filling. This flag overrides stencil buffer
     * creation.
     * MAKE THIS PRIVATE?
     */
    kNoStencil_GrTextureFlagBit     = 0x2,
    /**
     * Hint that the CPU may modify this texture after creation.
     */
    kDynamicUpdate_GrTextureFlagBit = 0x4,
};

GR_MAKE_BITFIELD_OPS(GrTextureFlags)

enum {
   /**
    *  For Index8 pixel config, the colortable must be 256 entries
    */
    kGrColorTableSize = 256 * 4 //sizeof(GrColor)
};

/**
 * Describes a texture to be created.
 */
struct GrTextureDesc {
    GrTextureFlags         fFlags;  //!< bitfield of TextureFlags
    int                    fWidth;  //!< Width of the texture
    int                    fHeight; //!< Height of the texture

    /**
     * Format of source data of the texture. Not guaraunteed to be the same as
     * internal format used by 3D API.
     */
    GrPixelConfig          fConfig;

    /**
     * The number of samples per pixel or 0 to disable full scene AA. This only
     * applies if the kRenderTarget_GrTextureFlagBit is set. The actual number
     * of samples may not exactly match the request. The request will be rounded
     * up to the next supported sample count, or down if it is larger than the
     * max supportex count.
     */
    int fSampleCnt;
};

/**
 * Set Operations used to construct clips.
 */
enum GrSetOp {
    kReplace_SetOp,
    kIntersect_SetOp,
    kUnion_SetOp,
    kXor_SetOp,
    kDifference_SetOp,
    kReverseDifference_SetOp,
};

/**
 * Clips are composed from these objects.
 */
enum GrClipType {
    kRect_ClipType,
    kPath_ClipType
};

/**
 * Commands used to describe a path. Each command
 * is accompanied by some number of points.
 */
enum GrPathCmd {
    kMove_PathCmd,      //!< Starts a new subpath at
                        //   at the returned point
                        // 1 point
    kLine_PathCmd,      //!< Adds a line segment
                        // 2 points
    kQuadratic_PathCmd, //!< Adds a quadratic segment
                        // 3 points
    kCubic_PathCmd,     //!< Adds a cubic segment
                        // 4 points
    kClose_PathCmd,     //!< Closes the current subpath
                        //   by connecting a line to the
                        //   starting point.
                        // 0 points
    kEnd_PathCmd        //!< Indicates the end of the last subpath
                        //   when iterating
                        // 0 points.
};

/**
 * Gets the number of points associated with a path command.
 */
static int inline NumPathCmdPoints(GrPathCmd cmd) {
    static const int gNumPoints[] = {
        1, 2, 3, 4, 0, 0
    };
    return gNumPoints[cmd];
}

/**
 * Path filling rules
 */
enum GrPathFill {
    kWinding_PathFill,
    kEvenOdd_PathFill,
    kInverseWinding_PathFill,
    kInverseEvenOdd_PathFill,
    kHairLine_PathFill,

    kPathFillCount
};

static inline GrPathFill GrNonInvertedFill(GrPathFill fill) {
    static const GrPathFill gNonInvertedFills[] = {
        kWinding_PathFill, // kWinding_PathFill
        kEvenOdd_PathFill, // kEvenOdd_PathFill
        kWinding_PathFill, // kInverseWinding_PathFill
        kEvenOdd_PathFill, // kInverseEvenOdd_PathFill
        kHairLine_PathFill,// kHairLine_PathFill
    };
    GR_STATIC_ASSERT(0 == kWinding_PathFill);
    GR_STATIC_ASSERT(1 == kEvenOdd_PathFill);
    GR_STATIC_ASSERT(2 == kInverseWinding_PathFill);
    GR_STATIC_ASSERT(3 == kInverseEvenOdd_PathFill);
    GR_STATIC_ASSERT(4 == kHairLine_PathFill);
    GR_STATIC_ASSERT(5 == kPathFillCount);
    return gNonInvertedFills[fill];
}

static inline bool GrIsFillInverted(GrPathFill fill) {
    static const bool gIsFillInverted[] = {
        false, // kWinding_PathFill
        false, // kEvenOdd_PathFill
        true,  // kInverseWinding_PathFill
        true,  // kInverseEvenOdd_PathFill
        false, // kHairLine_PathFill
    };
    GR_STATIC_ASSERT(0 == kWinding_PathFill);
    GR_STATIC_ASSERT(1 == kEvenOdd_PathFill);
    GR_STATIC_ASSERT(2 == kInverseWinding_PathFill);
    GR_STATIC_ASSERT(3 == kInverseEvenOdd_PathFill);
    GR_STATIC_ASSERT(4 == kHairLine_PathFill);
    GR_STATIC_ASSERT(5 == kPathFillCount);
    return gIsFillInverted[fill];
}

///////////////////////////////////////////////////////////////////////////////

// opaque type for 3D API object handles
typedef intptr_t GrPlatform3DObject;

/**
 * Gr can wrap an existing texture created by the client with a GrTexture
 * object. The client is responsible for ensuring that the texture lives at
 * least as long as the GrTexture object wrapping it. We require the client to 
 * explicitly provide information about the texture, such as width, height, 
 * and pixel config, rather than querying the 3D APIfor these values. We expect
 * these to be immutable even if the 3D API doesn't require this (OpenGL).
 *
 * Textures that are also render targets are supported as well. Gr will manage
 * any ancillary 3D API (stencil buffer, FBO id, etc) objects necessary for
 * Gr to draw into the render target. To access the render target object
 * call GrTexture::asRenderTarget().
 *
 * If in addition to the render target flag, the caller also specifies a sample
 * count Gr will create an MSAA buffer that resolves into the texture. Gr auto-
 * resolves when it reads from the texture. The client can explictly resolve
 * using the GrRenderTarget interface.
 */

enum GrPlatformTextureFlags {
    /**
     * No flags enabled
     */
    kNone_GrPlatformTextureFlag              = 0x0,
    /**
     * Indicates that the texture is also a render target, and thus should have
     * a GrRenderTarget object.
     *
     * D3D (future): client must have created the texture with flags that allow
     * it to be used as a render target.
     */
    kRenderTarget_GrPlatformTextureFlag      = 0x1,
};
GR_MAKE_BITFIELD_OPS(GrPlatformTextureFlags)

struct GrPlatformTextureDesc {
    GrPlatformTextureDesc() { memset(this, 0, sizeof(*this)); }
    GrPlatformTextureFlags          fFlags;
    int                             fWidth;         //<! width in pixels
    int                             fHeight;        //<! height in pixels
    GrPixelConfig                   fConfig;        //<! color format
    /**
     * If the render target flag is set and sample count is greater than 0
     * then Gr will create an MSAA buffer that resolves to the texture.
     */
    int                             fSampleCnt;
    /**
     * Handle to the 3D API object.
     * OpenGL: Texture ID.
     */
    GrPlatform3DObject              fTextureHandle;
};

///////////////////////////////////////////////////////////////////////////////

/**
 * Gr can wrap an existing render target created by the client in the 3D API
 * with a GrRenderTarget object. The client is responsible for ensuring that the
 * underlying 3D API object lives at least as long as the GrRenderTarget object
 * wrapping it. We require the client to explicitly provide information about 
 * the target, such as width, height, and pixel config rather than querying the
 * 3D API for these values. We expect these properties to be immutable even if
 * the 3D API doesn't require this (OpenGL).
 */

struct GrPlatformRenderTargetDesc {
    GrPlatformRenderTargetDesc() { memset(this, 0, sizeof(*this)); }
    int                             fWidth;         //<! width in pixels
    int                             fHeight;        //<! height in pixels
    GrPixelConfig                   fConfig;        //<! color format
    /**
     * The number of samples per pixel. Gr uses this to influence decisions
     * about applying other forms of antialiasing.
     */
    int                             fSampleCnt;
    /**
     * Number of bits of stencil per-pixel.
     */
    int                             fStencilBits;
    /**
     * Handle to the 3D API object.
     * OpenGL: FBO ID
     */
    GrPlatform3DObject              fRenderTargetHandle;
};


///////////////////////////////////////////////////////////////////////////////

// this is included only to make it easy to use this debugging facility
#include "GrInstanceCounter.h"

#endif
