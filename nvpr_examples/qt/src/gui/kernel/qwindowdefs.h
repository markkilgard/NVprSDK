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

#ifndef QWINDOWDEFS_H
#define QWINDOWDEFS_H

#include <QtCore/qobjectdefs.h>
#include <QtCore/qnamespace.h>

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

// Class forward definitions

class QPaintDevice;
class QWidget;
class QDialog;
class QColor;
class QPalette;
#ifdef QT3_SUPPORT
class QColorGroup;
#endif
class QCursor;
class QPoint;
class QSize;
class QRect;
class QPolygon;
class QPainter;
class QRegion;
class QFont;
class QFontMetrics;
class QFontInfo;
class QPen;
class QBrush;
class QMatrix;
class QPixmap;
class QBitmap;
class QMovie;
class QImage;
class QPicture;
class QPrinter;
class QTimer;
class QTime;
class QClipboard;
class QString;
class QByteArray;
class QApplication;

template<typename T> class QList;
typedef QList<QWidget *> QWidgetList;

QT_END_NAMESPACE
QT_END_HEADER

// Window system dependent definitions

#if defined(Q_WS_MAC) && !defined(Q_WS_QWS)

#include <QtGui/qmacdefines_mac.h>

#ifdef Q_WS_MAC32
typedef int WId;
#else
typedef long WId;
#endif

#endif // Q_WS_MAC

#if defined(Q_WS_WIN)
#include <QtGui/qwindowdefs_win.h>
#endif // Q_WS_WIN

#if defined(Q_WS_X11)

typedef struct _XDisplay Display;
typedef union  _XEvent XEvent;
typedef struct _XGC *GC;
typedef struct _XRegion *Region;
typedef unsigned long  WId;

#endif // Q_WS_X11

#if defined(Q_WS_QWS)

typedef unsigned long  WId;
QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE
struct QWSEvent;
QT_END_NAMESPACE
QT_END_HEADER

#endif // Q_WS_QWS

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

template<class K, class V> class QHash;
typedef QHash<WId, QWidget *> QWidgetMapper;

template<class V> class QSet;
typedef QSet<QWidget *> QWidgetSet;

QT_END_NAMESPACE
QT_END_HEADER

#if defined(QT_NEEDS_QMAIN)
#define main qMain
#endif

// Global platform-independent types and functions

#endif // QWINDOWDEFS_H
