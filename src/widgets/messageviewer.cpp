#include "messageviewer.h"
#include "../core/objecttreeparser.h"
#include "attachmentview_p.h"
#include <KLocalizedString>
#include <MimeTreeParserCore/AttachmentModel>
#include <MimeTreeParserCore/MessageParser>
#include <MimeTreeParserCore/PartModel>
#include <QAction>
#include <QDebug>
#include <QFormLayout>
#include <QLabel>
#include <QMenu>
#include <QScrollArea>
#include <QSplitter>
#include <QVBoxLayout>

class MessageViewer::Private
{
public:
    Private(MessageViewer *q_ptr)
        : q{q_ptr}
    {
        createActions();
    }

    MessageViewer *q;

    QVBoxLayout *layout = nullptr;
    KMime::Message::Ptr message;
    MessageParser parser;
    QVector<QWidget *> widgets;
    QScrollArea *scrollArea = nullptr;
    QFormLayout *formLayout = nullptr;
    AttachmentView *attachmentView = nullptr;
    MimeTreeParser::MessagePart::List selectedParts;

    QAction *saveAttachmentAction = nullptr;
    QAction *openAttachmentAction = nullptr;
    QAction *importPublicKeyAction = nullptr;

    void createActions()
    {
        saveAttachmentAction = new QAction(QIcon::fromTheme(QStringLiteral("document-save-as")), i18n("&Save Attachment As..."), q);
        connect(saveAttachmentAction, &QAction::triggered, q, [this]() {
            saveSelectedAttachments();
        });

        openAttachmentAction = new QAction(i18nc("to open", "Open"), q);
        connect(openAttachmentAction, &QAction::triggered, q, [this]() {
            openSelectedAttachments();
        });

        importPublicKeyAction = new QAction(i18nc("@action:inmenu", "Import public key"), q);
        connect(importPublicKeyAction, &QAction::triggered, q, [this]() {
            importPublicKey();
        });
    }

    void openSelectedAttachments();
    void saveSelectedAttachments();
    void selectionChanged();
    void showContextMenu();
    void importPublicKey();
};

void MessageViewer::Private::openSelectedAttachments()
{
    Q_ASSERT(selectedParts.count() >= 1);
    for (const auto &part : std::as_const(selectedParts)) {
        parser.attachments()->openAttachment(part);
    }
}

void MessageViewer::Private::saveSelectedAttachments()
{
    Q_ASSERT(selectedParts.count() >= 1);
    for (const auto &part : std::as_const(selectedParts)) {
        parser.attachments()->saveAttachmentToDisk(part);
    }
}

void MessageViewer::Private::importPublicKey()
{
    Q_ASSERT(selectedParts.count() == 1);
    parser.attachments()->importPublicKey(selectedParts[0]);
}

void MessageViewer::Private::showContextMenu()
{
    const int numberOfParts(selectedParts.count());
    QMenu menu;
    if (numberOfParts == 1) {
        const QString mimetype = QString::fromLatin1(selectedParts.first()->mimeType());
        if (mimetype == QLatin1String("application/pgp-keys")) {
            menu.addAction(importPublicKeyAction);
        }
    }

    menu.addAction(openAttachmentAction);
    menu.addAction(saveAttachmentAction);

    menu.exec(QCursor::pos());
}

void MessageViewer::Private::selectionChanged()
{
    const QModelIndexList selectedRows = attachmentView->selectionModel()->selectedRows();
    MimeTreeParser::MessagePart::List selectedParts;
    selectedParts.reserve(selectedRows.count());
    for (const QModelIndex &index : selectedRows) {
        auto part = attachmentView->model()->data(index, AttachmentModel::AttachmentPartRole).value<MimeTreeParser::MessagePart::Ptr>();
        selectedParts.append(part);
    }
    this->selectedParts = selectedParts;
}

MessageViewer::MessageViewer(QWidget *parent)
    : QSplitter(Qt::Vertical, parent)
    , d(std::make_unique<MessageViewer::Private>(this))
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

    d->attachmentView = new AttachmentView(this);
    addWidget(d->attachmentView);

    connect(d->attachmentView, &AttachmentView::contextMenuRequested, this, [this] {
        d->selectionChanged();
        d->showContextMenu();
    });
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

    connect(d->attachmentView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this] {
        d->selectionChanged();
    });
}
