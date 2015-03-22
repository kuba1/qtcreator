#ifndef EASYMOTIONHANDLER_H
#define EASYMOTIONHANDLER_H

class QTextCursor;
class QPlainTextEdit;
class QChar;

namespace FakeVim {
namespace Internal {
namespace EasyMotion {

class Context;
class IState;

///
/// \brief The EasyMotionHandler class handler for easy motion. Encapsulates
/// state management and provides simple interface for FakeVim handler
///
class Handler
{
public:
    Handler();
    Handler(QTextCursor *cursor, QPlainTextEdit *editor);
    ~Handler();

    ///
    /// \brief handle handle incoming input (character)
    /// \return proprely handled by the state machine
    ///
    bool handle(const QChar &input);

    ///
    /// \brief reset set state machine to Reset state
    ///
    void reset();

    ///
    /// \brief isReset check whether state machine (and thus the handler) is in
    /// Reset state
    ///
    bool isReset() const;

private:
    ///
    /// \brief m_h helper class for data manipulation
    ///
    QScopedPointer<Context> m_ctx;

    ///
    /// \brief m_state this class owns state, the states objects just produce
    /// appropriate states
    ///
    QScopedPointer<IState> m_state;
};

}
}
}

#endif // EASYMOTIONHANDLER_H
