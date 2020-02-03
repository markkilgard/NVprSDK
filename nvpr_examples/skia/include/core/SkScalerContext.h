
/*
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef SkScalerContext_DEFINED
#define SkScalerContext_DEFINED

#include "SkMask.h"
#include "SkMatrix.h"
#include "SkPaint.h"
#include "SkPath.h"
#include "SkPoint.h"

//#define SK_USE_COLOR_LUMINANCE

class SkDescriptor;
class SkMaskFilter;
class SkPathEffect;
class SkRasterizer;

// needs to be != to any valid SkMask::Format
#define MASK_FORMAT_UNKNOWN         (0xFF)
#define MASK_FORMAT_JUST_ADVANCE    MASK_FORMAT_UNKNOWN

#define kMaxGlyphWidth (1<<13)

struct SkGlyph {
    void*       fImage;
    SkPath*     fPath;
    SkFixed     fAdvanceX, fAdvanceY;

    uint32_t    fID;
    uint16_t    fWidth, fHeight;
    int16_t     fTop, fLeft;

    uint8_t     fMaskFormat;
    int8_t      fRsbDelta, fLsbDelta;  // used by auto-kerning

    void init(uint32_t id) {
        fID             = id;
        fImage          = NULL;
        fPath           = NULL;
        fMaskFormat     = MASK_FORMAT_UNKNOWN;
    }

    /**
     *  Compute the rowbytes for the specified width and mask-format.
     */
    static unsigned ComputeRowBytes(unsigned width, SkMask::Format format) {
        unsigned rb = width;
        if (SkMask::kBW_Format == format) {
            rb = (rb + 7) >> 3;
		} else if (SkMask::kARGB32_Format == format ||
                   SkMask::kLCD32_Format == format)
        {
			rb <<= 2;
		} else if (SkMask::kLCD16_Format == format) {
			rb = SkAlign4(rb << 1);
        } else {
            rb = SkAlign4(rb);
        }
        return rb;
    }

    unsigned rowBytes() const {
        return ComputeRowBytes(fWidth, (SkMask::Format)fMaskFormat);
    }

    bool isJustAdvance() const {
        return MASK_FORMAT_JUST_ADVANCE == fMaskFormat;
    }

    bool isFullMetrics() const {
        return MASK_FORMAT_JUST_ADVANCE != fMaskFormat;
    }

    uint16_t getGlyphID() const {
        return ID2Code(fID);
    }

    unsigned getGlyphID(unsigned baseGlyphCount) const {
        unsigned code = ID2Code(fID);
        SkASSERT(code >= baseGlyphCount);
        return code - baseGlyphCount;
    }

    unsigned getSubX() const {
        return ID2SubX(fID);
    }

    SkFixed getSubXFixed() const {
        return SubToFixed(ID2SubX(fID));
    }

    SkFixed getSubYFixed() const {
        return SubToFixed(ID2SubY(fID));
    }

    size_t computeImageSize() const;

    /** Call this to set all of the metrics fields to 0 (e.g. if the scaler
        encounters an error measuring a glyph). Note: this does not alter the
        fImage, fPath, fID, fMaskFormat fields.
     */
    void zeroMetrics();

    enum {
        kSubBits = 2,
        kSubMask = ((1 << kSubBits) - 1),
        kSubShift = 24, // must be large enough for glyphs and unichars
        kCodeMask = ((1 << kSubShift) - 1),
        // relative offsets for X and Y subpixel bits
        kSubShiftX = kSubBits,
        kSubShiftY = 0
    };

    static unsigned ID2Code(uint32_t id) {
        return id & kCodeMask;
    }

    static unsigned ID2SubX(uint32_t id) {
        return id >> (kSubShift + kSubShiftX);
    }

    static unsigned ID2SubY(uint32_t id) {
        return (id >> (kSubShift + kSubShiftY)) & kSubMask;
    }

    static unsigned FixedToSub(SkFixed n) {
        return (n >> (16 - kSubBits)) & kSubMask;
    }

    static SkFixed SubToFixed(unsigned sub) {
        SkASSERT(sub <= kSubMask);
        return sub << (16 - kSubBits);
    }

    static uint32_t MakeID(unsigned code) {
        return code;
    }

    static uint32_t MakeID(unsigned code, SkFixed x, SkFixed y) {
        SkASSERT(code <= kCodeMask);
        x = FixedToSub(x);
        y = FixedToSub(y);
        return (x << (kSubShift + kSubShiftX)) |
               (y << (kSubShift + kSubShiftY)) |
               code;
    }

    void toMask(SkMask* mask) const;
};

