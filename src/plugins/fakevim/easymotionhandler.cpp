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
#include <QPair>

#include <texteditor/texteditor.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/fontsettings.h>
#include <texteditor/colorscheme.h>
#include <texteditor/texteditorsettings.h>

#include "easymotionhandler.h"

#include <QDebug>

namespace FakeVim {
namespace Internal {
namespace EasyMotion {

class EasyMotionOverlay;
class Context;
class IState;

struct HandlerPrivate
{
    QScopedPointer<Context> m_ctx;
    QScopedPointer<IState> m_state;
};

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

struct MoveForward
{
    unsigned int adjustRealPosition(unsigned int position)
    {
        return --position;
    }

    bool isAtLastPosition(QTextCursor &tc)
    {
        return tc.atEnd();
    }

    void movePosition(QTextCursor &tc)
    {
        tc.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    }
};

struct MoveBackward
{
    unsigned int adjustRealPosition(unsigned int position)
    {
        return position;
    }

    bool isAtLastPosition(QTextCursor &tc)
    {
        return tc.atStart();
    }

    void movePosition(QTextCursor &tc)
    {
        tc.movePosition(QTextCursor::PreviousCharacter,
                        QTextCursor::KeepAnchor);
    }
};

constexpr int MULTIPLE_PLACEHOLDER = -1;

template<typename Mover>
class Search : public State
{
public:
    Search(const State *old);
    Search(const Search<Mover> *old);

    bool handle(const QChar &input) override;

protected:
    bool contains(const QString &character) const;
    QList<int> find(const QString &character) const;
    QVector<unsigned int> copyVisibleDocument();
    void insertPlaceholders(const QVector<unsigned int> &positions,
                            const QChar &input);
    bool prepareAndSendOverlay();

private:
    bool placeholderAvailable();
    QString placeholder();
    bool firstLineVisible();

    Mover m_mover;

    int m_placeholderCnt;
    QScopedPointer<QTextDocument> m_document;
    QMultiMap<QString, int> m_foundPositions;
    QStringList m_placeholders = QString::fromUtf8(
                "a b c d e f g h i j k l m n o p q r s t u v w x y z A B C D E"
                "F G H I J K L M N O P Q R S T U V W X Y Z"
            ).split(QString::fromUtf8(" "));
};

template<typename Mover>
class Selected : public Search<Mover>
{
public:
    Selected(const Search<Mover> *old,
             const QString &originalInput);

