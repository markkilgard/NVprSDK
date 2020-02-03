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

#ifndef QWINDOWSURFACE_RASTER_P_H
#define QWINDOWSURFACE_RASTER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <qglobal.h>
#include "private/qwindowsurface_p.h"

QT_BEGIN_NAMESPACE

#ifdef Q_WS_WIN
#define Q_WS_EX_LAYERED           0x00080000 // copied from WS_EX_LAYERED in winuser.h
#define Q_LWA_ALPHA               0x00000002 // copied from LWA_ALPHA in winuser.h
#define Q_ULW_ALPHA               0x00000002 // copied from ULW_ALPHA in winuser.h
#define Q_AC_SRC_ALPHA            0x00000001 // copied from AC_SRC_ALPHA in winuser.h

struct Q_BLENDFUNCTION {
  BYTE     BlendOp;
  BYTE     BlendFlags;
  BYTE     SourceConstantAlpha;
  BYTE     AlphaFormat;
};

struct Q_UPDATELAYEREDWINDOWINFO {
    DWORD cbSize;
    HDC hdcDst;
    const POINT *pptDst;
    const SIZE *psize;
    HDC hdcSrc;
    const POINT *pptSrc;
    COLORREF crKey;
    const Q_BLENDFUNCTION *pblend;
    DWORD dwFlags;
    const RECT *prcDirty;
};

typedef BOOL (WINAPI *PtrUpdateLayeredWindowIndirect)(HWND hwnd, const Q_UPDATELAYEREDWINDOWINFO *pULWInfo);
extern PtrUpdateLayeredWindowIndirect ptrUpdateLayeredWindowIndirect;
#endif

class QPaintDevice;
class QPoint;
class QRegion;
class QRegion;
class QSize;
class QWidget;
class QRasterWindowSurfacePrivate;
class QNativeImage;

class Q_GUI_EXPORT QRasterWindowSurface : public QWindowSurface
{
public:
    QRasterWindowSurface(QWidget *widget);
    ~QRasterWindowSurface();

    QPaintDevice *paintDevice();
    void flush(QWidget *widget, const QRegion &region, const QPoint &offset);
    void beginPaint(const QRegion &rgn);
    void setGeometry(const QRect &rect);
    bool scroll(const QRegion &area, int dx, int dy);

private:
    void prepareBuffer(QImage::Format format, QWidget *widget);
    Q_DECLARE_PRIVATE(QRasterWindowSurface)
    QRasterWindowSurfacePrivate *d_ptr;
};

QT_END_NAMESPACE

#endif // QWINDOWSURFACE_RASTER_P_H