class SkScalerContext {
public:
    enum Flags {
        kFrameAndFill_Flag        = 0x0001,
        kDevKernText_Flag         = 0x0002,
        kEmbeddedBitmapText_Flag  = 0x0004,
        kEmbolden_Flag            = 0x0008,
        kSubpixelPositioning_Flag = 0x0010,
        kAutohinting_Flag         = 0x0020,
        kVertical_Flag            = 0x0040,

        // together, these two flags resulting in a two bit value which matches
        // up with the SkPaint::Hinting enum.
        kHinting_Shift            = 7, // to shift into the other flags above
        kHintingBit1_Flag         = 0x0080,
        kHintingBit2_Flag         = 0x0100,

        // these should only ever be set if fMaskFormat is LCD16 or LCD32
        kLCD_Vertical_Flag        = 0x0200,    // else Horizontal
        kLCD_BGROrder_Flag        = 0x0400,    // else RGB order

        // Generate A8 from LCD source (for GDI), only meaningful if fMaskFormat is kA8
        // Perhaps we can store this (instead) in fMaskFormat, in hight bit?
        kGenA8FromLCD_Flag        = 0x0800,

#ifdef SK_USE_COLOR_LUMINANCE
        kLuminance_Bits           = 3,
#else
        // luminance : 0 for black text, kLuminance_Max for white text
        kLuminance_Shift          = 13, // shift to land in the high 3-bits of Flags
        kLuminance_Bits           = 3,  // ensure Flags doesn't exceed 16bits
#endif
    };
    
    // computed values
    enum {
        kHinting_Mask   = kHintingBit1_Flag | kHintingBit2_Flag,
#ifdef SK_USE_COLOR_LUMINANCE
#else
        kLuminance_Max  = (1 << kLuminance_Bits) - 1,
        kLuminance_Mask = kLuminance_Max << kLuminance_Shift,
#endif
    };

    struct Rec {
        uint32_t    fOrigFontID;
        uint32_t    fFontID;
        SkScalar    fTextSize, fPreScaleX, fPreSkewX;
        SkScalar    fPost2x2[2][2];
        SkScalar    fFrameWidth, fMiterLimit;
#ifdef SK_USE_COLOR_LUMINANCE
        uint32_t    fLumBits;
#endif
        uint8_t     fMaskFormat;
        uint8_t     fStrokeJoin;
        uint16_t    fFlags;
        // Warning: when adding members note that the size of this structure
        // must be a multiple of 4. SkDescriptor requires that its arguments be
        // multiples of four and this structure is put in an SkDescriptor in
        // SkPaint::MakeRec.

        void    getMatrixFrom2x2(SkMatrix*) const;
        void    getLocalMatrix(SkMatrix*) const;
        void    getSingleMatrix(SkMatrix*) const;

        SkPaint::Hinting getHinting() const {
            unsigned hint = (fFlags & kHinting_Mask) >> kHinting_Shift;
            return static_cast<SkPaint::Hinting>(hint);
        }

        void setHinting(SkPaint::Hinting hinting) {
            fFlags = (fFlags & ~kHinting_Mask) | (hinting << kHinting_Shift);
        }
        
        SkMask::Format getFormat() const {
            return static_cast<SkMask::Format>(fMaskFormat);
        }
        
#ifdef SK_USE_COLOR_LUMINANCE
        SkColor getLuminanceColor() const {
            return fLumBits;
        }
        
        void setLuminanceColor(SkColor c) {
            fLumBits = c;
        }
#else
        unsigned getLuminanceBits() const {
            return (fFlags & kLuminance_Mask) >> kLuminance_Shift;
        }
        
        void setLuminanceBits(unsigned lum) {
            SkASSERT(lum <= kLuminance_Max);
            fFlags = (fFlags & ~kLuminance_Mask) | (lum << kLuminance_Shift);
        }

        U8CPU getLuminanceByte() const {
            SkASSERT(3 == kLuminance_Bits);
            unsigned lum = this->getLuminanceBits();
            lum |= (lum << kLuminance_Bits);
            lum |= (lum << kLuminance_Bits*2);
            return lum >> (4*kLuminance_Bits - 8);
        }
#endif
    };

