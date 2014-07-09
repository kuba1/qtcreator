#include <QCoreApplication>
#include <QSharedPointer>
#include <QChar>
#include <QString>
#include <QTextCursor>
#include <QTextBlock>
#include <QPlainTextEdit>
#include <QFont>
#include <QRect>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QTextBlockUserData>
#include <QScopedPointer>
#include <QSharedPointer>

#include <texteditor/texteditor.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/fontsettings.h>
#include <texteditor/colorscheme.h>
#include <texteditor/texteditorsettings.h>

#include "easymotionhandler.h"

namespace FakeVim {
namespace Internal {

class EasyMotionOverlay;

///
/// \brief The SharedState struct data structure shared between this handler and
/// states objects. Contains all the data needed to make everything work
/// plus some helper functions.
///
struct SharedState
{
public:
    void prepareDocument();
    bool prepareAndSendOverlay();
    int copyVisibleDocument();
    void insertPlaceholdersForward(int fromPosition);
    void insertPlaceholdersBackward(int fromPosition);
    void resetPlaceholders();
    QString placeholder();
    bool placeholderAvailable();
    bool firstLineVisible();

    QTextDocument* m_document;
    QChar m_input;
    int m_placeholderCnt;
    QVector<int> m_foundPositions;
    QString m_character;
    QTextCursor* m_cursor;
    QPlainTextEdit* m_editor;
    QSharedPointer<EasyMotionOverlay> m_overlay;
    QStringList m_placeholders;
};

///
/// \brief The EasyMotionState class base class for the state, defines
/// state interface. Each state object keeps pointer to data structure so
/// it has access to current data
///
class EasyMotionState
{
public:
    EasyMotionState(SharedState* h);
    virtual ~EasyMotionState();

    ///
    /// \brief handle do the job associated with current state, set
    /// @see m_next to next state. This is invoked after an event (new
    /// input)
    /// \return true if event has been handled properly
    ///
    virtual bool handle() = 0;

    ///
    /// \brief isReset check whether state machine is reseted (current
    /// state is Reset)
    /// \return is this state Reset?
    ///
    bool isReset() const;

    ///
    /// \brief next get next state
    /// \return pointer to next state
    ///
    EasyMotionState* next();

    ///
    /// \brief reset force reset of the state machine. Clean up some state
    /// data and set Reset as next state
    ///
    void reset();

protected:

    ///
    /// \brief m_h helper class for data manipulation, owned by handler
    ///
    SharedState* m_h;

    ///
    /// \brief m_next next state to return with @see next()
    ///
    EasyMotionState* m_next;

    ///
    /// \brief m_isReset true for Reset state, false for any other state,
    /// used by @see isReset() function
    ///
    bool m_isReset;
};

///
/// \brief The Reset class represents starting state
///
class Reset : public EasyMotionState
{
public:
    Reset(SharedState* h);

    bool handle();
};

///
/// \brief The SearchBackward class state indicating that searching
/// backwards has been chosen
///
class SearchBackward : public EasyMotionState
{
public:
    SearchBackward(SharedState* h);

    bool handle();
};

///
/// \brief The SearchForward class state indicating that searching forward
/// has been chosen
///
class SearchForward : public EasyMotionState
{
public:
    SearchForward(SharedState* h);

    bool handle();
};

///
/// \brief The Selected class state indicating that the character to move to
/// has been selected (after that, move and set state to Reset)
///
class Selected : public EasyMotionState
{
public:
    Selected(SharedState* h);

    bool handle();
};

///
/// \brief The BlockPositionData class simple class to attach block positions
/// from real document to blocks in prepared overlay document
///
class BlockPositionData : public QTextBlockUserData
{
public:
    int m_position;
};

///
/// \brief The EasyMotionOverlay class overlay an overlay with prepared document
/// to be drawn over current document in the editor
///
class EasyMotionOverlay : public TextEditor::IOverlay
{
public:
    EasyMotionOverlay(const QPlainTextEdit* editor, QTextDocument* document) :
        m_paintUpperMargin(false)
    {
        m_paintRect = editor->viewport()->rect();
        m_backColor = editor->palette().foreground();
        m_document.reset(document);
    }

    ///
    /// \brief paint invoked from paintEvent in the text editor widget
    ///
    void paint(QPainter &painter)
    {
        painter.save();

        auto colorScheme =
                TextEditor::TextEditorSettings::fontSettings().colorScheme();
        auto bgColor = colorScheme.formatFor(TextEditor::C_TEXT).background();
        painter.setBrush(bgColor);
        painter.setPen(bgColor);
        painter.drawRect(m_paintRect);

        // remove top margin
        if (!m_paintUpperMargin)
            painter.translate(0.0, -m_document->documentMargin());
        m_document->drawContents(&painter);

        painter.restore();
    }

    ///
    /// \brief m_backColor background color
    ///
    QBrush m_backColor;

