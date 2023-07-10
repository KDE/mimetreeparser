#include "messageviewer.h"
#include <MimeTreeParser/MessageParser>
#include <MimeTreeParser/PartModel>
#include <QDebug>
#include <QVBoxLayout>

class MessageViewer::Private
{
public:
    QVBoxLayout *layout = nullptr;
    KMime::Message::Ptr message;
    MessageParser parser;
};

MessageViewer::MessageViewer(QWidget *parent)
    : QScrollArea(parent)
    , d(std::make_unique<MessageViewer::Private>())
{
    d->layout = new QVBoxLayout(this);
}

KMime::Message::Ptr MessageViewer::message() const
{
    return d->parser.message();
}

void MessageViewer::setMessage(const KMime::Message::Ptr message)
{
    d->parser.setMessage(message);

    const auto parts = d->parser.parts();

    for (int i = 0, count = parts->rowCount(); i < count; i++) {
        qWarning() << parts->data(parts->index(i, 0), PartModel::ContentRole);
    }
}