    bool handle(const QChar &input);

private:
    QString m_originalInput;
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

Handler::Handler(QTextCursor *cursor, QPlainTextEdit *editor) :
    d{new HandlerPrivate}
{
    d->m_ctx.reset(new Context);
    d->m_state.reset(new Reset(d->m_ctx.data()));
    d->m_ctx->m_cursor = cursor;
    d->m_ctx->m_editor = editor;
}

Handler::~Handler()
{
}

bool Handler::handle(const QChar &input)
{
    bool handled = d->m_state->handle(input);
    d->m_state.reset(d->m_state->next());

    return handled;
}

void Handler::reset()
{
    if (!d->m_state->isReset()) {
        d->m_state->reset();
        d->m_state.reset(d->m_state->next());
    }
}

bool Handler::isReset() const
{
    return d->m_state->isReset();
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
Search<Mover>::Search(const Search<Mover> *old) :
    State{old},
    m_foundPositions(std::move(old->m_foundPositions))
{

}

template<typename Mover>
QVector<unsigned int> Search<Mover>::copyVisibleDocument()
{
    m_document.reset(new QTextDocument);

    auto beginCursor = editor()->cursorForPosition(
                editor()->viewport()->rect().topLeft());
    auto endCursor = editor()->cursorForPosition(
                editor()->viewport()->rect().bottomRight());

    endCursor.movePosition(QTextCursor::NextBlock);

    auto *editorDocument = editor()->document();
    auto firstVisibleBlock = editorDocument->findBlock(beginCursor.position());
    auto lastVisibleBlock = editorDocument->findBlock(endCursor.position());

    auto colorScheme =
            TextEditor::TextEditorSettings::fontSettings().colorScheme();
    auto fgColor = colorScheme.formatFor(TextEditor::C_TEXT).foreground();
    fgColor.setAlpha(128);

    //set tab width
    m_document->setDefaultTextOption(editorDocument->defaultTextOption());
    m_document->setDefaultStyleSheet(editorDocument->defaultStyleSheet());
    m_document->setDefaultCursorMoveStyle(
                editorDocument->defaultCursorMoveStyle());

    QVector<unsigned int> blockPositions;
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

    m_foundPositions.clear();

    return blockPositions;
}

template<typename Mover>
QString Search<Mover>::placeholder()
{
    if (m_placeholderCnt < m_placeholders.size() - 1)
        return m_placeholders[m_placeholderCnt++];
    else
        return m_placeholders[m_placeholderCnt];
}

template<typename Mover>
bool Search<Mover>::firstLineVisible()
{
    auto beginCursor =
            editor()->cursorForPosition(editor()->viewport()->rect().topLeft());

    auto firstVisibleBlock =
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
        setNext(new Selected<Mover>(this, input));

    return true;
}

template<typename Mover>
bool Search<Mover>::contains(const QString &character) const
{
    return m_foundPositions.contains(character);
}

template<typename Mover>
QList<int> Search<Mover>::find(const QString &character) const
{
    return m_foundPositions.values(character);
}

unsigned int positionToRealPosition(const QVector<unsigned int> &realPositions,
                                    const QTextCursor &tc)
{
    return realPositions[tc.blockNumber()] + tc.positionInBlock();
}

void realPositionToPosition(const QVector<unsigned int> &realPositions,
                            QTextCursor &tc,
                            const QTextCursor &tcReal)
{
    unsigned int realPosition = tcReal.position();

    while (positionToRealPosition(realPositions, tc) < realPosition)
        tc.movePosition(QTextCursor::NextBlock);

    tc.movePosition(QTextCursor::PreviousBlock);

    while(positionToRealPosition(realPositions, tc) != realPosition)
        tc.movePosition(QTextCursor::NextCharacter);
}

template<typename Mover>
void Search<Mover>::insertPlaceholders(const QVector<unsigned int> &positions,
                                       const QChar &input)
{
    QTextCursor tc(m_document.data());

    realPositionToPosition(positions, tc, editor()->textCursor());

    m_placeholderCnt = 0;

    while (!m_mover.isAtLastPosition(tc))
    {
        m_mover.movePosition(tc);

        QString character;
        character.append(input);

        if (tc.selectedText() == character) {
            QTextCharFormat format(tc.block().charFormat());
            format.setForeground(QColor(255,0,0,255));

            QString ph = placeholder();
            m_foundPositions.insert(ph, m_mover.adjustRealPosition(
                        positionToRealPosition(positions, tc)));

            tc.removeSelectedText();
            tc.insertText(ph, format);
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

template<typename Mover>
Selected<Mover>::Selected(const Search<Mover> *old,
                          const QString &originalInput) :
    Search<Mover>{old},
    m_originalInput{originalInput}
{
}

template<typename Mover>
bool Selected<Mover>::handle(const QChar& input)
{
    QString character;
    character.append(input);

    if (this->contains(character)) {
        auto valuesForSelected = this->find(character);

        if (valuesForSelected.size() > 1) { // go to stage 2
            this->cursor()->setPosition(valuesForSelected.last());

            this->insertPlaceholders(this->copyVisibleDocument(), m_originalInput.at(0));
            if (this->prepareAndSendOverlay())
                this->reset();
            else
                this->setNext(new Selected<Mover>(this, m_originalInput));
            /*
            QMultiMap<QString, int> nextPositions;

            foreach(auto value, valuesForSelected) {
                //nextPositions
            }

            this->setNext(new Selected<Mover>(this, std::move(nextPositions)));
            */

            return true;
        } else
            this->cursor()->setPosition(valuesForSelected.first());
    }
    this->reset();

    return true;
}

}
}
}
