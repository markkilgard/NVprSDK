
/* scene_cairo.hpp - path rendering scene graph. */

#ifndef __scene_cairo_hpp__
#define __scene_cairo_hpp__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "nvpr_svg_config.h"  // configure path renderers to use

#include "scene.hpp"

#if USE_CAIRO

#include "renderer_cairo.hpp"

// Cairo graphics library
#include <cairo.h>

namespace CairoVisitors {

    typedef shared_ptr<class ClipMask> ClipMaskPtr;
    class ClipMask {
    public:
        ClipMask(cairo_t *cr_, cairo_pattern_t *pattern_) : cr(cr_), pattern(pattern_) { }
        ~ClipMask() { cairo_pattern_destroy(pattern); }

        void intersectWith(ClipMaskPtr mask);
        void apply();

    protected:
        cairo_t *cr;
        cairo_pattern_t *pattern;
    };

    class Draw : public MatrixSaveVisitor
    {
    public:
        Draw(CairoRendererPtr renderer_);

        void visit(ShapePtr shape);

        void apply(TransformPtr transform);
        void unapply(TransformPtr transform);

        void apply(ClipPtr clip);
        void unapply(ClipPtr clip);

    protected:
        CairoRendererPtr renderer;

        struct ClipPath {
            ClipMaskPtr mask;
            cairo_matrix_t matrix;
            ClipPath() : mask(ClipMaskPtr()) { }
            ClipPath(ClipMaskPtr mask_, const cairo_matrix_t &matrix_) 
                : mask(mask_), matrix(matrix_) { }
        };
        stack<ClipPath> clip_stack;
    };

    class ClipVisitor : public MatrixSaveVisitor
    {
    public:
        ClipVisitor(CairoRendererPtr renderer_) 
            : renderer(renderer_), child_mask(ClipMaskPtr()) { }
        ~ClipVisitor();

        void visit(ShapePtr shape);

        void apply(ClipPtr clip);
        void unapply(ClipPtr clip);

        ClipMaskPtr startClipping(ClipMerge clip_merge);

    protected:
        CairoRendererPtr renderer;
        vector<cairo_path_t*> paths;
        ClipMaskPtr child_mask;
    };
    typedef shared_ptr<ClipVisitor> ClipVisitorPtr;


};

#endif // USE_CAIRO

#endif // __scene_cairo_hpp__
