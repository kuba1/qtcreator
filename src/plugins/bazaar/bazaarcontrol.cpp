/**************************************************************************
**
** Copyright (C) 2015 Hugues Delorme
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
#include "bazaarcontrol.h"
#include "bazaarclient.h"

#include <vcsbase/vcsbaseconstants.h>

#include <utils/fileutils.h>

#include <QFileInfo>
#include <QVariant>
#include <QStringList>

using namespace Bazaar::Internal;

BazaarControl::BazaarControl(BazaarClient *client)
    : m_bazaarClient(client)
{
}

QString BazaarControl::displayName() const
{
    return tr("Bazaar");
}

Core::Id BazaarControl::id() const
{
    return Core::Id(VcsBase::Constants::VCS_ID_BAZAAR);
}

bool BazaarControl::managesDirectory(const QString &directory, QString *topLevel) const
{
    QFileInfo dir(directory);
    const QString topLevelFound = m_bazaarClient->findTopLevelForFile(dir);
    if (topLevel)
        *topLevel = topLevelFound;
    return !topLevelFound.isEmpty();
}

bool BazaarControl::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    return m_bazaarClient->managesFile(workingDirectory, fileName);
}

bool BazaarControl::isConfigured() const
{
    const Utils::FileName binary = m_bazaarClient->settings()->binaryPath();
    if (binary.isEmpty())
        return false;
    QFileInfo fi = binary.toFileInfo();
    return fi.exists() && fi.isFile() && fi.isExecutable();
}

bool BazaarControl::supportsOperation(Operation operation) const
{
    bool supported = isConfigured();

    switch (operation) {
    case Core::IVersionControl::AddOperation:
    case Core::IVersionControl::DeleteOperation:
    case Core::IVersionControl::MoveOperation:
    case Core::IVersionControl::CreateRepositoryOperation:
    case Core::IVersionControl::AnnotateOperation:
        break;
    case Core::IVersionControl::SnapshotOperations:
        supported = false;
        break;
    }
    return supported;
}

bool BazaarControl::vcsOpen(const QString &filename)
{
    Q_UNUSED(filename)
    return true;
}

bool BazaarControl::vcsAdd(const QString &filename)
{
    const QFileInfo fi(filename);
    return m_bazaarClient->synchronousAdd(fi.absolutePath(), fi.fileName());
}

bool BazaarControl::vcsDelete(const QString &filename)
{
    const QFileInfo fi(filename);
    return m_bazaarClient->synchronousRemove(fi.absolutePath(), fi.fileName());
}

bool BazaarControl::vcsMove(const QString &from, const QString &to)
{
    const QFileInfo fromInfo(from);
    const QFileInfo toInfo(to);
    return m_bazaarClient->synchronousMove(fromInfo.absolutePath(),
                                           fromInfo.absoluteFilePath(),
                                           toInfo.absoluteFilePath());
}

bool BazaarControl::vcsCreateRepository(const QString &directory)
{
    return m_bazaarClient->synchronousCreateRepository(directory);
}

bool BazaarControl::vcsAnnotate(const QString &file, int line)
{
    const QFileInfo fi(file);
    m_bazaarClient->annotate(fi.absolutePath(), fi.fileName(), QString(), line);
    return true;
}

void BazaarControl::changed(const QVariant &v)
{
    switch (v.type()) {
    case QVariant::String:
        emit repositoryChanged(v.toString());
        break;
    case QVariant::StringList:
        emit filesChanged(v.toStringList());
        break;
    default:
        break;
    }
}

void BazaarControl::emitConfigurationChanged()
{
    emit configurationChanged();
}
