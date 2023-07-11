#include "messageviewer.h"
#include <KLocalizedString>
#include <MimeTreeParser/MessageParser>
#include <MimeTreeParser/PartModel>
#include <QDebug>
#include <QFormLayout>
#include <QLabel>
#include <QScrollArea>
#include <QSplitter>
#include <QTreeView>
#include <QVBoxLayout>

class MessageViewer::Private
{
public:
    QVBoxLayout *layout = nullptr;
    KMime::Message::Ptr message;
    MessageParser parser;
    QVector<QWidget *> widgets;
    QScrollArea *scrollArea = nullptr;
    QFormLayout *formLayout = nullptr;
    QTreeView *attachmentView = nullptr;
};

MessageViewer::MessageViewer(QWidget *parent)
    : QSplitter(Qt::Vertical, parent)
    , d(std::make_unique<MessageViewer::Private>())
{
    setObjectName(QStringLiteral("MessageViewerSplitter"));
    setChildrenCollapsible(false);
    setSizes({0});

    auto headersArea = new QWidget(this);
    headersArea->setSizePolicy(sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    addWidget(headersArea);

    d->formLayout = new QFormLayout(headersArea);

    auto scrollArea = new QScrollArea(this);
    addWidget(scrollArea);

    d->layout = new QVBoxLayout(scrollArea);

    d->attachmentView = new QTreeView(this);
    addWidget(d->attachmentView);
}

MessageViewer::~MessageViewer()
{
    qDeleteAll(d->widgets);
    d->widgets.clear();
}

KMime::Message::Ptr MessageViewer::message() const
{
    return d->parser.message();
}

void MessageViewer::setMessage(const KMime::Message::Ptr message)
{
    d->parser.setMessage(message);

    for (int i = d->formLayout->rowCount() - 1; i >= 0; i--) {
        d->formLayout->removeRow(i);
    }
    d->formLayout->addRow(i18n("&Subject:"), new QLabel(d->parser.subject()));
    d->formLayout->addRow(i18n("&From:"), new QLabel(d->parser.from()));
    d->formLayout->addRow(i18n("&To:"), new QLabel(d->parser.to()));

    const auto parts = d->parser.parts();

    for (auto widget : std::as_const(d->widgets)) {
        d->layout->removeWidget(widget);
        delete widget;
    }
    d->widgets.clear();

    for (int i = 0, count = parts->rowCount(); i < count; i++) {
        const auto type = static_cast<PartModel::Types>(parts->data(parts->index(i, 0), PartModel::TypeRole).toUInt());
        const auto content = parts->data(parts->index(i, 0), PartModel::ContentRole).toString();

        switch (type) {
        case PartModel::Types::Plain: {
            auto label = new QLabel();
            label->setText(content);
            d->widgets.append(label);
            d->layout->addWidget(label);
            break;
        }

        case PartModel::Types::Error:
        default:
            qWarning() << parts->data(parts->index(i, 0), PartModel::ContentRole) << type;
        }
    }

    d->attachmentView->setModel(d->parser.attachments());
}
