// SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "messageviewer.h"

#include "attachmentview_p.h"
#include "messagecontainerwidget_p.h"
#include "mimetreeparser_widgets_debug.h"
#include "partmodel.h"
#include "urlhandler.h"

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
#include <QFontDatabase>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QPointer>
#include <QScrollArea>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <qnamespace.h>

using namespace MimeTreeParser::Widgets;
using namespace MimeTreeParser::Core;
using namespace Qt::Literals::StringLiterals;

class MessageViewer::Private
{
public:
    explicit Private(MessageViewer *q_ptr)
        : q{q_ptr}
        , messageWidget(new KMessageWidget(q_ptr))
    {
        messageWidget->setCloseButtonVisible(true);
        messageWidget->hide();
    }

    MessageViewer *const q;

    QVBoxLayout *layout = nullptr;
    std::shared_ptr<KMime::Message> message;
    QPointer<MessageParser> parser = nullptr;
    QScrollArea *scrollArea = nullptr;
    QFormLayout *formLayout = nullptr;
    AttachmentView *attachmentView = nullptr;
    UrlHandler *urlHandler = nullptr;
    KMessageWidget *const messageWidget = nullptr;

    QList<QSharedPointer<MimeTreeParser::Core::MessagePart>> partsForActions;
    bool fixedFont = false;

    void openSelectedAttachments(const QList<QSharedPointer<MimeTreeParser::Core::MessagePart>> &selectedParts);
    void saveSelectedAttachments(const QList<QSharedPointer<MimeTreeParser::Core::MessagePart>> &selectedParts);
    void showContextMenu(const QList<QSharedPointer<MimeTreeParser::Core::MessagePart>> &parts);
    QList<QSharedPointer<MimeTreeParser::Core::MessagePart>> partsSelectedInView() const;
    void importPublicKey(const QSharedPointer<MimeTreeParser::Core::MessagePart> &part);
    void recursiveBuildViewer(PartModel *parts, QVBoxLayout *layout, const QModelIndex &parent, QList<MessageWidgetContainer *> &containerStack);
};

void MessageViewer::Private::openSelectedAttachments(const QList<QSharedPointer<MimeTreeParser::Core::MessagePart>> &selectedParts)
{
    Q_ASSERT(parser);
    Q_ASSERT(selectedParts.count() >= 1);
    for (const auto &part : std::as_const(selectedParts)) {
        parser->attachments()->openAttachment(part);
    }
}

void MessageViewer::Private::saveSelectedAttachments(const QList<QSharedPointer<MimeTreeParser::Core::MessagePart>> &selectedParts)
{
    Q_ASSERT(parser);
    Q_ASSERT(selectedParts.count() >= 1);

    for (const auto &part : std::as_const(selectedParts)) {
        QString pname = part->filename(MessagePart::FallbackToNameOrPlaceholder);
        const QString path = QFileDialog::getSaveFileName(q, i18n("Save Attachment As"), pname);
        if (!path.isEmpty()) {
            parser->attachments()->saveAttachmentToPath(part, path);
        }
    }
}

void MessageViewer::Private::importPublicKey(const QSharedPointer<MimeTreeParser::Core::MessagePart> &part)
{
    Q_ASSERT(parser);
    parser->attachments()->importPublicKey(part);
}

void MessageViewer::Private::showContextMenu(const QList<QSharedPointer<MimeTreeParser::Core::MessagePart>> &selectedParts)
{
    const int numberOfParts(selectedParts.count());
    QMenu menu;
    if (numberOfParts == 1) {
        const QString mimetype = QString::fromLatin1(selectedParts.first()->mimeType());
        if (mimetype == QLatin1StringView("application/pgp-keys")) {
            auto importPublicKeyAction = new QAction(QIcon::fromTheme(u"document-import-key-symbolic"_s), i18nc("@action:inmenu", "Import public key"), q);
            connect(importPublicKeyAction, &QAction::triggered, q, [this, selectedParts]() {
                importPublicKey(selectedParts.first());
            });
            menu.addAction(importPublicKeyAction);
        }
    }

    auto openAttachmentAction = new QAction(QIcon::fromTheme(u"document-open-symbolic"_s), i18nc("to open", "Open"), q);
    connect(openAttachmentAction, &QAction::triggered, q, [this, selectedParts]() {
        openSelectedAttachments(selectedParts);
    });
    menu.addAction(openAttachmentAction);

    auto saveAttachmentAction = new QAction(QIcon::fromTheme(u"document-save-as-symbolic"_s), i18n("&Save Attachment As…"), q);
    connect(saveAttachmentAction, &QAction::triggered, q, [this, selectedParts]() {
        saveSelectedAttachments(selectedParts);
    });
    menu.addAction(saveAttachmentAction);

    menu.exec(QCursor::pos());
}

