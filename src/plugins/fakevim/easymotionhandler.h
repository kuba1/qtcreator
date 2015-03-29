#ifndef EASYMOTIONHANDLER_H
#define EASYMOTIONHANDLER_H

class QTextCursor;
class QPlainTextEdit;
class QChar;

namespace FakeVim {
namespace Internal {
namespace EasyMotion {

struct HandlerPrivate;

class Handler
{
public:
    Handler();
    Handler(QTextCursor *cursor, QPlainTextEdit *editor);
    ~Handler();

    bool handle(const QChar &input);
    void reset();
    bool isReset() const;

private:
    QScopedPointer<HandlerPrivate> d;
};

}
}
}

#endif // EASYMOTIONHANDLER_H
