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

#ifndef REMOTEGDBSERVERADAPTER_H
#define REMOTEGDBSERVERADAPTER_H

#include "gdbengine.h"

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// RemoteGdbAdapter
//
///////////////////////////////////////////////////////////////////////

class GdbRemoteServerEngine : public GdbEngine
{
    Q_OBJECT

public:
    explicit GdbRemoteServerEngine(const DebuggerStartParameters &startParameters);

private:
    void setupEngine();
    void setupInferior();
    void runEngine();
    void interruptInferior2();
    void shutdownEngine();

signals:
    /*
     * For "external" clients of a debugger run control that need to do
     * further setup before the debugger is started (e.g. RemoteLinux).
     * Afterwards, handleSetupDone() or handleSetupFailed() must be called
     * to continue or abort debugging, respectively.
     * This signal is only emitted if the start parameters indicate that
     * a server start script should be used, but none is given.
     */
    void requestSetup();

private:
    Q_SLOT void readUploadStandardOutput();
    Q_SLOT void readUploadStandardError();
    Q_SLOT void uploadProcError(QProcess::ProcessError error);
    Q_SLOT void uploadProcFinished();
    Q_SLOT void callTargetRemote();

    void notifyEngineRemoteServerRunning(const QByteArray &serverChannel, int inferiorPid);
    void notifyEngineRemoteSetupFinished(const RemoteSetupResult &result);

    void handleSetTargetAsync(const DebuggerResponse &response);
    void handleFileExecAndSymbols(const DebuggerResponse &response);
    void handleTargetRemote(const DebuggerResponse &response);
    void handleTargetExtendedRemote(const DebuggerResponse &response);
    void handleTargetExtendedAttach(const DebuggerResponse &response);
    void handleTargetQnx(const DebuggerResponse &response);
    void handleAttach(const DebuggerResponse &response);
    void handleSetNtoExecutable(const DebuggerResponse &response);
    void handleInterruptInferior(const DebuggerResponse &response);
    void handleExecRun(const DebuggerResponse &response);

    QProcess m_uploadProc;
    bool m_startAttempted;
};

} // namespace Internal
} // namespace Debugger

#endif // REMOTEGDBSERVERADAPTER_H