    ///
    /// \brief m_foreColor foreground color
    ///
    QColor m_foreColor;

    ///
    /// \brief m_paintRect visible editor rectangle (we will paint over
    /// everythin)
    ///
    QRect m_paintRect;

    ///
    /// \brief m_paintUpperMargin true only if first line is visible
    ///
    bool m_paintUpperMargin;

    ///
    /// \brief m_document specially prepared document that will be painter over
    /// what is currently visible in the editor
    ///
    QScopedPointer<QTextDocument> m_document;
};

EasyMotionHandler::EasyMotionHandler(QTextCursor* cursor,
                                     QPlainTextEdit* editor)
{
    m_h.reset(new SharedState);
    m_state.reset(new Reset(m_h.data()));
    m_h->m_cursor = cursor;
    m_h->m_editor = editor;
    m_h->m_placeholders = QString::fromUtf8(
        "a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y|z|A|B|C|D|E|F|G|H|"
        "I|J|K|L|M|N|O|P|Q|R|S|T|U|V|W|X|Y|Z|1|2|3|4|5|6|7|8|9|0|[|]|;|'|,|.|"
                "/|!|@|#|$|%|^|&|*|(|)|{|}|:|\"|<|>|?").split(QString::fromUtf8("|"));
}

EasyMotionHandler::~EasyMotionHandler()
{
}

bool EasyMotionHandler::handle(const QChar &input)
{
    m_h->m_input = input;
    bool handled = m_state->handle();
    m_state.reset(m_state->next());

    return handled;
}

void EasyMotionHandler::reset()
{
    m_state->reset();
    m_state.reset(m_state->next());
}

bool EasyMotionHandler::isReset() const
{
    return m_state->isReset();
}

void SharedState::prepareDocument()
{
    QString character;
    character.append(m_input);
    m_character = character;

    // overlay will own this document
    m_document = new QTextDocument;
}

bool SharedState::prepareAndSendOverlay()
{
    TextEditor::EasyMotionOverlayEvent ev;

    m_overlay = QSharedPointer<EasyMotionOverlay>(
                    new EasyMotionOverlay(m_editor, m_document));
    if (firstLineVisible())
        m_overlay->m_paintUpperMargin = true;
    // overlay owns the document now
    m_document = nullptr;

    if (m_foundPositions.count() == 0) {
        // none found, don't do anything
        return true;
    } else if (m_foundPositions.count() == 1) {
        // only one found, move immediately
        m_cursor->setPosition(m_foundPositions.first());
        return true;
    }

    ev.setOverlay(m_overlay);
    ev.setShow(true);

    QCoreApplication::sendEvent(m_editor, &ev);

    return false;
}

struct RealBlockPosition : public QTextBlockUserData
{
    int m_position;
};

int SharedState::copyVisibleDocument()
{
    QTextCursor beginCursor =
        m_editor->cursorForPosition(m_editor->viewport()->rect().topLeft());
    QTextCursor endCursor =
        m_editor->cursorForPosition(m_editor->viewport()->rect().bottomRight());
    endCursor.movePosition(QTextCursor::NextBlock);

    QTextDocument* editorDocument = m_editor->document();
    const QTextBlock firstVisibleBlock =
        editorDocument->findBlock(beginCursor.position());
    const QTextBlock lastVisibleBlock =
        editorDocument->findBlock(endCursor.position());

    QVector<int> blockPositions;

    auto colorScheme =
            TextEditor::TextEditorSettings::fontSettings().colorScheme();
    QColor fgColor = colorScheme.formatFor(TextEditor::C_TEXT).foreground();
    fgColor.setAlpha(128);

    //set tab width
    m_document->setDefaultTextOption(editorDocument->defaultTextOption());
    m_document->setDefaultStyleSheet(editorDocument->defaultStyleSheet());
    m_document->setDefaultCursorMoveStyle(
                editorDocument->defaultCursorMoveStyle());

    resetPlaceholders();
    auto currentBlock = lastVisibleBlock;
    do {
        currentBlock = currentBlock.previous();

        if (currentBlock.isValid() && currentBlock.isVisible()) {
            QTextCursor tc(m_document);
            QTextCharFormat format;
            format.setFont(currentBlock.charFormat().font());
            format.setForeground(fgColor);
            blockPositions.push_front(currentBlock.position());

            tc.setCharFormat(format);
            tc.setBlockCharFormat(format);
            tc.insertText(currentBlock.text());
            tc.insertBlock();
        }
    } while (currentBlock != firstVisibleBlock);

    auto block = m_document->begin();
    int i = 0;
    do {
        auto* pos = new RealBlockPosition;
        pos->m_position = blockPositions[i];

        block.setUserData(pos);
        block = block.next();
        i++;
    } while (i < blockPositions.count());

    m_foundPositions.clear();

    return m_editor->textCursor().position();
}

void SharedState::insertPlaceholdersForward(int fromPosition)
{
    QTextCursor tc(m_document);

    while (!tc.atEnd() && placeholderAvailable())
    {
        auto* pos = static_cast<RealBlockPosition*>(tc.block().userData());
        if (!pos)
            break;

        int position = pos->m_position + tc.positionInBlock();

        // select each character and compare to what we are looking for
        tc.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);

        if (position > fromPosition) {
            if (tc.selectedText() == m_character) {
                QTextCharFormat format(tc.block().charFormat());
                format.setForeground(QColor(255,0,0,255));

                m_foundPositions.push_back(position);

                // remove found character and insert a placeholder instead
                tc.removeSelectedText();
                tc.insertText(placeholder(), format);
            }
        }
        // move anchor
        tc.setPosition(tc.position());
    }
}

void SharedState::insertPlaceholdersBackward(int fromPosition)
{
    QTextCursor tc(m_document);
    tc.movePosition(QTextCursor::End);

    while (!tc.atStart() && placeholderAvailable())
    {
        tc.movePosition(QTextCursor::PreviousCharacter,
                        QTextCursor::KeepAnchor);

        auto* pos = static_cast<RealBlockPosition*>(tc.block().userData());
        if (!pos)
            break;

        int position = pos->m_position + tc.positionInBlock();
        if (position <= fromPosition) {
            if (tc.selectedText() == m_character) {
                QTextCharFormat format(tc.block().charFormat());
                format.setForeground(QColor(255,0,0,255));

                m_foundPositions.push_back(position);

                tc.removeSelectedText();
                tc.insertText(placeholder(), format);
            }
        }
        // move anchor
        tc.setPosition(tc.position());
    }
}

void SharedState::resetPlaceholders()
{
    m_placeholderCnt = 0;
}

QString SharedState::placeholder()
{
    if (placeholderAvailable())
        return m_placeholders[m_placeholderCnt++];
    else
        return QString::fromUtf8("ERROR");
}

bool SharedState::placeholderAvailable()
{
    return m_placeholderCnt < m_placeholders.size();
}

bool SharedState::firstLineVisible()
{
    QTextCursor beginCursor =
        m_editor->cursorForPosition(m_editor->viewport()->rect().topLeft());

    const QTextBlock firstVisibleBlock =
        m_editor->document()->findBlock(beginCursor.position());

    return firstVisibleBlock == m_editor->document()->firstBlock();
}

EasyMotionState::EasyMotionState(SharedState* h) :
    m_h(h),
    m_next(0),
    m_isReset(false)
{
}

EasyMotionState::~EasyMotionState()
{
}

bool EasyMotionState::isReset() const
{
    return m_isReset;
}

EasyMotionState* EasyMotionState::next()
{
    if (m_next)
        return m_next;
    else
        return new Reset(m_h);
}

void EasyMotionState::reset()
{
    TextEditor::EasyMotionOverlayEvent ev;

    ev.setOverlay(m_h->m_overlay);
    ev.setShow(false);

    m_h->m_overlay.clear();
    m_h->m_foundPositions.clear();
    m_next = new Reset(m_h);

    QCoreApplication::sendEvent(m_h->m_editor, &ev);
}

Reset::Reset(SharedState* priv) : EasyMotionState(priv)
{
    m_isReset = true;
}

bool Reset::handle()
{
    if (m_h->m_input ==  QLatin1Char('f'))
        m_next = new SearchForward(m_h);
    else if (m_h->m_input == QLatin1Char('F'))
        m_next = new SearchBackward(m_h);
    else
        return false;
    return true;
}

SearchBackward::SearchBackward(SharedState* priv) : EasyMotionState(priv)
{
}

bool SearchBackward::handle()
{
    if (m_h->m_overlay.isNull()) {
        m_h->prepareDocument();
        m_h->insertPlaceholdersBackward(m_h->copyVisibleDocument());
        if (m_h->prepareAndSendOverlay())
            reset();
        else
            m_next = new Selected(m_h);
    } else
        reset();

    return true;
}

SearchForward::SearchForward(SharedState* priv) : EasyMotionState(priv)
{
}

bool SearchForward::handle()
{
    if (m_h->m_overlay.isNull()) {
        m_h->prepareDocument();
        m_h->insertPlaceholdersForward(m_h->copyVisibleDocument());
        if (m_h->prepareAndSendOverlay())
            reset();
        else
            m_next = new Selected(m_h);
    } else
        reset();

    return true;
}

Selected::Selected(SharedState* priv) : EasyMotionState(priv)
{
}

bool Selected::handle()
{
    QString character;
    int index;

    character.append(m_h->m_input);

    if ((index = m_h->m_placeholders.indexOf(character)) >= 0 &&
            index < m_h->m_foundPositions.size())
        m_h->m_cursor->setPosition(m_h->m_foundPositions[index]);
    reset();

    return true;
}

}
}
