
/* scene_openvg.hpp - OpenVG renderer usage of path rendering scene graph. */

#ifndef __scene_openvg_hpp__
#define __scene_openvg_hpp__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "nvpr_svg_config.h"  // configure path renderers to use

#include "scene.hpp"
#include "renderer_openvg.hpp"

#if USE_OPENVG

struct VGmatrix {
    VGfloat m[9];
    
    VGmatrix() {}
    VGmatrix(const float4x4 &src);
    float4x4 glMatrix();
};

namespace VGVisitors {

    class Draw : public MatrixSaveVisitor {
    public:
        Draw(VGRendererPtr renderer_);

        void visit(ShapePtr shape);

        void apply(TransformPtr transform);
        void unapply(TransformPtr transform);

    protected:
        VGRendererPtr renderer;
    };


};

#endif // USE_OPENVG

#endif // __scene_openvg_hpp__
