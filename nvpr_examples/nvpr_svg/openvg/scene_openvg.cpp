
/* scene_openvg.cpp - OpenVG renderer usage of path rendering scene graph. */

// Copyright (c) NVIDIA Corporation. All rights reserved.

#include "nvpr_svg_config.h"  // configure path renderers to use

#include "scene_openvg.hpp"

#if USE_OPENVG

VGmatrix::VGmatrix(const float4x4 &src)
{
    m[0] = src[0].x; m[1] = src[1].x; m[2] = src[2].x;
    m[3] = src[0].y; m[4] = src[1].y; m[5] = src[2].y;
    m[6] = src[0].w; m[7] = src[1].w; m[8] = src[2].w;
}

float4x4 VGmatrix::glMatrix()
{
    return float4x4(m[0], m[3], 0, m[6],
                    m[1], m[4], 0, m[7],
                    0, 0, 1, 0,
                    m[2], m[5], 0, m[8]);
}

namespace VGVisitors {

Draw::Draw(VGRendererPtr renderer_) : renderer(renderer_)
{
    VGmatrix current_matrix;
    matrix_stack.pop();
    vgGetMatrix(current_matrix.m);
    matrix_stack.push(current_matrix.glMatrix());
}

void Draw::visit(ShapePtr shape)
{
    ShapeRendererStatePtr renderer_state = shape->getRendererState(renderer);
    VGShapeRendererStatePtr shape_renderer = dynamic_pointer_cast<VGShapeRendererState>(renderer_state);
    shape_renderer->draw();
}

void Draw::apply(TransformPtr transform)
{
    MatrixSaveVisitor::apply(transform);
    vgLoadMatrix(VGmatrix(matrix_stack.top()).m);
}

void Draw::unapply(TransformPtr transform)
{
    MatrixSaveVisitor::unapply(transform);
    vgLoadMatrix(VGmatrix(matrix_stack.top()).m);
}


};


#endif
