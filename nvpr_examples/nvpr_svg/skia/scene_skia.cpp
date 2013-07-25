
/* scene_skia.cpp - Skia renderer usage of path rendering scene graph. */

// Copyright (c) NVIDIA Corporation. All rights reserved.

#include "nvpr_svg_config.h"  // configure path renderers to use

#if USE_SKIA

#include "scene_skia.hpp"

inline SkMatrix gl2skia(const float4x4 &m)
{
    SkMatrix sk_matrix;

    // First (X) row
    sk_matrix[0] = m[0][0];
    sk_matrix[1] = m[0][1];
    sk_matrix[2] = m[0][3];
    // Second (Y) row
    sk_matrix[3] = m[1][0];
    sk_matrix[4] = m[1][1];
    sk_matrix[5] = m[1][3];
    // Last (projective) row
    sk_matrix[6] = m[3][0];
    sk_matrix[7] = m[3][1];
    sk_matrix[8] = m[3][3];

    return sk_matrix;
}

namespace SkiaVisitors {

///////////////////////////////////////////////////////////////////////////////
// Draw
void Draw::visit(ShapePtr shape)
{
    ShapeRendererStatePtr renderer_state = shape->getRendererState(renderer);
    SkiaShapeRendererStatePtr shape_renderer = dynamic_pointer_cast<SkiaShapeRendererState>(renderer_state);
    shape_renderer->draw();
}

void Draw::apply(TransformPtr transform)
{
    renderer->canvas.save(SkCanvas::kMatrix_SaveFlag);
    renderer->canvas.concat(gl2skia(transform->getMatrix()));
}

void Draw::unapply(TransformPtr transform)
{
    renderer->canvas.restore();
}

void Draw::apply(ClipPtr clip)
{
    renderer->canvas.save(SkCanvas::kClip_SaveFlag);
    ClipVisitorPtr clip_visitor(new ClipVisitor(renderer));
    clip->path->traverse(clip_visitor);
    clip_visitor->startClipping(clip->clip_merge);
}

void Draw::unapply(ClipPtr clip)
{
    renderer->canvas.restore();
}

///////////////////////////////////////////////////////////////////////////////
// ClipVisitor
void ClipVisitor::visit(ShapePtr shape)
{
    ShapeRendererStatePtr renderer_state = shape->getRendererState(renderer);
    SkiaShapeRendererStatePtr shape_renderer = dynamic_pointer_cast<SkiaShapeRendererState>(renderer_state);

    vector<SkPath*> temp_paths;
    shape_renderer->getPaths(temp_paths);

    SkMatrix matrix = gl2skia(matrix_stack.top());
    for (size_t i = 0; i < temp_paths.size(); i++) {
        SkPath path(*temp_paths[i]);
        path.transform(matrix);
        paths.push_back(path);
    }
}

void ClipVisitor::apply(ClipPtr clip)
{
    ClipVisitorPtr clip_visitor(new ClipVisitor(renderer));
    clip->path->traverse(clip_visitor);
    clip_visitor->startClipping(clip->clip_merge);
}

void ClipVisitor::unapply(ClipPtr clip)
{
    // ignore
}

void ClipVisitor::startClipping(ClipMerge clip_merge)
{
    SkPath path;
    for (size_t i = 0; i < paths.size(); i++) {
        path.addPath(paths[i]);
    }

    path.setFillType(clip_merge == SUM_WINDING_NUMBERS ? 
        SkPath::kWinding_FillType : SkPath::kEvenOdd_FillType);
    renderer->canvas.clipPath(path, SkRegion::kIntersect_Op);
}


};

#endif  // USE_SKIA
