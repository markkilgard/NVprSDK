/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QEVENTDISPATCHER_GLIB_P_H
#define QEVENTDISPATCHER_GLIB_P_H

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

#include "qabstracteventdispatcher.h"
#include "qabstracteventdispatcher_p.h"

#include <QtCore/qhash.h>

typedef struct _GMainContext GMainContext;

QT_BEGIN_NAMESPACE

class QEventDispatcherGlibPrivate;

class Q_CORE_EXPORT QEventDispatcherGlib : public QAbstractEventDispatcher
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QEventDispatcherGlib)

public:
    explicit QEventDispatcherGlib(QObject *parent = 0);
    explicit QEventDispatcherGlib(GMainContext *context, QObject *parent = 0);
    ~QEventDispatcherGlib();

    bool processEvents(QEventLoop::ProcessEventsFlags flags);
    bool hasPendingEvents();

    void registerSocketNotifier(QSocketNotifier *socketNotifier);
    void unregisterSocketNotifier(QSocketNotifier *socketNotifier);

    void registerTimer(int timerId, int interval, QObject *object);
    bool unregisterTimer(int timerId);
    bool unregisterTimers(QObject *object);
    QList<TimerInfo> registeredTimers(QObject *object) const;

    void wakeUp();
    void interrupt();
    void flush();

    static bool versionSupported();

protected:
    QEventDispatcherGlib(QEventDispatcherGlibPrivate &dd, QObject *parent);
};

struct GPostEventSource;
struct GSocketNotifierSource;
struct GTimerSource;

class Q_CORE_EXPORT QEventDispatcherGlibPrivate : public QAbstractEventDispatcherPrivate
{

public:
    QEventDispatcherGlibPrivate(GMainContext *context = 0);
    GMainContext *mainContext;
    GPostEventSource *postEventSource;
    GSocketNotifierSource *socketNotifierSource;
    GTimerSource *timerSource;
};

QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_GLIB_P_H
