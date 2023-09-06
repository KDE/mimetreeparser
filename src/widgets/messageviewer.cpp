// SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "messageviewer.h"
#include "../core/objecttreeparser.h"
#include "attachmentview_p.h"
#include "messagecontainerwidget_p.h"
#include "urlhandler_p.h"
#include <KCalendarCore/Event>
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/Incidence>
#include <KMessageWidget>
#define TRANSLATION_DOMAIN "mimetreeparser"
#include <KLocalizedString>
#include <MimeTreeParserCore/AttachmentModel>
#include <MimeTreeParserCore/MessageParser>
#include <MimeTreeParserCore/PartModel>
#include <QAction>
#include <QDebug>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QScrollArea>
#include <QSplitter>
#include <QVBoxLayout>

using namespace MimeTreeParser::Widgets;

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
    UrlHandler *urlHandler = nullptr;

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
    void recursiveBuildViewer(PartModel *parts, QVBoxLayout *layout, const QModelIndex &parent);
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

    d->urlHandler = new UrlHandler(this);

    d->formLayout = new QFormLayout(headersArea);

    auto widget = new QWidget(this);
    d->layout = new QVBoxLayout(widget);
    d->layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

    auto scrollArea = new QScrollArea(this);
    scrollArea->setWidget(widget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setBackgroundRole(QPalette::Base);
    addWidget(scrollArea);
    setStretchFactor(1, 2);

    d->attachmentView = new AttachmentView(this);
    d->attachmentView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    addWidget(d->attachmentView);

    connect(d->attachmentView, &AttachmentView::contextMenuRequested, this, [this] {
        d->selectionChanged();
        d->showContextMenu();
    });

    setMinimumSize(600, 600);
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

void MessageViewer::Private::recursiveBuildViewer(PartModel *parts, QVBoxLayout *layout, const QModelIndex &parent)
{
    for (int i = 0, count = parts->rowCount(parent); i < count; i++) {
        const auto type = static_cast<PartModel::Types>(parts->data(parts->index(i, 0, parent), PartModel::TypeRole).toUInt());
        const auto content = parts->data(parts->index(i, 0, parent), PartModel::ContentRole).toString();

        const auto signatureInfo = parts->data(parts->index(i, 0, parent), PartModel::SignatureDetails).value<SignatureInfo>();
        const auto isSigned = parts->data(parts->index(i, 0, parent), PartModel::IsSignedRole).toBool();
        const auto signatureSecurityLevel =
            static_cast<PartModel::SecurityLevel>(parts->data(parts->index(i, 0, parent), PartModel::SignatureSecurityLevelRole).toInt());

        const auto encryptionInfo = parts->data(parts->index(i, 0, parent), PartModel::EncryptionDetails).value<SignatureInfo>();
        const auto isEncrypted = parts->data(parts->index(i, 0, parent), PartModel::IsEncryptedRole).toBool();
        const auto encryptionSecurityLevel =
            static_cast<PartModel::SecurityLevel>(parts->data(parts->index(i, 0, parent), PartModel::EncryptionSecurityLevelRole).toInt());

        const auto displayEncryptionInfo =
            i == 0 || parts->data(parts->index(i - 1, 0, parent), PartModel::EncryptionDetails).value<SignatureInfo>().keyId != encryptionInfo.keyId;

        const auto displaySignatureInfo =
            i == 0 || parts->data(parts->index(i - 1, 0, parent), PartModel::SignatureDetails).value<SignatureInfo>().keyId != signatureInfo.keyId;

        switch (type) {
        case PartModel::Types::Plain: {
            auto container = new MessageWidgetContainer(isSigned,
                                                        signatureInfo,
                                                        signatureSecurityLevel,
                                                        displaySignatureInfo,
                                                        isEncrypted,
                                                        encryptionInfo,
                                                        encryptionSecurityLevel,
                                                        displayEncryptionInfo,
                                                        urlHandler);
            auto label = new QLabel(content);
            label->setTextInteractionFlags(Qt::TextSelectableByMouse);
            container->layout()->addWidget(label);
            widgets.append(container);
            layout->addWidget(container);
            break;
        }
        case PartModel::Types::Ical: {
            auto container = new MessageWidgetContainer(isSigned,
                                                        signatureInfo,
                                                        signatureSecurityLevel,
                                                        displaySignatureInfo,
                                                        isEncrypted,
                                                        encryptionInfo,
                                                        encryptionSecurityLevel,
                                                        displayEncryptionInfo,
                                                        urlHandler);

            KCalendarCore::ICalFormat format;
            auto incidence = format.fromString(content);

            auto widget = new QGroupBox(container);
            widget->setTitle(i18n("Invitation"));

            auto incidenceLayout = new QFormLayout(widget);
            incidenceLayout->addRow(i18n("&Summary:"), new QLabel(incidence->summary()));
            incidenceLayout->addRow(i18n("&Organizer:"), new QLabel(incidence->organizer().fullName()));
            if (incidence->location().length() > 0) {
                incidenceLayout->addRow(i18n("&Location:"), new QLabel(incidence->location()));
            }
            incidenceLayout->addRow(i18n("&Start date:"), new QLabel(incidence->dtStart().toLocalTime().toString()));
            if (const auto event = incidence.dynamicCast<KCalendarCore::Event>()) {
                incidenceLayout->addRow(i18n("&End date:"), new QLabel(event->dtEnd().toLocalTime().toString()));
            }
            if (incidence->description().length() > 0) {
                incidenceLayout->addRow(i18n("&Details:"), new QLabel(incidence->description()));
            }

            container->layout()->addWidget(widget);

            widgets.append(container);
            layout->addWidget(container);
            break;
        }
        case PartModel::Types::Encapsulated: {
            auto container = new MessageWidgetContainer(isSigned,
                                                        signatureInfo,
                                                        signatureSecurityLevel,
                                                        displaySignatureInfo,
                                                        isEncrypted,
                                                        encryptionInfo,
                                                        encryptionSecurityLevel,
                                                        displayEncryptionInfo,
                                                        urlHandler);

            auto groupBox = new QGroupBox(container);
            groupBox->setSizePolicy(QSizePolicy::MinimumExpanding, q->sizePolicy().verticalPolicy());
            groupBox->setTitle(i18n("Encapsulated email"));

            auto encapsulatedLayout = new QVBoxLayout(groupBox);

            auto header = new QWidget(groupBox);
            auto headerLayout = new QFormLayout(header);
            const auto from = parts->data(parts->index(i, 0, parent), PartModel::SenderRole).toString();
            const auto date = parts->data(parts->index(i, 0, parent), PartModel::DateRole).toDateTime();
            headerLayout->addRow(i18n("From:"), new QLabel(from));
            headerLayout->addRow(i18n("Date:"), new QLabel(date.toLocalTime().toString()));

            encapsulatedLayout->addWidget(header);

            recursiveBuildViewer(parts, encapsulatedLayout, parts->index(i, 0, parent));

            container->layout()->addWidget(groupBox);

            widgets.append(container);
            layout->addWidget(container);
            break;
        }

        case PartModel::Types::Error: {
            const auto errorString = parts->data(parts->index(i, 0, parent), PartModel::ErrorString).toString();
            auto errorWidget = new KMessageWidget(i18n("Error: %1", errorString));
            errorWidget->setMessageType(KMessageWidget::MessageType::Error);
            widgets.append(errorWidget);
            layout->addWidget(errorWidget);
            break;
        }
        default:
            qWarning() << parts->data(parts->index(i, 0, parent), PartModel::ContentRole) << type;
        }
    }
}

void MessageViewer::setMessage(const KMime::Message::Ptr message)
{
    d->parser.setMessage(message);

    for (int i = d->formLayout->rowCount() - 1; i >= 0; i--) {
        d->formLayout->removeRow(i);
    }
    if (!d->parser.subject().isEmpty()) {
        d->formLayout->addRow(i18n("&Subject:"), new QLabel(d->parser.subject()));
    }
    if (!d->parser.from().isEmpty()) {
        d->formLayout->addRow(i18n("&From:"), new QLabel(d->parser.from()));
    }
    if (!d->parser.to().isEmpty()) {
        d->formLayout->addRow(i18n("&To:"), new QLabel(d->parser.to()));
    }

    const auto parts = d->parser.parts();

    for (auto widget : std::as_const(d->widgets)) {
        d->layout->removeWidget(widget);
        delete widget;
    }
    d->widgets.clear();

    d->recursiveBuildViewer(parts, d->layout, {});
    d->layout->addStretch();

    d->attachmentView->setModel(d->parser.attachments());
    d->attachmentView->setVisible(d->parser.attachments()->rowCount() > 0);

    connect(d->attachmentView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this] {
        d->selectionChanged();
    });
}
