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

#ifndef CPPEDITOR_INTERNAL_CLANGCOMPLETION_H
#define CPPEDITOR_INTERNAL_CLANGCOMPLETION_H

#include "clangcompleter.h"

#include <cplusplus/Icons.h>

#include <cpptools/cppcompletionassistprocessor.h>
#include <cpptools/cppcompletionassistprovider.h>
#include <cpptools/cppmodelmanager.h>

#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/assistinterface.h>

#include <QStringList>
#include <QTextCursor>

namespace ClangCodeModel {

namespace Internal {
class ClangAssistProposalModel;

class ClangCompletionAssistProvider : public CppTools::CppCompletionAssistProvider
{
public:
    ClangCompletionAssistProvider();

    virtual TextEditor::IAssistProcessor *createProcessor() const;
    virtual TextEditor::AssistInterface *createAssistInterface(
            const QString &filePath, QTextDocument *document,
            const CPlusPlus::LanguageFeatures &languageFeatures,
            int position, TextEditor::AssistReason reason) const;

private:
    ClangCodeModel::ClangCompleter::Ptr m_clangCompletionWrapper;
};

} // namespace Internal

class CLANG_EXPORT ClangCompletionAssistInterface: public TextEditor::AssistInterface
{
public:
    ClangCompletionAssistInterface(ClangCodeModel::ClangCompleter::Ptr clangWrapper,
                                   QTextDocument *document,
                                   int position,
                                   const QString &fileName,
                                   TextEditor::AssistReason reason,
                                   const QStringList &options,
                                   const QList<CppTools::ProjectPart::HeaderPath> &headerPaths,
                                   const Internal::PchInfo::Ptr &pchInfo,
                                   const CPlusPlus::LanguageFeatures &features);

    ClangCodeModel::ClangCompleter::Ptr clangWrapper() const
    { return m_clangWrapper; }

    const ClangCodeModel::Internal::UnsavedFiles &unsavedFiles() const
    { return m_unsavedFiles; }

    bool objcEnabled() const;

    const QStringList &options() const
    { return m_options; }

    const QList<CppTools::ProjectPart::HeaderPath> &headerPaths() const
    { return m_headerPaths; }

    CPlusPlus::LanguageFeatures languageFeatures() const
    { return m_languageFeatures; }

private:
    ClangCodeModel::ClangCompleter::Ptr m_clangWrapper;
    ClangCodeModel::Internal::UnsavedFiles m_unsavedFiles;
    QStringList m_options;
    QList<CppTools::ProjectPart::HeaderPath> m_headerPaths;
    Internal::PchInfo::Ptr m_savedPchPointer;
    CPlusPlus::LanguageFeatures m_languageFeatures;
};

class CLANG_EXPORT ClangCompletionAssistProcessor : public CppTools::CppCompletionAssistProcessor
{
    Q_DECLARE_TR_FUNCTIONS(ClangCodeModel::Internal::ClangCompletionAssistProcessor)

public:
    ClangCompletionAssistProcessor();
    virtual ~ClangCompletionAssistProcessor();

    virtual TextEditor::IAssistProposal *perform(const TextEditor::AssistInterface *interface);

private:
    int startCompletionHelper();
    int startOfOperator(int pos, unsigned *kind, bool wantFunctionCall) const;
    int findStartOfName(int pos = -1) const;
    bool accepts() const;
    TextEditor::IAssistProposal *createContentProposal();

    int startCompletionInternal(const QString fileName,
                                unsigned line, unsigned column,
                                int endOfExpression);

    bool completeInclude(const QTextCursor &cursor);
    void completeIncludePath(const QString &realPath, const QStringList &suffixes);
    void completePreprocessor();
    void addCompletionItem(const QString &text,
                           const QIcon &icon = QIcon(),
                           int order = 0,
                           const QVariant &data = QVariant());

private:
    QScopedPointer<const ClangCompletionAssistInterface> m_interface;
    QScopedPointer<Internal::ClangAssistProposalModel> m_model;
};

} // namespace Clang

#endif // CPPEDITOR_INTERNAL_CLANGCOMPLETION_H
