
/* scene_skia.hpp - Skia renderer usage of path rendering scene graph. */

#ifndef __scene_skia_hpp__
#define __scene_skia_hpp__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "nvpr_svg_config.h"  // configure path renderers to use

#include "scene.hpp"
#include "scene_skia.hpp"
#include "renderer_skia.hpp"

#if USE_SKIA

namespace SkiaVisitors {

    class Draw : public Visitor {
    public:
        Draw(SkiaRendererPtr renderer_) : renderer(renderer_) { }

        void visit(ShapePtr shape);

        void apply(TransformPtr transform);
        void unapply(TransformPtr transform);

        void apply(ClipPtr clip);
        void unapply(ClipPtr clip);

    protected:
        SkiaRendererPtr renderer;
    };

    class ClipVisitor : public MatrixSaveVisitor {
    public:
        ClipVisitor(SkiaRendererPtr renderer_) : renderer(renderer_) { }

        void visit(ShapePtr shape);

        void apply(ClipPtr clip);
        void unapply(ClipPtr clip);

        void startClipping(ClipMerge clip_merge);

    protected:
        SkiaRendererPtr renderer;
        vector<SkPath> paths;
    };
    typedef shared_ptr<ClipVisitor> ClipVisitorPtr;


};


#endif  // USE_SKIA

#endif // __scene_skia_hpp__
