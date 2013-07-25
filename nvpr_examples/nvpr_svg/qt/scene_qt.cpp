
/* scene_qt.cpp - Qt renderer usage of path rendering scene graph. */

// Copyright (c) NVIDIA Corporation. All rights reserved.

#include "nvpr_svg_config.h"  // configure path renderers to use

#include "scene_qt.hpp"

#if USE_QT

void transformQtPath(QPainterPath &path, float4x4 &matrix)
{
    for (int j = 0; j < path.elementCount(); j++) {
        QPainterPath::Element elem = path.elementAt(j);
        float2 point(elem.x, elem.y);
        path.setElementPositionAt(j, 
            dot(point, matrix[0].xy) + matrix[0].w,
            dot(point, matrix[1].xy) + matrix[1].w);
    }
}

inline QTransform gl2qt(const float4x4 &m)
{
    return QTransform(
        m[0][0], m[1][0],  // xx, xy
        m[0][1], m[1][1],  // yx, yy
        m[0][3], m[1][3]); // x0, y0
}

inline float4x4 qt2gl(const QTransform &m)
{
    return float4x4(
        m.m11(), m.m21(), 0, m.m31(),
        m.m12(), m.m22(), 0, m.m32(),
        0, 0, 1, 0,
        m.m13(), m.m23(), 0, m.m33());
}

namespace QtVisitors {

///////////////////////////////////////////////////////////////////////////////
// Draw
Draw::Draw(QtRendererPtr renderer_) : renderer(renderer_)
{
    matrix_stack.pop();
    matrix_stack.push(qt2gl(renderer->painter->transform()));
}

void Draw::visit(ShapePtr shape)
{
    ShapeRendererStatePtr renderer_state = shape->getRendererState(renderer);
    QtShapeRendererStatePtr shape_renderer = dynamic_pointer_cast<QtShapeRendererState>(renderer_state);
    shape_renderer->draw();
}

void Draw::apply(TransformPtr transform)
{
    MatrixSaveVisitor::apply(transform);
    renderer->painter->setTransform(gl2qt(matrix_stack.top()));
}

void Draw::unapply(TransformPtr transform)
{
    MatrixSaveVisitor::unapply(transform);
    renderer->painter->setTransform(gl2qt(matrix_stack.top()));
}

void Draw::apply(ClipPtr clip)
{
    renderer->painter->save();
    ClipVisitorPtr clip_visitor(new ClipVisitor(renderer));
    clip->path->traverse(clip_visitor);
    clip_visitor->startClipping(clip->clip_merge);
}

void Draw::unapply(ClipPtr clip)
{
    renderer->painter->restore();
}

///////////////////////////////////////////////////////////////////////////////
// ClipVisitor
void ClipVisitor::visit(ShapePtr shape)
{
    vector<QPainterPath*> temp_paths;
    ShapeRendererStatePtr renderer_state = shape->getRendererState(renderer);
    QtShapeRendererStatePtr shape_renderer = dynamic_pointer_cast<QtShapeRendererState>(renderer_state);
    
    shape_renderer->getPaths(temp_paths);

    for (size_t i = 0; i < temp_paths.size(); i++) {
        QPainterPath path(*temp_paths[i]);
        transformQtPath(path, matrix_stack.top());
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
    QPainterPath path;
    for (size_t i = 0; i < paths.size(); i++) {
        path.addPath(paths[i]);
    }

    path.setFillRule(clip_merge == SUM_WINDING_NUMBERS ? Qt::WindingFill : Qt::OddEvenFill);
    renderer->painter->setClipPath(path, Qt::IntersectClip);
}


};

#endif // USE_QT
