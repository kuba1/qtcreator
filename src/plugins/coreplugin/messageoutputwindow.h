/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef MESSAGEOUTPUTWINDOW_H
#define MESSAGEOUTPUTWINDOW_H

#include <coreplugin/ioutputpane.h>

#include <QtGui/QShowEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QTextEdit>

namespace Core {
namespace Internal {

class TextView : public QTextEdit
{
    Q_OBJECT

public:
    TextView(QWidget *parent = 0) : QTextEdit(parent) {}

    void showEvent(QShowEvent *);
    void scrollToBottom();
    bool isScrollbarAtBottom() const;

protected:
    void resizeEvent(QResizeEvent *e);
};

class MessageOutputWindow : public Core::IOutputPane
{
    Q_OBJECT

public:
    MessageOutputWindow();
    ~MessageOutputWindow();

    QWidget *outputWidget(QWidget *parent);
    QList<QWidget*> toolBarWidgets() const { return QList<QWidget *>(); }

    QString displayName() const;
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool visible);

    void append(const QString &text);
    bool canFocus();
    bool hasFocus();
    void setFocus();

    virtual bool canNext();
    virtual bool canPrevious();
    virtual void goToNext();
    virtual void goToPrev();
    bool canNavigate();

private:
    TextView *m_widget;
};

} // namespace Internal
} // namespace Core

#endif // MESSAGEOUTPUTWINDOW_H
