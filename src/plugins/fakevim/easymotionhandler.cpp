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
namespace EasyMotion {

class EasyMotionOverlay;

struct Context
{
    QTextCursor *m_cursor;
    QPlainTextEdit *m_editor;
};

class IState
{
public:
    virtual ~IState()
    {
    }

    virtual bool handle(const QChar &input) = 0;
    virtual bool isReset() const = 0;
    virtual IState *next() const = 0;
    virtual void reset() = 0;
};

class State : public IState
{
public:
    State(const Context *ctx) :
        m_ctx{ctx},
        m_next{nullptr},
        m_isReset{true}
    {
    }

    State(const State *old, bool isReset = false) :
        m_ctx{old->m_ctx},
        m_next{nullptr},
        m_isReset{isReset}
    {
    }

    bool isReset() const override
    {
        return m_isReset;
    }

    IState *next() const override;
    void reset() override;

protected:
    void setNext(State *next)
    {
        m_next = next;
    }

    QTextCursor *cursor() const
    {
        return m_ctx->m_cursor;
    }

    QPlainTextEdit *editor() const
    {
        return m_ctx->m_editor;
    }

private:
    const Context *m_ctx;
    IState *m_next;
    bool m_isReset;
};

class Reset : public State
{
public:
    Reset(const Context *ctx) : State{ctx}
    {
    }

    Reset(const State *old) : State{old, true}
    {
    }

    bool handle(const QChar &input) override;
};

struct RealBlockPosition : public QTextBlockUserData
{
    int m_position;
};

struct MoveForward
{
    void initPosition(QTextCursor &)
    {
    }

    bool isAtLastPosition(QTextCursor &tc)
    {
        return tc.atEnd();
    }

    bool movePosition(QTextCursor &tc, int &position)
    {
        auto *pos = dynamic_cast<RealBlockPosition *>(tc.block().userData());
        if (!pos)
            return false;

        position = pos->m_position + tc.positionInBlock();

        tc.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);

        return true;
    }

    bool checkPosition(int position, int fromPosition)
    {
        return position > fromPosition;
    }
};

struct MoveBackward
{
    void initPosition(QTextCursor &tc)
    {
        tc.movePosition(QTextCursor::End);
    }

    bool isAtLastPosition(QTextCursor &tc)
    {
        return tc.atStart();
    }

    bool movePosition(QTextCursor &tc, int &position)
    {
        tc.movePosition(QTextCursor::PreviousCharacter,
                        QTextCursor::KeepAnchor);

        auto* pos = dynamic_cast<RealBlockPosition*>(tc.block().userData());
        if (!pos)
            return false;

        position = pos->m_position + tc.positionInBlock();

        return true;
    }

    bool checkPosition(int position, int fromPosition)
    {
        return position <= fromPosition;
    }
};

template<typename Mover>
class Search : public State
{
public:
    Search(const State *old);
    bool handle(const QChar &input) override;

private:
    void insertPlaceholders(int fromPosition, const QChar &input);
    int copyVisibleDocument();
    bool prepareAndSendOverlay();
    bool placeholderAvailable();
    QString placeholder();
    bool firstLineVisible();

    Mover m_mover;

    int m_placeholderCnt;
    QScopedPointer<QTextDocument> m_document;
    QMap<QString, int> m_foundPositions;
    QStringList m_placeholders = QString::fromUtf8(
                "a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y|z|A|B|C|D|E|F|G|H|"
                "I|J|K|L|M|N|O|P|Q|R|S|T|U|V|W|X|Y|Z|1|2|3|4|5|6|7|8|9|0|[|]|;|'|,|.|"
                "/|!|@|#|$|%|^|&|*|(|)|{|}|:|\"|<|>|?").split(QString::fromUtf8("|"));
};

class Selected : public State
{
public:
    Selected(const State *old, const QMap<QString, int> &&foundPositions);

    bool handle(const QChar &input);

private:
    const QMap<QString, int> m_foundPositions;
};

class EasyMotionOverlay : public TextEditor::IOverlay
{
public:
    EasyMotionOverlay(const QPlainTextEdit *editor,
                      QScopedPointer<QTextDocument> &document) :
        m_paintUpperMargin{false}
    {
        m_paintRect = editor->viewport()->rect();
        m_backColor = editor->palette().foreground();
        m_document.swap(document);
    }

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

    void setPaintUpperMargin()
    {
        m_paintUpperMargin = true;
    }

private:
    QBrush m_backColor;
    QColor m_foreColor;
    QRect m_paintRect;
    bool m_paintUpperMargin;
    QScopedPointer<QTextDocument> m_document;
};

Handler::Handler(QTextCursor *cursor, QPlainTextEdit *editor)
{
    m_ctx.reset(new Context);
    m_state.reset(new Reset(m_ctx.data()));
    m_ctx->m_cursor = cursor;
    m_ctx->m_editor = editor;
}