    SkScalerContext(const SkDescriptor* desc);
    virtual ~SkScalerContext();

    SkMask::Format getMaskFormat() const {
        return (SkMask::Format)fRec.fMaskFormat;
    }

    bool isSubpixel() const {
        return SkToBool(fRec.fFlags & kSubpixelPositioning_Flag);
    }
    
    // remember our glyph offset/base
    void setBaseGlyphCount(unsigned baseGlyphCount) {
        fBaseGlyphCount = baseGlyphCount;
    }

    /** Return the corresponding glyph for the specified unichar. Since contexts
        may be chained (under the hood), the glyphID that is returned may in
        fact correspond to a different font/context. In that case, we use the
        base-glyph-count to know how to translate back into local glyph space.
     */
    uint16_t charToGlyphID(SkUnichar uni);

    /** Map the glyphID to its glyph index, and then to its char code. Unmapped
        glyphs return zero.
    */
    SkUnichar glyphIDToChar(uint16_t glyphID);

    unsigned    getGlyphCount() { return this->generateGlyphCount(); }
    void        getAdvance(SkGlyph*);
    void        getMetrics(SkGlyph*);
    void        getImage(const SkGlyph&);
    void        getPath(const SkGlyph&, SkPath*);
    void        getFontMetrics(SkPaint::FontMetrics* mX,
                               SkPaint::FontMetrics* mY);

#ifdef SK_BUILD_FOR_ANDROID
    unsigned getBaseGlyphCount(SkUnichar charCode);
#endif

    static inline void MakeRec(const SkPaint&, const SkMatrix*, Rec* rec);
    static inline void PostMakeRec(Rec*);

    static SkScalerContext* Create(const SkDescriptor*);

protected:
    Rec         fRec;
    unsigned    fBaseGlyphCount;

    virtual unsigned generateGlyphCount() = 0;
    virtual uint16_t generateCharToGlyph(SkUnichar) = 0;
    virtual void generateAdvance(SkGlyph*) = 0;
    virtual void generateMetrics(SkGlyph*) = 0;
    virtual void generateImage(const SkGlyph&) = 0;
    virtual void generatePath(const SkGlyph&, SkPath*) = 0;
    virtual void generateFontMetrics(SkPaint::FontMetrics* mX,
                                     SkPaint::FontMetrics* mY) = 0;
    // default impl returns 0, indicating failure.
    virtual SkUnichar generateGlyphToChar(uint16_t);

    void forceGenerateImageFromPath() { fGenerateImageFromPath = true; }

private:
    SkPathEffect*   fPathEffect;
    SkMaskFilter*   fMaskFilter;
    SkRasterizer*   fRasterizer;

    // if this is set, we draw the image from a path, rather than
    // calling generateImage.
    bool fGenerateImageFromPath;

    void internalGetPath(const SkGlyph& glyph, SkPath* fillPath,
                         SkPath* devPath, SkMatrix* fillToDevMatrix);

    // return the next context, treating fNextContext as a cache of the answer
    SkScalerContext* getNextContext();

    // returns the right context from our link-list for this glyph. If no match
    // is found, just returns the original context (this)
    SkScalerContext* getGlyphContext(const SkGlyph& glyph);

    // link-list of context, to handle missing chars. null-terminated.
    SkScalerContext* fNextContext;
};

#define kRec_SkDescriptorTag            SkSetFourByteTag('s', 'r', 'e', 'c')
#define kPathEffect_SkDescriptorTag     SkSetFourByteTag('p', 't', 'h', 'e')
#define kMaskFilter_SkDescriptorTag     SkSetFourByteTag('m', 's', 'k', 'f')
#define kRasterizer_SkDescriptorTag     SkSetFourByteTag('r', 'a', 's', 't')

///////////////////////////////////////////////////////////////////////////////

enum SkAxisAlignment {
    kNone_SkAxisAlignment,
    kX_SkAxisAlignment,
    kY_SkAxisAlignment
};

/**
 *  Return the axis (if any) that the baseline for horizontal text will land on
 *  after running through the specified matrix.
 *
 *  As an example, the identity matrix will return kX_SkAxisAlignment
 */
SkAxisAlignment SkComputeAxisAlignmentForHText(const SkMatrix& matrix);

#endif

