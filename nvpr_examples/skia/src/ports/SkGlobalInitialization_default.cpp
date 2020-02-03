/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
 
#include "SkTypes.h"

#if !SK_ALLOW_STATIC_GLOBAL_INITIALIZERS

#include "SkBitmapProcShader.h"
#include "SkFlipPixelRef.h"
#include "SkImageRef_ashmem.h"
#include "SkImageRef_GlobalPool.h"
#include "SkMallocPixelRef.h"
#include "SkPathEffect.h"
#include "SkPixelRef.h"
#include "SkShape.h"
#include "SkXfermode.h"

#include "Sk1DPathEffect.h"
#include "Sk2DPathEffect.h"
#include "SkAvoidXfermode.h"
#include "SkBlurDrawLooper.h"
#include "SkBlurImageFilter.h"
#include "SkBlurMaskFilter.h"
#include "SkColorFilter.h"
#include "SkColorMatrixFilter.h"
#include "SkColorShader.h"
#include "SkCornerPathEffect.h"
#include "SkDashPathEffect.h"
#include "SkDiscretePathEffect.h"
#include "SkEmptyShader.h"
#include "SkFlattenable.h"
#include "SkGradientShader.h"
#include "SkGroupShape.h"
#include "SkLayerDrawLooper.h"
#include "SkLayerRasterizer.h"
#include "SkMorphologyImageFilter.h"
#include "SkPathEffect.h"
#include "SkPixelXorXfermode.h"
#include "SkRectShape.h"
#include "SkTableColorFilter.h"
#include "SkTestImageFilters.h"

void SkFlattenable::InitializeFlattenables() {

    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkAvoidXfermode)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkBitmapProcShader)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkBlurDrawLooper)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkBlurImageFilter)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkColorMatrixFilter)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkColorShader)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkComposePathEffect)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkCornerPathEffect)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkDashPathEffect)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkDilateImageFilter)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkDiscretePathEffect)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkEmptyShader)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkErodeImageFilter)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkGroupShape)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkLayerDrawLooper)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkLayerRasterizer)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkPath1DPathEffect)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkPath2DPathEffect)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkPixelXorXfermode)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkRectShape)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkStrokePathEffect)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkSumPathEffect)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkShape)

    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkOffsetImageFilter)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkComposeImageFilter)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkMergeImageFilter)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkColorFilterImageFilter)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkDownSampleImageFilter)

    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkFlipPixelRef)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkImageRef_GlobalPool)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkMallocPixelRef)

    SkBlurMaskFilter::InitializeFlattenables();
    SkColorFilter::InitializeFlattenables();
    SkGradientShader::InitializeFlattenables();
    SkTableColorFilter::InitializeFlattenables();
    SkXfermode::InitializeFlattenables();
}

#endif