Handler::~Handler()
{
}

bool Handler::handle(const QChar &input)
{
    bool handled = m_state->handle(input);
    m_state.reset(m_state->next());

    return handled;
}

void Handler::reset()
{
    if (!m_state->isReset()) {
        m_state->reset();
        m_state.reset(m_state->next());
    }
}

bool Handler::isReset() const
{
    return m_state->isReset();
}

IState *State::next() const
{
    if (m_next)
        return m_next;
    else
        return new Reset(this);
}

void State::reset()
{
    m_next = new Reset(this);

    TextEditor::HideOverlayEvent ev;
    QCoreApplication::sendEvent(editor(), &ev);
}

template<typename Mover>
Search<Mover>::Search(const State *old) : State(old)
{
}

template<typename Mover>
int Search<Mover>::copyVisibleDocument()
{
    m_document.reset(new QTextDocument);

    QTextCursor beginCursor =
            editor()->cursorForPosition(editor()->viewport()->rect().topLeft());
    QTextCursor endCursor =
            editor()->cursorForPosition(editor()->viewport()->rect().bottomRight());
    endCursor.movePosition(QTextCursor::NextBlock);

    QTextDocument* editorDocument = editor()->document();
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

    m_placeholderCnt = 0;
    auto currentBlock = lastVisibleBlock;
    do {
        currentBlock = currentBlock.previous();

        if (currentBlock.isValid() && currentBlock.isVisible()) {
            QTextCursor tc(m_document.data());
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

    return editor()->textCursor().position();
}

template<typename Mover>
QString Search<Mover>::placeholder()
{
    if (placeholderAvailable())
        return m_placeholders[m_placeholderCnt++];
    else
        return QString::fromUtf8("ERROR");
}

template<typename Mover>
bool Search<Mover>::placeholderAvailable()
{
    return m_placeholderCnt < m_placeholders.size();
}

template<typename Mover>
bool Search<Mover>::firstLineVisible()
{
    QTextCursor beginCursor =
            editor()->cursorForPosition(editor()->viewport()->rect().topLeft());

    const QTextBlock firstVisibleBlock =
            editor()->document()->findBlock(beginCursor.position());

    return firstVisibleBlock == editor()->document()->firstBlock();
}

template<typename Mover>
bool Search<Mover>::handle(const QChar &input)
{
    insertPlaceholders(copyVisibleDocument(), input);
    if (prepareAndSendOverlay())
        reset();
    else
        setNext(new Selected(this, std::move(m_foundPositions)));

    return true;
}

template<typename Mover>
void Search<Mover>::insertPlaceholders(int fromPosition, const QChar &input)
{
    QTextCursor tc(m_document.data());

    m_mover.initPosition(tc);

    while (!m_mover.isAtLastPosition(tc) && placeholderAvailable())
    {
        int position;

        if (!m_mover.movePosition(tc, position))
            break;

        if (m_mover.checkPosition(position, fromPosition)) {
            QString character;
            character.append(input);

            if (tc.selectedText() == character) {
                QTextCharFormat format(tc.block().charFormat());
                format.setForeground(QColor(255,0,0,255));

                QString ph = placeholder();
                m_foundPositions[ph] = position;

                tc.removeSelectedText();
                tc.insertText(ph, format);
            }
        }
        // move anchor
        tc.setPosition(tc.position());
    }
}

template<typename Mover>
bool Search<Mover>::prepareAndSendOverlay()
{
    EasyMotionOverlay* overlay =
            new EasyMotionOverlay(editor(), m_document);

    if (firstLineVisible())
        overlay->setPaintUpperMargin();
    // overlay owns the document now

    if (m_foundPositions.count() == 0) {
        // none found, don't do anything
        return true;
    } else if (m_foundPositions.count() == 1) {
        // only one found, move immediately
        cursor()->setPosition(m_foundPositions.first());
        return true;
    }

    TextEditor::ShowOverlayEvent ev;
    ev.setOverlay(QSharedPointer<EasyMotionOverlay>(overlay));
    QCoreApplication::sendEvent(editor(), &ev);

    return false;
}

bool Reset::handle(const QChar &input)
{
    if (input ==  QLatin1Char('f'))
        setNext(new Search<MoveForward>(this));
    else if (input == QLatin1Char('F'))
        setNext(new Search<MoveBackward>(this));
    else
        return false;
    return true;
}

Selected::Selected(const State *old,
                   const QMap<QString, int>&& foundPositions) :
    State{old},
    m_foundPositions(std::move(foundPositions))
{
}

bool Selected::handle(const QChar& input)
{
    QString character;
    character.append(input);

    if (m_foundPositions.contains(character))
        cursor()->setPosition(m_foundPositions[character]);
    reset();

    return true;
}

}
}
}
