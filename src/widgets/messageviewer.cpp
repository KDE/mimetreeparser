// SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "messageviewer.h"

#include "attachmentview_p.h"
#include "messagecontainerwidget_p.h"
#include "mimetreeparser_widgets_debug.h"
#include "urlhandler_p.h"

#include <KCalendarCore/Event>
#include <KCalendarCore/ICalFormat>
#include <KLocalizedString>
#include <KMessageWidget>
#include <MimeTreeParserCore/AttachmentModel>
#include <MimeTreeParserCore/MessageParser>
#include <MimeTreeParserCore/ObjectTreeParser>
#include <MimeTreeParserCore/PartModel>

#include <QAction>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QScrollArea>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <qnamespace.h>

using namespace MimeTreeParser::Widgets;

class MessageViewer::Private
{
public:
    Private(MessageViewer *q_ptr)
        : q{q_ptr}
        , messageWidget(new KMessageWidget(q_ptr))
    {
        createActions();

        messageWidget->setCloseButtonVisible(true);
        messageWidget->hide();
    }

    MessageViewer *q;

    QVBoxLayout *layout = nullptr;
    KMime::Message::Ptr message;
    MessageParser parser;
    QScrollArea *scrollArea = nullptr;
    QFormLayout *formLayout = nullptr;
    AttachmentView *attachmentView = nullptr;
    MimeTreeParser::MessagePart::List selectedParts;
    UrlHandler *urlHandler = nullptr;
    KMessageWidget *const messageWidget = nullptr;

    QAction *saveAttachmentAction = nullptr;
    QAction *openAttachmentAction = nullptr;
    QAction *importPublicKeyAction = nullptr;

    void createActions()
    {
        saveAttachmentAction = new QAction(QIcon::fromTheme(QStringLiteral("document-save-as")), i18n("&Save Attachment As…"), q);
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
        QString pname = part->filename();
        if (pname.isEmpty()) {
            pname = i18nc("Fallback when file has no name", "unnamed");
        }

        const QString path = QFileDialog::getSaveFileName(q, i18n("Save Attachment As"), pname);
        parser.attachments()->saveAttachmentToPath(part, path);
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
        if (mimetype == QLatin1StringView("application/pgp-keys")) {
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
    setObjectName(QLatin1StringView("MessageViewerSplitter"));
    setChildrenCollapsible(false);
    setSizes({0});

    addWidget(d->messageWidget);

    auto mainWidget = new QWidget(this);
    auto mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->setContentsMargins({});
    mainLayout->setSpacing(0);

    auto headersArea = new QWidget(mainWidget);
    headersArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    mainLayout->addWidget(headersArea);

    d->urlHandler = new UrlHandler(this);

    d->formLayout = new QFormLayout(headersArea);

    auto widget = new QWidget(this);
    d->layout = new QVBoxLayout(widget);
    d->layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    d->layout->setObjectName(QLatin1StringView("PartLayout"));

    d->scrollArea = new QScrollArea(this);
    d->scrollArea->setWidget(widget);
    d->scrollArea->setWidgetResizable(true);
    d->scrollArea->setBackgroundRole(QPalette::Base);
    mainLayout->addWidget(d->scrollArea);
    mainLayout->setStretchFactor(d->scrollArea, 2);
    setStretchFactor(1, 2);

    d->attachmentView = new AttachmentView(this);
    d->attachmentView->setProperty("_breeze_borders_sides", QVariant::fromValue(QFlags{Qt::BottomEdge}));
    d->attachmentView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    addWidget(d->attachmentView);

    connect(d->attachmentView, &AttachmentView::contextMenuRequested, this, [this] {
        d->selectionChanged();
        d->showContextMenu();
    });
}

MessageViewer::~MessageViewer()
{
    QLayoutItem *child;
    while ((child = d->layout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
}

QString MessageViewer::subject() const
{
    return d->parser.subject();
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
            label->setTextInteractionFlags(Qt::TextBrowserInteraction);
            label->setOpenExternalLinks(true);
            label->setWordWrap(true);
            container->layout()->addWidget(label);
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

            layout->addWidget(container);
            break;
        }

        case PartModel::Types::Error: {
            const auto errorString = parts->data(parts->index(i, 0, parent), PartModel::ErrorString).toString();
            auto errorWidget = new KMessageWidget(errorString);
            errorWidget->setCloseButtonVisible(false);
            errorWidget->setMessageType(KMessageWidget::MessageType::Error);
            QObject::connect(errorWidget, &KMessageWidget::linkActivated, errorWidget, [this, errorWidget](const QString &link) {
                QUrl url(link);
                if (url.path() == QStringLiteral("showCertificate")) {
                    urlHandler->handleClick(QUrl(link), errorWidget->window()->windowHandle());
                }
            });
            errorWidget->setWordWrap(true);
            layout->addWidget(errorWidget);
            break;
        }
        default:
            qCWarning(MIMETREEPARSER_WIDGET_LOG) << parts->data(parts->index(i, 0, parent), PartModel::ContentRole) << type;
        }
    }
}

class HeaderLabel : public QLabel
{
public:
    HeaderLabel(const QString &content)
        : QLabel(content)
    {
        setWordWrap(true);
        setTextFormat(Qt::PlainText);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    }

