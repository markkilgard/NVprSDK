
/* scene_d2d.cpp - D2D renderer usage of path rendering scene graph. */

// Copyright (c) NVIDIA Corporation. All rights reserved.

#include "nvpr_svg_config.h"  // configure path renderers to use

#include "scene_d2d.hpp"

#if USE_D2D

inline D2D1::Matrix3x2F d2dMatrix(const float4x4 &m)
{
    return D2D1::Matrix3x2F(
        m[0][0], m[1][0], 
        m[0][1], m[1][1], 
        m[0][3], m[1][3]
    );
}

namespace D2DVisitors {

///////////////////////////////////////////////////////////////////////////////
// Draw
Draw::Draw(D2DRendererPtr renderer_) 
    : renderer(renderer_)
{
    matrix_stack.pop();
    matrix_stack.push(float4x4(
        renderer->m_mViewMatrix._11, renderer->m_mViewMatrix._21, 0, renderer->m_mViewMatrix._31,
        renderer->m_mViewMatrix._12, renderer->m_mViewMatrix._22, 0, renderer->m_mViewMatrix._32,
        0,0,1,0,
        0,0,0,1));
}

void Draw::visit(ShapePtr shape)
{
    ShapeRendererStatePtr renderer_state = shape->getRendererState(renderer);
    D2DShapeRendererStatePtr shape_renderer = dynamic_pointer_cast<D2DShapeRendererState>(renderer_state);
    shape_renderer->draw();
}

void Draw::apply(TransformPtr transform)
{
    MatrixSaveVisitor::apply(transform);
    renderer->m_pRenderTarget->SetTransform(d2dMatrix(matrix_stack.top()));
}

void Draw::unapply(TransformPtr transform)
{
    MatrixSaveVisitor::unapply(transform);
    renderer->m_pRenderTarget->SetTransform(d2dMatrix(matrix_stack.top()));
}

void Draw::apply(ClipPtr clip)
{
    clip_stack.push(ClipVisitorPtr(new ClipVisitor(renderer)));
    clip->path->traverse(clip_stack.top());
    clip_stack.top()->startClipping(clip->clip_merge);
}

void Draw::unapply(ClipPtr clip)
{
    clip_stack.top()->stopClipping();
    clip_stack.pop();
}


///////////////////////////////////////////////////////////////////////////////
// ClipVisitor
ClipVisitor::ClipVisitor(D2DRendererPtr renderer_) 
    : renderer(renderer_)
    , layer(NULL)
    , child(ClipVisitorPtr())
{
}

ClipVisitor::~ClipVisitor()
{
    assert(!layer);
    assert(!geometry.size());
}

void ClipVisitor::visit(ShapePtr shape)
{
    ShapeRendererStatePtr renderer_state = shape->getRendererState(renderer);
    D2DShapeRendererStatePtr shape_renderer = dynamic_pointer_cast<D2DShapeRendererState>(renderer_state);
    vector<ID2D1Geometry*> temp_geometry;

    shape_renderer->getGeometry(&temp_geometry);

    for (size_t i = 0; i < temp_geometry.size(); i++) {
        ID2D1TransformedGeometry* transformed_geometry;
        HRESULT hr = renderer->m_pFactory->CreateTransformedGeometry(
            temp_geometry[i], &d2dMatrix(matrix_stack.top()), &transformed_geometry);

        if (SUCCEEDED(hr)) {
            geometry.push_back(transformed_geometry);
        }
    }
}

void ClipVisitor::apply(ClipPtr clip)
{
    assert(!child);
    child = ClipVisitorPtr(new ClipVisitor(renderer));
    clip->path->traverse(child);
    child->startClipping(clip->clip_merge);
}

void ClipVisitor::unapply(ClipPtr clip)
{
    // ignore
}

void ClipVisitor::startClipping(ClipMerge clip_merge)
{
    if (!geometry.size()) {
        return;
    }

    ID2D1GeometryGroup *geometryGroup;
    HRESULT hr = renderer->m_pFactory->CreateGeometryGroup(
        clip_merge == SUM_WINDING_NUMBERS ? D2D1_FILL_MODE_WINDING : D2D1_FILL_MODE_ALTERNATE, 
        &geometry[0], UINT(geometry.size()), &geometryGroup);
    if (SUCCEEDED(hr)) {
        assert(!layer);
        HRESULT hr = renderer->m_pRenderTarget->CreateLayer(&layer);
        if (SUCCEEDED(hr)) {
            renderer->m_pRenderTarget->PushLayer(
                D2D1::LayerParameters(
                    D2D1::InfiniteRect(), geometryGroup, 
                    D2D1_ANTIALIAS_MODE_PER_PRIMITIVE, 
                    D2D1::IdentityMatrix()),
                layer);
        }
        geometryGroup->Release();
    }
}

void ClipVisitor::stopClipping()
{
    if (layer) {
        renderer->m_pRenderTarget->PopLayer();
        layer->Release();
        layer = NULL;
    }

    for (size_t i = 0; i < geometry.size(); i++) {
        geometry[i]->Release();
    }
    geometry.clear();

    if (child) {
        child->stopClipping();
    }
}


};

#endif
