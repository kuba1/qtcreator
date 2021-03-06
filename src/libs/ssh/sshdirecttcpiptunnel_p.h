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
#ifndef SSHDIRECTTCPIPTUNNEL_P_H
#define SSHDIRECTTCPIPTUNNEL_P_H

#include "sshchannel_p.h"

#include "sshconnection.h"

namespace QSsh {
class SshDirectTcpIpTunnel;

namespace Internal {

class SshDirectTcpIpTunnelPrivate : public AbstractSshChannel
{
    Q_OBJECT

    friend class QSsh::SshDirectTcpIpTunnel;

public:
    explicit SshDirectTcpIpTunnelPrivate(quint32 channelId, quint16 remotePort,
            const SshConnectionInfo &connectionInfo, SshSendFacility &sendFacility);

signals:
    void initialized();
    void readyRead();
    void error(const QString &reason);
    void closed();

private slots:
    void handleEof();

private:
    void handleChannelSuccess();
    void handleChannelFailure();

    void handleOpenSuccessInternal();
    void handleOpenFailureInternal(const QString &reason);
    void handleChannelDataInternal(const QByteArray &data);
    void handleChannelExtendedDataInternal(quint32 type, const QByteArray &data);
    void handleExitStatus(const SshChannelExitStatus &exitStatus);
    void handleExitSignal(const SshChannelExitSignal &signal);

    void closeHook();

    const quint16 m_remotePort;
    const SshConnectionInfo m_connectionInfo;
    QByteArray m_data;
};

} // namespace Internal
} // namespace QSsh

#endif // SSHDIRECTTCPIPTUNNEL_P_H