QList<QSharedPointer<MimeTreeParser::Core::MessagePart>> MessageViewer::Private::partsSelectedInView() const
{
    const QModelIndexList selectedRows = attachmentView->selectionModel()->selectedRows();
    QList<QSharedPointer<MimeTreeParser::Core::MessagePart>> selectedMessageParts;
    selectedMessageParts.reserve(selectedRows.count());
    for (const QModelIndex &index : selectedRows) {
        auto part = attachmentView->model()->data(index, AttachmentModel::AttachmentPartRole).value<QSharedPointer<MimeTreeParser::Core::MessagePart>>();
        selectedMessageParts.append(part);
    }
    return selectedMessageParts;
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
        d->showContextMenu(d->partsSelectedInView());
    });

    connect(d->attachmentView, &QAbstractItemView::doubleClicked, this, [this](const QModelIndex &) {
        // Since this is only emitted if a valid index is double clicked we can assume
        // that the first click of the double click set the selection accordingly.
        d->openSelectedAttachments(d->partsSelectedInView());
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

class HeaderLabel : public QLabel
{
public:
    explicit HeaderLabel(const QString &content)
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

QString MessageViewer::subject() const
{
    return d->parser ? d->parser->subject() : QString{};
}

std::shared_ptr<KMime::Message> MessageViewer::message() const
{
    return d->parser ? d->parser->message() : std::shared_ptr<KMime::Message>{};
}

std::shared_ptr<MimeTreeParser::Core::ObjectTreeParser> MessageViewer::parser() const
{
    return d->parser ? d->parser->parser() : nullptr;
}

// TODO: remove params lay and parts
void MessageViewer::Private::recursiveBuildViewer(PartModel *parts,
                                                  QVBoxLayout *lay,
                                                  const QModelIndex &parent,
                                                  QList<MessageWidgetContainer *> &containerStack)
{
    for (int i = 0, count = parts->rowCount(parent); i < count; i++) {
        const auto idx = parts->index(i, 0, parent);
        const auto type = idx.data(PartModel::TypeRole).value<PartModel::Types>();
        const auto content = idx.data(PartModel::ContentRole).toString();
        const auto part = parts->part(idx);

        // Quite unfortunately, the tree structure in PartModel is not the same as the tree structure we want
        // to build visually. So first, sort out, where we want to insert.
        bool reuse_container = false;
        while (!containerStack.isEmpty()) {
            if (part->encryption() == containerStack.last()->containerPart()->encryption()
                && part->signature() == containerStack.last()->containerPart()->signature()) {
                reuse_container = true;
                break;
            }
            if (part->isChildOf(containerStack.last()->containerPart())) {
                break;
            }
            containerStack.takeLast();
        }
        if (!reuse_container) {
            containerStack.append(new MessageWidgetContainer(idx, urlHandler));
            lay->addWidget(containerStack.last());
        }
        auto container = containerStack.last();

        switch (type) {
        case PartModel::Types::Plain: {
            auto label = new QLabel(content);
            label->setTextInteractionFlags(Qt::TextBrowserInteraction);
            label->setOpenExternalLinks(true);
            label->setWordWrap(true);
            if (fixedFont) {
                label->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
            }
            container->innerLayout()->addWidget(label);

            connect(container, &MessageWidgetContainer::attachmentContextMenu, q, [this](const QSharedPointer<MimeTreeParser::Core::MessagePart> part) {
                showContextMenu({part});
            });

            break;
        }
        case PartModel::Types::Ical: {
            KCalendarCore::ICalFormat format;
            auto incidence = format.fromString(content);

            auto widget = new QGroupBox(container);
            widget->setTitle(i18n("Invitation"));

            auto incidenceLayout = new QFormLayout(widget);
            if (incidence) {
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
            } else {
                incidenceLayout->addRow(new QLabel(i18nc("@info:status", "Invitation is broken")));
            }

            container->innerLayout()->addWidget(widget);

            break;
        }
        case PartModel::Types::Encapsulated: {
            auto groupBox = new QGroupBox(container);
            groupBox->setSizePolicy(QSizePolicy::MinimumExpanding, q->sizePolicy().verticalPolicy());
            groupBox->setTitle(i18n("Encapsulated email"));

            auto encapsulatedLayout = new QVBoxLayout(groupBox);

            auto header = new QWidget(groupBox);
            auto headerLayout = new QFormLayout(header);
            const auto encapsulated = part.dynamicCast<const EncapsulatedRfc822MessagePart>();
            Q_ASSERT(encapsulated);
            if (const auto subject = encapsulated->subject(); subject.isEmpty()) {
                headerLayout->addRow(i18n("Subject:"), new QLabel(u"<i>"_s + i18nc("Email has no subject", "No subject") + u"</i>"_s));
            } else {
                headerLayout->addRow(i18n("Subject:"), new HeaderLabel(subject));
            }
            const auto from = idx.data(PartModel::SenderRole).toString();
            const auto date = idx.data(PartModel::DateRole).toDateTime();
            if (from.isEmpty()) {
                headerLayout->addRow(i18n("From:"),
                                     new QLabel(u"<i>"_s + i18nc("@info:status missing 'From:', i.e. an unknown author", "Unknown") + u"</i>"_s));
            } else {
                headerLayout->addRow(i18n("From:"), new HeaderLabel(from));
            }
            headerLayout->addRow(i18n("Date:"), new QLabel(date.toLocalTime().toString()));

            encapsulatedLayout->addWidget(header);

            recursiveBuildViewer(parts, encapsulatedLayout, idx, containerStack);

            container->innerLayout()->addWidget(groupBox);

            break;
        }

        case PartModel::Types::Error: {
            const auto errorString = idx.data(PartModel::ErrorString).toString();
            auto errorWidget = new KMessageWidget(errorString);
            errorWidget->setCloseButtonVisible(false);
            errorWidget->setMessageType(KMessageWidget::MessageType::Error);
            QObject::connect(errorWidget, &KMessageWidget::linkActivated, errorWidget, [this, errorWidget](const QString &link) {
                QUrl url(link);
                if (url.path() == QLatin1StringView("showCertificate")) {
                    urlHandler->handleClick(QUrl(link), errorWidget->window()->windowHandle());
                }
            });
            errorWidget->setWordWrap(true);
            container->innerLayout()->addWidget(errorWidget);
            break;
        }
        default:
            qCWarning(MIMETREEPARSER_WIDGET_LOG) << idx.data(PartModel::ContentRole) << type;
        }
    }
}

void MessageViewer::setMessage(const std::shared_ptr<KMime::Message> &message)
{
    setUpdatesEnabled(false);
    if (message) {
        if (!d->parser) {
            d->parser = new MessageParser{this};
        }
        if (message != d->parser->message()) {
            d->parser->setMessage(message);
            connect(d->parser->attachments(), &AttachmentModel::info, this, [this](const QString &message) {
                d->messageWidget->setMessageType(KMessageWidget::Information);
                d->messageWidget->setText(message);
                d->messageWidget->animatedShow();
            });
            connect(d->parser->attachments(), &AttachmentModel::errorOccurred, this, [this](const QString &message) {
                d->messageWidget->setMessageType(KMessageWidget::Error);
                d->messageWidget->setText(message);
                d->messageWidget->animatedShow();
            });
        }
    } else {
        delete d->parser;
    }

    for (int i = d->formLayout->rowCount() - 1; i >= 0; i--) {
        d->formLayout->removeRow(i);
    }
    if (d->parser) {
        if (!d->parser->subject().isEmpty()) {
            const auto label = new QLabel(d->parser->subject());
            label->setTextFormat(Qt::PlainText);
            d->formLayout->addRow(i18n("&Subject:"), label);
        }
        if (!d->parser->from().isEmpty()) {
            d->formLayout->addRow(i18n("&From:"), new HeaderLabel(d->parser->from()));
        } else {
            d->formLayout->addRow(i18n("&From:"), new QLabel(u"<i>"_s + i18nc("@info:status missing 'From:', i.e. an unknown author", "Unknown") + u"</i>"_s));
        }
        if (!d->parser->sender().isEmpty() && d->parser->from() != d->parser->sender()) {
            d->formLayout->addRow(i18n("&Sender:"), new HeaderLabel(d->parser->sender()));
        }
        if (!d->parser->to().isEmpty()) {
            d->formLayout->addRow(i18n("&To:"), new HeaderLabel(d->parser->to()));
        }
        if (!d->parser->cc().isEmpty()) {
            d->formLayout->addRow(i18n("&CC:"), new HeaderLabel(d->parser->cc()));
        }
        if (!d->parser->bcc().isEmpty()) {
            d->formLayout->addRow(i18n("&BCC:"), new HeaderLabel(d->parser->bcc()));
        }
        if (!d->parser->date().isNull()) {
            d->formLayout->addRow(i18n("&Date:"), new HeaderLabel(QLocale::system().toString(d->parser->date().toLocalTime(), QLocale::ShortFormat)));
        }
    }

    QLayoutItem *child;
    while ((child = d->layout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    if (d->parser) {
        const auto parts = d->parser->parts();
        QList<MessageWidgetContainer *> containerStack;
        d->recursiveBuildViewer(parts, d->layout, {}, containerStack);
    }
    d->layout->addStretch();

    d->attachmentView->setModel(d->parser ? d->parser->attachments() : nullptr);
    d->attachmentView->setVisible(d->parser && (d->parser->attachments()->rowCount() > 0));

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

bool MessageViewer::fixedFont() const
{
    return d->fixedFont;
}

void MessageViewer::setFixedFont(bool checked)
{
    d->fixedFont = checked;

    setMessage(message()); // rebuild UI
}