    void resizeEvent(QResizeEvent *event) override
    {
        int height = heightForWidth(width());
        setMaximumHeight(height);
        setMinimumHeight(height);

        QLabel::resizeEvent(event);
    }
};

void MessageViewer::setMessage(const KMime::Message::Ptr message)
{
    setUpdatesEnabled(false);
    d->parser.setMessage(message);

    connect(d->parser.attachments(), &AttachmentModel::info, this, [this](const QString &message) {
        d->messageWidget->setMessageType(KMessageWidget::Information);
        d->messageWidget->setText(message);
        d->messageWidget->animatedShow();
    });

    connect(d->parser.attachments(), &AttachmentModel::errorOccurred, this, [this](const QString &message) {
        d->messageWidget->setMessageType(KMessageWidget::Error);
        d->messageWidget->setText(message);
        d->messageWidget->animatedShow();
    });

    for (int i = d->formLayout->rowCount() - 1; i >= 0; i--) {
        d->formLayout->removeRow(i);
    }
    if (!d->parser.subject().isEmpty()) {
        const auto label = new QLabel(d->parser.subject());
        label->setTextFormat(Qt::PlainText);
        d->formLayout->addRow(i18n("&Subject:"), label);
    }
    if (!d->parser.from().isEmpty()) {
        d->formLayout->addRow(i18n("&From:"), new HeaderLabel(d->parser.from()));
    }
    if (!d->parser.sender().isEmpty() && d->parser.from() != d->parser.sender()) {
        d->formLayout->addRow(i18n("&Sender:"), new HeaderLabel(d->parser.sender()));
    }
    if (!d->parser.to().isEmpty()) {
        d->formLayout->addRow(i18n("&To:"), new HeaderLabel(d->parser.to()));
    }
    if (!d->parser.cc().isEmpty()) {
        d->formLayout->addRow(i18n("&CC:"), new HeaderLabel(d->parser.cc()));
    }
    if (!d->parser.bcc().isEmpty()) {
        d->formLayout->addRow(i18n("&BCC:"), new HeaderLabel(d->parser.bcc()));
    }
    if (!d->parser.date().isNull()) {
        d->formLayout->addRow(i18n("&Date:"), new HeaderLabel(QLocale::system().toString(d->parser.date().toLocalTime())));
    }

    const auto parts = d->parser.parts();

    QLayoutItem *child;
    while ((child = d->layout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    d->recursiveBuildViewer(parts, d->layout, {});
    d->layout->addStretch();

    d->attachmentView->setModel(d->parser.attachments());
    d->attachmentView->setVisible(d->parser.attachments()->rowCount() > 0);

    connect(d->attachmentView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this] {
        d->selectionChanged();
    });

    connect(d->attachmentView, &QAbstractItemView::doubleClicked, this, [this](const QModelIndex &) {
        // Since this is only emitted if a valid index is double clicked we can assume
        // that the first click of the double click set the selection accordingly.
        d->openSelectedAttachments();
    });

    setUpdatesEnabled(true);
}

void MessageViewer::print(QPainter *painter, int width)
{
    const auto oldSize = size();
    resize(width - 30, oldSize.height());
    d->scrollArea->setFrameShape(QFrame::NoFrame);
    render(painter);
    d->scrollArea->setFrameShape(QFrame::StyledPanel);
    resize(oldSize);
}
