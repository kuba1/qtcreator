/**************************************************************************
**
** Copyright (C) 2015 Brian McGillion and Hugues Delorme
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

#ifndef VCSBASE_COMMAND_H
#define VCSBASE_COMMAND_H

#include "vcsbase_global.h"

#include <QObject>

QT_BEGIN_NAMESPACE
class QMutex;
class QStringList;
class QVariant;
class QProcessEnvironment;
template <typename T>
class QFutureInterface;
QT_END_NAMESPACE

namespace Utils {
struct SynchronousProcessResponse;
class ExitCodeInterpreter;
class FileName;
}

namespace VcsBase {

namespace Internal { class VcsCommandPrivate; }

class VCSBASE_EXPORT ProgressParser
{
public:
    ProgressParser();
    virtual ~ProgressParser();

protected:
    virtual void parseProgress(const QString &text) = 0;
    void setProgressAndMaximum(int value, int maximum);

private:
    void setFuture(QFutureInterface<void> *future);

    QFutureInterface<void> *m_future;
    QMutex *m_futureMutex;
    friend class VcsCommand;
};

class VCSBASE_EXPORT VcsCommand : public QObject
{
    Q_OBJECT

public:
    VcsCommand(const Utils::FileName &binary,
               const QString &workingDirectory,
               const QProcessEnvironment &environment);
    ~VcsCommand();

    void addJob(const QStringList &arguments, Utils::ExitCodeInterpreter *interpreter = 0);
    void addJob(const QStringList &arguments, int timeout, Utils::ExitCodeInterpreter *interpreter = 0);
    void execute();
    void abort();
    bool lastExecutionSuccess() const;
    int lastExecutionExitCode() const;

    const Utils::FileName &binaryPath() const;
    const QString &workingDirectory() const;
    const QProcessEnvironment &processEnvironment() const;

    int defaultTimeout() const;
    void setDefaultTimeout(int timeout);

    unsigned flags() const;
    void addFlags(unsigned f);

    const QVariant &cookie() const;
    void setCookie(const QVariant &cookie);

    QTextCodec *codec() const;
    void setCodec(QTextCodec *codec);

    void setProgressParser(ProgressParser *parser);
    void setProgressiveOutput(bool progressive);

    Utils::SynchronousProcessResponse runVcs(const QStringList &arguments, int timeoutMS,
                                             Utils::ExitCodeInterpreter *interpreter = 0);
    // Make sure to not pass through the event loop at all:
    bool runFullySynchronous(const QStringList &arguments, int timeoutMS,
                             QByteArray *outputData, QByteArray *errorData);

private:
    void run(QFutureInterface<void> &future);
    Utils::SynchronousProcessResponse runSynchronous(const QStringList &arguments, int timeoutMS,
                                                     Utils::ExitCodeInterpreter *interpreter = 0);
    void emitRepositoryChanged();

public slots:
    void cancel();

signals:
    void output(const QString &);
    void errorText(const QString &);
    void finished(bool ok, int exitCode, const QVariant &cookie);
    void success(const QVariant &cookie);

private slots:
    void bufferedOutput(const QString &text);
    void bufferedError(const QString &text);
    void coreAboutToClose();

signals:
    void terminate(); // Internal

private:
    class Internal::VcsCommandPrivate *const d;
};

} // namespace VcsBase

#endif // VCSBASE_COMMAND_H
