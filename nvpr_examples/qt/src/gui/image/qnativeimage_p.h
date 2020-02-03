/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://www.qtsoftware.com/contact.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QNATIVEIMAGE_P_H
#define QNATIVEIMAGE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qimage.h"

#ifdef Q_WS_WIN
#include "qt_windows.h"

#elif defined(Q_WS_X11)
#include <private/qt_x11_p.h>

#elif defined(Q_WS_MAC)
#include <private/qt_mac_p.h>

#endif

QT_BEGIN_NAMESPACE

class QWidget;

class Q_GUI_EXPORT QNativeImage
{
public:
    QNativeImage(int width, int height, QImage::Format format, bool isTextBuffer = false, QWidget *widget = 0);
    ~QNativeImage();

    inline int width() const;
    inline int height() const;

    QImage image;

    static QImage::Format systemFormat();

#ifdef Q_WS_WIN
    HDC hdc;
    HBITMAP bitmap;
    HBITMAP null_bitmap;

#elif defined(Q_WS_X11) && !defined(QT_NO_MITSHM)
    XImage *xshmimg;
    Pixmap xshmpm;
    XShmSegmentInfo xshminfo;

#elif defined(Q_WS_MAC)
    CGContextRef cg;
    CGColorSpaceRef cgColorSpace;
#endif

private:
    Q_DISABLE_COPY(QNativeImage)
};

inline int QNativeImage::width() const { return image.width(); }
inline int QNativeImage::height() const { return image.height(); }

QT_END_NAMESPACE

#endif // QNATIVEIMAGE_P_H
