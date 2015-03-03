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

#include "diffeditordocument.h"
#include "diffeditorconstants.h"
#include "diffeditorcontroller.h"
#include "diffeditormanager.h"
#include "diffeditorreloader.h"
#include "diffutils.h"

#include <coreplugin/editormanager/editormanager.h>

#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QTextCodec>

namespace DiffEditor {
namespace Internal {

DiffEditorDocument::DiffEditorDocument() :
    Core::BaseTextDocument(),
    m_controller(new DiffEditorController(this))
{
    setId(Constants::DIFF_EDITOR_ID);
    setMimeType(QLatin1String(Constants::DIFF_EDITOR_MIMETYPE));
    setTemporary(true);
}

DiffEditorController *DiffEditorDocument::controller() const
{
    return m_controller;
}

bool DiffEditorDocument::setContents(const QByteArray &contents)
{
    Q_UNUSED(contents);
    return true;
}

QString DiffEditorDocument::defaultPath() const
{
    return m_controller->workingDirectory();
}

bool DiffEditorDocument::save(QString *errorString, const QString &fileName, bool autoSave)
{
    Q_UNUSED(errorString)
    Q_UNUSED(autoSave)

    const bool ok = write(fileName, format(), m_controller->contents(), errorString);

    if (!ok)
        return false;

    m_controller->setReloader(0);
    m_controller->setDescription(QString());
    m_controller->setDescriptionEnabled(false);

    DiffEditorManager::removeDocument(this);
    const QFileInfo fi(fileName);
    setTemporary(false);
    setFilePath(Utils::FileName::fromString(fi.absoluteFilePath()));
    setPreferredDisplayName(QString());
    return true;
}

bool DiffEditorDocument::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(type)
    if (flag == FlagIgnore)
        return true;
    return open(errorString, filePath().toString());
}

bool DiffEditorDocument::open(QString *errorString, const QString &fileName)
{
    QString patch;
    if (read(fileName, &patch, errorString) != Utils::TextFileFormat::ReadSuccess)
        return false;

    bool ok = false;
    QList<FileData> fileDataList = DiffUtils::readPatch(patch, &ok);
    if (!ok) {
        *errorString = tr("Could not parse patch file \"%1\". "
                          "The content is not of unified diff format.")
                .arg(fileName);
        return false;
    }

    const QFileInfo fi(fileName);
    setTemporary(false);
    setFilePath(Utils::FileName::fromString(fi.absoluteFilePath()));
    m_controller->setDiffFiles(fileDataList, fi.absolutePath());
    return true;
}

QString DiffEditorDocument::suggestedFileName() const
{
    enum { maxSubjectLength = 50 };
    QString result = QStringLiteral("0001");
    const QString description = m_controller->description();
    if (!description.isEmpty()) {
        // Derive "git format-patch-type" file name from subject.
        const int pos = description.indexOf(QLatin1String("\n\n    "));
        const int endPos = pos >= 0 ? description.indexOf(QLatin1Char('\n'), pos + 6) : -1;
        if (endPos > pos) {
            const QChar space(QLatin1Char(' '));
            const QChar dash(QLatin1Char('-'));
            QString subject = description.mid(pos, endPos - pos);
            for (int i = 0; i < subject.size(); ++i) {
                if (!subject.at(i).isLetterOrNumber())
                    subject[i] = space;
            }
            subject = subject.simplified();
            if (subject.size() > maxSubjectLength) {
                const int lastSpace = subject.lastIndexOf(space, maxSubjectLength);
                subject.truncate(lastSpace > 0 ? lastSpace : maxSubjectLength);
             }
            subject.replace(space, dash);
            result += dash;
            result += subject;
        }
    }
    return result + QStringLiteral(".patch");
}

QString DiffEditorDocument::plainText() const
{
    return m_controller->contents();
}

} // namespace Internal
} // namespace DiffEditor
