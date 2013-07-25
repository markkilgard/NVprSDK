
/* scene_qt.hpp - Qt renderer usage of path rendering scene graph. */

#ifndef __scene_qt_hpp__
#define __scene_qt_hpp__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "nvpr_svg_config.h"  // configure path renderers to use

#if USE_QT

#include "scene.hpp"
#include "scene_qt.hpp"
#include "renderer_qt.hpp"

// Qt 4.5 headers
#include <QBrush>  // http://doc.trolltech.com/4.5/qbrush.html
#include <QPen>  // http://doc.trolltech.com/4.5/qbrush.html
#include <QImage>  // http://doc.trolltech.com/4.5/qimage.html
#include <QTransform>  // http://doc.trolltech.com/4.5/qtransform.html
#include <QPainter>  // http://doc.trolltech.com/4.5/qpainter.html
#include <QPainterPath>  // http://doc.trolltech.com/4.5/qpainterpath.html

namespace QtVisitors {

    class Draw : public MatrixSaveVisitor {
    public:
        Draw(QtRendererPtr renderer_);

        void visit(ShapePtr shape);

        void apply(TransformPtr transform);
        void unapply(TransformPtr transform);

        void apply(ClipPtr clip);
        void unapply(ClipPtr clip);

    protected:
        QtRendererPtr renderer;
    };

    class ClipVisitor : public MatrixSaveVisitor {
    public:
        ClipVisitor(QtRendererPtr renderer_) : renderer(renderer_) { }

        void visit(ShapePtr shape);

        void apply(ClipPtr clip);
        void unapply(ClipPtr clip);

        void startClipping(ClipMerge clip_merge);

    protected:
        QtRendererPtr renderer;
        vector<QPainterPath> paths;
    };
    typedef shared_ptr<ClipVisitor> ClipVisitorPtr;


};

#endif // USE_QT

#endif // __scene_qt_hpp__
