
/* scene_d2d.hpp - path rendering scene graph. */

#ifndef __scene_d2d_hpp__
#define __scene_d2d_hpp__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "nvpr_svg_config.h"  // configure path renderers to use

#include "scene.hpp"

#if USE_D2D

#include "renderer_d2d.hpp"

namespace D2DVisitors {

    typedef shared_ptr<class ClipVisitor> ClipVisitorPtr;

    class Draw : public MatrixSaveVisitor
    {
    public:
        Draw(D2DRendererPtr renderer_);

        void visit(ShapePtr shape);

        void apply(TransformPtr transform);
        void unapply(TransformPtr transform);

        void apply(ClipPtr clip);
        void unapply(ClipPtr clip);

    protected:
        D2DRendererPtr renderer;
        stack<ClipVisitorPtr> clip_stack;
    };

    class ClipVisitor : public MatrixSaveVisitor
    {
    public:
        ClipVisitor(D2DRendererPtr renderer_);
        ~ClipVisitor();

        void visit(ShapePtr shape);

        void apply(ClipPtr clip);
        void unapply(ClipPtr clip);

    public:
        void startClipping(ClipMerge clip_merge);
        void stopClipping();

    protected:
        vector<ID2D1Geometry*> geometry;
        D2DRendererPtr renderer;
        ClipVisitorPtr child;
        ID2D1Layer *layer;
    };


};

#endif // USE_D2D

#endif // __scene_d2d_hpp__
