/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGERRUNCONTROL_H
#define DEBUGGERRUNCONTROL_H

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <projectexplorer/runconfiguration.h>

namespace ProjectExplorer { class Kit;  }

namespace Debugger {

class RemoteSetupResult;

namespace Internal { class DebuggerEngine; }

class DebuggerStartParameters;

class DEBUGGER_EXPORT DebuggerRunControl
    : public ProjectExplorer::RunControl
{
    Q_OBJECT

public:
    ~DebuggerRunControl();

    // ProjectExplorer::RunControl
    void start();
    bool promptToStop(bool *prompt = 0) const;
    StopResult stop(); // Called from SnapshotWindow.
    bool isRunning() const;
    QString displayName() const;

    void startFailed();
    void notifyEngineRemoteServerRunning(const QByteArray &msg, int pid);
    void notifyEngineRemoteSetupFinished(const RemoteSetupResult &result);
    void notifyInferiorIll();
    Q_SLOT void notifyInferiorExited();
    void quitDebugger();
    void abortDebugger();
    void debuggingFinished();

    void showMessage(const QString &msg, int channel = LogDebug);

    DebuggerStartParameters &startParameters();

signals:
    void requestRemoteSetup();
    void aboutToNotifyInferiorSetupOk();
    void stateChanged(Debugger::DebuggerState state);

private:
    void handleFinished();
    friend class DebuggerRunControlFactory;
    DebuggerRunControl(ProjectExplorer::RunConfiguration *runConfiguration,
                       Internal::DebuggerEngine *engine);

    Internal::DebuggerEngine *m_engine;
    bool m_running;
};

class DEBUGGER_EXPORT DebuggerRunControlFactory
    : public ProjectExplorer::IRunControlFactory
{
public:
    explicit DebuggerRunControlFactory(QObject *parent);

    // FIXME: Used by qmljsinspector.cpp:469
    ProjectExplorer::RunControl *create(
        ProjectExplorer::RunConfiguration *runConfiguration,
        ProjectExplorer::RunMode mode,
        QString *errorMessage);

    bool canRun(ProjectExplorer::RunConfiguration *runConfiguration,
        ProjectExplorer::RunMode mode) const;

    static Internal::DebuggerEngine *createEngine(DebuggerEngineType et,
        const DebuggerStartParameters &sp,
        QString *errorMessage);

    static bool fillParametersFromKit(DebuggerStartParameters *sp,
         const ProjectExplorer::Kit *kit, QString *errorMessage = 0);

    static bool fillParametersFromLocalRunConfiguration(DebuggerStartParameters *sp,
         const ProjectExplorer::RunConfiguration *rc, QString *errorMessage = 0);

    static DebuggerRunControl *createAndScheduleRun(const DebuggerStartParameters &sp);

    static DebuggerRunControl *doCreate(const DebuggerStartParameters &sp, QString *errorMessage);

    ProjectExplorer::IRunConfigurationAspect *createRunConfigurationAspect(
            ProjectExplorer::RunConfiguration *rc);
};

} // namespace Debugger

#endif // DEBUGGERRUNCONTROL_H
