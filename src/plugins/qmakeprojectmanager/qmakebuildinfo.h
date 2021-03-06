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

#ifndef QMAKEBUILDINFO_H
#define QMAKEBUILDINFO_H

#include "qmakebuildconfiguration.h"

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kitmanager.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

namespace QmakeProjectManager {

class QmakeBuildInfo : public ProjectExplorer::BuildInfo
{
public:
    QmakeBuildInfo(const QmakeBuildConfigurationFactory *f) : ProjectExplorer::BuildInfo(f) { }

    ProjectExplorer::BuildConfiguration::BuildType type;
    QString additionalArguments;
    QString makefile;

    bool operator==(const QmakeBuildInfo &o) {
        return displayName == o.displayName
                && typeName == o.typeName
                && buildDirectory == o.buildDirectory
                && kitId == o.kitId
                && supportsShadowBuild == o.supportsShadowBuild
                && type == o.type
                && additionalArguments == o.additionalArguments;
    }

    QList<ProjectExplorer::Task> reportIssues(const QString &projectPath,
                                              const QString &buildDir) const
    {
        ProjectExplorer::Kit *k = ProjectExplorer::KitManager::find(kitId);
        QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
        return version ? version->reportIssues(projectPath, buildDir)
                       : QList<ProjectExplorer::Task>();
    }
};

} // namespace QmakeProjectManager

#endif // QMAKEBUILDINFO_H
