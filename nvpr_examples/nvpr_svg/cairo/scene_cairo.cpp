
/* scene_cairo.cpp - Cairo renderer usage of path rendering scene graph. */

// Copyright (c) NVIDIA Corporation. All rights reserved.

#include "nvpr_svg_config.h"  // configure path renderers to use

#include "scene_cairo.hpp"

#if USE_CAIRO

inline cairo_matrix_t gl2cairo(const float4x4 &m)
{
    cairo_matrix_t matrix;
    cairo_matrix_init(&matrix,
        m[0][0], m[1][0],  // xx, xy
        m[0][1], m[1][1],  // yx, yy
        m[0][3], m[1][3]); // x0, y0

    return matrix;
}

inline float4x4 cairo2gl(const cairo_matrix_t &m)
{
    return float4x4(
        m.xx, m.xy, 0, m.x0,
        m.yx, m.yy, 0, m.y0,
        0, 0, 1, 0,
        0, 0, 0, 1);
}

static void transformCairoPath(cairo_path_t *path, float4x4 &matrix)
{
    // Transform the path to match the current transformation matrix of the clip path
    for (int i = 0; i < path->num_data; i += path->data[i].header.length) {
        for (int j = 1; j < path->data[i].header.length; j++) {
            float2 point(path->data[j+i].point.x, path->data[j+i].point.y);
            path->data[j+i].point.x = dot(point, matrix[0].xy) + matrix[0].w;
            path->data[j+i].point.y = dot(point, matrix[1].xy) + matrix[1].w;
        }
    }
}

namespace CairoVisitors {

///////////////////////////////////////////////////////////////////////////////
// ClipMask
void ClipMask::intersectWith(ClipMaskPtr mask)
{
    cairo_push_group_with_content(cr, CAIRO_CONTENT_ALPHA);
    cairo_set_source(cr, pattern);
    cairo_pattern_destroy(pattern);
    cairo_mask(cr, mask->pattern);
    pattern = cairo_pop_group(cr);
}

void ClipMask::apply()
{
    cairo_mask(cr, pattern);
}

///////////////////////////////////////////////////////////////////////////////
// Draw
Draw::Draw(CairoRendererPtr renderer_) : renderer(renderer_) 
{ 
    cairo_matrix_t m;
    cairo_get_matrix(renderer->cr, &m);
    matrix_stack.pop();
    matrix_stack.push(cairo2gl(m));
    clip_stack.push(ClipPath(ClipMaskPtr(), m));
}

void Draw::visit(ShapePtr shape)
{
    ShapeRendererStatePtr renderer_state = shape->getRendererState(renderer);
    CairoShapeRendererStatePtr shape_renderer = dynamic_pointer_cast<CairoShapeRendererState>(renderer_state);
    
    if (clip_stack.top().mask) {
        cairo_push_group_with_content(renderer->cr, CAIRO_CONTENT_COLOR_ALPHA);
    }
    
    shape_renderer->draw();

    if (clip_stack.top().mask) {
        cairo_pop_group_to_source(renderer->cr);
        cairo_set_matrix(renderer->cr, &clip_stack.top().matrix);
        clip_stack.top().mask->apply();
        cairo_matrix_t m = gl2cairo(matrix_stack.top());
        cairo_set_matrix(renderer->cr, &m);
    }
}

void Draw::apply(TransformPtr transform)
{
    MatrixSaveVisitor::apply(transform);
    cairo_matrix_t m = gl2cairo(matrix_stack.top());
    cairo_set_matrix(renderer->cr, &m);
}

void Draw::unapply(TransformPtr transform)
{
    MatrixSaveVisitor::unapply(transform);
    cairo_matrix_t m = gl2cairo(matrix_stack.top());
    cairo_set_matrix(renderer->cr, &m);
}

void Draw::apply(ClipPtr clip)
{
    cairo_save(renderer->cr);
    ClipVisitorPtr clip_visitor(new ClipVisitor(renderer));
    clip->path->traverse(clip_visitor);
    ClipMaskPtr mask = clip_visitor->startClipping(clip->clip_merge);
    
    if (clip_stack.top().mask) {
        if (mask) {
            mask->intersectWith(clip_stack.top().mask);
        } else {
            mask = clip_stack.top().mask;
        }
    }
    clip_stack.push(ClipPath(mask, gl2cairo(matrix_stack.top())));
}

void Draw::unapply(ClipPtr clip)
{
    clip_stack.pop();
    cairo_restore(renderer->cr);
}

///////////////////////////////////////////////////////////////////////////////
// ClipVisitor
ClipVisitor::~ClipVisitor()
{
    for (size_t i = 0; i < paths.size(); i++) {
        cairo_path_destroy(paths[i]);
    }
}

void ClipVisitor::visit(ShapePtr shape)
{
    cairo_t *cr = renderer->cr;
    ShapeRendererStatePtr renderer_state = shape->getRendererState(renderer);
    CairoShapeRendererStatePtr shape_renderer = dynamic_pointer_cast<CairoShapeRendererState>(renderer_state);
    vector<cairo_path_t*> temp_paths;
    shape_renderer->getPaths(temp_paths);

    for (size_t i = 0; i < temp_paths.size(); i++) {
        cairo_new_path(renderer->cr);
        cairo_append_path(renderer->cr, temp_paths[i]);
        cairo_path_t *path = cairo_copy_path(cr);
        transformCairoPath(path, matrix_stack.top());
        paths.push_back(path);
    }
}

void ClipVisitor::apply(ClipPtr clip)
{
    assert(!child_mask);
    ClipVisitorPtr clip_visitor(new ClipVisitor(renderer));
    clip->path->traverse(clip_visitor);
    child_mask = clip_visitor->startClipping(clip->clip_merge);
}

void ClipVisitor::unapply(ClipPtr clip)
{
    // ignore
}

ClipMaskPtr ClipVisitor::startClipping(ClipMerge clip_merge)
{
    if (clip_merge == CLIP_COVERAGE_UNION && paths.size() > 1) {
        cairo_push_group_with_content(renderer->cr, CAIRO_CONTENT_ALPHA);
        cairo_set_source_rgba(renderer->cr, 1, 1, 1, 1);
        for (size_t i = 0; i < paths.size(); i++) {
            cairo_new_path(renderer->cr);
            cairo_append_path(renderer->cr, paths[i]);
            cairo_fill(renderer->cr);
        }

        ClipMaskPtr mask(new ClipMask(renderer->cr, cairo_pop_group(renderer->cr)));
        if (child_mask) {
            mask->intersectWith(child_mask);
        }
        return mask;
    } else {
        cairo_new_path(renderer->cr);
        for (size_t i = 0; i < paths.size(); i++) {
            cairo_append_path(renderer->cr, paths[i]);
        }

        cairo_set_fill_rule(renderer->cr, 
            clip_merge == SUM_WINDING_NUMBERS_MOD_2 ? 
                            CAIRO_FILL_RULE_EVEN_ODD : 
                            CAIRO_FILL_RULE_WINDING);
        cairo_clip(renderer->cr);
        return child_mask;
    }
}


};


#endif
